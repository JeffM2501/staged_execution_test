
#include "raylib.h"
#include "raymath.h"
#include "GameState.h"
#include "Task.h"
#include "TaskManager.h"

#include <vector>
#include <atomic>
#include <algorithm>
#include <execution>

std::atomic<bool> IsRunning = true;
std::atomic<Color> ClearColor = BLACK;

std::atomic<float> FPSDeltaTime = 1.0f / 60.0f;

float FixedUpdateTime = 1.0f / 60.0f;
float Accumulator = FixedUpdateTime;

double LastFrameTime = 0;

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

struct Entity
{
    bool IsPlayer = false;
    Vector2 Position = Vector2Zeros;
    Vector2 Velocity = Vector2Zeros;
    float Size = 0;
};

std::vector<Entity> Entitites;

class InputTask : public Task
{
public:
    DECLARE_TASK(InputTask);
    InputTask()
    {
        DependsOnState = GameState::PreUpdate;
        BlocksState = GameState::Update;
        RunInMainThread = true;
    }

    Vector2 InputVector = { 0.0f, 0.0f };

protected:
    void RunOneFrame() override
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
        
        if (Vector2Length(InputVector) > 1.0f)
        {
            InputVector = Vector2Normalize(InputVector);
        }
    }
};

class PlayerMovementTask : public Task
{
public:
    DECLARE_TASK(PlayerMovementTask);
    PlayerMovementTask(InputTask* input)
        : Input(input)
    {
        DependsOnState = GameState::Update;
        BlocksState = GameState::PostUpdate;
    }

    float PlayerSpeed = 100.0f;
    InputTask* Input = nullptr;
protected:
    void RunOneFrame() override
    {
        for (auto& entity : Entitites)
        {
            if (!entity.IsPlayer)
                continue;

            entity.Position += Input->InputVector * PlayerSpeed * GetDeltaTime();
            return;
        }
    }
};

bool MoveEntity(Entity& entity, Vector2 delta, const BoundingBox2D & bounds)
{
    bool hit = false;

    Vector2 newPos = entity.Position + delta;

    if (newPos.x > bounds.Max.x - entity.Size)
    {
        newPos.x = bounds.Max.x - entity.Size;
        entity.Velocity.x *= -1;
        hit = true;
    }
    else if (newPos.x < bounds.Min.x + entity.Size)
    {
        newPos.x = bounds.Min.x + entity.Size;
        entity.Velocity.x *= -1;
        hit = true;
    }

    if (newPos.y > bounds.Max.y - entity.Size)
    {
        newPos.y = bounds.Max.y - entity.Size;
        entity.Velocity.y *= -1;
        hit = true;
    }
    else if (newPos.y < bounds.Min.y + entity.Size)
    {
        newPos.y = bounds.Min.y + entity.Size;
        entity.Velocity.y *= -1;
        hit = true;
    }

    entity.Position = newPos;
    return hit;
}

class AIUpdateTask : public Task
{
public:
    DECLARE_TASK(AIUpdateTask);
    AIUpdateTask()
    {
        DependsOnState = GameState::FixedUpdate;
        BlocksState = GameState::None;
    }
protected:
    void RunOneFrame() override
    {
        std::for_each(std::execution::par, Entitites.begin(), Entitites.end(),[]
        (auto& entity)
            {
                if (entity.IsPlayer)
                    return;

                MoveEntity(entity, entity.Velocity * GetDeltaTime(), WorldBounds.load());
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
        BlocksState = GameState::Present;
        RunInMainThread = true;
    }

    void RunOneFrame() override
    {
        for (auto& entity : Entitites)
        {
            if (entity.IsPlayer)
            {
                DrawRectangleRec(Rectangle(entity.Position.x, entity.Position.y, entity.Size, entity.Size), GREEN);
            }
            else
            {
                DrawRectangleRec(Rectangle(entity.Position.x, entity.Position.y, entity.Size, entity.Size), BLUE);
            }
        }
    }
};

class PresentTask : public Task
{
public:
    DECLARE_TASK(DrawTask);
    PresentTask()
    {
        DependsOnState = GameState::Present;
        BlocksState = GameState::PostDraw;
        RunInMainThread = true;
    }

    void RunOneFrame() override
    {
        DrawRectangle(0, 0, 850, 120, ColorAlpha(BLACK, 0.5f));
        DrawFPS(10, 10);
        DrawText(TextFormat("Frame Time %0.3f ms", LastFrameTime * 1000), 100, 10, 20, WHITE);

#if defined(DEBUG)
        int y = 30;
        for (GameState state = GameState::FrameHead; state <= GameState::FrameTail; ++state)
        {
            auto& stats = TaskManager::GetStatsForState(state);
            if (stats.TaskCount == 0)
                continue;

            DrawText(TextFormat("%s %d Tasks in %0.3f ms [Max %0.3f] (Blocked for %0.3f ms [Max %0.3f])", GetStateName(state), stats.TaskCount, stats.Durration * 1000.0, stats.MaxDurration * 1000.0, stats.BlockedDurration * 1000.0, stats.MaxBlockedDurration * 1000.0), 10, y, 20, GRAY);
            y += 20;
        }
#endif
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
    InitWindow(1280, 800, "Example");
    SetTargetFPS(144);
    WorldBounds.store(BoundingBox2D{ Vector2{0,0}, Vector2{float(GetScreenWidth()), float(GetScreenHeight())} });
    FPSDeltaTime = 1.0f / float(GetMonitorRefreshRate(0));

    auto inputTask = TaskManager::AddTask<InputTask>();
    TaskManager::AddTask<PlayerMovementTask>(inputTask);
    TaskManager::AddTask<AIUpdateTask>();
    TaskManager::AddTask<DrawTask>(); 
    TaskManager::AddTask<PresentTask>();

    Entitites.push_back(Entity(true, Vector2(100, 100), Vector2Zeros, 10));

    constexpr float nonPlayerSize = 20;
    constexpr float nonPlayerSpeed = 20;
    constexpr size_t npcCount = 4000;

    for (size_t i = 0; i < npcCount; i++)
    {
        Vector2 pos = GetRandomPosInBounds(WorldBounds, nonPlayerSize);
        Vector2 vel = GetRandomVector(nonPlayerSpeed);

        Entitites.push_back(Entity(false, pos, vel, nonPlayerSize));
    }
}

void GameCleanup()
{
    TaskManager::Cleanup();
    CloseWindow();
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

        ClearBackground(ClearColor.load());

        Accumulator += GetDeltaTime();

        double frameStartTime = GetTime();

        for (GameState state = GameState::FrameHead; state <= GameState::FrameTail; ++state)
        {
            if (state == GameState::FixedUpdate)
            {
                while (Accumulator >= FixedUpdateTime)
                {
                    TaskManager::RunTasksForState(GameState::FixedUpdate);
                    Accumulator -= FixedUpdateTime;
                }
            }
            else
            {
                TaskManager::RunTasksForState(state);
                if (state == GameState::Present)
                    EndDrawing();
            }
        } 

        LastFrameTime = GetTime() - frameStartTime;
        if (WindowShouldClose())
            IsRunning.store(false);
    }
    GameCleanup();

    return 0;
}