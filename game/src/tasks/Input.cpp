#include "tasks/Input.h"

#include "EntitySystem.h"
#include "GameInfo.h"

#include "components/PlayerComponent.h"

void InputTask::Tick()
{
    Vector2 inputVector = { 0.0f, 0.0f };

    if (IsKeyDown(KEY_W))
        inputVector.y -= 1.0f;
    if (IsKeyDown(KEY_S))
        inputVector.y += 1.0f;
    if (IsKeyDown(KEY_A))
        inputVector.x -= 1.0f;
    if (IsKeyDown(KEY_D))
        inputVector.x += 1.0f;

    if (IsKeyPressed(KEY_SPACE))
        UseInterpolateNPCs = !UseInterpolateNPCs;

    if (Vector2LengthSqr(inputVector) > 0.001f)
    {
        inputVector = Vector2Normalize(inputVector);
    }

    auto* player = EntitySystem::GetFirstComponentOfType<PlayerComponent>();
    if (player)
        player->Input = inputVector;
}