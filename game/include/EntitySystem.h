#pragma once

#include "CRC64.h"

#include <functional>
#include <memory>
#include <algorithm>
#include <execution>

#define DECLARE_COMPONENT(CompoentName) \
 static size_t GetComponentId() { return Hashes::CRC64Str(#CompoentName); }

#define DECLARE_SIMPLE_COMPONENT(CompoentName) \
 static size_t GetComponentId() { return Hashes::CRC64Str(#CompoentName); } \
 CompoentName(size_t entityId) : EntityComponent(entityId) {}


namespace EntitySystem
{
    struct EntityComponent;
   

    struct IComponentTable
    {
        virtual EntityComponent* Add(size_t id) = 0;
        virtual void Remove(size_t id) = 0;
        virtual bool HasEntity(size_t id) = 0;
        virtual EntityComponent* Get(size_t id) = 0;
        virtual EntityComponent* TryGet(size_t id) = 0;
        virtual void Clear() = 0;

        virtual size_t GetComponentType() const = 0;

        virtual void DoForEach(std::function<void(EntityComponent&)> func, bool paralel = false) = 0;

        virtual ~IComponentTable() = default;
    };

    template<class T>
    struct ComponentTable : public IComponentTable
    {
        std::vector<T> Components;
        std::unordered_map<size_t, size_t> ComponentsByID;

        size_t GetComponentType() const override { return T::GetComponentId(); }

        EntityComponent* Add(size_t id) override
        {
            Components.emplace_back(id);
            ComponentsByID[id] = Components.size() - 1;
            return &Components.back();
        }

        template<class... Args>
        T* Add(size_t id, Args&&... args)
        {
            Components.emplace_back(id, std::forward<Args>(args)...);
            ComponentsByID[id] = Components.size() - 1;
            return &Components.back();
        }

        void Remove(size_t id) override
        {
            auto itr = ComponentsByID.find(id);
            if (itr == ComponentsByID.end())
                return;
            size_t index = itr->second;

            std::swap(Components[index], Components.back());
            Components.pop_back();
            ComponentsByID[Components[index].EntityID] = index;
            ComponentsByID.erase(itr);
        }

        void Clear() override
        {
            Components.clear();
            ComponentsByID.clear();
        }

        bool HasEntity(size_t id) override
        {
            return ComponentsByID.contains(id);
        }
        
        EntityComponent* Get(size_t id) override
        {
            auto itr = ComponentsByID.find(id);
            if (itr == ComponentsByID.end())
                return Add(id);

            return &Components[itr->second];
        }

        EntityComponent* TryGet(size_t id) override
        {
            auto itr = ComponentsByID.find(id);
            if (itr == ComponentsByID.end())
                return nullptr;

            return &Components[itr->second];
        }

        void DoForEach(std::function<void(EntityComponent&)> func, bool paralel = false)
        {
            if (paralel)
            {
                std::for_each(std::execution::par, Components.begin(), Components.end(), [func]
                (auto& component)
                    {
                        func(component);
                    });
            }
            else
            {
                std::for_each(Components.begin(), Components.end(), [func]
                (auto& component)
                    {
                        func(component);
                    });
            }
        }

        void DoForEach(std::function<void(T&)> func, bool paralel = false)
        {
            DoForEach([&func](EntityComponent& component)
            {
                    func(static_cast<T&>(component));
            }, paralel);
        }
    };

    void DoForEachEntityWithComponent(size_t componentType, std::function<void(size_t&)> func, bool paralel = false);
    void DoForEachComponent(size_t componentType, std::function<void(EntityComponent&)> func, bool paralel = false);
    
    template<class T>
    void DoForEachComponent(std::function<void(T&)> func, bool paralel = false)
    {
        DoForEachComponent(T::GetComponentId(), [&func](EntityComponent& component)
        {
                func(static_cast<T&>(component));
        }, paralel);
    }

    EntityComponent* GetEntityComponent(size_t entityId, size_t componentType);

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
    T* GetComponentTable()
    {
        auto* table = GetComponentTable(T::GetComponentId());
        if (table->GetComponentType() != T::GetComponentId())
            return nullptr;

        return static_cast<T*>(table);
    }

    EntityComponent* AddComponent(size_t entityId, size_t componentType);

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

    void RemoveEntity(size_t entityId);

    void ClearAllEntities();

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
}