#include "VieworksCameraManager.h"
#include "ml_vp50mx.h"

VieworksCameraManager::VieworksCameraManager()
{
}

VieworksCameraManager::~VieworksCameraManager()
{
}

void VieworksCameraManager::Initialize()
{
}

MLCamera* VieworksCameraManager::CreateCameraInstance()
{
	return new ml_vp50mx;
}

void VieworksCameraManager::Terminate()
{
}