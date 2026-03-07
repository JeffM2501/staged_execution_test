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
    DrawRectangleGradientEx(PresentationManager::GetCurrentLayerRect(), BLACK, Color(0, 40, 60, 255), Color(0, 20, 40, 255), Color(40,40,40,255));
    PresentationManager::EndLayer();

    PresentationManager::BeginLayer(PlayerLayer);
    EntitySystem::DoForEachComponent<PlayerComponent>([&](auto& player)
        {
            auto transform = player.GetEntityComponent<TransformComponent>().Get();
            if (transform)
            {
                player.Sprite.Draw(transform->Position, WHITE);
            }
        });

    EntitySystem::DoForEachComponent<BulletComponent>([&](auto& bullet)
        {
            auto transform = bullet.GetEntityComponent<TransformComponent>().Get();
            if (transform)
            {
                bullet.Sprite.Draw(transform->Position, bullet.Tint);
            }
        });
    PresentationManager::EndLayer();

    PresentationManager::BeginLayer(NPCLayer);
    double now = GetTime();
    EntitySystem::DoForEachComponent<NPCComponent>([&](NPCComponent& npc)
        {
            auto transform = npc.GetEntityComponent<TransformComponent>().Get();
            if (transform)
            {
                Vector2 interpPos = transform->Position - Vector2(npc.Size, npc.Size);
                if (UseInterpolateNPCs)
                    interpPos += transform->Velocity * float(now - npc.LastUpdateTime);

                npc.Sprite.Scale = npc.Size / 2.0f;
                npc.Sprite.Draw(interpPos, npc.Tint);
            }
        });
    PresentationManager::EndLayer();
}
