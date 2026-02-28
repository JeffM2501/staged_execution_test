#pragma once

#include <vector>
#include <string>
#include <span>
#include <cstdint>
#include <rapidjson/document.h>

// Serialization interface for components
namespace ComponentSerialization
{
    void SetupSerializers();
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

template<typename T>
inline void WriteArrayToOut(const std::span<T> & value, std::vector<uint8_t>& out)
{
    for (auto c : value)
    {
        out.insert(out.end(),
            reinterpret_cast<const uint8_t*>(&c),
            reinterpret_cast<const uint8_t*>(&c) + sizeof(T));
    }
}

