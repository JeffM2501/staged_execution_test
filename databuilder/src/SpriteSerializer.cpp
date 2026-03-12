#include "SpriteSerializer.h"

#include <iostream>

static constexpr uint32_t SpriteMagic = 0x53505254; // "SPRT" in ASCII
static constexpr uint32_t SpriteVersion = 1;

void SerializeSprite(BufferWriter& buffer, rapidjson::Document& sprite)
{
    // header
    buffer.Write(SpriteMagic);
    buffer.Write(SpriteVersion);

    auto image = sprite.FindMember("Image");
    if (image == sprite.MemberEnd() || !image->value.IsString())
    {
        std::cerr << "Invalid Sprite format in file: " << std::endl;
        return;
    }

    size_t imageHash = Hashes::CRC64Str(image->value.GetString());
    buffer.Write(imageHash);

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

                buffer.Write<uint32_t>(width * height); // frame count
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
                        buffer.WriteArray(frameRect);
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
                buffer.Write(frameCount); // frame count
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
                        buffer.WriteArray(frameRect);
                    }
                }
            }
        }
    }

    if (!validFrameDef)
    {
        // just write one rectangle for the entire sprite
        buffer.Write<uint32_t>(1); // frame count
        buffer.WriteArray(std::vector<float>{0, 0, 1, 1}); // frame rect
    }
}