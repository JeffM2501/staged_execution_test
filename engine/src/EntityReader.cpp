#include "EntityReader.h"
#include "EntitySystem.h"
#include "ResourceManager.h"
#include <set>

namespace EntityReader
{
    void Reader::ReadEntitiesFromResource(size_t resourceHash)
    {
        using namespace ResourceManager;
        auto parseFile = [this](const ResourceInfoRef& resource)
            {
                std::lock_guard<std::mutex> lock(resource->Lock);
                const auto& dataVariant = resource->Data;
                if (!std::holds_alternative<std::vector<unsigned char>>(dataVariant))
                    return;

                BufferReader reader(std::get<std::vector<unsigned char>>(dataVariant));
        
                std::set<size_t> createdEntities;
                while (!reader.Done())
                {
                    int64_t entityId = reader.Read<int64_t>();

                    size_t realEnityId = static_cast<size_t>(entityId);
                    if (entityId <= 0)
                        realEnityId = EntitySystem::NewEntityId();
      
                    uint32_t componentCount = reader.Read<uint32_t>();
                
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
            };

        LoadResource(resourceHash, ResourceType::File, parseFile);
    }
}