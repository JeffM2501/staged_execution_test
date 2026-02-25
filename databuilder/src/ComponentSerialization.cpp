#include "ComponentSerialization.h"

namespace ComponentSerialization
{
    void SerializeTransform(const rapidjson::Value& j, std::vector<uint8_t>& out)
    {
        float position[2] = { 0,0 };
        float velocity[2] = { 0,0 };
        auto pos = j.FindMember("Position");
        if (pos != j.MemberEnd() && pos->value.IsArray())
        {
            const auto& posArray = pos->value;
            for (rapidjson::SizeType i = 0; i < posArray.Size() && i < 2; ++i)
            {
                if (posArray[i].IsFloat())
                    position[i] = posArray[i].GetFloat();
            }
        }

        auto vel = j.FindMember("Velocity");
        if (vel != j.MemberEnd() && vel->value.IsArray())
        {
            const auto& velArray = vel->value;
            for (rapidjson::SizeType i = 0; i < velArray.Size() && i < 2; ++i)
            {
                if (velArray[i].IsFloat())
                    velocity[i] = velArray[i].GetFloat();
            }
        }
        WriteToOut(position, out);
        WriteToOut(velocity, out);
    }

    void SerializePlayer(const rapidjson::Value& j, std::vector<uint8_t>& out)
    {
        float size = 0;
        float health = 0;
        float playerSpeed = 0;
        float reloadTime = 0;

        auto sizeIt = j.FindMember("Size");
        if (sizeIt != j.MemberEnd() && sizeIt->value.IsNumber())
            size = sizeIt->value.GetFloat();

        auto healthIt = j.FindMember("Health");
        if (healthIt != j.MemberEnd() && healthIt->value.IsNumber())
            health = healthIt->value.GetFloat();

        auto speedIt = j.FindMember("PlayerSpeed");
        if (speedIt != j.MemberEnd() && speedIt->value.IsNumber())
            playerSpeed = speedIt->value.GetFloat();

        auto reloadIt = j.FindMember("ReloadTime");
        if (reloadIt != j.MemberEnd() && reloadIt->value.IsNumber())
            reloadTime = reloadIt->value.GetFloat();

        WriteToOut(size, out);
        WriteToOut(health, out);
        WriteToOut(playerSpeed, out);
        WriteToOut(reloadTime, out);
    }

    void Serialize(const std::string& type, const rapidjson::Value& j, std::vector<uint8_t>& out)
    {
        if (type == "TransformComponent")
            SerializeTransform(j, out);
        else if (type == "PlayerComponent")
            SerializePlayer(j, out);
        // Add more component types as needed
    }
}