
#include "raylib.h"
#include "raymath.h"
#include "FrameStage.h"
#include "TaskManager.h"
#include "TimeUtils.h"
#include "PresentationManager.h"
#include "EntitySystem.h"
#include "ValueTracker.h"
#include "ComponentTasks.h"
#include "TextureManager.h"
#include "ResourceManager.h"
#include "EntityReader.h"

#include "GameInfo.h"

#include "components/TransformComponent.h"
#include "components/PlayerComponent.h"
#include "components/NPCComponent.h"
#include "components/BulletComponent.h"
#include "components/PlayerSpawnComponent.h"
#include "components/NPCSpawnComponent.h"

#include "ComponentReader.h"

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

std::unique_ptr<LambdaTask> LoaderTask;

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

Vector2 GetRandomVector(float scaler)
{
    constexpr int resolution = 90000;
    float x = float(GetRandomValue(-resolution, resolution));
    float y = float(GetRandomValue(-resolution, resolution));

    return Vector2Normalize(Vector2(x, y)) * scaler;
}

// setup
void RegisterTasks()
{
    TaskManager::AddTask<InputTask>();
    TaskManager::AddTask<DrawTask>();
    TaskManager::AddTask<OverlayTask>();
    TaskManager::AddTask<GUITask>();
}

void RegisterComponents()
{
    EntitySystem::RegisterComponent<TransformComponent>();
    RegisterComponentWithUpdate<PlayerComponent>(FrameStage::Update, true);
    RegisterComponentWithUpdate<NPCComponent>(FrameStage::FixedUpdate, true);
    RegisterComponentWithUpdate<BulletComponent>(FrameStage::PreUpdate, true);
    RegisterComponentWithUpdate<NPCSpawnComponent>(FrameStage::FixedUpdate, true);
    EntitySystem::RegisterComponent<PlayerSpawnComponent>();
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

ComponentReader PrefabReader;
ComponentReader SceneReader;

void GameInit()
{
    TaskManager::Init();
    ResourceManager::Init();
    EntitySystem::Init();

    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 800, "Task Test");
    SetTargetFPS(GetMonitorRefreshRate(0));

    TextureManager::Init();
   
    FPSDeltaTime = 1.0f / float(GetMonitorRefreshRate(0));

    RegisterLayers();

    RegisterTasks();
    RegisterComponents();

    SceneReader.ReadSceneFromResource(Hashes::CRC64Str("levels/test.scene.json"));

    WorldBounds.store(BoundingBox2D{ Vector2{0,0}, Vector2{float(GetScreenWidth()), float(GetScreenHeight())} });
}

void GameCleanup()
{
    EntitySystem::ClearAllEntities();
    TaskManager::Shutdown();
    PresentationManager::Shutdown();
    ResourceManager::Shutdown();
    TextureManager::Shutdown();
    CloseWindow();
}

std::atomic<double> FrameStartTime = 0;
double GetFrameStartTime()
{
    return FrameStartTime.load();
}

/* TODO
* Add Entity group tracking for loaded resources
* Resource Manager preloads
*/

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

        ResourceManager::Update();
        TextureManager::Update();
        PresentationManager::Update();
        ClearBackground(ClearColor.load());

        FrameStartTime.store(GetTime());
        TaskManager::TickFrame();
        EntitySystem::FlushMorgue();
        LastFrameTime = GetTime() - FrameStartTime;
        FameTimeTracker.AddValue(float(LastFrameTime));

        // TODO, move this into a UI state manager?
        if (WindowShouldClose())
            IsRunning.store(false);
    }
    GameCleanup();

    return 0;
}