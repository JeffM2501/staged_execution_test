#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <rapidjson/document.h>

// Serialization interface for components
namespace ComponentSerialization
{
    // Serialize TransformComponent from RapidJSON to binary
    void SerializeTransform(const rapidjson::Value& j, std::vector<uint8_t>& out);

    // Serialize PlayerComponent from RapidJSON to binary
    void SerializePlayer(const rapidjson::Value& j, std::vector<uint8_t>& out);

    // General dispatcher for serialization by component type name
    void Serialize(const std::string& type, const rapidjson::Value& j, std::vector<uint8_t>& out);
}