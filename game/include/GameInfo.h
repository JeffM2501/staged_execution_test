#pragma once

#include "raylib.h"
#include "raymath.h"
#include "ValueTracker.h"
#include "ComponentReader.h"

#include <atomic>

struct BoundingBox2D
{
    Vector2 Min = Vector2Zeros;
    Vector2 Max = Vector2Zeros;
};
extern std::atomic<BoundingBox2D> WorldBounds;

extern bool UseInterpolateNPCs;

extern std::atomic<bool> IsRunning;
extern std::atomic<Color> ClearColor;

extern std::atomic<float> FPSDeltaTime;

extern size_t BackgroundLayer;
extern size_t NPCLayer;
extern size_t PlayerLayer;
extern size_t GUILayer;
extern size_t DebugLayer ;

extern double LastFrameTime;

extern ValueTracker FameTimeTracker;

extern ComponentReader PrefabReader;
