#include <PrjCommon\Session.h>
#include "IQT4Recipe.h"
#include "loggingwrapper.h"
#include "GenCSVHeader.h"
#include "iqtmodel.h"
#include "EtManage.h"

#include "MLColorimeterMode.h"
#include "MLColorimeterHelp.h"
#include "MLColorimeterCommon.h"
#include "Focus/AutoFocusModel.h"
#include "Parallel/IQ_BasicDataType.h"
#include "Parallel/MetricsProcessorProxy.h"
#include "Parallel/CameraSimulator.h"
#include "iqmetricconfig.h"
#include "ImageRotationConfig.h"
#include"ML_NineCrossDetection.h"
#include"MLSolidDetection.h"
#include "piMotionActor/Motion2DModel.h"
#include "spectrometer.h"

using namespace IQT;
using namespace IQ_Parallel_NS;
IQT4Recipe* IQT4Recipe::self = nullptr;
IQT4Recipe* IQT4Recipe::getInstance() {

	if (!self)
	{
		self = new IQT4Recipe();
	}
	return self;
}

IQT4Recipe::IQT4Recipe(QObject* parent)
	: QObject(parent)
{
	ObjectManager::getInstance()->registerObject("IQT4Recipe", static_cast<void*>(this));
}

IQT4Recipe::~IQT4Recipe()
{
}

QString IQT4Recipe::getNodeValueByName(BT::TreeNode& node, std::string name)
{
	auto f_value = node.getInput<std::string>(name);
	if (!f_value)
	{
		throw BT::RuntimeError("Missing input [force]: ", f_value.error());
	}
	return QString::fromStdString(f_value.value());
}

void IQT4Recipe::loadConfig()
{
	std::ifstream jsonFile(m_configPath);
	std::string contents = std::string((std::istreambuf_iterator<char>(jsonFile)), (std::istreambuf_iterator<char>()));
	jsonFile.close();
	settingJsonObj = Json::parse(contents);
}

void IQT4Recipe::saveMetricsInfo(QString info)
{
	QString imgPaths = MetricsData::instance()->getMTFImgsDir();
	QFile infoFile(imgPaths + "info.txt");
	if (!infoFile.open(QIODevice::Append))
	{
		LoggingWrapper::instance()->error("Save info.txt failed, open file error.");
		return;
	}
	infoFile.write(info.toUtf8());
	infoFile.write("\n");
	infoFile.close();
}

NodeStatus IQT4Recipe::IQ_Connect()
{
	Result res = MLColorimeterMode::Instance()->Connect();
	if (!res.success)
	{
		QString message = QString("Recipe Node [ Colorimeter_Connect_All ] run error, %1").arg(QString::fromStdString(res.errorMsg));
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}

	return BT::NodeStatus::SUCCESS;
}

NodeStatus IQT4Recipe::IQ_Connect_Async() {
	Result res = MLColorimeterMode::Instance()->ConnectAsync();
	if (!res.success)
	{
		QString message = QString("Recipe Node [ Colorimeter_Connect_All_Async ] run error, %1").arg(QString::fromStdString(res.errorMsg));
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}

	return BT::NodeStatus::SUCCESS;
}

NodeStatus IQT4Recipe::IQ_Camera_StopGrabbing()
{

	Result res = MLColorimeterMode::Instance()->StopGrabbing(true);
	if (!res.success)
	{
		QString message = QString("Recipe Node [ Colorimeter_Camera_StopGrabbing ] run error, %1").arg(QString::fromStdString(res.errorMsg));
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}
	return BT::NodeStatus::SUCCESS;
}

NodeStatus IQT4Recipe::IQ_Camera_StartGrabbing()
{
	Result res = MLColorimeterMode::Instance()->StopGrabbing(false);
	if (!res.success)
	{
		QString message = QString("Recipe Node [ Colorimeter_Camera_StartGrabbing ] run error, %1").arg(QString::fromStdString(res.errorMsg));
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}
	return BT::NodeStatus::SUCCESS;
}

NodeStatus IQT4Recipe::IQ_Camera_SetBinning(BT::TreeNode& node)
{
	QString binning = getNodeValueByName(node, "binning");

	Result res;
	res = MLColorimeterMode::Instance()->SetBinning(binning.toInt());
	if (!res.success) {
		QString message = QString("Recipe Node [ Colorimeter_Camera_SetBinning ] run error, %1").arg(QString::fromStdString(res.errorMsg));
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}
	return BT::NodeStatus::SUCCESS;
}

NodeStatus IQT4Recipe::IQ_Camera_SetBinningMode(BT::TreeNode& node)
{
	QString binning_mode = getNodeValueByName(node, "binning_mode");

	Result res;
	res = MLColorimeterMode::Instance()->SetBinningMode(binning_mode);
	if (!res.success) {
		QString message = QString("Recipe Node [ Colorimeter_Camera_SetBinningMode ] run error, %1").arg(QString::fromStdString(res.errorMsg));
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}
	return BT::NodeStatus::SUCCESS;
}

NodeStatus IQT4Recipe::IQ_Camera_SetBinningSelector(BT::TreeNode& node)
{
	QString binning_selector = getNodeValueByName(node, "binning_selector");
	try
	{
		Result res = MLColorimeterMode::Instance()->SetBinningSelector(BinningSelector::Sensor);
		if (!res.success)throw std::runtime_error(res.errorMsg);
		if (binning_selector.toInt() > 0)
		{
			res = MLColorimeterMode::Instance()->SetBinning(ML::CameraV2::Binning::ONE_BY_ONE);
			if (!res.success) throw std::runtime_error(res.errorMsg);
			res = MLColorimeterMode::Instance()->SetBinningSelector(BinningSelector::Logic);
			if (!res.success) throw std::runtime_error(res.errorMsg);
		}
		return BT::NodeStatus::SUCCESS;
	}
	catch (const std::exception& ex)
	{
		QString message = QString("Recipe Node [ Colorimeter_Camera_SetBinningSelector ] run error, %1")
			.arg(QString::fromStdString(ex.what()));
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}
}

NodeStatus IQT4Recipe::IQ_Camera_SetBitDepth(BT::TreeNode& node)
{
	QString bit_depth = getNodeValueByName(node, "bit_depth");

	Result res;
	res = MLColorimeterMode::Instance()->SetBitDepth(bit_depth.toInt());
	if (!res.success) {
		QString message = QString("Recipe Node [ Colorimeter_Camera_SetBitDepth ] run error, %1").arg(QString::fromStdString(res.errorMsg));
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}
	return BT::NodeStatus::SUCCESS;
}

NodeStatus IQT4Recipe::IQ_Camera_SetExposureByAuto(BT::TreeNode& node)
{
	QString init_time = getNodeValueByName(node, "init_time");

	ExposureSetting exposure = { ExposureMode::Auto, init_time.toFloat() };
	Result res = MLColorimeterMode::Instance()->SetExposureTime(exposure);
	if (!res.success) {
		QString message = QString("Recipe Node [ Colorimeter_Camera_SetExposureByAuto ] run error, %1").arg(QString::fromStdString(res.errorMsg));
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}
	return BT::NodeStatus::SUCCESS;
}

NodeStatus IQT4Recipe::IQ_Camera_SetExposureByFix(BT::TreeNode& node)
{
	QString exposure_time = getNodeValueByName(node, "exposure_time");

	ExposureSetting exposure = { ExposureMode::Fixed, exposure_time.toFloat() };
	Result res = MLColorimeterMode::Instance()->SetExposureTime(exposure);
	if (!res.success)
	{
		QString message = QString("Recipe Node [ Colorimeter_Camera_SetExposureByFix ] run error, %1").arg(QString::fromStdString(res.errorMsg));
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}
	return BT::NodeStatus::SUCCESS;
}

NodeStatus IQT4Recipe::IQ_Camera_SetOverExposure(BT::TreeNode& node)
{
	QString over_exposure_factor = getNodeValueByName(node, "over_exposure_factor");

	double expo = MLColorimeterMode::Instance()->GetExposureTime();
	ExposureSetting exposure = { ExposureMode::Fixed, expo * over_exposure_factor.toFloat() };
	Result res = MLColorimeterMode::Instance()->SetExposureTime(exposure);
	if (!res.success)
	{
		QString message = QString("Recipe Node [ Colorimeter_Camera_SetOverExposure ] run error, %1").arg(QString::fromStdString(res.errorMsg));
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}
	return BT::NodeStatus::SUCCESS;
}

NodeStatus IQT4Recipe::IQ_SwitchColorFilter(BT::TreeNode& node)
{
	QString color_filter = getNodeValueByName(node, "color_filter");

	Result res = MLColorimeterMode::Instance()->SetColorFilter(color_filter);
	if (!res.success) {
		QString message = QString("Recipe Node [ Colorimeter_Filter_ColorSwitch ] run error, %1").arg(QString::fromStdString(res.errorMsg));
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}
	return BT::NodeStatus::SUCCESS;
}

NodeStatus IQT4Recipe::IQ_SwitchNDFilter(BT::TreeNode& node)
{
	QString nd_filter = getNodeValueByName(node, "nd_filter");

	Result res = MLColorimeterMode::Instance()->SetNDFilter(nd_filter);
	if (!res.success) {
		QString message = QString("Recipe Node [ Colorimeter_Filter_NDSwitch ] run error, %1").arg(QString::fromStdString(res.errorMsg));
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}
	return BT::NodeStatus::SUCCESS;
}

NodeStatus IQT4Recipe::IQ_SetVID(BT::TreeNode& node)
{
	QString vid = getNodeValueByName(node, "vid");
	Result ret = MLColorimeterMode::Instance()->SetVidSync(vid.toDouble());
	if (!ret.success)
	{
		QString message = QString("Recipe Node [ Colorimeter_Motion_SetVID ] run error, %1").arg(QString::fromStdString(ret.errorMsg));
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}
	return BT::NodeStatus::SUCCESS;
}

NodeStatus IQT4Recipe::IQ_SetSphere(BT::TreeNode& node)
{
	QString sphere = getNodeValueByName(node, "sphere");
	Result ret = MLColorimeterMode::Instance()->SetSphereSync(sphere.toDouble());
	if (!ret.success)
	{
		QString message = QString("Recipe Node [ Colorimeter_Motion_SetSphere ] run error, %1").arg(QString::fromStdString(ret.errorMsg));
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}
	return BT::NodeStatus::SUCCESS;
}

NodeStatus IQT4Recipe::IQ_SetFocusMotion(BT::TreeNode& node)
{
	QString focus = getNodeValueByName(node, "focus");
	Result ret = MLColorimeterMode::Instance()->SetFocusMotionPosSync(focus.toDouble());
	if (!ret.success)
	{
		QString message = QString("Recipe Node [ Colorimeter_Motion_SetFocus ] run error, %1").arg(QString::fromStdString(ret.errorMsg));
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}
	return BT::NodeStatus::SUCCESS;
}

NodeStatus IQT4Recipe::IQ_SetLightFocusMotion(BT::TreeNode& node)
{
	QString color = getNodeValueByName(node, "color").toUpper();
	Result ret = AutoFocusModel::instance()->SetFocusMotionByLight(color);
	if (!ret.success)
	{
		QString message = QString("Recipe Node [ Colorimeter_Motion_SetLightFocus ] run error, %1").arg(QString::fromStdString(ret.errorMsg));
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}
	return BT::NodeStatus::SUCCESS;
}

NodeStatus IQT4Recipe::IQ_LoadCalibrationData(BT::TreeNode& node)
{
	// CalibrationConfig2
	QString aperture = getNodeValueByName(node, "aperture");
	QString binning = getNodeValueByName(node, "binning");
	QString colorfilter_list = getNodeValueByName(node, "colorfilter_list");
	QString lightsource_list = getNodeValueByName(node, "lightsource_list");
	QString ndfliter_list = getNodeValueByName(node, "ndfliter_list");
	// CalibrationFlag
	QString is_load_dark = getNodeValueByName(node, "is_load_dark");
	QString is_load_colorshift = getNodeValueByName(node, "is_load_colorshift");
	QString is_load_ffc = getNodeValueByName(node, "is_load_ffc");
	QString is_load_distortion = getNodeValueByName(node, "is_load_distortion");
	QString is_load_exposure = getNodeValueByName(node, "is_load_exposure");
	QString is_load_fliprotation = getNodeValueByName(node, "is_load_fliprotation");
	QString is_load_luminance = getNodeValueByName(node, "is_load_luminance");
	QString is_load_fourcolor = getNodeValueByName(node, "is_load_fourcolor");

	CalibrationConfig2 c_config;
	c_config.Aperture = aperture.toStdString() + "mm";
	c_config.Bin = MLColorimeterMode::Instance()->TransIntToBinning(binning.toInt());
	c_config.ColorFilterList.clear();
	QStringList c_list = colorfilter_list.split(",");
	for (const QString& item : c_list)
	{
		c_config.ColorFilterList.push_back(MLColorimeterHelp::instance()->TransStrToFilterEnum(item.toStdString()));
	}
	//c_config.InputPath;
	c_config.LightSourceList.clear();
	QStringList l_list = lightsource_list.split(",");
	for (const QString& item : l_list)
	{
		c_config.LightSourceList.push_back(item.toStdString());
	}
	c_config.NDFilterList.clear();
	QStringList nd_list = ndfliter_list.split(",");
	for (const QString& item : nd_list)
	{
		c_config.NDFilterList.push_back(MLColorimeterHelp::instance()->TransStrToFilterEnum(item.toStdString()));
	}
	c_config.RX = { 0,0,0 };
	CalibrationFlag c_flag;
	c_flag.Dark_Flag = is_load_dark.toInt() == 1;
	c_flag.ColorShift_Flag = is_load_colorshift.toInt() == 1;
	c_flag.FFC_Flag = is_load_ffc.toInt() == 1;
	c_flag.Distortion_Flag = is_load_distortion.toInt() == 1;
	c_flag.Exposure_Flag = is_load_exposure.toInt() == 1;
	c_flag.Flip_Rotate_Flag = is_load_fliprotation.toInt() == 1;
	c_flag.Luminance_Flag = is_load_luminance.toInt() == 1;
	c_flag.FourColor_Flag = is_load_fourcolor.toInt() == 1;
	Result ret = MLColorimeterMode::Instance()->LoadCalibrationData(c_config, c_flag);
	if (!ret.success)
	{
		QString message = QString("Recipe Node [ Colorimeter_LoadCalibrationData ] run error, %1").arg(QString::fromStdString(ret.errorMsg));
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}
	return BT::NodeStatus::SUCCESS;
}

NodeStatus IQT4Recipe::IQ_LoadDarkCalibrationData(BT::TreeNode& node)
{
	// CalibrationConfig2
	QString aperture = getNodeValueByName(node, "aperture");
	QString binning = getNodeValueByName(node, "binning");
	QString colorfilter_list = getNodeValueByName(node, "colorfilter_list");
	QString lightsource_list = getNodeValueByName(node, "lightsource_list");
	QString ndfliter_list = getNodeValueByName(node, "ndfliter_list");

	CalibrationConfig2 c_config;
	c_config.Aperture = aperture.toStdString() + "mm";
	c_config.Bin = MLColorimeterMode::Instance()->TransIntToBinning(binning.toInt());
	c_config.ColorFilterList.clear();
	QStringList c_list = colorfilter_list.split(",");
	for (const QString& item : c_list)
	{
		c_config.ColorFilterList.push_back(MLColorimeterHelp::instance()->TransStrToFilterEnum(item.toStdString()));
	}
	//c_config.InputPath;
	c_config.LightSourceList.clear();
	QStringList l_list = lightsource_list.split(",");
	for (const QString& item : l_list)
	{
		c_config.LightSourceList.push_back(item.toStdString());
	}
	c_config.NDFilterList.clear();
	QStringList nd_list = ndfliter_list.split(",");
	for (const QString& item : nd_list)
	{
		c_config.NDFilterList.push_back(MLColorimeterHelp::instance()->TransStrToFilterEnum(item.toStdString()));
	}
	c_config.RX = { 0,0,0 };
	CalibrationFlag c_flag;
	c_flag.Dark_Flag = true;
	Result ret = MLColorimeterMode::Instance()->LoadCalibrationData(c_config, c_flag);
	if (!ret.success)
	{
		QString message = QString("Recipe Node [ Colorimeter_LoadDarkCalibrationData ] run error, %1").arg(QString::fromStdString(ret.errorMsg));
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}
	return BT::NodeStatus::SUCCESS;
}

NodeStatus IQT4Recipe::IQ_LoadFFCCalibrationData(BT::TreeNode& node)
{
	// CalibrationConfig2
	QString aperture = getNodeValueByName(node, "aperture");
	QString binning = getNodeValueByName(node, "binning");
	QString colorfilter_list = getNodeValueByName(node, "colorfilter_list");
	QString lightsource_list = getNodeValueByName(node, "lightsource_list");
	QString ndfliter_list = getNodeValueByName(node, "ndfliter_list");

	CalibrationConfig2 c_config;
	c_config.Aperture = aperture.toStdString() + "mm";
	c_config.Bin = MLColorimeterMode::Instance()->TransIntToBinning(binning.toInt());
	c_config.ColorFilterList.clear();
	QStringList c_list = colorfilter_list.split(",");
	for (const QString& item : c_list)
	{
		c_config.ColorFilterList.push_back(MLColorimeterHelp::instance()->TransStrToFilterEnum(item.toStdString()));
	}
	//c_config.InputPath;
	c_config.LightSourceList.clear();
	QStringList l_list = lightsource_list.split(",");
	for (const QString& item : l_list)
	{
		c_config.LightSourceList.push_back(item.toStdString());
	}
	c_config.NDFilterList.clear();
	QStringList nd_list = ndfliter_list.split(",");
	for (const QString& item : nd_list)
	{
		c_config.NDFilterList.push_back(MLColorimeterHelp::instance()->TransStrToFilterEnum(item.toStdString()));
	}
	c_config.RX = { 0,0,0 };
	CalibrationFlag c_flag;
	c_flag.FFC_Flag = true;
	Result ret = MLColorimeterMode::Instance()->LoadCalibrationData(c_config, c_flag);
	if (!ret.success)
	{
		QString message = QString("Recipe Node [ Colorimeter_LoadFFCCalibrationData ] run error, %1").arg(QString::fromStdString(ret.errorMsg));
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}
	return BT::NodeStatus::SUCCESS;
}

NodeStatus IQT4Recipe::IQ_Capture_XYZImage_Sync(BT::TreeNode& node)
{
	QString aperture = getNodeValueByName(node, "aperture");
	QString dutid = getNodeValueByName(node, "dutid");
	QString eyeboxid = getNodeValueByName(node, "eyeboxid");
	QString is_auto_et = getNodeValueByName(node, "is_auto_et");
	QString x_exposure_time = getNodeValueByName(node, "x_exposure_time");
	QString y_exposure_time = getNodeValueByName(node, "y_exposure_time");
	QString z_exposure_time = getNodeValueByName(node, "z_exposure_time");
	QString is_convert_16bit = getNodeValueByName(node, "is_convert_16bit");
	QString convert_method = getNodeValueByName(node, "convert_method");
	QString save_raw_image = getNodeValueByName(node, "save_raw_image");
	QString save_calibration_image = getNodeValueByName(node, "save_calibration_image");
	QString binning = getNodeValueByName(node, "binning");
	QString ndfilter = getNodeValueByName(node, "nd_filter");
	QString light_source = getNodeValueByName(node, "light_source");
	QString image_type = getNodeValueByName(node, "image_type");

	ImageCaptureConfig config;
	config.binning = MLColorimeterMode::Instance()->TransIntToBinning(binning.toInt());
	//norx
	//config.rx = MLColorimeterMode::Instance()->GetRX();
	ExposureMode mode = is_auto_et.toInt() == 1 ? ExposureMode::Auto : ExposureMode::Fixed;
	ExposureSetting et_setting_x = { mode, x_exposure_time.toDouble() };
	ExposureSetting et_setting_y = { mode, y_exposure_time.toDouble() };
	ExposureSetting et_setting_z = { mode, z_exposure_time.toDouble() };
	config.colorFilterToExposureMap = { {MLFilterEnum::X, et_setting_x}, {MLFilterEnum::Y, et_setting_y}, {MLFilterEnum::Z, et_setting_z} };
	config.lightSource = light_source.toStdString();
	config.aperture = aperture.toStdString() + "mm";
	config.ndFilter = MLColorimeterHelp::instance()->TransStrToFilterEnum(ndfilter.toStdString());
	//config.rx = { 0,0,0 };
	config.isSaveCali = save_calibration_image.toInt() > 0;
	config.isSaveRaw = save_raw_image.toInt() > 0;
	CalibrationFlag p_flag;
	p_flag.Dark_Flag = true;
	p_flag.ColorShift_Flag = true;
	p_flag.Distortion_Flag = true;
	p_flag.FFC_Flag = true;
	p_flag.Flip_Rotate_Flag = true;
	p_flag.Exposure_Flag = true;
	p_flag.FourColor_Flag = true;
	config.calibrationFlag = p_flag;

	SaveDataMeta s_config;
	s_config.DutID = dutid.toStdString();
	s_config.EyeboxID = eyeboxid.toStdString();
	s_config.ImageType = image_type.toStdString();
	s_config.SavePath = MetricsData::instance()->getMTFImgsDir().toStdString();
	s_config.Is_Convert = is_convert_16bit.toInt() > 0;
	s_config.ConvertMethod = convert_method.toInt();
	config.saveDataMeta = s_config;

	Result result = MLColorimeterMode::Instance()->GetCaptureTaskManager()->CaptureImageSyncTask(config);
	if (!result.success)
	{
		QString message = QString("Recipe Node [ Colorimeter_Capture_XYZImage_Sync ] run error, %1").arg(QString::fromStdString(result.errorMsg));
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}
	return BT::NodeStatus::SUCCESS;
}

NodeStatus IQT4Recipe::IQ_Capture_GrayImage_Sync(BT::TreeNode& node)
{
	QString aperture = getNodeValueByName(node, "aperture");
	QString dutid = getNodeValueByName(node, "dutid");
	QString eyeboxid = getNodeValueByName(node, "eyeboxid");
	QString is_set_et = getNodeValueByName(node, "is_set_et");
	QString is_auto_et = getNodeValueByName(node, "is_auto_et");
	QString exposure_time = getNodeValueByName(node, "exposure_time");
	QString is_convert_16bit = getNodeValueByName(node, "is_convert_16bit");
	QString convert_method = getNodeValueByName(node, "convert_method");
	QString save_raw_image = getNodeValueByName(node, "save_raw_image");
	QString save_calibration_image = getNodeValueByName(node, "save_calibration_image");
	QString is_hdr = getNodeValueByName(node, "is_hdr");
	QString max_exposure_time = getNodeValueByName(node, "max_exposure_time");
	QString binning = getNodeValueByName(node, "binning");
	QString nd_filter = getNodeValueByName(node, "nd_filter");
	QString color_filter = getNodeValueByName(node, "color_filter");
	QString light_source = getNodeValueByName(node, "light_source");
	QString image_type = getNodeValueByName(node, "image_type");

	ImageCaptureConfig config;
	config.binning = MLColorimeterMode::Instance()->TransIntToBinning(binning.toInt());
	//norx
	//config.rx = MLColorimeterMode::Instance()->GetRX();
	//config.rx = { 0,0,0 };
	config.ndFilter = MLColorimeterHelp::instance()->TransStrToFilterEnum(nd_filter.toStdString());
	config.lightSource = light_source.toStdString();
	config.aperture = aperture.toStdString() + "mm";
	config.hdrConfig.isHDR = is_hdr.toInt() > 0;
	config.hdrConfig.maxExposure = max_exposure_time.toDouble();
	ExposureSetting et_setting;
	if (is_set_et.isEmpty() || is_set_et.toInt() == 1)
	{
		ExposureMode mode = is_auto_et.toInt() == 1 ? ExposureMode::Auto : ExposureMode::Fixed;
		et_setting = { mode, exposure_time.toDouble() };
		if (exposure_time.isEmpty())
		{
			double time;
			std::string name = (light_source + "_" + image_type + "_" + eyeboxid + "_" + color_filter).toStdString();
			Result ret = MLColorimeterMode::Instance()->GetFixExposureTime(name, time);
			if (!ret.success)
			{
				QString message = QString("Recipe Node [ Colorimeter_Capture_GrayImage_Sync ] run error, %1").arg(QString::fromStdString(ret.errorMsg));
				LoggingWrapper::instance()->error(message);
				return BT::NodeStatus::FAILURE;
			}
			et_setting = { mode, time };
		}
	}
	else
	{
		et_setting = { ExposureMode::Fixed, MLColorimeterMode::Instance()->GetExposureTime() };
	}
	config.colorFilterToExposureMap = {
		{MLColorimeterHelp::instance()->TransStrToFilterEnum(color_filter.toStdString()), et_setting}
	};
	config.isSaveCali = save_calibration_image.toInt() > 0;
	config.isSaveRaw = save_raw_image.toInt() > 0;
	CalibrationFlag p_flag;
	p_flag.Dark_Flag = true;
	p_flag.ColorShift_Flag = true;
	p_flag.Distortion_Flag = true;
	p_flag.FFC_Flag = true;
	p_flag.Flip_Rotate_Flag = true;
	config.calibrationFlag = p_flag;

	SaveDataMeta s_config;
	s_config.DutID = dutid.toStdString();
	s_config.EyeboxID = eyeboxid.toStdString();
	s_config.ImageType = image_type.toStdString();
	s_config.SavePath = MetricsData::instance()->getMTFImgsDir().toStdString();
	s_config.Is_Convert = is_convert_16bit.toInt() > 0;
	s_config.ConvertMethod = convert_method.toInt();
	config.saveDataMeta = s_config;

	Result result = MLColorimeterMode::Instance()->GetCaptureTaskManager()->CaptureImageSyncTask(config);
	if (!result.success)
	{
		QString message = QString("Recipe Node [ Colorimeter_Capture_GrayImage_Sync ] run error, %1").arg(QString::fromStdString(result.errorMsg));
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}
	return BT::NodeStatus::SUCCESS;
}

NodeStatus IQT4Recipe::IQ_Capture_LuminanceImage_Sync(BT::TreeNode& node)
{
	QString aperture = getNodeValueByName(node, "aperture");
	QString dutid = getNodeValueByName(node, "dutid");
	QString eyeboxid = getNodeValueByName(node, "eyeboxid");
	QString is_set_et = getNodeValueByName(node, "is_set_et");
	QString is_auto_et = getNodeValueByName(node, "is_auto_et");
	QString exposure_time = getNodeValueByName(node, "exposure_time");
	QString is_convert_16bit = getNodeValueByName(node, "is_convert_16bit");
	QString convert_method = getNodeValueByName(node, "convert_method");
	QString save_raw_image = getNodeValueByName(node, "save_raw_image");
	QString save_calibration_image = getNodeValueByName(node, "save_calibration_image");
	QString is_hdr = getNodeValueByName(node, "is_hdr");
	QString max_exposure_time = getNodeValueByName(node, "max_exposure_time");
	QString binning = getNodeValueByName(node, "binning");
	QString nd_filter = getNodeValueByName(node, "nd_filter");
	QString color_filter = getNodeValueByName(node, "color_filter");
	QString light_source = getNodeValueByName(node, "light_source");
	QString image_type = getNodeValueByName(node, "image_type");

	ImageCaptureConfig config;
	config.binning = MLColorimeterMode::Instance()->TransIntToBinning(binning.toInt());
	//norx
	//config.rx = MLColorimeterMode::Instance()->GetRX();
	//config.rx = { 0,0,0 };
	config.ndFilter = MLColorimeterHelp::instance()->TransStrToFilterEnum(nd_filter.toStdString());
	config.lightSource = light_source.toStdString();
	config.aperture = aperture.toStdString() + "mm";
	config.hdrConfig.isHDR = is_hdr.toInt() > 0;
	config.hdrConfig.maxExposure = max_exposure_time.toDouble();
	ExposureSetting et_setting;
	if (is_set_et.isEmpty() || is_set_et.toInt() == 1)
	{
		ExposureMode mode = is_auto_et.toInt() == 1 ? ExposureMode::Auto : ExposureMode::Fixed;
		et_setting = { mode, exposure_time.toDouble() };
		if (exposure_time.isEmpty())
		{
			double time;
			std::string name = (light_source + "_" + image_type + "_" + eyeboxid + "_" + color_filter).toStdString();
			Result ret = MLColorimeterMode::Instance()->GetFixExposureTime(name, time);
			if (!ret.success)
			{
				QString message = QString("Recipe Node [ Colorimeter_Capture_LuminanceImage_Sync ] run error, %1").arg(QString::fromStdString(ret.errorMsg));
				LoggingWrapper::instance()->error(message);
				return BT::NodeStatus::FAILURE;
			}
			et_setting = { mode, time };
		}
	}
	else
	{
		et_setting = { ExposureMode::Fixed, MLColorimeterMode::Instance()->GetExposureTime() };
	}
	config.colorFilterToExposureMap = {
		{MLColorimeterHelp::instance()->TransStrToFilterEnum(color_filter.toStdString()), et_setting}
	};
	config.isSaveCali = save_calibration_image.toInt() > 0;
	config.isSaveRaw = save_raw_image.toInt() > 0;
	CalibrationFlag p_flag;
	p_flag.Dark_Flag = true;
	p_flag.ColorShift_Flag = true;
	p_flag.Distortion_Flag = true;
	p_flag.FFC_Flag = true;
	p_flag.Exposure_Flag = true;
	p_flag.Luminance_Flag = true;
	p_flag.Flip_Rotate_Flag = true;
	config.calibrationFlag = p_flag;

	SaveDataMeta s_config;
	s_config.DutID = dutid.toStdString();
	s_config.EyeboxID = eyeboxid.toStdString();
	s_config.ImageType = image_type.toStdString();
	s_config.SavePath = MetricsData::instance()->getMTFImgsDir().toStdString();
	s_config.Is_Convert = is_convert_16bit.toInt() > 0;
	s_config.ConvertMethod = convert_method.toInt();
	config.saveDataMeta = s_config;

	Result result = MLColorimeterMode::Instance()->GetCaptureTaskManager()->CaptureImageSyncTask(config);
	if (!result.success)
	{
		QString message = QString("Recipe Node [ Colorimeter_Capture_LuminanceImage_Sync ] run error, %1").arg(QString::fromStdString(result.errorMsg));
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}
	return BT::NodeStatus::SUCCESS;
}

NodeStatus IQT4Recipe::IQ_Capture_XYZImage_Async(BT::TreeNode& node)
{
	QString aperture = getNodeValueByName(node, "aperture");
	QString dutid = getNodeValueByName(node, "dutid");
	QString eyeboxid = getNodeValueByName(node, "eyeboxid");
	QString is_auto_et = getNodeValueByName(node, "is_auto_et");
	QString x_exposure_time = getNodeValueByName(node, "x_exposure_time");
	QString y_exposure_time = getNodeValueByName(node, "y_exposure_time");
	QString z_exposure_time = getNodeValueByName(node, "z_exposure_time");
	QString is_convert_16bit = getNodeValueByName(node, "is_convert_16bit");
	QString convert_method = getNodeValueByName(node, "convert_method");
	QString save_raw_image = getNodeValueByName(node, "save_raw_image");
	QString save_calibration_image = getNodeValueByName(node, "save_calibration_image");
	QString binning = getNodeValueByName(node, "binning");
	QString ndfilter = getNodeValueByName(node, "nd_filter");
	QString light_source = getNodeValueByName(node, "light_source");
	QString image_type = getNodeValueByName(node, "image_type");
	QString nameRule = getNodeValueByName(node, "nameRule");

	ImageCaptureConfig config;
	config.binning = MLColorimeterMode::Instance()->TransIntToBinning(binning.toInt());
	//norx
	//config.rx = MLColorimeterMode::Instance()->GetRX();
	ExposureMode mode = is_auto_et.toInt() == 1 ? ExposureMode::Auto : ExposureMode::Fixed;
	ExposureSetting et_setting_x = { mode, x_exposure_time.toDouble() };
	ExposureSetting et_setting_y = { mode, y_exposure_time.toDouble() };
	ExposureSetting et_setting_z = { mode, z_exposure_time.toDouble() };
	config.colorFilterToExposureMap = { {MLFilterEnum::X, et_setting_x}, {MLFilterEnum::Y, et_setting_y}, {MLFilterEnum::Z, et_setting_z} };
	config.lightSource = light_source.toStdString();
	config.aperture = aperture.toStdString() + "mm";
	config.ndFilter = MLColorimeterHelp::instance()->TransStrToFilterEnum(ndfilter.toStdString());
	//config.rx = { 0,0,0 };
	config.isSaveCali = save_calibration_image.toInt() > 0;
	config.isSaveRaw = save_raw_image.toInt() > 0;
	CalibrationFlag p_flag;
	p_flag.Dark_Flag = true;
	p_flag.ColorShift_Flag = true;
	p_flag.Distortion_Flag = true;
	p_flag.FFC_Flag = true;
	p_flag.Flip_Rotate_Flag = false;
	p_flag.Exposure_Flag = true;
	p_flag.FourColor_Flag = true;
	config.calibrationFlag = p_flag;

	SaveDataMeta s_config;
	s_config.DutID = dutid.toStdString();
	s_config.EyeboxID = eyeboxid.toStdString();
	s_config.ImageType = image_type.toStdString();
	s_config.SavePath = MetricsData::instance()->getMTFImgsDir().toStdString();
	s_config.Is_Convert = is_convert_16bit.toInt() > 0;
	s_config.ConvertMethod = convert_method.toInt();
	s_config.ImageNameRule = nameRule.toStdString();
	config.saveDataMeta = s_config;

	Result result = MLColorimeterMode::Instance()->GetCaptureTaskManager()->CaptureImageAsyncTask(config);
	if (!result.success)
	{
		QString message = QString("Recipe Node [ Colorimeter_Capture_XYZImage_Async ] run error, %1").arg(QString::fromStdString(result.errorMsg));
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}
	return BT::NodeStatus::SUCCESS;
}

NodeStatus IQT4Recipe::IQ_Capture_GrayImage_Async(BT::TreeNode& node)
{
	QString aperture = getNodeValueByName(node, "aperture");
	QString dutid = getNodeValueByName(node, "dutid");
	QString eyeboxid = getNodeValueByName(node, "eyeboxid");
	QString is_set_et = getNodeValueByName(node, "is_set_et");
	QString is_auto_et = getNodeValueByName(node, "is_auto_et");
	QString exposure_time = getNodeValueByName(node, "exposure_time");
	QString is_convert_16bit = getNodeValueByName(node, "is_convert_16bit");
	QString convert_method = getNodeValueByName(node, "convert_method");
	QString save_raw_image = getNodeValueByName(node, "save_raw_image");
	QString save_calibration_image = getNodeValueByName(node, "save_calibration_image");
	QString is_hdr = getNodeValueByName(node, "is_hdr");
	QString max_exposure_time = getNodeValueByName(node, "max_exposure_time");
	QString binning = getNodeValueByName(node, "binning");
	QString nd_filter = getNodeValueByName(node, "nd_filter");
	QString color_filter = getNodeValueByName(node, "color_filter");
	QString light_source = getNodeValueByName(node, "light_source");
	QString image_type = getNodeValueByName(node, "image_type");

	ImageCaptureConfig config;
	config.binning = MLColorimeterMode::Instance()->TransIntToBinning(binning.toInt());
	//norx
	//config.rx = MLColorimeterMode::Instance()->GetRX();
	//config.rx = { 0,0,0 };
	config.isSaveCali = save_calibration_image.toInt() > 0;
	config.isSaveRaw = save_raw_image.toInt() > 0;
	config.ndFilter = MLColorimeterHelp::instance()->TransStrToFilterEnum(nd_filter.toStdString());
	config.lightSource = light_source.toStdString();
	config.aperture = aperture.toStdString() + "mm";
	config.hdrConfig.isHDR = is_hdr.toInt() > 0;
	config.hdrConfig.maxExposure = max_exposure_time.toDouble();
	ExposureSetting et_setting;
	if (is_set_et.isEmpty() || is_set_et.toInt() == 1)
	{
		ExposureMode mode = is_auto_et.toInt() == 1 ? ExposureMode::Auto : ExposureMode::Fixed;
		et_setting = { mode, exposure_time.toDouble() };
		if (exposure_time.isEmpty())
		{
			double time;
			std::string name = (light_source + "_" + image_type + "_" + eyeboxid + "_" + color_filter).toStdString();
			Result ret = MLColorimeterMode::Instance()->GetFixExposureTime(name, time);
			if (!ret.success)
			{
				QString message = QString("Recipe Node [ Colorimeter_Capture_GrayImage_Async ] run error, %1").arg(QString::fromStdString(ret.errorMsg));
				LoggingWrapper::instance()->error(message);
				return BT::NodeStatus::FAILURE;
			}
			et_setting = { mode, time };
		}
	}
	else
	{
		et_setting = { ExposureMode::Fixed, MLColorimeterMode::Instance()->GetExposureTime() };
	}
	config.colorFilterToExposureMap = {
		{MLColorimeterHelp::instance()->TransStrToFilterEnum(color_filter.toStdString()), et_setting}
	};
	CalibrationFlag p_flag;
	p_flag.Dark_Flag = true;
	p_flag.ColorShift_Flag = true;
	p_flag.Distortion_Flag = true;
	p_flag.FFC_Flag = true;
	p_flag.Flip_Rotate_Flag = true;
	config.calibrationFlag = p_flag;

	SaveDataMeta s_config;
	s_config.DutID = dutid.toStdString();
	s_config.EyeboxID = eyeboxid.toStdString();
	s_config.ImageType = image_type.toStdString();
	s_config.SavePath = MetricsData::instance()->getMTFImgsDir().toStdString();
	s_config.Is_Convert = is_convert_16bit.toInt() > 0;
	s_config.ConvertMethod = convert_method.toInt();
	s_config.ImageNameRule = "GrayImage";
	config.saveDataMeta = s_config;

	Result result = MLColorimeterMode::Instance()->GetCaptureTaskManager()->CaptureImageAsyncTask(config);
	if (!result.success)
	{
		QString message = QString("Recipe Node [ Colorimeter_Capture_GrayImage_Async ] run error, %1").arg(QString::fromStdString(result.errorMsg));
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}
	return BT::NodeStatus::SUCCESS;
}

NodeStatus IQT4Recipe::IQ_Capture_LuminanceImage_Async(BT::TreeNode& node)
{
	QString aperture = getNodeValueByName(node, "aperture");
	QString dutid = getNodeValueByName(node, "dutid");
	QString eyeboxid = getNodeValueByName(node, "eyeboxid");
	QString is_set_et = getNodeValueByName(node, "is_set_et");
	QString is_auto_et = getNodeValueByName(node, "is_auto_et");
	QString exposure_time = getNodeValueByName(node, "exposure_time");
	QString is_convert_16bit = getNodeValueByName(node, "is_convert_16bit");
	QString convert_method = getNodeValueByName(node, "convert_method");
	QString save_raw_image = getNodeValueByName(node, "save_raw_image");
	QString save_calibration_image = getNodeValueByName(node, "save_calibration_image");
	QString is_hdr = getNodeValueByName(node, "is_hdr");
	QString max_exposure_time = getNodeValueByName(node, "max_exposure_time");
	QString binning = getNodeValueByName(node, "binning");
	QString nd_filter = getNodeValueByName(node, "nd_filter");
	QString color_filter = getNodeValueByName(node, "color_filter");
	QString light_source = getNodeValueByName(node, "light_source");
	QString image_type = getNodeValueByName(node, "image_type");

	ImageCaptureConfig config;
	config.binning = MLColorimeterMode::Instance()->TransIntToBinning(binning.toInt());
	//norx
	//config.rx = MLColorimeterMode::Instance()->GetRX();
	//config.rx = { 0,0,0 };
	config.isSaveCali = save_calibration_image.toInt() > 0;
	config.isSaveRaw = save_raw_image.toInt() > 0;
	config.ndFilter = MLColorimeterHelp::instance()->TransStrToFilterEnum(nd_filter.toStdString());
	config.lightSource = light_source.toStdString();
	config.aperture = aperture.toStdString() + "mm";
	config.hdrConfig.isHDR = is_hdr.toInt() > 0;
	config.hdrConfig.maxExposure = max_exposure_time.toDouble();
	ExposureSetting et_setting;
	if (is_set_et.isEmpty() || is_set_et.toInt() == 1)
	{
		ExposureMode mode = is_auto_et.toInt() == 1 ? ExposureMode::Auto : ExposureMode::Fixed;
		et_setting = { mode, exposure_time.toDouble() };
		if (exposure_time.isEmpty())
		{
			double time;
			std::string name = (light_source + "_" + image_type + "_" + eyeboxid + "_" + color_filter).toStdString();
			Result ret = MLColorimeterMode::Instance()->GetFixExposureTime(name, time);
			if (!ret.success)
			{
				QString message = QString("Recipe Node [ Colorimeter_Capture_LuminanceImage_Async ] run error, %1").arg(QString::fromStdString(ret.errorMsg));
				LoggingWrapper::instance()->error(message);
				return BT::NodeStatus::FAILURE;
			}
			et_setting = { mode, time };
		}
	}
	else
	{
		et_setting = { ExposureMode::Fixed, MLColorimeterMode::Instance()->GetExposureTime() };
	}
	config.colorFilterToExposureMap = {
		{MLColorimeterHelp::instance()->TransStrToFilterEnum(color_filter.toStdString()), et_setting}
	};
	CalibrationFlag p_flag;
	p_flag.Dark_Flag = true;
	p_flag.ColorShift_Flag = true;
	p_flag.Distortion_Flag = true;
	p_flag.FFC_Flag = true;
	p_flag.Exposure_Flag = true;
	p_flag.Luminance_Flag = true;
	p_flag.Flip_Rotate_Flag = true;
	config.calibrationFlag = p_flag;

	SaveDataMeta s_config;
	s_config.DutID = dutid.toStdString();
	s_config.EyeboxID = eyeboxid.toStdString();
	s_config.ImageType = image_type.toStdString();
	s_config.SavePath = MetricsData::instance()->getMTFImgsDir().toStdString();
	s_config.Is_Convert = is_convert_16bit.toInt() > 0;
	s_config.ConvertMethod = convert_method.toInt();
	s_config.ImageNameRule = "LuminanceImage_123";
	config.saveDataMeta = s_config;

	Result result = MLColorimeterMode::Instance()->GetCaptureTaskManager()->CaptureImageAsyncTask(config);
	if (!result.success)
	{
		QString message = QString("Recipe Node [ Colorimeter_Capture_LuminanceImage_Async ] run error, %1").arg(QString::fromStdString(result.errorMsg));
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}
	return BT::NodeStatus::SUCCESS;
}

void IQT4Recipe::captureGrayImageAutoExpAsync(QString dut_id, QString eyebox_id, int center_x, int center_y, int pixel_count, int roi_size)
{
	QString aperture = "2";
	QString dutid = dut_id;
	QString eyeboxid = eyebox_id;
	QString is_set_et = "1";
	QString is_auto_et = "1";
	QString exposure_time = "10";
	QString is_convert_16bit = "0";
	QString convert_method = "0";
	QString save_raw_image = "1";
	QString save_calibration_image = "0";
	QString is_hdr = "0";
	QString max_exposure_time = "";
	QString binning = "4";
	QString nd_filter = "";
	QString color_filter = "Clear";
	QString light_source = "W";
	QString image_type = "solid";

	ImageCaptureConfig config;
	//设置ROI参数
	if (roi_size > 500)
	{
		roi_size = 500;
	}
	if (roi_size < 10)
	{
		roi_size = 10;
	}

	config.roiExposureParam.isUseROIs = 1;
	config.roiExposureParam.pixelCount = pixel_count;
	config.roiExposureParam.rois["ROI1"].name = "ROI1";
	config.roiExposureParam.rois["ROI1"].weight = 1;
	config.roiExposureParam.rois["ROI1"].height = roi_size;
	config.roiExposureParam.rois["ROI1"].width = roi_size;
	config.roiExposureParam.rois["ROI1"].x = center_x - roi_size / 2;
	config.roiExposureParam.rois["ROI1"].y = center_y - roi_size / 2;
	//防止roi区域超出图片
	if (config.roiExposureParam.rois["ROI1"].x < 0)
	{
		config.roiExposureParam.rois["ROI1"].x = 0;
	}
	if (config.roiExposureParam.rois["ROI1"].y < 0)
	{
		config.roiExposureParam.rois["ROI1"].y = 0;
	}
	if (config.roiExposureParam.rois["ROI1"].x + roi_size > 2816)
	{
		config.roiExposureParam.rois["ROI1"].width = 2816 - center_x + roi_size / 2;
	}
	if (config.roiExposureParam.rois["ROI1"].y + roi_size > 2300)
	{
		config.roiExposureParam.rois["ROI1"].height = 2300 - center_y + roi_size / 2;
	}
	
	config.binning = MLColorimeterMode::Instance()->TransIntToBinning(binning.toInt());
	config.isSaveCali = save_calibration_image.toInt() > 0;
	config.isSaveRaw = save_raw_image.toInt() > 0;
	config.ndFilter = MLColorimeterHelp::instance()->TransStrToFilterEnum(nd_filter.toStdString());
	config.lightSource = light_source.toStdString();
	config.aperture = aperture.toStdString() + "mm";
	config.hdrConfig.isHDR = is_hdr.toInt() > 0;
	config.hdrConfig.maxExposure = max_exposure_time.toDouble();
	ExposureSetting et_setting;
	if (is_set_et.isEmpty() || is_set_et.toInt() == 1)
	{
		ExposureMode mode = is_auto_et.toInt() == 1 ? ExposureMode::Auto : ExposureMode::Fixed;
		et_setting = { mode, exposure_time.toDouble() };
		if (exposure_time.isEmpty())
		{
			double time;
			std::string name = (light_source + "_" + image_type + "_" + eyeboxid + "_" + color_filter).toStdString();
			Result ret = MLColorimeterMode::Instance()->GetFixExposureTime(name, time);
			if (!ret.success)
			{
				QString message = QString("Recipe Node [ Colorimeter_Capture_GrayImage_AutoExp_Async ] run error, %1").arg(QString::fromStdString(ret.errorMsg));
				LoggingWrapper::instance()->error(message);
			}
			et_setting = { mode, time };
		}
	}
	else
	{
		et_setting = { ExposureMode::Fixed, MLColorimeterMode::Instance()->GetExposureTime() };
	}
	config.colorFilterToExposureMap = {
		{MLColorimeterHelp::instance()->TransStrToFilterEnum(color_filter.toStdString()), et_setting}
	};
	CalibrationFlag p_flag;
	p_flag.Dark_Flag = true;
	p_flag.ColorShift_Flag = true;
	p_flag.Distortion_Flag = true;
	p_flag.FFC_Flag = true;
	p_flag.Flip_Rotate_Flag = true;
	config.calibrationFlag = p_flag;

	SaveDataMeta s_config;
	s_config.DutID = dutid.toStdString();
	s_config.EyeboxID = eyeboxid.toStdString();
	s_config.ImageType = image_type.toStdString();
	s_config.SavePath = MetricsData::instance()->getMTFImgsDir().toStdString();
	s_config.Is_Convert = is_convert_16bit.toInt() > 0;
	s_config.ConvertMethod = convert_method.toInt();
	s_config.ImageNameRule = "GrayImage";
	config.saveDataMeta = s_config;

	Result result = MLColorimeterMode::Instance()->GetCaptureTaskManager()->CaptureImageAsyncTask(config);
	if (!result.success)
	{
		QString message = QString("Recipe Node [ Colorimeter_Capture_GrayImage_AutoExp_Async ] run error, %1").arg(QString::fromStdString(result.errorMsg));
		LoggingWrapper::instance()->error(message);
	}

}

void IQT4Recipe::createResultFile(int theta, int phi, double wavelength, QString dut_id)
{
	QString current_date = QDate::currentDate().toString("yyyyMMdd");
	QString time = QDateTime().currentDateTime().toString("yyyyMMddTHHmmss");
	QString root_dir = "E:\\SAT_GRR\\" + current_date + "_AutoExpTest";
	QString dut_seq = dut_id + "_" + time + "_" + QString::number(wavelength);

	int phi_degree = phi;
	int theta_degree = theta;

	QString strX = QString::number(phi_degree);
	QString strY = QString::number(theta_degree);

	dut_seq = dut_seq + "_" + strX + "_" + strY;

	//QString all_csv_path = root_dir + QDir::separator() + all_csvname + ".csv";
	//QString csv_path = root_dir + QDir::separator() + dut_seq + QDir::separator() + csvname + ".csv";
	QString iq_dir = root_dir + QDir::separator() + dut_seq + QDir::separator() + "IQ" + QDir::separator();

	//MetricsData::instance()->setCsvPath(csv_path, all_csv_path);
	MetricsData::instance()->setDutTestSeqName(dut_seq);
	MetricsData::instance()->setMTFImgsDir(iq_dir);
	bool ret = MLColorimeterMode::Instance()->isDirExist(iq_dir);
	if (!ret)
	{
		QString message = QString("Recipe Node [ Colorimeter_Capture_GrayImage_AutoExp_Async ] run error, create iq_dir error!");
		LoggingWrapper::instance()->error(message);
		return;
	}

}

void IQT4Recipe::dealWithPLC2DAndLight(int theta, int phi, double wavelength)
{
	int phi_degree = phi;
	int theta_degree = theta;
	QString message;
	Result result;
	std::string msg;

	//从底层调用光源接口
	if (!spec_move_to_wave(0, wavelength))
	{
		msg = "move wavelength failed.";
		message = QString("Recipe Node [ Colorimeter_Capture_GrayImage_AutoExp_Async ] run error, %1").arg(QString::fromStdString(msg));
		LoggingWrapper::instance()->error(message);
		return;
	}

	//y轴硬件实际角度有偏移，大概是7度
	int offset_y = 7;

	if (phi_degree < -180 || phi_degree > 180 || (phi_degree > 85 && phi_degree < 90))
	{
		message = QString("Recipe Node [ Colorimeter_Capture_GrayImage_AutoExp_Async ] run error, phi beyond limit");
		LoggingWrapper::instance()->error(message);
		return;
	}

	if (theta_degree < 0 || theta_degree > 90)
	{
		message = QString("Recipe Node [ Colorimeter_Capture_GrayImage_AutoExp_Async ] run error, theta beyond limit");
		LoggingWrapper::instance()->error(message);
		return;
	}

	//phi和theta与大轴小轴角度的转换 
	int X_degree;
	if (phi_degree >= -180 && phi_degree <= 85)
	{
		X_degree = phi_degree + 270;
	}
	else
	{
		X_degree = phi_degree - 90;
	}

	int Y_degree = theta_degree + 90 + offset_y;


	if (X_degree > 150 && X_degree < 210)
	{
		if (Y_degree > 135)
		{
			Y_degree = 135;
			message = QString("Y degree should be less than 135 at cur X degree");
			LoggingWrapper::instance()->warn(message);
		}
	}

	result = Motion2DModel::getInstance(PLC2D)->Motion2DMoveAbs(X_degree, 97);
	while (Motion2DModel::getInstance(PLC2D)->is2DMoving())
	{
		QCoreApplication::processEvents();
		_sleep(100);
	}
	if (!result.success)
	{
		msg = "plc 2D  motion move failed.";
		message = QString("Recipe Node [ Colorimeter_Capture_GrayImage_AutoExp_Async ] run error, %1").arg(QString::fromStdString(msg));
		LoggingWrapper::instance()->error(message);
		return;
	}
	
	result = Motion2DModel::getInstance(PLC2D)->Motion2DMoveAbs(X_degree, Y_degree);
	while (Motion2DModel::getInstance(PLC2D)->is2DMoving())
	{
		QCoreApplication::processEvents();
		_sleep(100);
	}
	if (!result.success)
	{
		msg = "plc 2D  motion move failed.";
		message = QString("Recipe Node [ Colorimeter_Capture_GrayImage_AutoExp_Async ] run error, %1").arg(QString::fromStdString(msg));
		LoggingWrapper::instance()->error(message);
		return;
	}

}

NodeStatus IQT4Recipe::IQ_Capture_GrayImage_AutoExp_Async(BT::TreeNode& node)
{
	QString csv_path = getNodeValueByName(node, "csv_path");
	QString pixel_count = getNodeValueByName(node, "pixel_count");
	QString dut_id = getNodeValueByName(node, "dut_id");
	QString eyebox_id = getNodeValueByName(node, "eyebox_id"); 
	QString roi_size = getNodeValueByName(node, "roi_size");
	QFile file(csv_path);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QString message = "无法打开文件: " + csv_path;
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}

	QTextStream in(&file);
	bool isFirstLine = true;

	while (!in.atEnd()) 
	{
		QString line = in.readLine().trimmed();

		// 跳过空行
		if (line.isEmpty()) continue;
		// 跳过表头
		if (isFirstLine) 
		{
			isFirstLine = false;
			continue;
		}

		QStringList fields = line.split(',', QString::SkipEmptyParts);
		if (fields.size() < 6) {
			QString message = "CSV格式不正确，字段不足: " + line;
			LoggingWrapper::instance()->error(message);
			continue;
		}

		// 解析字段
		int theta = fields[0].toInt();
		int phi = fields[1].toInt();
		double wavelength = fields[2].toDouble();
		double centerX = fields[3].toDouble();
		double centerY = fields[4].toDouble();
		int center_x = centerX;
		int center_y = centerY;
		int rainbowPtsNum = fields[5].toInt();

		// 调用业务处理
		dealWithPLC2DAndLight(theta, phi, wavelength);
		createResultFile(theta, phi, wavelength, dut_id);
		captureGrayImageAutoExpAsync(dut_id, eyebox_id, center_x, center_y, pixel_count.toInt(), rainbowPtsNum);
	}

	file.close();
	return BT::NodeStatus::SUCCESS;
}

NodeStatus IQT4Recipe::IQ_WaitForCaptureEnd(BT::TreeNode& node)
{
	QString timeout = getNodeValueByName(node, "timeout");

	Result ret1 = MLColorimeterMode::Instance()->GetCaptureTaskManager()->GetNormalTasksResult(timeout.toInt());
	if (!ret1.success)
	{
		QString message = QString("Recipe Node [ Colorimeter_WaitForCaptureEnd ] run error, %1").arg(QString::fromStdString(ret1.errorMsg));
		LoggingWrapper::instance()->error(message);
	}
	Result ret2 = MLColorimeterMode::Instance()->GetCaptureTaskManager()->GetFourcolorTasksResult(timeout.toInt());
	if (!ret2.success)
	{
		QString message = QString("Recipe Node [ Colorimeter_WaitForCaptureEnd ] run error, %1").arg(QString::fromStdString(ret2.errorMsg));
		LoggingWrapper::instance()->error(message);
	}
	if (!ret1.success || !ret2.success) {
		return BT::NodeStatus::FAILURE;
	}
	return BT::NodeStatus::SUCCESS;
}

NodeStatus IQT4Recipe::IQ_SetTestState(BT::TreeNode& node)
{
	QString is_dut = getNodeValueByName(node, "is_dut");
	QString size = getNodeValueByName(node, "size");
	QString is_update_slb = getNodeValueByName(node, "is_update_slb");
	MLUtils::TestState state;
	state.IsDut = is_dut.toInt() > 0;
	state.IsUpdateSLB = is_update_slb.toInt() > 0;
	state.size = MLUtils::MLUtilCommon::instance()->TransIntToSize(size.toInt());
	MetricsData::instance()->SetTestState(state);
	std::string type = MLUtils::MLUtilCommon::instance()->TransSizeToStr(state.size);
	node.setOutput("size_key", type);
	return BT::NodeStatus::SUCCESS;
}

NodeStatus IQT4Recipe::IQ_Metrics_CreateDBTable()
{
	QString eyeboxs = MetricsData::instance()->getEyeboxQueue();
	QStringList eyeboxlist = eyeboxs.split(",", Qt::SkipEmptyParts);
	bool ok = GenCSVHeader::crateMetricsTable(eyeboxlist);
	if (!ok)
	{
		QString message = QString("Recipe Node [ IQ_Metrics_CreateDBTable ] run error, create Metrics table errors");
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}

	Result ret = GenCSVHeader::writeCsvHeadersToDB();
	if (!ret.success)
	{
		QString message = QString("Recipe Node [ IQ_Metrics_CreateDBTable ] run error, Write csv headers to db error");
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}
	return BT::NodeStatus::SUCCESS;
}

NodeStatus IQT4Recipe::IQ_Metrics_CreateResultFile(BT::TreeNode& node)
{
	QString root_dir = getNodeValueByName(node, "root_dir");
	QString dut_seq = getNodeValueByName(node, "dut_seq");
	//QString csvname = getNodeValueByName(node, "csvname").remove(',');
	//QString all_csvname = getNodeValueByName(node, "all_csvname").remove(',');
	int isOffline = getNodeValueByName(node, "IsOffline").toInt();
	int isBatchProcessing = getNodeValueByName(node, "IsBatchProcessing").toInt();

	if (isOffline != 1)
	{
		float phi_degree = getNodeValueByName(node, "phi_degree").toFloat();
		float theta_degree = getNodeValueByName(node, "theta_degree").toFloat();
		float exp_time = getNodeValueByName(node, "exp_time").toFloat();

		QString strX = QString::number(phi_degree);
		QString strY = QString::number(theta_degree);
		//QString expTime = QString::number(exp_time);
		dut_seq = dut_seq + "_" + strX + "_" + strY;
	}

	//QString all_csv_path = root_dir + QDir::separator() + all_csvname + ".csv";
	//QString csv_path = root_dir + QDir::separator() + dut_seq + QDir::separator() + csvname + ".csv";
	QString iq_dir = root_dir + QDir::separator() + dut_seq + QDir::separator() + "IQ" + QDir::separator();
	if(!isBatchProcessing)
		iq_dir = root_dir + QDir::separator() + dut_seq + QDir::separator() + "IQ" + QDir::separator();
	else
		iq_dir = root_dir;

	//QString throughfocus_dir = root_dir + QDir::separator() + dut_seq + QDir::separator() + "ThroughFocus" + QDir::separator();
	//MetricsData::instance()->setCsvPath(csv_path, all_csv_path);
	MetricsData::instance()->setDutTestSeqName(dut_seq);
	MetricsData::instance()->setMTFImgsDir(iq_dir);
	MetricsData::instance()->setIsBatchProcessing(isBatchProcessing);
	bool ret = MLColorimeterMode::Instance()->isDirExist(iq_dir);
	if (!ret)
	{
		QString message = QString("Recipe Node [ IQ_Metrics_CreateResultFile ] run error, create iq_dir error!");
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}

	//QFile file(all_csv_path);
	//if (!file.exists())
	//{
	//	bool ok = false;
	//	ok = IQTModel::instance()->createMetricsResutCSVFile(all_csv_path);
	//	if (!ok)
	//	{
	//		QString message = QString("Recipe Node [ IQ_Metrics_CreateResultFile ] run error, create all csv file error!");
	//		LoggingWrapper::instance()->error(message);
	//		return BT::NodeStatus::FAILURE;
	//	}
	//}
	//QFile file1(csv_path);
	//if (!file1.exists())
	//{
	//	bool ok = false;
	//	ok = IQTModel::instance()->createMetricsResutCSVFile(csv_path);
	//	if (!ok)
	//	{
	//		QString message = QString("Recipe Node [ IQ_Metrics_CreateResultFile ] run error, create csv file error!");
	//		LoggingWrapper::instance()->error(message);
	//		return BT::NodeStatus::FAILURE;
	//	}
	//}
	return BT::NodeStatus::SUCCESS;
}

NodeStatus IQT4Recipe::IQ_Metrics_Calculate(BT::TreeNode& node)
{
	QString metrics_name = getNodeValueByName(node, "metrics_name");
	try {
		Result res = IQTModel::instance()->calulateOneMetricsByRecipe(metrics_name);
		if (!res.success)
		{
			QString message = QString("Recipe Node [ IQ_Metrics_Calculate ] run error, Metrics %1 %2").arg(metrics_name).arg(QString::fromStdString(res.errorMsg));
			LoggingWrapper::instance()->error(message);
			return BT::NodeStatus::FAILURE;
		}
	}
	catch (const std::exception e)
	{
		QString message = QString("Recipe Node [ IQ_Metrics_Calculate ] run error, Metrics %1 %2").arg(metrics_name).arg(e.what());
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}
	return BT::NodeStatus::SUCCESS;
}

NodeStatus IQT::IQT4Recipe::IQ_Metrics_Location_Clear(BT::TreeNode& node)
{
	MLIQMetrics::MLNineCrossDetection cross;
	cross.setDetectionFlag(false, MLIQMetrics::SMALLFOV);
	cross.setDetectionFlag(false, MLIQMetrics::BIGFOV);
	MLIQMetrics::MLSolidDetection solid;
	solid.setDetectionFlag(false, MLIQMetrics::SMALLFOV);
	solid.setDetectionFlag(false, MLIQMetrics::BIGFOV);
	//MLIQMetrics::isNineCrossDetectionFlagBig = false;
	//MLIQMetrics::isNineCrossDetectionFlagSmall = false;
	//MLIQMetrics::isSolidDetectionFlagBig = false;
	//MLIQMetrics::isSolidDetectionFlagSmall = false;
	return NodeStatus::SUCCESS;
}

NodeStatus IQT4Recipe::IQ_Metrics_ExportToCSV()
{
	QString csvPath = MetricsData::instance()->getCsvPath();
	QString allcsvPath = MetricsData::instance()->getAllCsvPath();

	Result ret = IQTModel::instance()->saveRecipeMetricsToCsv(csvPath);
	if (!ret.success)
	{
		QString message = QString("Recipe Node [ IQ_Metrics_ExportToCSV ] run error, %1").arg(QString::fromStdString(ret.errorMsg));
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}
	QString testresult = QString::fromStdString(ret.errorMsg);
	IQTModel::instance()->saveRecipeMetricsToCsv(allcsvPath);
	IQTModel::instance()->deleteDataBase();

	//transpose csv
	bool res = IQTModel::instance()->transposeCsv(csvPath);
	res = IQTModel::instance()->transposeCsv(allcsvPath);

	if (!testresult.contains("PASS")) {
		QString message = QString("Recipe Node [ IQ_Metrics_ExportToCSV ] run error, %1").arg(testresult);
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}
	return BT::NodeStatus::SUCCESS;
}

NodeStatus IQT4Recipe::IQ_ThroughFocus_Diopter(BT::TreeNode& node) {

	QString file_name = getNodeValueByName(node, "file_name");
	QString direction = getNodeValueByName(node, "direction");
	QString diopter_max = getNodeValueByName(node, "diopter_max");
	QString diopter_min = getNodeValueByName(node, "diopter_min");
	QString rough_step_num = getNodeValueByName(node, "rough_step_num");
	QString is_use_fine = getNodeValueByName(node, "is_use_fine");
	QString fine_step = getNodeValueByName(node, "fine_step");
	QString is_chess_mode = getNodeValueByName(node, "is_chess_mode");
	QString is_lpmm_unit = getNodeValueByName(node, "is_lpmm_unit");
	QString freq = getNodeValueByName(node, "freq");
	QString is_save_images = getNodeValueByName(node, "is_save_images");

	DiopterScanConfig config;
	config.DiopterMax = diopter_max.toDouble();
	config.DiopterMin = diopter_min.toDouble();
	config.RoughStepNumber = rough_step_num.toInt();
	config.IsUseFineAdjust = is_use_fine > 0;
	config.FineStepNumber = fine_step.toInt();
	config.ChessMode = is_chess_mode > 0;
	config.LpmmUnit = is_lpmm_unit > 0;
	config.StoreResultImg = is_save_images > 0;
	config.Freq = freq.toDouble();

	Result ret = AutoFocusModel::instance()->DiopterScanProcess(config, file_name.toStdString(), direction.toStdString());
	if (!ret.success)
	{
		QString message = QString::fromStdString("Recipe Node [ Colorimeter_Motion_AutoFocus_Diopter ] run error, " + ret.errorMsg);
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}
	return BT::NodeStatus::SUCCESS;
}

NodeStatus IQT::IQT4Recipe::IQ_StartParallelCalculate(BT::TreeNode& node)
{
	QStringList metricslist = getNodeValueByName(node, "metrics").split(",", Qt::SkipEmptyParts);
	QStringList eyeboxlist = getNodeValueByName(node, "eyeboxlist").split(",", Qt::SkipEmptyParts);

	Result res = MetricsProcessorProxy::GetInstance()->StartParallelCalculate(metricslist, eyeboxlist);
	if (!res.success)
	{
		QString message = QString("Recipe Node [ IQ_StartParallelCalculate ] run error, %1").arg(QString::fromStdString(res.errorMsg));
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}
	return BT::NodeStatus::SUCCESS;
}

NodeStatus IQT::IQT4Recipe::IQ_StopParallelCalculate(BT::TreeNode& node)
{
	Result res = MetricsProcessorProxy::GetInstance()->StopParallelCalculate();
	if (!res.success)
	{
		QString message = QString("Recipe Node [ IQ_StopParallelCalculate ] run error, %1").arg(QString::fromStdString(res.errorMsg));
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}
	return BT::NodeStatus::SUCCESS;
}

NodeStatus IQT::IQT4Recipe::IQ_WaitParallelCalculateEnd(BT::TreeNode& node)
{
	int TimeOut = 100;
	QString sTimeOut = getNodeValueByName(node, "timeout");

	if (!sTimeOut.isEmpty())
		TimeOut = sTimeOut.toInt();

	std::atomic<bool> stop_flag(false);

	std::future<void> result = std::async(std::launch::async, [&stop_flag, &node]()
		{
			while (1)
			{
				if (stop_flag.load() || node.IsStopChildNodes()) {
					break;
				}

				if (IQ_TaskState::Idle == MetricsProcessorProxy::GetInstance()->getRunningStatus())
					break;

				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
		});

	std::future_status status = result.wait_for(std::chrono::seconds(TimeOut));

	if (future_status::timeout == status)
	{
		qInfo() << "future_status::timeout";
		stop_flag = true;
		result.get();
		MetricsProcessorProxy::GetInstance()->StopParallelCalculate();
		return BT::NodeStatus::SUCCESS;
	}
	else if (future_status::ready == status)
	{
		qInfo() << "future_status::ready";
		MetricsProcessorProxy::GetInstance()->ClearCache();
		return BT::NodeStatus::SUCCESS;
	}
	else if (future_status::deferred == status)
	{
		MetricsProcessorProxy::GetInstance()->ClearCache();
		return BT::NodeStatus::FAILURE;
	}

	return BT::NodeStatus::SUCCESS;
}

NodeStatus IQT4Recipe::IQ_ThroughFocus_MotionPos(BT::TreeNode& node) {

	QString file_name = getNodeValueByName(node, "file_name");
	QString direction = getNodeValueByName(node, "direction");
	QString focus_max = getNodeValueByName(node, "focus_max");
	QString focus_min = getNodeValueByName(node, "focus_min");
	QString rough_step = getNodeValueByName(node, "rough_step");
	QString is_use_fine = getNodeValueByName(node, "is_use_fine");
	QString fine_range = getNodeValueByName(node, "fine_range");
	QString fine_step = getNodeValueByName(node, "fine_step");
	QString is_chess_mode = getNodeValueByName(node, "is_chess_mode");
	QString is_lpmm_unit = getNodeValueByName(node, "is_lpmm_unit");
	QString freq = getNodeValueByName(node, "freq");

	FocusScanConfig config;
	config.FocusMin = focus_max.toDouble();
	config.FocusMax = focus_min.toDouble();
	config.RoughStep = rough_step.toDouble();
	config.IsUseFineAdjust = is_use_fine > 0;
	config.FineRange = fine_range.toDouble();
	config.FineStep = fine_step.toDouble();
	config.ChessMode = is_chess_mode > 0;
	config.LpmmUnit = is_lpmm_unit > 0;
	config.Freq = freq.toDouble();

	Result ret = AutoFocusModel::instance()->FocusScanProcess(config, file_name.toStdString(), direction.toStdString());
	if (!ret.success)
	{
		QString message = QString::fromStdString("Recipe Node [ Colorimeter_Motion_AutoFocus_Position ] run error, " + ret.errorMsg);
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}
	return BT::NodeStatus::SUCCESS;
}

NodeStatus IQT4Recipe::IQ_SetEyeboxList(BT::TreeNode& node)
{
	QString eyeboxlist = getNodeValueByName(node, "eyeboxlist");
	if (eyeboxlist == "preset")
	{
		QStringList ids = GenCSVHeader::getMetricsEyeboxIds();
		eyeboxlist = ids.join(",");
	}
	MetricsData::instance()->setEyeboxQueue(eyeboxlist);
	node.setOutput("key", eyeboxlist.toStdString());
	return BT::NodeStatus::SUCCESS;
}

NodeStatus IQT4Recipe::IQ_Metrics_LoadLimitConfig(BT::TreeNode& node)
{
	QString file_name = getNodeValueByName(node, "file_name");
	QString file_path = "./config/limit/" + file_name;
	IQMetricConfig::instance()->loadLimitInfos(file_path.toStdString().c_str());
	QString message = QString("load limit file " + file_path + " finish.");
	LoggingWrapper::instance()->info(message);
	return BT::NodeStatus::SUCCESS;
}

NodeStatus IQT4Recipe::IQ_SetDutID(BT::TreeNode& node)
{
	QString dutid = getNodeValueByName(node, "id_value");
	MetricsData::instance()->setDutBarCode(dutid);
	node.setOutput("id_key", dutid.toStdString());
	return BT::NodeStatus::SUCCESS;
}

NodeStatus IQT4Recipe::IQ_UpdateFlipRotation()
{
	MLUtils::TestState state = MetricsData::instance()->GetTestState();
	if (state.IsDut && state.eyeType == MLUtils::EyeType::EyeType_UnKnown)
	{
		QString message = QString::fromStdString("Recipe Node [ IQ_UpdateFlipRotation ] run error, current eye type is unknown.");
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}
	Flipping flip_x; Flipping flip_y; ML::MLColorimeter::Rotation rotate;
	if (state.IsDut && state.eyeType == MLUtils::EyeType::Right)
		ImageRotationConfig::instance()->GetFilpRotationConfig_DutRightEye(flip_x, flip_y, rotate);
	else if (state.IsDut && state.eyeType == MLUtils::EyeType::Left)
		ImageRotationConfig::instance()->GetFilpRotationConfig_DutLeftEye(flip_x, flip_y, rotate);
	else
		ImageRotationConfig::instance()->GetFilpRotationConfig_SLB(flip_x, flip_y, rotate);
	if (!MLColorimeterMode::Instance()->UpdateFlipRotation(flip_x, flip_y, rotate))
	{
		QString message = QString::fromStdString("Recipe Node [ IQ_UpdateFlipRotation ] run error, update Flip_Rotate.json error.");
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}
	return BT::NodeStatus::SUCCESS;
}

NodeStatus IQT4Recipe::IQ_LoadFixExposureTime()
{
	if (MLColorimeterMode::Instance()->ReadFixExposureTime())
		return BT::NodeStatus::SUCCESS;
	return BT::NodeStatus::FAILURE;
}

NodeStatus IQT4Recipe::IQ_UpdateFixExposureTimeConfig()
{
	if (MLColorimeterMode::Instance()->UpdateFixExposureTimeConfig(MetricsData::instance()->getMTFImgsDir().toStdString()))
		return BT::NodeStatus::SUCCESS;
	return BT::NodeStatus::FAILURE;
}

NodeStatus IQT4Recipe::IQ_CameraSimulator_Create(BT::TreeNode& node)
{
	int timer_interval = 5;

	QString root_dir = getNodeValueByName(node, "img_folder");
	QString s_timer_interval = getNodeValueByName(node, "timer_interval");

	if (!s_timer_interval.isEmpty())
		timer_interval = s_timer_interval.toInt();

	bool ret = CameraSimulator::getInstance()->isDirExist(root_dir);
	if (!ret)
	{
		QString message = QString("Recipe Node [ IQ_CameraSimulator_Create ] run error, create root_dir error!");
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}

	CameraSimulator::getInstance()->initialize(root_dir, timer_interval);
	return BT::NodeStatus::SUCCESS;

}

NodeStatus IQT4Recipe::IQ_CameraSimulator_StartWork(BT::TreeNode& node)
{
	if (CameraSimulator::getInstance()->startWork(node))
		return BT::NodeStatus::SUCCESS;
	else
		return BT::NodeStatus::FAILURE;
}