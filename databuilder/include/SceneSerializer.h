#pragma once

#include <vector>
#include "rapidjson/document.h"

#include "SerializationUtils.h"

void SerializeScene(BufferWriter& buffer, rapidjson::Document& scene);