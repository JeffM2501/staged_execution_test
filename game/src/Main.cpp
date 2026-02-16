
#include "raylib.h"
#include "raymath.h"
#include "GameState.h"
#include "Task.h"
#include "TaskManager.h"
#include "TimeUtils.h"
#include "PresentationManager.h"
#include "EntitySystem.h"
#include "ValueTracker.h"
#include "ComponentTasks.h"

#include "GameInfo.h"

#include "components/TransformComponent.h"
#include "components/PlayerComponent.h"
#include "components/NPCComponent.h"

#include "tasks/Input.h"
#include "tasks/Draw.h"
#include "tasks/Overlay.h"
#include "tasks/GUI.h"

#include <atomic>

// global stuff
bool UseInterpolateNPCs = true;

std::atomic<bool> IsRunning = true;
std::atomic<Color> ClearColor = BLACK;

std::atomic<float> FPSDeltaTime = 1.0f / 60.0f;

size_t BackgroundLayer = 0;
size_t NPCLayer = 10;
size_t PlayerLayer = 20;
size_t GUILayer = 100;
size_t DebugLayer = 200;

double LastFrameTime = 0;

ValueTracker FameTimeTracker(300, "FrameTime");

std::atomic<BoundingBox2D> WorldBounds;

float GetDeltaTime()
{
    return FPSDeltaTime.load();
}

// world generation
Vector2 GetRandomPosInBounds(const BoundingBox2D & bounds, float size)
{
    float x = float(GetRandomValue(int(bounds.Min.x + size), int(bounds.Max.x - size)));
    float y = float(GetRandomValue(int(bounds.Min.y + size), int(bounds.Max.y - size)));

    return Vector2(x, y);
}

Vector2 GetRandomVector(float scaler = 1)
{
    constexpr int resolution = 90000;
    float x = float(GetRandomValue(-resolution, resolution));
    float y = float(GetRandomValue(-resolution, resolution));

    return Vector2Normalize(Vector2(x, y)) * scaler;
}

void SetupScene()
{
    WorldBounds.store(BoundingBox2D{ Vector2{0,0}, Vector2{float(GetScreenWidth()), float(GetScreenHeight())} });

    auto player = EntitySystem::AddComponent<PlayerComponent>(EntitySystem::NewEntityId());
    player->AddComponent<TransformComponent>()->Position = Vector2(100, 200);
    player->Size = 10;
    player->Health = 100;
    player->PlayerSpeed = 200;

    constexpr float nonPlayerSize = 20;
    constexpr float nonPlayerSpeed = 50;
    constexpr size_t npcCount = 200;

    for (size_t i = 0; i < npcCount; i++)
    {
        auto npc = EntitySystem::AddComponent<NPCComponent>(EntitySystem::NewEntityId());
        npc->Size = float(GetRandomValue(5, 15));
        npc->Tint = GetRandomValue(0, 10) >= 5 ? DARKBLUE : DARKPURPLE;
        auto transform = npc->AddComponent<TransformComponent>();
        transform->Position = GetRandomPosInBounds(WorldBounds, nonPlayerSize);
        transform->Velocity = GetRandomVector(float(GetRandomValue(int(nonPlayerSpeed / 2), int(nonPlayerSpeed))));
    }
}

// setup
void RegisterTasks()
{
    TaskManager::AddTask<InputTask>();
    TaskManager::AddTask<DrawTask>();
    TaskManager::AddTask<OverlayTask>();
    TaskManager::AddTask<GUITask>();
    TaskManager::AddTaskOnState<LambdaTask>(GameState::Present, Hashes::CRC64Str("Present"), []() { PresentationManager::Present(); }, true);
}

void RegisterComponents()
{
    EntitySystem::RegisterComponent<TransformComponent>();
    RegisterComponentWithUpdate<PlayerComponent>(GameState::Update, true);
    RegisterComponentWithUpdate<NPCComponent>(GameState::FixedUpdate, true);
}

void RegisterLayers()
{
    PresentationManager::Init();

    BackgroundLayer = PresentationManager::DefineLayer(uint8_t(BackgroundLayer), true, 0.1f);
    NPCLayer = PresentationManager::DefineLayer(uint8_t(NPCLayer));
    PlayerLayer = PresentationManager::DefineLayer(uint8_t(PlayerLayer));
    GUILayer = PresentationManager::DefineLayer(uint8_t(GUILayer));
    DebugLayer = PresentationManager::DefineLayer(uint8_t(DebugLayer));
}

void GameInit()
{
    TaskManager::Init();

    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 800, "Task Test");
    SetTargetFPS(GetMonitorRefreshRate(0));
   
    FPSDeltaTime = 1.0f / float(GetMonitorRefreshRate(0));

    RegisterLayers();

    RegisterTasks();
    RegisterComponents();
    SetupScene();
}

void GameCleanup()
{
    EntitySystem::ClearAllEntities();
    TaskManager::Cleanup();
    PresentationManager::Cleanup();
    CloseWindow();
}

std::atomic<double> FrameStartTime = 0;
double GetFrameStartTime()
{
    return FrameStartTime.load();
}

int main()
{
    GameInit();

    while (IsRunning.load())
    {
        BeginDrawing();
#if !defined(DEBUG)
        FPSDeltaTime.store(GetFrameTime());
#endif
        if (IsWindowResized())
            WorldBounds.store(BoundingBox2D{ Vector2{0,0}, Vector2{float(GetScreenWidth()), float(GetScreenHeight())} });

        PresentationManager::Update();

        ClearBackground(ClearColor.load());

        FrameStartTime.store(GetTime());

        TaskManager::TickFrame();

        LastFrameTime = GetTime() - FrameStartTime;

        FameTimeTracker.AddValue(float(LastFrameTime));

        if (WindowShouldClose())
            IsRunning.store(false);
    }
    GameCleanup();

    return 0;
}