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

template<typename T>
inline void WriteToOut(const T& value, std::vector<uint8_t>& out)
{
    out.insert(out.end(),
        reinterpret_cast<const uint8_t*>(&value),
        reinterpret_cast<const uint8_t*>(&value) + sizeof(T));
}