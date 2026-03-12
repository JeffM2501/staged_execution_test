#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <rapidjson/document.h>

#include "SerializationUtils.h"

// Serialization interface for components
namespace ComponentSerialization
{
    void SetupSerializers();
    // General dispatcher for serialization by component type name
    void Serialize(const std::string& type, const rapidjson::Value& j, BufferWriter& out);
}
