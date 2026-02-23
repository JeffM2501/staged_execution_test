#include "EntitySystem.h"
#include "TaskManager.h"
#include "FrameStage.h"
#include "ResourceManager.h"

#include <memory>
#include <unordered_map>
#include <set>

namespace EntitySystem
{
    std::recursive_mutex MorgueLock;

    std::set<size_t> EntityMorgue;
    static std::unordered_map<size_t, std::unique_ptr<IComponentTable>> ComponentTables;
    size_t NextEntityId = 1;

    struct EntityInfo
    {
        bool Awake = false;
        bool Enabled = true;
    };

    std::recursive_mutex EntityInfoLock;
    static std::unordered_map<size_t, EntityInfo> EntityInfoCache;

    void Init()
    {
        TaskManager::AddTaskOnState<LambdaTask>(FrameStage::FrameTail, Hashes::CRC64Str("FlushEntities"), []() { EntitySystem::FlushMorgue(); }, true);
    }

    size_t NewEntityId()
    {
        return NextEntityId++;
    }

    EntityComponent* GetEntityComponent(size_t entityId, size_t componentType)
    {
        auto itr = ComponentTables.find(componentType);
        if (itr == ComponentTables.end())
            return nullptr;
        return itr->second->TryGet(entityId);
    }

    bool EntityHasComponent(size_t entityId, size_t componentType)
    {
        auto itr = ComponentTables.find(componentType);
        if (itr == ComponentTables.end())
            return false;
        return itr->second->HasEntity(entityId);
    }

    void RegisterComponent(size_t compnentType, std::unique_ptr<IComponentTable> table)
    {
        ComponentTables.insert_or_assign(compnentType, std::move(table));
    }

    IComponentTable* GetComponentTable(size_t componentType)
    {
        auto itr = ComponentTables.find(componentType);
        if (itr == ComponentTables.end())
            return nullptr;
        return itr->second.get();
    }

    EntityComponent* AddComponent(size_t entityId, size_t componentType)
    {
        if (!EntityInfoCache.contains(entityId))
            EntityInfoCache.insert_or_assign(entityId, EntityInfo());

        auto itr = ComponentTables.find(componentType);
        if (itr == ComponentTables.end())
            return nullptr;
        return itr->second->Add(entityId);
    }

    void RemoveEntity(size_t entityId)
    {
        EnableEntity(entityId, false);

        std::lock_guard<std::recursive_mutex> lock(MorgueLock);
        EntityMorgue.insert(entityId);
    }

    void AwakeAllEntities()
    {
        std::lock_guard<std::recursive_mutex> lock(EntityInfoLock);
        for (auto& [entity, info] : EntityInfoCache)
        {
            info.Awake = true;
        }
    }

    bool IsEntityReady(size_t entityId)
    {
        std::lock_guard<std::recursive_mutex> lock(EntityInfoLock);
        auto itr = EntityInfoCache.find(entityId);
        return itr != EntityInfoCache.end() && itr->second.Awake;
    }

    bool IsEntityEnabled(size_t entityId)
    {
        std::lock_guard<std::recursive_mutex> lock(EntityInfoLock);
        auto itr = EntityInfoCache.find(entityId);
        return itr != EntityInfoCache.end() && itr->second.Awake && itr->second.Enabled;
    }

    void EnableEntity(size_t entityId, bool enabled)
    {
        std::lock_guard<std::recursive_mutex> lock(EntityInfoLock);
        auto itr = EntityInfoCache.find(entityId);
        if (itr != EntityInfoCache.end())
            itr->second.Enabled = enabled;
    }

    void AwakeEntity(size_t entityId)
    {
        std::lock_guard<std::recursive_mutex> lock(EntityInfoLock);
        auto itr = EntityInfoCache.find(entityId);
        if (itr != EntityInfoCache.end())
            itr->second.Awake = true;
    }

    void ClearAllEntities()
    {
        for (auto& [componentType, table] : ComponentTables)
        {
            table->Clear();
        }
        std::lock_guard<std::recursive_mutex> lock(EntityInfoLock);
        EntityInfoCache.clear();
    }

    void FlushMorgue()
    {
        std::lock_guard<std::recursive_mutex> lock(MorgueLock);
        std::lock_guard<std::recursive_mutex> infoLock(EntityInfoLock);
        for (size_t entityId : EntityMorgue)
        {
            for (auto& [componentType, table] : ComponentTables)
            {
                table->Remove(entityId);
            }

            EntityInfoCache.erase(entityId);
        }

        EntityMorgue.clear();
    }

    void DoForEachEntityWithComponent(size_t componentType, std::function<void(size_t&)> func, bool paralel, bool enabledOnly)
    {
        auto itr = ComponentTables.find(componentType);
        if (itr == ComponentTables.end() || !func)
            return;

        itr->second->DoForEach([&func](EntityComponent& component)
            {
                func(component.EntityID);
            }, paralel, enabledOnly);
    }

    void DoForEachComponent(size_t componentType, std::function<void(EntityComponent&)> func, bool paralel, bool enabledOnly)
    {
        auto itr = ComponentTables.find(componentType);
        if (itr == ComponentTables.end() || !func)
            return;

        itr->second->DoForEach([&func](EntityComponent& component)
            {
                func(component);
            }, paralel, enabledOnly);
    }

    void EntityReader::ReadEntitiesFromResource(size_t resourceHash)
    {
        using namespace ResourceManager;
        auto resource = LoadResource(resourceHash, ResourceType::File);
        if (!resource || !resource->IsReady())
            return;

        std::lock_guard<std::mutex> lock(resource->Lock);
        const auto& dataVariant = resource->Data;
        if (!std::holds_alternative<std::vector<unsigned char>>(dataVariant))
            return;

        const auto& data = std::get<std::vector<unsigned char>>(dataVariant);
        size_t offset = 0;

        while (offset + sizeof(size_t) * 2 <= data.size())
        {
            size_t entityId;
            size_t componentCount;
            memcpy(&entityId, &data[offset], sizeof(size_t));
            offset += sizeof(size_t);
            memcpy(&componentCount, &data[offset], sizeof(size_t));
            offset += sizeof(size_t);

            for (size_t i = 0; i < componentCount; ++i)
            {
                if (offset + sizeof(size_t) + sizeof(uint32_t) > data.size())
                    return;

                size_t componentId;
                uint32_t dataSize;
                memcpy(&componentId, &data[offset], sizeof(size_t));
                offset += sizeof(size_t);
                memcpy(&dataSize, &data[offset], sizeof(uint32_t));
                offset += sizeof(uint32_t);

                if (offset + dataSize > data.size())
                    return;

                std::vector<uint8_t> buffer(data.begin() + offset, data.begin() + offset + dataSize);
                offset += dataSize;

                EntityComponent* component = AddComponent(entityId, componentId);
                if (component)
                {
                    OnComponentData(component, buffer);
                }
            }
        }
    }
}
