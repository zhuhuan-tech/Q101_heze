#pragma once
#include <PrjCommon/service/ml_camera.h>


using namespace CORE;
class  MLCameraFactory
{
public:
    static MLCameraManager& CameraManagerInstance(std::string cameraType);

};

