#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <rapidjson/document.h>

// Serialization interface for components
namespace ComponentSerialization
{
    void SetupSerializers();
    // General dispatcher for serialization by component type name
    void Serialize(const std::string& type, const rapidjson::Value& j, std::vector<uint8_t>& out);
}
