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
            size_t componentCount = entity["Components"].Size();

            binary.insert(binary.end(), reinterpret_cast<uint8_t*>(&entityId), reinterpret_cast<uint8_t*>(&entityId) + sizeof(entityId));
            binary.insert(binary.end(), reinterpret_cast<uint8_t*>(&componentCount), reinterpret_cast<uint8_t*>(&componentCount) + sizeof(componentCount));

            for (auto& comp : entity["Components"].GetArray())
            {
                std::string type = comp["Type"].GetString();
                size_t componentId = Hashes::CRC64Str(type.c_str());

                binary.insert(binary.end(), reinterpret_cast<uint8_t*>(&componentId), reinterpret_cast<uint8_t*>(&componentId) + sizeof(componentId));

                std::vector<uint8_t> compData;
                ComponentSerialization::Serialize(type, comp, compData);

                uint32_t dataSize = static_cast<uint32_t>(compData.size());
                binary.insert(binary.end(), reinterpret_cast<uint8_t*>(&dataSize), reinterpret_cast<uint8_t*>(&dataSize) + sizeof(dataSize));
                binary.insert(binary.end(), compData.begin(), compData.end());
            }
        }

        std::string outputPath = outputFolder + "/" + std::to_string(Hashes::CRC64Str(path.stem().string())) + ".bin";
        std::ofstream out(outputPath, std::ios::binary);
        out.write(reinterpret_cast<const char*>(binary.data()), binary.size());
        out.close();
    }

    return 0;
}