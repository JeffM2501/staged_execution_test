#include "ComponentSerialization.h"

#include "rapidjson/rapidjson.h"

#include <unordered_map>
#include <functional>

namespace ComponentSerialization
{

    bool ReadColor(uint8_t color[4], const rapidjson::Value& colorValue)
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
    bool ReadValueNumber(std::string_view name, T& out, const rapidjson::Value& value)
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
    bool ReadValueNumberArray(std::string_view name, std::span<T> out, const rapidjson::Value& value)
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
    void SerializeNumber(std::string_view name, T defaultValue, const rapidjson::Value& value, std::vector<uint8_t>& out)
    {
        T binValue = defaultValue;
        ReadValueNumber(name, binValue, value);
        WriteToOut(binValue, out);
    }

    template<typename T>
    void SerializeNumberArray(std::string_view name, const std::vector<T>& defaultValue, const rapidjson::Value& value, std::vector<uint8_t>& out)
    {
        // Make a mutable copy initialized from the provided default.
        std::vector<T> binValue = defaultValue;

        // Create a span over the vector so ReadValueNumberArray can fill it.
        std::span<T> binSpan(binValue);

        // Read values from JSON into our span (will overwrite elements up to span.size()).
        ReadValueNumberArray<T>(name, binSpan, value);

        // Serialize the resulting numeric array to output.
        WriteToOut(binValue, out);
    }


    void SerializeColor(std::string_view name, const std::vector<uint8_t>& defaultValue, const rapidjson::Value& value, std::vector<uint8_t>& out)
    {
        // Make a mutable copy initialized from the provided default.
        std::vector<uint8_t> binValue = defaultValue;

        auto it = value.FindMember(name.data());
        if (it != value.MemberEnd() && it->value.IsArray())
        {
            const auto& valueArray = it->value;
            for (rapidjson::SizeType i = 0; i < valueArray.Size() && i < out.size(); ++i)
            {
                if (valueArray[i].IsNumber())
                    binValue[i] = uint8_t(valueArray[i].GetUint());
            }
        }

        // Serialize the resulting numeric array to output.
        WriteToOut(binValue, out);
    }


    void SerializeTransform(const rapidjson::Value& j, std::vector<uint8_t>& out)
    {
        SerializeNumberArray<float>("Position", { 0,0 }, j, out);
        SerializeNumberArray<float>("Velocity", { 0,0 }, j, out);
    }

    void SerializePlayer(const rapidjson::Value& j, std::vector<uint8_t>& out)
    {
        SerializeNumber("Size", 10.0f, j, out);
        SerializeNumber("Health", 100.0f, j, out);
        SerializeNumber("PlayerSpeed", 100.0f, j, out);
        SerializeNumber("ReloadTime", 0.25f, j, out);
    }

    void SerializeNPC(const rapidjson::Value& j, std::vector<uint8_t>& out)
    {
        SerializeNumber("Size", 20.0f, j, out);
        SerializeColor("Tint", { 0,0,255,255 }, j, out);
    }

    void SerializeBullet(const rapidjson::Value& j, std::vector<uint8_t>& out)
    {
        SerializeNumber("Size", 4.0f, j, out);
        SerializeNumber("Damage", 10.0f, j, out);
        SerializeNumber("Lifetime", 1.0f, j, out);
        SerializeColor("Tint", { 255,255,0,255 }, j, out);
    }

    std::unordered_map<std::string, std::function<void(const rapidjson::Value&, std::vector<uint8_t>&)>> Serializers;
    void SetupSerializers()
    {
        Serializers["TransformComponent"] = SerializeTransform;
        Serializers["PlayerComponent"] = SerializePlayer;
        Serializers["NPCComponent"] = SerializeNPC;
        Serializers["BulletComponent"] = SerializeBullet;
    }

    void Serialize(const std::string& type, const rapidjson::Value& j, std::vector<uint8_t>& out)
    {
        auto itr = Serializers.find(type);
        if (itr != Serializers.end())
        {
            itr->second(j, out);
        }
    }
}