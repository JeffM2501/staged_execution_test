
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

#include <atomic>

bool UseInterpolateNPCs = true;

std::atomic<bool> IsRunning = true;
std::atomic<Color> ClearColor = BLACK;

std::atomic<float> FPSDeltaTime = 1.0f / 60.0f;

static size_t BackgroundLayer = 0;
static size_t NPCLayer = 10;
static size_t PlayerLayer = 20;
static size_t GUILayer = 100;
static size_t DebugLayer = 200;

double LastFrameTime = 0;

ValueTracker FameTimeTracker(144, "FrameTime");

struct BoundingBox2D
{
    Vector2 Min = Vector2Zeros;
    Vector2 Max = Vector2Zeros;
};
std::atomic<BoundingBox2D> WorldBounds;

float GetDeltaTime()
{
    return FPSDeltaTime.load();
}


struct TransformComponent : public EntitySystem::EntityComponent
{
    Vector2 Position = Vector2Zeros;
    Vector2 Velocity = Vector2Zeros;
    DECLARE_SIMPLE_COMPONENT(TransformComponent);
};


bool MoveEntity(TransformComponent& entity, float size, Vector2 delta, const BoundingBox2D& bounds)
{
    bool hit = false;

    Vector2 newPos = entity.Position + delta;

    if (newPos.x > bounds.Max.x - size)
    {
        newPos.x = bounds.Max.x - size;
        entity.Velocity.x *= -1;
        hit = true;
    }
    else if (newPos.x < bounds.Min.x + size)
    {
        newPos.x = bounds.Min.x + size;
        entity.Velocity.x *= -1;
        hit = true;
    }

    if (newPos.y > bounds.Max.y - size)
    {
        newPos.y = bounds.Max.y - size;
        entity.Velocity.y *= -1;
        hit = true;
    }
    else if (newPos.y < bounds.Min.y + size)
    {
        newPos.y = bounds.Min.y + size;
        entity.Velocity.y *= -1;
        hit = true;
    }

    entity.Position = newPos;
    return hit;
}

struct PlayerComponent : public EntitySystem::EntityComponent
{
    DECLARE_SIMPLE_COMPONENT(PlayerComponent);

    Vector2 Input = Vector2Zeros;

    float Size = 10;
    float Health = 100;

    float PlayerSpeed = 100.0f;

    void Update()
    {
        auto transform = GetEntityComponent<TransformComponent>();
        if (transform)
        {
            transform->Position += Input * PlayerSpeed * GetDeltaTime();
        }
    }
};

struct NPCComponent : public EntitySystem::EntityComponent
{
    DECLARE_SIMPLE_COMPONENT(NPCComponent);
    float Size = 20;
    Color Tint = BLUE;
    double LastUpdateTime = 0;

    void Update()
    {
        auto transform = GetEntityComponent<TransformComponent>();
        if (transform)
        {
            float delta = TaskManager::GetFixedDeltaTime();
            MoveEntity(*transform, Size, transform->Velocity * delta, WorldBounds.load());
            LastUpdateTime = GetFrameStartTime();
        }
    }
};

class InputTask : public Task
{
public:
    DECLARE_TASK(InputTask);
    InputTask()
    {
        DependsOnState = GameState::PreUpdate;
        RunInMainThread = true;
    }

    Vector2 InputVector = { 0.0f, 0.0f };

protected:
    void Tick() override
    {
        InputVector = { 0.0f, 0.0f };

        if (IsKeyDown(KEY_W))
            InputVector.y -= 1.0f;
        if (IsKeyDown(KEY_S))
            InputVector.y += 1.0f;
        if (IsKeyDown(KEY_A))
            InputVector.x -= 1.0f;
        if (IsKeyDown(KEY_D))
            InputVector.x += 1.0f;

        if (IsKeyPressed(KEY_SPACE))
            UseInterpolateNPCs = !UseInterpolateNPCs;

        if (Vector2Length(InputVector) > 1.0f)
        {
            InputVector = Vector2Normalize(InputVector);
        }

        EntitySystem::DoForEachComponent<PlayerComponent>([&](PlayerComponent& player)
            {
                player.Input = InputVector;
            });
    }
};

class DrawTask : public Task
{
public:
    DECLARE_TASK(DrawTask);
    DrawTask()
    {
        DependsOnState = GameState::Draw;
        RunInMainThread = true;
    }

    void Tick() override
    {
        PresentationManager::BeginLayer(PlayerLayer);

        EntitySystem::DoForEachComponent<PlayerComponent>([&](PlayerComponent& player)
            {
                auto transform = player.GetEntityComponent<TransformComponent>();
                if (transform)
                {
                    DrawCircleV(transform->Position, player.Size, GREEN);
                }
            });

        PresentationManager::EndLayer();

        PresentationManager::BeginLayer(NPCLayer);

        double now = GetTime();
        EntitySystem::DoForEachComponent<NPCComponent>([&](NPCComponent& npc)
            {
                auto transform = npc.GetEntityComponent<TransformComponent>();
                if (transform)
                {
                    Vector2 interpPos = transform->Position - Vector2(npc.Size, npc.Size);
                    if (UseInterpolateNPCs)
                        interpPos += transform->Velocity * float(now - npc.LastUpdateTime);

                    DrawRectangleRec(Rectangle(interpPos.x, interpPos.y, npc.Size*2, npc.Size*2), npc.Tint);
                }
            });
        PresentationManager::EndLayer();
    }
};

class OverlayTask : public Task
{
public:
    DECLARE_TASK(OverlayTask);
    OverlayTask()
    {
        DependsOnState = GameState::Draw;
        RunInMainThread = true;
    }

    void Tick() override
    {
        PresentationManager::BeginLayer(DebugLayer);
        DrawRectangle(0, 0, 750, 80, ColorAlpha(DARKBLUE, 0.85f));
        DrawFPS(10, 10);
        if (LastFrameTime > 0)
            DrawText(TextFormat("Instant %0.1fFPS", 1.0f/LastFrameTime), 100, 10, 20, WHITE);

        int x = 320;
        int y = 10;

        if (UseInterpolateNPCs)
            DrawText("Interpolation: ON (Press Space to toggle)", x, y, 20, GREEN);
        else
            DrawText("Interpolation: OFF (Press Space to toggle)", x, y, 20, RED);

        Rectangle graphBounds = { float(x + 450), float(y+3), 400, 50 };
        FameTimeTracker.DrawGraph(graphBounds);

#if defined(DEBUG)
        y = 30;
        for (GameState state = GameState::FrameHead; state <= GameState::FrameTail; ++state)
        {
            auto& stats = TaskManager::GetStatsForState(state);
            if (stats.TaskCount == 0)
                continue;

            const char* text = TextFormat("%s %d Tasks in %0.3f ms [Max %0.3f] (Blocked for %0.3f ms [Max %0.3f])", 
                GetStateName(state),
                stats.TaskCount, 
                stats.Durration * 1000.0,
                stats.MaxDurration * 1000.0,
                stats.BlockedDurration * 1000.0,
                stats.MaxBlockedDurration * 1000.0);

            DrawText(text, 20, y, 10, GRAY);

            DrawRectangle(5, y, 8, 8, stats.TickedThisFrame ? GREEN : RED);

            y += 10;
        }
#endif
        PresentationManager::EndLayer();
    }
};

class PresentTask : public Task
{
public:
    DECLARE_TASK(PresentTask);
    PresentTask()
    {
        DependsOnState = GameState::Present;
        RunInMainThread = true;
    }

    void Tick() override
    {
        PresentationManager::Present();
    }
};

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

void GameInit()
{
    TaskManager::Init();

    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 800, "Task Test");
    SetTargetFPS(GetMonitorRefreshRate(0));
    WorldBounds.store(BoundingBox2D{ Vector2{0,0}, Vector2{float(GetScreenWidth()), float(GetScreenHeight())} });
    FPSDeltaTime = 1.0f / float(GetMonitorRefreshRate(0));

    TaskManager::AddTask<InputTask>();
    TaskManager::AddTask<DrawTask>(); 
    TaskManager::AddTask<OverlayTask>();
    TaskManager::AddTask<PresentTask>();

    EntitySystem::RegisterComponent<TransformComponent>();
    RegisterComponentWithUpdate<PlayerComponent>(GameState::Update, true);
    RegisterComponentWithUpdate<NPCComponent>(GameState::FixedUpdate, true);

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
        auto transform = npc->AddComponent<TransformComponent>();
        transform->Position = GetRandomPosInBounds(WorldBounds, nonPlayerSize);
        transform->Velocity = GetRandomVector(nonPlayerSpeed);
    }

    PresentationManager::Init();

    // create the presentation layers
    BackgroundLayer = PresentationManager::DefineLayer(uint8_t(BackgroundLayer));
    NPCLayer = PresentationManager::DefineLayer(uint8_t(NPCLayer));
    PlayerLayer = PresentationManager::DefineLayer(uint8_t(PlayerLayer));
    GUILayer = PresentationManager::DefineLayer(uint8_t(GUILayer));
    DebugLayer = PresentationManager::DefineLayer(uint8_t(DebugLayer));
}

void GameCleanup()
{
    TaskManager::Cleanup();
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