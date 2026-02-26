#include "EntitySystem.h"

#include <memory>
#include <unordered_map>
#include <map>
#include <set>
#include <functional>

namespace EntitySystem
{
    std::recursive_mutex MorgueLock;

    std::set<size_t> EntityMorgue;
    static std::unordered_map<size_t, std::unique_ptr<IComponentTable>> ComponentTables;
    size_t NextEntityId = 1;

    std::mutex ReusableEntityIDsLock;
    std::vector<size_t> ReusableEntityIDs;

    struct EntityInfo
    {
        bool Awake = false;
        bool Enabled = true;
    };

    std::recursive_mutex EntityInfoLock;
    static std::map<size_t, EntityInfo> EntityInfoCache;

    void Init()
    {
    }

    void ReleaseEntityId(size_t id)
    {
        TraceLog(LOG_INFO, "Released Entity %zu", id);
        std::lock_guard<std::mutex> lock(ReusableEntityIDsLock);
        ReusableEntityIDs.push_back(id);
    }

    size_t NewEntityId()
    {
        std::lock_guard<std::mutex> lock(ReusableEntityIDsLock);
        if (!ReusableEntityIDs.empty())
        {
            size_t id = ReusableEntityIDs.back();
            ReusableEntityIDs.pop_back();
            TraceLog(LOG_INFO, "Reused Entity %zu", id);
            return id;
        }
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
        {
            std::lock_guard<std::recursive_mutex> lock(EntityInfoLock);
            auto itr = EntityInfoCache.find(entityId);
            if (itr != EntityInfoCache.end())
                EntityInfoCache.erase(itr);
        }

        {
            TraceLog(LOG_INFO, "Placing Entity %zu in morgue", entityId);
            std::lock_guard<std::recursive_mutex> lock(MorgueLock);
            EntityMorgue.insert(entityId);

        }
    }

    void AwakeAllEntities()
    {
        std::lock_guard<std::recursive_mutex> lock(EntityInfoLock);
        for (auto& [entity, info] : EntityInfoCache)
        {
            info.Awake = true;
        }

        TraceLog(LOG_INFO, "Awake All Entities");
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

        TraceLog(LOG_INFO, "Awake Entity %zu", entityId);
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
 
        for (size_t entityId : EntityMorgue)
        {
            ReleaseEntityId(entityId);

            for (auto& [componentType, table] : ComponentTables)
            {
                table->Remove(entityId);
            }
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
}
