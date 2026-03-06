#pragma once

#include "EntityReader.h"
#include "BufferReader.h"

class ComponentReader : public EntityReader::Reader
{
protected:
    void OnComponentData(EntitySystem::EntityComponent* component, size_t componentId, BufferReader& buffer) override;
};