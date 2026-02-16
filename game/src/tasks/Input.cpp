#include "tasks/Input.h"

#include "EntitySystem.h"
#include "GameInfo.h"

#include "components/PlayerComponent.h"

InputTask::InputTask()
{
    DependsOnState = GameState::PreUpdate;
    RunInMainThread = true;
}

void InputTask::Tick()
{
    Vector2 InputVector = { 0.0f, 0.0f };

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