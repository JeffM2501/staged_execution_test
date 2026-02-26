#include "EntityReader.h"
#include "EntitySystem.h"
#include "ResourceManager.h"
#include <set>

namespace EntityReader
{

    static constexpr uint32_t Magic = 0x50465242; // "PFRB" in ASCII
    static constexpr uint32_t Version = 1;


    void Reader::ReadEntitiesFromResource(size_t resourceHash)
    {
        using namespace ResourceManager;
        auto parseFile = [this, resourceHash](const ResourceInfoRef& resource)
            {
                TraceLog(LOG_INFO, "Loaded Entity Resource %zu", resourceHash);

                uint32_t spawnable = 0;
                {
                    std::lock_guard<std::mutex> lock(resource->Lock);
                    const auto& dataVariant = resource->Data;
                    if (!std::holds_alternative<std::vector<unsigned char>>(dataVariant))
                        return;

                    std::set<size_t> createdEntities;

                    BufferReader reader(std::get<std::vector<unsigned char>>(dataVariant));

                    auto magic = reader.Read<uint32_t>(); // magic
                    auto version = reader.Read<uint32_t>(); // version
                    spawnable = reader.Read<uint32_t>(); // spwanable

                    if (magic == Magic && version == Version)
                    {
                        while (!reader.Done())
                        {
                            int64_t entityId = reader.Read<int64_t>();

                            size_t realEnityId = static_cast<size_t>(entityId);
                            if (entityId <= 0)
                                realEnityId = EntitySystem::NewEntityId();

                            uint32_t componentCount = reader.Read<uint32_t>();
                            TraceLog(LOG_INFO, "Loaded Entity %zu with %d components from resource %zu", realEnityId, componentCount, resourceHash);

                            createdEntities.insert(realEnityId);
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
                    }

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