#pragma once

#include <vector>
#include "rapidjson/document.h"

#include "SerializationUtils.h"

void SerializePrefab(BufferWriter& buffer, rapidjson::Document& scene);