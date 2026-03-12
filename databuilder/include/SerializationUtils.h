#pragma once

#include <vector>
#include <string_view>
#include <span>
#include <cstdint>
#include <rapidjson/document.h>

#include "CRC64.h"


class BufferWriter
{
public:
    std::vector<uint8_t> Buffer;

    template<typename T>
    inline void Write(const T& value)
    {
        Buffer.insert(Buffer.end(),
            reinterpret_cast<const uint8_t*>(&value),
            reinterpret_cast<const uint8_t*>(&value) + sizeof(T));
    }

    template<typename T>
    inline void WriteArray(const std::vector<T>& value)
    {
        for (auto c : value)
        {
            Buffer.insert(Buffer.end(),
                reinterpret_cast<const uint8_t*>(&c),
                reinterpret_cast<const uint8_t*>(&c) + sizeof(T));
        }
    }

    inline void WriteBufferr(BufferWriter& buffer)
    {
        uint32_t size = uint32_t(buffer.Buffer.size());
        Write(size);
        Buffer.insert(Buffer.end(), buffer.Buffer.begin(), buffer.Buffer.end());
    }
};

inline bool ReadColor(uint8_t color[4], const rapidjson::Value& colorValue)
{
    if (colorValue.IsArray())
    {
        const auto& colorArray = colorValue;
        for (rapidjson::SizeType i = 0; i < colorArray.Size() && i < 4; ++i)
        {
            if (colorArray[i].IsUint())
                color[i] = static_cast<uint8_t>(colorArray[i].GetUint());
        }
        return true;
    }
    return false;
}

template<typename T>
inline bool ReadValueNumber(std::string_view name, T& out, const rapidjson::Value& value)
{
    auto it = value.FindMember(name.data());
    if (it != value.MemberEnd() && it->value.IsNumber())
    {
        out = static_cast<T>(it->value.Get<T>());
        return true;
    }
    return false;
}

template<typename T>
inline bool ReadValueNumberArray(std::string_view name, std::span<T> out, const rapidjson::Value& value)
{
    auto it = value.FindMember(name.data());
    if (it != value.MemberEnd() && it->value.IsArray())
    {
        const auto& valueArray = it->value;
        for (rapidjson::SizeType i = 0; i < valueArray.Size() && i < out.size(); ++i)
        {
            if (valueArray[i].IsNumber())
                out[i] = valueArray[i].Get<T>();
        }
        return true;
    }
    return false;
}

template<typename T>
inline void SerializeNumber(std::string_view name, T defaultValue, const rapidjson::Value& value, BufferWriter& out)
{
    T binValue = defaultValue;
    ReadValueNumber(name, binValue, value);
    out.Write(binValue);
}

template<typename T>
inline void SerializeNumberArray(std::string_view name, const std::vector<T>& defaultValue, const rapidjson::Value& value, BufferWriter& out)
{
    // Make a mutable copy initialized from the provided default.
    std::vector<T> binValue = defaultValue;

    // Create a span over the vector so ReadValueNumberArray can fill it.
    std::span<T> binSpan(binValue);

    // Read values from JSON into our span (will overwrite elements up to span.size()).
    ReadValueNumberArray<T>(name, binSpan, value);

    // Serialize the resulting numeric array to output.
    out.WriteArray<T>(binValue);
}

inline void SerializeColor(std::string_view name, const std::vector<uint8_t>& defaultValue, const rapidjson::Value& value, BufferWriter& out)
{
    // Make a mutable copy initialized from the provided default.
    std::vector<uint8_t> binValue = defaultValue;

    auto it = value.FindMember(name.data());
    if (it != value.MemberEnd() && it->value.IsArray())
    {
        const auto& valueArray = it->value;
        for (rapidjson::SizeType i = 0; i < valueArray.Size() && i < defaultValue.size(); ++i)
        {
            if (valueArray[i].IsNumber())
                binValue[i] = uint8_t(valueArray[i].GetUint());
        }
    }

    // Serialize the resulting numeric array to output.
    out.WriteArray<uint8_t>(binValue);
}

inline void SerializeRectangle(std::string_view name, const std::vector<float>& defaultValue, const rapidjson::Value& value, BufferWriter& out)
{
    // Make a mutable copy initialized from the provided default.
    std::vector<float> binValue = defaultValue;

    auto it = value.FindMember(name.data());
    if (it != value.MemberEnd() && it->value.IsArray())
    {
        const auto& valueArray = it->value;
        for (rapidjson::SizeType i = 0; i < valueArray.Size() && i < defaultValue.size(); ++i)
        {
            if (valueArray[i].IsNumber())
                binValue[i] = valueArray[i].GetFloat();
        }
    }

    // Serialize the resulting numeric array to output.
    out.WriteArray(binValue);
}

inline void SeralizeAssetReference(std::string_view name, const rapidjson::Value& value, BufferWriter& out)
{
    // Make a mutable copy initialized from the provided default.
    size_t nameValue = 0;

    auto it = value.FindMember(name.data());
    if (it != value.MemberEnd() && it->value.IsString())
    {
        nameValue = Hashes::CRC64Str(it->value.GetString());
    }

    // Serialize the resulting numeric array to output.
    out.Write<size_t>(nameValue);
}

template<typename T>
inline T ReadJsonMemberValue(std::string_view name, const rapidjson::Value& value, const T& defaultValue)
{
    T numberValue = defaultValue;
    auto it = value.FindMember(name.data());
    if (it != value.MemberEnd())
    {
        numberValue = static_cast<T>(it->value.Get<T>());
    }
    return numberValue;
}