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

    std::mutex TableLock;
    static std::unordered_map<size_t, std::unique_ptr<IComponentTable>> ComponentTables;
    size_t NextEntityId = 1;

    std::mutex ReusableEntityIDsLock;
    std::vector<size_t> ReusableEntityIDs;

    struct EntityInfo
    {
        bool Awake = false;
        bool Enabled = true;
        std::set<size_t> ComponentTypes; // componentType -> index in table
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
        IComponentTable* table = GetComponentTable(componentType);
        if (!table)
            return nullptr;

        return table->TryGet(entityId);
    }

    bool EntityHasComponent(size_t entityId, size_t componentType)
    {
        IComponentTable* table = GetComponentTable(componentType);
        if (!table)
            return false;

        return table->HasEntity(entityId);
    }

    void RegisterComponent(size_t compnentType, std::unique_ptr<IComponentTable> table)
    {
        std::lock_guard<std::mutex> lock(TableLock);
        ComponentTables.insert_or_assign(compnentType, std::move(table));
    }

    IComponentTable* GetComponentTable(size_t componentType)
    {
       // std::lock_guard<std::mutex> lock(TableLock);
        auto itr = ComponentTables.find(componentType);
        if (itr == ComponentTables.end())
            return nullptr;
        return itr->second.get();
    }

    EntityComponent* AddComponent(size_t entityId, size_t componentType)
    {
        {
            if (!EntityInfoCache.contains(entityId))
                EntityInfoCache.insert_or_assign(entityId, EntityInfo());
        }

        std::lock_guard<std::mutex> lock(TableLock);
        auto itr = ComponentTables.find(componentType);
        if (itr == ComponentTables.end())
            return nullptr;
        EntityInfoCache[entityId].ComponentTypes.insert(componentType);
        return itr->second->Add(entityId);
    }

    bool EntityExists(size_t entityId)
    {
        std::lock_guard<std::recursive_mutex> lock(EntityInfoLock);
        return EntityInfoCache.contains(entityId);
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
            DoForeachComponentOfEntity(entity, [](EntityComponent& componnent) { componnent.OnAwake(); });
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
 
        DoForeachComponentOfEntity(entityId, [enabled](EntityComponent& componnent)
            {
                if (enabled)
                    componnent.OnEnabled();
                else
                    componnent.OnDisabled();
            });
    }

    void AwakeEntity(size_t entityId)
    {
        {
            std::lock_guard<std::recursive_mutex> lock(EntityInfoLock);
            auto itr = EntityInfoCache.find(entityId);
            if (itr != EntityInfoCache.end())
                itr->second.Awake = true;
        }

        DoForeachComponentOfEntity(entityId, [](EntityComponent& componnent)
        {
            componnent.OnAwake();
        }
        );

        TraceLog(LOG_INFO, "Awake Entity %zu", entityId);
    }

    void ClearAllEntities()
    {
        {
            std::lock_guard<std::mutex> lock(TableLock);
            for (auto& [componentType, table] : ComponentTables)
            {
                table->Clear();
            }
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
            {
                std::lock_guard<std::mutex> lock(TableLock);
                for (auto& [componentType, table] : ComponentTables)
                {
                    table->Remove(entityId);
                }
            }
        }

        EntityMorgue.clear();
    }

    void DoForEachEntityWithComponent(size_t componentType, std::function<void(size_t&)> func, bool paralel, bool enabledOnly)
    {
        IComponentTable* table = nullptr;
        {
            std::lock_guard<std::mutex> lock(TableLock);
            auto itr = ComponentTables.find(componentType);
            if (itr == ComponentTables.end() || !func)
                return;

            table = itr->second.get();
        }

        table->DoForEach([&func](EntityComponent& component)
            {
                func(component.EntityID);
            }, paralel, enabledOnly);
    }

    void DoForEachComponent(size_t componentType, std::function<void(EntityComponent&)> func, bool paralel, bool enabledOnly)
    {
        IComponentTable* table = nullptr;
        {
            std::lock_guard<std::mutex> lock(TableLock);
            auto itr = ComponentTables.find(componentType);
            if (itr == ComponentTables.end() || !func)
                return;

            table = itr->second.get();
        }

        table->DoForEach([&func](EntityComponent& component)
            {
                func(component);
            }, paralel, enabledOnly);
    }

    void DoForeachComponentOfEntity(size_t entityId, std::function<void(EntityComponent&)> func)
    {
        std::set<size_t>* components = nullptr;
        {
            std::lock_guard<std::recursive_mutex> lock(EntityInfoLock);
            auto itr = EntityInfoCache.find(entityId);
            if (itr == EntityInfoCache.end())
                return;

            components = &itr->second.ComponentTypes;
        }
        for (auto componentType : *components)
        {
            auto comp = GetComponentTable(componentType)->TryGet(entityId);
            if (comp)
                func(*comp);
        }
    }
}
