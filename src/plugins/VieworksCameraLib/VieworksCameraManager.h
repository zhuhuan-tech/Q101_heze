#pragma once
#include <PrjCommon/service/ml_camera.h>

using namespace CORE;
class VieworksCameraManager :public MLCameraManager
{
public:
	VieworksCameraManager();
	~VieworksCameraManager();

	virtual void Initialize();

	virtual MLCamera* CreateCameraInstance();

	virtual void Terminate();

private:
};


