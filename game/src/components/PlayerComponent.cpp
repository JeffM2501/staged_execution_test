#include "components/PlayerComponent.h"
#include "components/TransformComponent.h"

#include "TimeUtils.h"

void PlayerComponent::Update()
{
    auto transform = GetEntityComponent<TransformComponent>();
    if (transform)
    {
        transform->Position += Input * PlayerSpeed * GetDeltaTime();
    }
}