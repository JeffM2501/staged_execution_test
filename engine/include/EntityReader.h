#pragma once

#include <vector>
#include <span>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <stdexcept>
#include <functional>
#include "BufferReader.h"

#include "raylib.h"

namespace EntitySystem
{
    struct EntityComponent;
}

namespace EntityReader
{
    // EntityReader reads entity/component data from a binary file using the resource manager file type.
    class Reader
    {
    public:

        using OnEntityReadCallback = std::function<void(std::span<size_t> loadedEntities)>;

        // Reads entities and components from a resource file (ResourceManager).
        // Each entity is created with the ID from the file, and components are added by component ID.
        // For each component, the buffer is passed to OnComponentData for initialization.
        void ReadEntitiesFromResource(size_t resourceHash, OnEntityReadCallback onReadComplete = nullptr);

        // Reads entities and components from a resource file (ResourceManager).
        // Each entity is created with the ID from the file, and components are added by component ID.
        // For each component, the buffer is passed to OnComponentData for initialization.
        void ReadSceneFromResource(size_t resourceHash, OnEntityReadCallback onReadComplete = nullptr);

    protected:
        // Called for each created component, passing the buffer with component data.
        // Override this in derived classes to handle component-specific deserialization.
        virtual void OnComponentData(EntitySystem::EntityComponent* component, size_t componentId, BufferReader& buffer) = 0;

    private:
        std::vector<size_t> ReadEntities(BufferReader& reader);
    };

}