#pragma once

#include <vector>
#include "rapidjson/document.h"

#include "SerializationUtils.h"

void SerializeSprite(BufferWriter& buffer, rapidjson::Document& sprite);
