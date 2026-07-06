#include "CollimatedLightTube.h"
#include "CollimatedLightTubeWidget.h"
#include "PrjCommon/recipewrapperregistry.h"
#include "CollimatedLightTubeMode.h"

BT_REGISTER_NODES(factory)
{
	registernode(factory);
}

CollimatedLightTube::CollimatedLightTube()
{
}

CollimatedLightTube::~CollimatedLightTube()
{
}

bool CollimatedLightTube::initialize(const QStringList& arguments, QString* errorMessage)
{
	//CollimatedLightTube4Recipe::getInstance();
	//CollimatedLightTubeWidget* widget = new CollimatedLightTubeWidget("Collimator");
	//connect(widget, &CollimatedLightTubeWidget::updateCollimatorAngle, CollimatedLightTubeMode::GetInstance(), &CollimatedLightTubeMode::updateCollimatorAngle, Qt::BlockingQueuedConnection);

	return true;
}

void CollimatedLightTube::extensionsInitialized()
{
}
