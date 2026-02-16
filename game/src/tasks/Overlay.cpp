#include "tasks/Overlay.h"

#include "raylib.h"
#include "GameInfo.h"
#include "PresentationManager.h"

OverlayTask::OverlayTask()
{
    DependsOnState = GameState::Draw;
    RunInMainThread = true;
}

void OverlayTask::Tick()
{
    PresentationManager::BeginLayer(DebugLayer);
    DrawRectangle(0, 0, 750, 100, ColorAlpha(DARKBLUE, 0.85f));
    DrawFPS(10, 10);
    if (LastFrameTime > 0)
        DrawText(TextFormat("Instant %0.1fFPS", 1.0f / LastFrameTime), 100, 10, 20, WHITE);

    int x = 300;
    int y = 10;

    if (UseInterpolateNPCs)
        DrawText("Interpolation: ON (Press Space to toggle)", x, y, 20, GREEN);
    else
        DrawText("Interpolation: OFF (Press Space to toggle)", x, y, 20, RED);

    Rectangle graphBounds = { float(x + 450), float(y + 3), 400, 50 };
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
