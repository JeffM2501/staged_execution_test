#include "tasks/GUI.h"

#include "PresentationManager.h"
#include "GameInfo.h"

GUITask::GUITask() : Task(FrameStage::PreDraw, true)
{
    Logo = LoadTexture("resources/logo.png");
}

GUITask::~GUITask()
{ 
    UnloadTexture(Logo);
}

void GUITask::Tick()
{
    PresentationManager::BeginLayer(GUILayer);
   
    Rectangle layerBounds = PresentationManager::GetCurrentLayerRect();

    DrawTexture(Logo, int(layerBounds.x), int(layerBounds.y + layerBounds.height - Logo.height), WHITE);
    PresentationManager::EndLayer();
}