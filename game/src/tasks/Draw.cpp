#include "tasks/Draw.h"

#include "components/TransformComponent.h"
#include "components/PlayerComponent.h"
#include "components/NPCComponent.h"
#include "components/BulletComponent.h"

#include "PresentationManager.h"
#include "EntitySystem.h"
#include "GameInfo.h"

void DrawTask::Tick()
{
    PresentationManager::BeginLayer(BackgroundLayer);
    DrawRectangleGradientEx(PresentationManager::GetCurrentLayerRect(), BLACK, BLACK, Color(0, 0, 40, 255), Color(40,40,40,255));
    PresentationManager::EndLayer();

    PresentationManager::BeginLayer(PlayerLayer);
    EntitySystem::DoForEachComponent<PlayerComponent>([&](auto& player)
        {
            auto transform = player.GetEntityComponent<TransformComponent>();
            if (transform)
            {
                DrawCircleV(transform->Position, player.Size, GREEN);
            }
        });

    EntitySystem::DoForEachComponent<BulletComponent>([&](auto& bullet)
        {
            auto transform = bullet.GetEntityComponent<TransformComponent>();
            if (transform)
            {
                DrawCircleV(transform->Position, bullet.Size, bullet.Tint);
                DrawText(TextFormat("%0.2f ID %zu", bullet.Lifetime, bullet.EntityID), int(transform->Position.x + bullet.Size), int(transform->Position.y-5), 10, GRAY);
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

                DrawRectangleRec(Rectangle(interpPos.x, interpPos.y, npc.Size * 2, npc.Size * 2), npc.Tint);
            }
        });
    PresentationManager::EndLayer();
}
