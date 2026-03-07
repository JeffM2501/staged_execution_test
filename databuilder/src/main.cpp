#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>
#include <rapidjson/document.h>
#include "ComponentSerialization.h"
#include "SerializationUtils.h"
#include "CRC64.h"
#include <iostream>

namespace fs = std::filesystem;

static constexpr uint32_t PrefabMagic = 0x50465242; // "PFRB" in ASCII
static constexpr uint32_t PrefabVersion = 1;

static constexpr uint32_t SceneMagic = 0x53434E45; // "SCNE" in ASCII
static constexpr uint32_t SceneVersion = 1;

static constexpr uint32_t SpriteMagic = 0x53505254; // "SPRT" in ASCII
static constexpr uint32_t SpriteVersion = 1;

std::string inputFolder = "assets";
std::string filesOutputFolder = "resources/files";
std::string imagesOutputFolder = "resources/images";


void SerailizePrefab(std::vector<uint8_t>& binary, rapidjson::Document& prefab)
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
    WriteToOut(PrefabMagic, binary);
    WriteToOut(PrefabVersion, binary);
    WriteToOut(spawnable, binary);

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
}

void SerailizeScene(std::vector<uint8_t>& binary, rapidjson::Document& scene)
{
    // header
    WriteToOut(SceneMagic, binary);
    WriteToOut(SceneVersion, binary);

    auto entityList = scene.FindMember("Entities");
    if (entityList == scene.MemberEnd() || !entityList->value.IsArray())
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
}

void SerailizeSprite(std::vector<uint8_t> &binary, rapidjson::Document& sprite)
{
    // header
    WriteToOut(SpriteMagic, binary);
    WriteToOut(SpriteVersion, binary);

    auto image = sprite.FindMember("Image");
    if (image == sprite.MemberEnd() || !image->value.IsString())
    {
        std::cerr << "Invalid Sprite format in file: " << std::endl;
        return;
    }

    size_t imageHash = Hashes::CRC64Str(image->value.GetString());
    WriteToOut(imageHash, binary); 

    auto frameType = sprite.FindMember("FrameType");

    bool validFrameDef = false;

    if (frameType != sprite.MemberEnd() && frameType->value.IsString())
    {
        std::string frameTypeStr = frameType->value.GetString();
        if (frameTypeStr == "Grid")
        {
            auto gridInfo = sprite.FindMember("GridInfo");
            if (gridInfo != sprite.MemberEnd() && gridInfo->value.IsObject())
            {
                validFrameDef = true;

                int width = ReadJsonMemberValue<int>("Width", gridInfo->value, 1);
                int height = ReadJsonMemberValue<int>("Height", gridInfo->value, 1);

                float xOffset = 1.0f / float(width);
                float yOffset = 1.0f / float(height);

                WriteToOut<uint32_t>(width*height, binary); // frame count
                for (int j = 0; j < height; ++j)
                {
                    for (int i = 0; i < width; ++i)
                    {
                        std::vector<float> frameRect{
                            i * xOffset, // x
                            j * yOffset, // y
                            xOffset,     // width
                            yOffset      // height
                        };
                        WriteArrayToOut(frameRect, binary);
                    }
                }
            }
        }
        else if (frameType->value.GetString() == "List")
        {
            auto frameList = sprite.FindMember("Frames");
            if (frameList != sprite.MemberEnd() && frameList->value.IsArray())
            {
                validFrameDef = true;
                uint32_t frameCount = static_cast<uint32_t>(frameList->value.Size());
                WriteToOut(frameCount, binary); // frame count
                for (auto& frame : frameList->value.GetArray())
                {
                    if (frame.IsArray())
                    {
                        std::vector<float> frameRect;
                        for (auto& v : frame.GetArray())
                        {
                            if (v.IsNumber())
                                frameRect.push_back(v.GetFloat());
                            else
                                frameRect.push_back(0.0f);
                        }
                        WriteArrayToOut(frameRect, binary);
                    }
                }
            }
        }
    }

    if (!validFrameDef)
    {
        // just write one rectangle for the entire sprite
        WriteToOut<uint32_t>(1, binary); // frame count
        WriteArrayToOut(std::vector<float>{0, 0, 1, 1}, binary); // frame rect
    }
}

void ProcessFolder(const std::string folder)
{
    for (const auto& entry : fs::directory_iterator(folder))
    {
        if (entry.is_directory())
        {
            ProcessFolder(entry.path().string());
            continue;
        }
        if (!entry.is_regular_file())
            continue;

        const auto& path = entry.path();
        std::cout << "Processing file : " << path << std::endl;
        if (path.extension() == ".json")
        {
            std::vector<uint8_t> binary;

            std::ifstream in(path);
            std::string jsonStr((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
            rapidjson::Document doc;
            if (doc.Parse(jsonStr.c_str()).HasParseError())
            {
                std::cerr << "Failed to parse JSON file: " << path << std::endl;
                continue;;
            }

            binary.clear();

            auto stem = path.stem();
            auto ext = stem.extension();
            if (ext.string() == ".prefab")
            {
                SerailizePrefab(binary, doc);
            }
            else if (ext.string() == ".scene")
            {
                SerailizeScene(binary, doc);
            }
            else if (ext.string() == ".sprite")
            {
                SerailizeSprite(binary, doc);
            }

            std::string hashedName = path.string();
            hashedName = hashedName.substr(inputFolder.size() + 1);

            std::replace(hashedName.begin(), hashedName.end(), '\\', '/');

            std::string outputPath = filesOutputFolder + "/" + std::to_string(Hashes::CRC64Str(hashedName)) + ".bin";
            std::ofstream out(outputPath, std::ios::binary);
            out.write(reinterpret_cast<const char*>(binary.data()), binary.size());
            out.close();
        }
        if (path.extension() == ".png")
        {
            std::string hashedName = path.string();
            hashedName = hashedName.substr(inputFolder.size() + 1);
            std::replace(hashedName.begin(), hashedName.end(), '\\', '/'); 
            std::string outputPath = imagesOutputFolder + "/" + std::to_string(Hashes::CRC64Str(hashedName)) + ".png";
            fs::copy_file(path, outputPath, fs::copy_options::overwrite_existing);
        }
    }
}

int main()
{
    ComponentSerialization::SetupSerializers();
    fs::create_directories(imagesOutputFolder);
    fs::create_directories(filesOutputFolder);

    ProcessFolder(inputFolder);
    return 0;
}