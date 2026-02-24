#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <rapidjson/document.h>
#include "ComponentSerialization.h"
#include "CRC64.h"
#include <iostream>

namespace fs = std::filesystem;

int main()
{
    std::string inputFolder = "assets";
    std::string outputFolder = "resources/files";

    std::vector<uint8_t> binary;

    for (const auto& entry : fs::directory_iterator(inputFolder))
    {
        if (!entry.is_regular_file())
            continue;

        const auto& path = entry.path();
        if (path.extension() != ".json")
            continue;

        std::ifstream in(path);
        std::string jsonStr((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        rapidjson::Document prefab;
        prefab.Parse(jsonStr.c_str());

        binary.clear();

        auto entityList = prefab.FindMember("Entities");
        if (entityList == prefab.MemberEnd() || !entityList->value.IsArray())
        {
            std::cerr << "Invalid prefab format in file: " << path << std::endl;
            continue;
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

                WriteToOut(entityId, binary);
                WriteToOut(componentCount, binary);
               
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

                    WriteToOut(componentId, binary);

                    std::vector<uint8_t> compData;
                    ComponentSerialization::Serialize(type, compValue, compData);

                    uint32_t dataSize = static_cast<uint32_t>(compData.size());
                    binary.insert(binary.end(), reinterpret_cast<uint8_t*>(&dataSize), reinterpret_cast<uint8_t*>(&dataSize) + sizeof(dataSize));
                    binary.insert(binary.end(), compData.begin(), compData.end());
                }
            }
        }

        std::string outputPath = outputFolder + "/" + std::to_string(Hashes::CRC64Str(path.stem().string())) + ".bin";
        std::ofstream out(outputPath, std::ios::binary);
        out.write(reinterpret_cast<const char*>(binary.data()), binary.size());
        out.close();
    }

    return 0;
}