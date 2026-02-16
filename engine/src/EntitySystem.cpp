#include "EntitySystem.h"
#include "TaskManager.h"
#include "FrameStage.h"

#include <memory>
#include <unordered_map>
#include <set>

namespace EntitySystem
{
    std::recursive_mutex MorgueLock;

    std::set<size_t> EntityMorgue;
    static std::unordered_map<size_t, std::unique_ptr<IComponentTable>> ComponentTables;
    size_t NextEntityId = 1;

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
        auto itr = ComponentTables.find(componentType);
        if (itr == ComponentTables.end())
            return nullptr;
        return itr->second->Add(entityId);
    }

    void RemoveEntity(size_t entityId)
    {
        std::lock_guard<std::recursive_mutex> lock(MorgueLock);
        EntityMorgue.insert(entityId);
    }

    void ClearAllEntities()
    {
        for (auto& [componentType, table] : ComponentTables)
        {
            table->Clear();
        }
    }

    void FlushMorgue()
    {
        std::lock_guard<std::recursive_mutex> lock(MorgueLock);
        for (size_t entityId : EntityMorgue)
        {
            for (auto& [componentType, table] : ComponentTables)
            {
                table->Remove(entityId);
            }
        }

        EntityMorgue.clear();
    }

    void DoForEachEntityWithComponent(size_t componentType, std::function<void(size_t&)> func, bool paralel)
    {
        auto itr = ComponentTables.find(componentType);
        if (itr == ComponentTables.end() || !func)
            return;

        itr->second->DoForEach([&func](EntityComponent& component)
            {
                func(component.EntityID);
            }, paralel);
    }

    void DoForEachComponent(size_t componentType, std::function<void(EntityComponent&)> func, bool paralel)
    {
        auto itr = ComponentTables.find(componentType);
        if (itr == ComponentTables.end() || !func)
            return;

        itr->second->DoForEach([&func](EntityComponent& component)
            {
                func(component);
            }, paralel);
    }
}
