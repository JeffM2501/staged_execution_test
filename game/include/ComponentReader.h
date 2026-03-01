#pragma once

#include "EntityReader.h"

class ComponentReader : public EntityReader::Reader
{
protected:
    void OnComponentData(EntitySystem::EntityComponent* component, size_t componentId, EntityReader::BufferReader& buffer) override;
};