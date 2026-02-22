#include "tasks/GUI.h"

#include "PresentationManager.h"
#include "GameInfo.h"

GUITask::GUITask() : Task(FrameStage::PreDraw, true)
{
    Logo = TextureManager::GetTexture(101010101010);
}

GUITask::~GUITask()
{ 
}

void GUITask::Tick()
{
    PresentationManager::BeginLayer(GUILayer);
   
    Rectangle layerBounds = PresentationManager::GetCurrentLayerRect();

    DrawTexture(Logo->ID, int(layerBounds.x), int(layerBounds.y + layerBounds.height - Logo->Bounds.height), WHITE);
    PresentationManager::EndLayer();
}