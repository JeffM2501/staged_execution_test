#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <filesystem>
#include <algorithm>
#include <functional>
#include <rapidjson/document.h>
#include "ComponentSerialization.h"
#include "SerializationUtils.h"
#include "CRC64.h"
#include <iostream>

#include "SceneSerializer.h"
#include "PrefabSerializer.h"
#include "SpriteSerializer.h"

namespace fs = std::filesystem;

std::string inputFolder = "assets";
std::string filesOutputFolder = "resources/files";
std::string imagesOutputFolder = "resources/images";

std::unordered_map<std::string, std::function<void(BufferWriter& writer, rapidjson::Document& document)>> JsonSerializationFunctions;
std::unordered_map<std::string, std::function<void(const std::filesystem::path &inPath)>> ExtensionSerializationFunctions;


void ProcessFile(const std::filesystem::path& path)
{
    std::cout << "Processing file : " << path << std::endl;

    auto handler = ExtensionSerializationFunctions.find(path.extension().string());
    if (handler != ExtensionSerializationFunctions.end())
    {
        handler->second(path);
    }
    else
    {
        std::cout << "No handler for file types : *." << path.extension() << std::endl;
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

        ProcessFile(entry.path());
    }
    std::cout.flush();
}

void ProcessJsonFile(const std::filesystem::path& inPath)
{
    BufferWriter buffer;

    std::ifstream in(inPath);
    std::string jsonStr((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    rapidjson::Document doc;
    if (doc.Parse(jsonStr.c_str()).HasParseError())
    {
        std::cerr << "Failed to parse JSON file: " << inPath << std::endl;
        return;
    }

    auto stem = inPath.stem();
    auto ext = stem.extension();

    auto serializer = JsonSerializationFunctions.find(ext.string());

    if (serializer != JsonSerializationFunctions.end())
    {
        serializer->second(buffer, doc);
    }

    std::string hashedName = inPath.string();
    hashedName = hashedName.substr(inputFolder.size() + 1);

    std::replace(hashedName.begin(), hashedName.end(), '\\', '/');

    std::string outputPath = filesOutputFolder + "/" + std::to_string(Hashes::CRC64Str(hashedName)) + ".bin";
    std::ofstream out(outputPath, std::ios::binary);
    out.write(reinterpret_cast<const char*>(buffer.Buffer.data()), buffer.Buffer.size());
    out.close();
}

void ProcessPNGFile(const std::filesystem::path& inPath)
{
    std::string hashedName = inPath.string();
    hashedName = hashedName.substr(inputFolder.size() + 1);
    std::replace(hashedName.begin(), hashedName.end(), '\\', '/');
    std::string outputPath = imagesOutputFolder + "/" + std::to_string(Hashes::CRC64Str(hashedName)) + ".png";
    fs::copy_file(inPath, outputPath, fs::copy_options::overwrite_existing);
}

int main(int argc, char* argv[])
{
    ComponentSerialization::SetupSerializers();

    JsonSerializationFunctions.insert_or_assign(".prefab", SerializePrefab);
    JsonSerializationFunctions.insert_or_assign(".scene", SerializeScene);
    JsonSerializationFunctions.insert_or_assign(".sprite", SerializeSprite);

    ExtensionSerializationFunctions.insert_or_assign(".json", ProcessJsonFile);
    ExtensionSerializationFunctions.insert_or_assign(".png", ProcessPNGFile);
    
    fs::create_directories(imagesOutputFolder);
    fs::create_directories(filesOutputFolder);

    if (argc < 2)
    {
        ProcessFolder(inputFolder);
    }
    else
    {
        std::filesystem::path inPath(argv[1]);
        if (std::filesystem::is_directory(inPath))
            ProcessFolder(inPath.string());
        else
            ProcessFile(inPath);
    }
    
    return 0;
}