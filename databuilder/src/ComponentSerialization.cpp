#include "ComponentSerialization.h"

#include "SerializationUtils.h"

#include <unordered_map>
#include <functional>

namespace ComponentSerialization
{
    void SeralizeSpriteReference(const std::string& name, const rapidjson::Value& value, BufferWriter& out)
    {
        auto spriteIt = value.FindMember(name.c_str());
        if (spriteIt != value.MemberEnd() && spriteIt->value.IsObject())
        {
            SeralizeAssetReference("Sheet", spriteIt->value, out);
            SerializeNumber<uint32_t>("Frame", 0, spriteIt->value, out);
            SerializeNumber<float>("Rotation", 0, spriteIt->value, out);
        }
        else
        {
            out.Write(size_t(0)); // default to no sprite
            out.Write(uint32_t(0)); // default to no sprite
            out.Write(float(0)); // default to no sprite
        }
    }

    void SerializeTransform(const rapidjson::Value& j, BufferWriter& out)
    {
        SerializeNumberArray<float>("Position", { 0,0 }, j, out);
        SerializeNumberArray<float>("Velocity", { 0,0 }, j, out);
    }

    void SerializePlayer(const rapidjson::Value& j, BufferWriter& out)
    {
        SerializeNumber("Size", 10.0f, j, out);
        SerializeNumber("Health", 100.0f, j, out);
        SerializeNumber("PlayerSpeed", 100.0f, j, out);
        SerializeNumber("ReloadTime", 0.25f, j, out);
        SeralizeAssetReference("BulletPrefab", j, out);
        SerializeNumber("BulletSpreadDelta", 50.0f, j, out);
        SerializeNumber("BulletSpeedMultiplier", 3.0f, j, out);
        SerializeNumber("BulletSpeedVariance", 1.0f, j, out);
        SeralizeSpriteReference("Sprite", j, out);
    }

    void SerializeNPC(const rapidjson::Value& j, BufferWriter& out)
    {
        SerializeNumber("Size", 20.0f, j, out);
        SerializeColor("Tint", { 0,0,255,255 }, j, out);
        SeralizeSpriteReference("Sprite", j, out);
    }

    void SerializeBullet(const rapidjson::Value& j, BufferWriter& out)
    {
        SerializeNumber("Size", 4.0f, j, out);
        SerializeNumber("Damage", 10.0f, j, out);
        SerializeNumber("Lifetime", 1.0f, j, out);
        SerializeColor("Tint", { 255,255,0,255 }, j, out);
        SeralizeSpriteReference("Sprite", j, out);
    }

    void SerializePlayerSpawn(const rapidjson::Value& j, BufferWriter& out)
    {
        SeralizeAssetReference("PlayerPrefab", j, out);
    }

    void SerializeNPCSpawn(const rapidjson::Value& j, BufferWriter& out)
    {
        SerializeNumberArray<float>("Interval", { 1, 3 }, j, out);
        SerializeNumberArray<float>("Velocity", { 20, 100 }, j, out);
        SerializeNumber<uint32_t>("MaxSpawnCount", 100, j, out);
        SeralizeAssetReference("NPCPrefab", j, out);
    }

    std::unordered_map<std::string, std::function<void(const rapidjson::Value&, BufferWriter&)>> Serializers;
    void SetupSerializers()
    {
        Serializers["TransformComponent"] = SerializeTransform;
        Serializers["PlayerComponent"] = SerializePlayer;
        Serializers["NPCComponent"] = SerializeNPC;
        Serializers["BulletComponent"] = SerializeBullet;
        Serializers["PlayerSpawnComponent"] = SerializePlayerSpawn;
        Serializers["NPCSpawnComponent"] = SerializeNPCSpawn;
    }

    void Serialize(const std::string& type, const rapidjson::Value& j, BufferWriter& out)
    {
        auto itr = Serializers.find(type);
        if (itr != Serializers.end())
        {
            itr->second(j, out);
        }
    }
}