#include "MLCameraFactory.h"
#include "VieworksCameraManager.h"

MLCameraManager& MLCameraFactory::CameraManagerInstance(std::string cameraType)
{
	if (cameraType == "Vieworks")
	{
		static VieworksCameraManager instance;
		return instance;
	}
	
}
