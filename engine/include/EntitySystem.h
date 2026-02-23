#pragma once

#include "CRC64.h"
#include "ResourceManager.h"

#include <functional>
#include <memory>
#include <algorithm>
#include <execution>
#include <mutex>
#include <vector>

#define DECLARE_COMPONENT(CompoentName) \
static size_t GetComponentId() { return Hashes::CRC64Str(#CompoentName); }

#define DECLARE_SIMPLE_COMPONENT(CompoentName) \
static size_t GetComponentId() { return Hashes::CRC64Str(#CompoentName); } \
CompoentName(size_t entityId) : EntityComponent(entityId) {}

namespace EntitySystem
{
    struct EntityComponent;

    EntityComponent* AddComponent(size_t entityId, size_t componentType);
    EntityComponent* GetEntityComponent(size_t entityId, size_t componentType);

    bool IsEntityReady(size_t entityId);
    bool IsEntityEnabled(size_t entityId);

    void EnableEntity(size_t entityId, bool enabled);
    void AwakeEntity(size_t entityId);

    struct EntityComponent
    {
        size_t EntityID = 0;

        EntityComponent(size_t entityId)
            : EntityID(entityId)
        {
        }

        template<class T>
        T* AddComponent()
        {
            return static_cast<T*>(EntitySystem::AddComponent(EntityID, T::GetComponentId()));
        }

        template<class T>
        T* GetEntityComponent()
        {
            return static_cast<T*>(EntitySystem::GetEntityComponent(EntityID, T::GetComponentId()));
        }

        template<class T>
        bool EntityHasComponent()
        {
            return static_cast<T*>(EntitySystem::GetEntityComponent(EntityID, T::GetComponentId()));
        }
    };
   
    struct IComponentTable
    {
        virtual EntityComponent* Add(size_t id) = 0;
        virtual void Remove(size_t id) = 0;
        virtual bool HasEntity(size_t id) = 0;
        virtual EntityComponent* Get(size_t id) = 0;
        virtual EntityComponent* TryGet(size_t id) = 0;
        virtual void Clear() = 0;

        virtual size_t GetComponentType() const = 0;

        virtual void DoForEach(std::function<void(EntityComponent&)> func, bool paralel = false, bool enabledOnly = true) = 0;

        virtual ~IComponentTable() = default;

        std::recursive_mutex ItteratorLock;
    };

    template<class T>
    struct ComponentTable : public IComponentTable
    {
        std::vector<T> Components;
        std::unordered_map<size_t, size_t> ComponentsByID;

        size_t GetComponentType() const override { return T::GetComponentId(); }

        EntityComponent* Add(size_t id) override
        {
            std::lock_guard<std::recursive_mutex> lock(ItteratorLock);
            Components.emplace_back(id);
            ComponentsByID[id] = Components.size() - 1;
            return &Components.back();
        }

        template<class... Args>
        T* Add(size_t id, Args&&... args)
        {
            std::lock_guard<std::recursive_mutex> lock(ItteratorLock);
            Components.emplace_back(id, std::forward<Args>(args)...);
            ComponentsByID[id] = Components.size() - 1;
            return &Components.back();
        }

        void Remove(size_t id) override
        {
            std::lock_guard<std::recursive_mutex> lock(ItteratorLock);

            auto itr = ComponentsByID.find(id);
            if (itr == ComponentsByID.end())
                return;
            size_t index = itr->second;

            // it's the tail
            if (index == Components.size() - 1)
            {
                Components.pop_back();
                ComponentsByID.erase(itr);
                return;
            }

            // it's in the middle, swap and erase
            std::swap(Components[index], Components.back());
            Components.pop_back();
            if (!Components.empty())
            {
                ComponentsByID[Components[index].EntityID] = index;
            }
            ComponentsByID.erase(itr);
        }

        void Clear() override
        {
            std::lock_guard<std::recursive_mutex> lock(ItteratorLock);
            Components.clear();
            ComponentsByID.clear();
        }

        bool HasEntity(size_t id) override
        {
            std::lock_guard<std::recursive_mutex> lock(ItteratorLock);
            return ComponentsByID.contains(id);
        }
        
        EntityComponent* Get(size_t id) override
        {
            std::lock_guard<std::recursive_mutex> lock(ItteratorLock);
            auto itr = ComponentsByID.find(id);
            if (itr == ComponentsByID.end())
                return Add(id);

            return &Components[itr->second];
        }

        EntityComponent* TryGet(size_t id) override
        {
            std::lock_guard<std::recursive_mutex> lock(ItteratorLock);
            auto itr = ComponentsByID.find(id);
            if (itr == ComponentsByID.end())
                return nullptr;

            return &Components[itr->second];
        }

        void DoForEach(std::function<void(EntityComponent&)> func, bool paralel = false, bool enabledOnly = true) override
        {
            std::lock_guard<std::recursive_mutex> lock(ItteratorLock);
            if (paralel)
            {
                std::for_each(std::execution::par, Components.begin(), Components.end(), [func, enabledOnly]
                (auto& component)
                    {
                        if (!enabledOnly || IsEntityEnabled(component.EntityID))
                            func(component);
                    });
            }
            else
            {
                std::for_each(Components.begin(), Components.end(), [func, enabledOnly]
                (auto& component)
                    {
                        if (!enabledOnly || IsEntityEnabled(component.EntityID))
                            func(component);
                    });
            }
        }

        void DoForEach(std::function<void(T&)> func, bool paralel = false, bool enabledOnly = true)
        {
            std::lock_guard<std::mutex> lock(ItteratorLock);
            DoForEach([func, enabledOnly](EntityComponent& component)
            {
                func(static_cast<T&>(component));
            }, paralel, enabledOnly);
        }
    };

    void Init();

    void DoForEachEntityWithComponent(size_t componentType, std::function<void(size_t&)> func, bool paralel = false, bool enabledOnly = true);
    void DoForEachComponent(size_t componentType, std::function<void(EntityComponent&)> func, bool paralel = false, bool enabledOnly = true);
    
    template<class T>
    void DoForEachComponent(std::function<void(T&)> func, bool paralel = false, bool enabledOnly = true)
    {
        DoForEachComponent(T::GetComponentId(), [&func](EntityComponent& component)
        {
                func(static_cast<T&>(component));
        }, paralel, enabledOnly);
    }

    bool EntityHasComponent(size_t entityId, size_t componentType);

    template<class T>
    T* GetEntityComponent(size_t entityId)
    {
        return static_cast<T*>(GetEntityComponent(entityId, T::GetComponentId()));
    }

    template<class T>
    bool EntityHasComponent(size_t entityId)
    {
        return EntityHasComponent(entityId, T::GetComponentId());
    }

    void RegisterComponent(size_t compnentType, std::unique_ptr<IComponentTable> table);

    template<class T>
    void RegisterComponent()
    {
        RegisterComponent(T::GetComponentId(), std::move(std::make_unique<ComponentTable<T>>()));
    }

    IComponentTable* GetComponentTable(size_t componentType);

    template<class T>
    ComponentTable<T>* GetComponentTable()
    {
        auto* table = GetComponentTable(T::GetComponentId());
        if (table->GetComponentType() != T::GetComponentId())
            return nullptr;

        return static_cast<ComponentTable<T>*>(table);
    }

    template<class T>
    T* GetFirstComponentOfType()
    {
        ComponentTable<T>* table = GetComponentTable<T>();
        if (!table || table->Components.empty())
            return nullptr;

        return &table->Components.front();
    }

    template<class T, class... Args>
    T* AddComponent(size_t entityId, Args&&... args)
    {
        ComponentTable<T>* table = GetComponentTable<T>();
        if (!table)
            return nullptr;
        return table->Add(entityId, std::forward<Args>(args)...);
    }

    size_t NewEntityId();

    template<class T>
    T* AddComponent(size_t entityId)
    {
        return static_cast<T*>(AddComponent(entityId, T::GetComponentId()));
    }

    void AwakeAllEntities();

    void RemoveEntity(size_t entityId);

    void ClearAllEntities();

    void FlushMorgue();

    // EntityReader reads entity/component data from a binary file using the resource manager file type.
    class EntityReader
    {
    public:
        // Reads entities and components from a resource file (ResourceManager).
        // Each entity is created with the ID from the file, and components are added by component ID.
        // For each component, the buffer is passed to OnComponentData for initialization.
        void ReadEntitiesFromResource(size_t resourceHash);

    protected:
        // Called for each created component, passing the buffer with component data.
        // Override this in derived classes to handle component-specific deserialization.
        virtual void OnComponentData(EntityComponent* component, const std::vector<uint8_t>& buffer) = 0;
    };
}