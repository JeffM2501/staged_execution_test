#include "EntityReader.h"
#include "EntitySystem.h"
#include "ResourceManager.h"
#include <set>

namespace EntityReader
{

    static constexpr uint32_t PrefabMagic = 0x50465242; // "PFRB" in ASCII
    static constexpr uint32_t PrefabVersion = 1;

    static constexpr uint32_t SceneMagic = 0x53434E45; // "SCNE" in ASCII
    static constexpr uint32_t SceneVersion = 1;


    std::vector<size_t> Reader::ReadEntities(BufferReader& reader)
    {
        std::vector<size_t> createdEntities;

        while (!reader.Done())
        {
            int64_t entityId = reader.Read<int64_t>();

            size_t realEnityId = static_cast<size_t>(entityId);
            if (entityId <= 0 || EntitySystem::EntityExists(entityId))
                realEnityId = EntitySystem::NewEntityId();

            uint32_t componentCount = reader.Read<uint32_t>();
            TraceLog(LOG_INFO, "Loaded Entity %zu with %d components", realEnityId, componentCount);

            createdEntities.push_back(realEnityId);
            for (size_t i = 0; i < componentCount; ++i)
            {
                uint64_t componentId = reader.Read<uint64_t>();
                uint32_t dataSize = reader.Read<uint32_t>();

                BufferReader componentData = reader.ReadBuffer(dataSize);

                EntitySystem::EntityComponent* component = EntitySystem::AddComponent(realEnityId, componentId);
                if (component)
                {
                    OnComponentData(component, componentId, componentData);
                }
            }
        }
        return createdEntities;
    }

    void Reader::ReadSceneFromResource(size_t resourceHash, OnEntityReadCallback onReadComplete)
    {
        using namespace ResourceManager;
        auto parseFile = [this, resourceHash, onReadComplete](const ResourceInfoRef& resource)
            {
                TraceLog(LOG_INFO, "Loading Scene Resource %zu", resourceHash);

                {
                    std::lock_guard<std::mutex> lock(resource->Lock);
                    const auto& dataVariant = resource->Data;
                    if (!std::holds_alternative<std::vector<unsigned char>>(dataVariant))
                    {
                        TraceLog(LOG_INFO, "Entity Resource %zu Invalid", resourceHash);
                    }

                    std::vector<size_t> createdEntities;

                    BufferReader reader(std::get<std::vector<unsigned char>>(dataVariant));

                    if (reader.Size() < sizeof(uint32_t) * 3)
                    {
                        TraceLog(LOG_INFO, "Entity Resource %zu Incorrect Size", resourceHash);
                        return;
                    }

                    auto magic = reader.Read<uint32_t>(); // magic
                    auto version = reader.Read<uint32_t>(); // version

                    if (magic == SceneMagic && version == SceneVersion)
                    {
                        createdEntities = ReadEntities(reader);
                    }

                    if (onReadComplete)
                        onReadComplete(createdEntities);

                    for (auto entityId : createdEntities)
                    {
                        TraceLog(LOG_INFO, "Waking Created Entity %zu", entityId);
                        EntitySystem::AwakeEntity(entityId);
                    }
                }

                TraceLog(LOG_INFO, "Releasing Scene Resource %zu", resourceHash);
                resource->Release();
            };
        TraceLog(LOG_INFO, "Loading Entity Resource %zu", resourceHash);
        LoadResource(resourceHash, ResourceType::File, parseFile);
    }

    void Reader::ReadEntitiesFromResource(size_t resourceHash, OnEntityReadCallback onReadComplete)
    {
        using namespace ResourceManager;
        auto parseFile = [this, resourceHash, onReadComplete](const ResourceInfoRef& resource)
            {
                TraceLog(LOG_INFO, "Loading Entity Resource %zu", resourceHash);

                uint32_t spawnable = 0;
                {
                    std::lock_guard<std::mutex> lock(resource->Lock);
                    const auto& dataVariant = resource->Data;
                    if (!std::holds_alternative<std::vector<unsigned char>>(dataVariant))
                    {
                        TraceLog(LOG_INFO, "Entity Resource %zu Invalid", resourceHash);
                    }

                    std::vector<size_t> createdEntities;

                    BufferReader reader(std::get<std::vector<unsigned char>>(dataVariant));

                    if (reader.Size() < sizeof(uint32_t) * 3)   
                    {
                        TraceLog(LOG_INFO, "Entity Resource %zu Incorrect Size", resourceHash);
                        return;
                    }

                    auto magic = reader.Read<uint32_t>(); // magic
                    auto version = reader.Read<uint32_t>(); // version
                    spawnable = reader.Read<uint32_t>(); // spwanable

                    if (magic == PrefabMagic && version == PrefabVersion)
                    {
                        createdEntities = ReadEntities(reader);
                    }

                    if (onReadComplete)
                        onReadComplete(createdEntities);

                    for (auto entityId : createdEntities)
                    {
                        TraceLog(LOG_INFO, "Waking Created Entity %zu", entityId);
                        EntitySystem::AwakeEntity(entityId);
                    }
                }

                // if the entity is not spawnable, release the resource
                if (spawnable == 0)
                {
                    TraceLog(LOG_INFO, "Releasing Entity Resource %zu", resourceHash);
                    resource->Release();
                }
            };
        TraceLog(LOG_INFO, "Loading Entity Resource %zu", resourceHash);
        LoadResource(resourceHash, ResourceType::File, parseFile);
    }
}