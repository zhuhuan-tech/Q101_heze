#include "CollimatedLightTube4Recipe.h"
#include "CollimatedLightTubeMode.h"
#include "CollimatedConfig.h"
#include "loggingwrapper.h"

CollimatedLightTube4Recipe* CollimatedLightTube4Recipe::self = nullptr;

CollimatedLightTube4Recipe* CollimatedLightTube4Recipe::getInstance()
{
	if (!self)
	{
		self = new CollimatedLightTube4Recipe();
	}
	return self;
}

CollimatedLightTube4Recipe::CollimatedLightTube4Recipe(QObject* parent)
	:QObject(parent)
{
	ObjectManager::getInstance()->registerObject("CollimatedLightTube4Recipe", static_cast<void*>(this));
}

CollimatedLightTube4Recipe::~CollimatedLightTube4Recipe()
{
}

QString CollimatedLightTube4Recipe::getNodeValueByName(BT::TreeNode& node, std::string name)
{
	auto f_value = node.getInput<std::string>(name);
	if (!f_value)
	{
		throw BT::RuntimeError("Missing input [force]: ", f_value.error());
	}
	return QString::fromStdString(f_value.value());
}

NodeStatus CollimatedLightTube4Recipe::CLT_Connect()
{
	std::string sn = CollimatedConfig::instance()->GetCollimatorSn();
	Result ret = CollimatedLightTubeMode::GetInstance()->Connect(sn.c_str());
	if (!ret.success)
	{
		QString strerr = QString("Recipe [CollimatedLightTubeMode:CLT_Connect] run error,%1").arg(QString::fromStdString(ret.errorMsg));
		LoggingWrapper::instance()->error(strerr);
		return NodeStatus::FAILURE;
	}
	return NodeStatus::SUCCESS;
}

NodeStatus CollimatedLightTube4Recipe::CLT_Close()
{
	bool ret = CollimatedLightTubeMode::GetInstance()->DisConnect();
	if (!ret)
	{
		QString strerr = QString("Recipe [CollimatedLightTubeMode:CLT_Close] run error");
		LoggingWrapper::instance()->error(strerr);
		return NodeStatus::FAILURE;
	}
	return NodeStatus::SUCCESS;
}

NodeStatus CollimatedLightTube4Recipe::CLT_SetExposure(BT::TreeNode& node)
{
	double dexposure = getNodeValueByName(node, "exposure").toDouble();
	bool bauto = getNodeValueByName(node, "is_auto").toInt();
	if (bauto)
	{
		CollimatedLightTubeMode::GetInstance()->SetMLExposureAuto();
	}
	else
	{
		CollimatedLightTubeMode::GetInstance()->SetExposureTime(dexposure);
	}
	return NodeStatus::SUCCESS;
}
