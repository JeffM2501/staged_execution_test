#include "PrefabSerializer.h"
#include "ComponentSerialization.h"

#include <iostream>

static constexpr uint32_t PrefabMagic = 0x50465242; // "PFRB" in ASCII
static constexpr uint32_t PrefabVersion = 1;

void SerializePrefab(BufferWriter& buffer, rapidjson::Document& prefab)
{
    int spawnable = 0;

    auto info = prefab.FindMember("Info");
    if (info != prefab.MemberEnd() && info->value.IsObject())
    {
        auto spawnableIt = info->value.FindMember("spawnable");
        if (spawnableIt != prefab.MemberEnd() && spawnableIt->value.IsBool())
            spawnable = spawnableIt->value.GetBool() ? 1 : 0;
    }

    // header
    buffer.Write(PrefabMagic);
    buffer.Write(PrefabVersion);
    buffer.Write(spawnable);

    auto entityList = prefab.FindMember("Entities");
    if (entityList == prefab.MemberEnd() || !entityList->value.IsArray())
    {
        std::cerr << "Invalid prefab format in file: " << std::endl;
        return;
    }

    for (auto& entity : entityList->value.GetArray())
    {
        int64_t entityId = entity["ID"].GetInt64();
        auto componentList = entity.FindMember("Components");

        if (componentList->value.IsObject())
        {
            uint32_t componentCount = 0;
            for (auto& m : componentList->value.GetObject())
                ++componentCount;

            buffer.Write(entityId);
            buffer.Write(componentCount);

            for (auto& m : componentList->value.GetObject())
            {
                const char* name = m.name.GetString();
                const rapidjson::Value& compValue = m.value;

                // Use member name as type if the component doesn't include a "Type" field
                std::string type;
                auto typeIt = compValue.FindMember("Type");
                if (typeIt != compValue.MemberEnd() && typeIt->value.IsString())
                    type = typeIt->value.GetString();
                else
                    type = name;

                uint64_t componentId = Hashes::CRC64Str(type);

                buffer.Write(componentId);

                BufferWriter compData;
                ComponentSerialization::Serialize(type, compValue, compData);

                buffer.WriteBufferr(compData);
            }
        }
    }
}