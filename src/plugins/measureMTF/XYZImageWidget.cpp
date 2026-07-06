#include "XYZImageWidget.h"
#include "MLColorimeterMode.h"
#include "MLColorimeterCommon.h"
#include "MLColorimeterHelp.h"
#include "ExposureWidget.h"
#include "loggingwrapper.h"

XYZImageWidget::XYZImageWidget(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	ui.loadStatus->setText("Not Load");
}

XYZImageWidget::~XYZImageWidget()
{

}

void XYZImageWidget::on_loadDataButton_clicked()
{
	// CalibrationConfig2
	QString aperture = "2";
	QString binning = "1";
	QString colorfilter_list = "X,Y,Z,Clear";
	QString lightsource_list = "R,G,B,W";
	// CalibrationFlag
	QString is_load_dark = "1";
	QString is_load_colorshift = "1";
	QString is_load_ffc = "1";
	QString is_load_distortion = "1";
	QString is_load_exposure = "1";
	QString is_load_fliprotation = "1";
	QString is_load_luminance = "0";
	QString is_load_fourcolor = "1";

	CalibrationConfig2 c_config;
	c_config.Aperture = aperture.toStdString() + "mm";
	c_config.Bin = MLColorimeterMode::Instance()->TransIntToBinning(binning.toInt());
	c_config.ColorFilterList.clear();
	QStringList c_list = colorfilter_list.split(",");
	for (const QString& item : c_list)
	{
		c_config.ColorFilterList.push_back(MLColorimeterHelp::instance()->TransStrToFilterEnum(item.toStdString()));
	}
	c_config.LightSourceList.clear();
	QStringList l_list = lightsource_list.split(",");
	for (const QString& item : l_list)
	{
		c_config.LightSourceList.push_back(item.toStdString());
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

	Result ret = MLColorimeterMode::Instance()->ConnectAsync();
	if (!ret.success)
	{
		QString message = QString("Colorimeter connect error, %1").arg(QString::fromStdString(ret.errorMsg));
		LoggingWrapper::instance()->error(message);
		ui.loadStatus->setText("Not Load");
	}

	ret = MLColorimeterMode::Instance()->LoadCalibrationData(c_config, c_flag);
	if (!ret.success)
	{
		QString message = QString("Colorimeter_LoadCalibrationData run error, %1").arg(QString::fromStdString(ret.errorMsg));
		LoggingWrapper::instance()->error(message);
		ui.loadStatus->setText("Not Load");
	}
	else
	{
		ui.loadStatus->setText("Load Success");
	}

}

void XYZImageWidget::on_saveXYZImage_clicked()
{
	QString labelText = ui.loadStatus->text();

	if (labelText == "Load Success")
	{
		QString aperture = "2";
		QString dutid = "SL1";
		QString eyeboxid = "5";
		QString is_auto_et = "0";
		QString x_exposure_time = "200";
		QString y_exposure_time = "200";
		QString z_exposure_time = "200";
		QString is_convert_16bit = "0";
		QString convert_method = "0";
		QString save_raw_image = "1";
		QString save_calibration_image = "0";
		QString binning = "1";
		QString light_source = "W";
		QString image_type = "solid";
		QString nameRule = "#ColorFilter#_#{color}#";

		ImageCaptureConfig config;
		config.binning = MLColorimeterMode::Instance()->TransIntToBinning(binning.toInt());

		ExposureMode mode = is_auto_et.toInt() == 1 ? ExposureMode::Auto : ExposureMode::Fixed;
		ExposureSetting et_setting_x = { mode, x_exposure_time.toDouble() };
		ExposureSetting et_setting_y = { mode, y_exposure_time.toDouble() };
		ExposureSetting et_setting_z = { mode, z_exposure_time.toDouble() };
		config.colorFilterToExposureMap = { {MLFilterEnum::X, et_setting_x}, {MLFilterEnum::Y, et_setting_y}, {MLFilterEnum::Z, et_setting_z} };
		config.lightSource = light_source.toStdString();
		config.aperture = aperture.toStdString() + "mm";
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
		
		QDate current_date = QDate::currentDate();
		std::string currentDate = current_date.toString("yyyyMMdd").toStdString();
		QDateTime time = QDateTime().currentDateTime();
		std::string currentDateTime = time.toString("yyyyMMddTHHmmss").toStdString();
		std::string root_dir = "E:/SAT_GRR/"+ currentDate + "_MetricsTest/";
		std::string dut_seq = "SL1_" + currentDateTime;
		s_config.SavePath = root_dir + dut_seq + "IQ/";
		s_config.Is_Convert = is_convert_16bit.toInt() > 0;
		s_config.ConvertMethod = convert_method.toInt();
		s_config.ImageNameRule = nameRule.toStdString();
		config.saveDataMeta = s_config;

		Result result = MLColorimeterMode::Instance()->GetCaptureTaskManager()->CaptureImageAsyncTask(config);
		if (!result.success)
		{
			QString message = QString("Colorimeter_Capture_XYZImage_Async failed").arg(QString::fromStdString(result.errorMsg));
			LoggingWrapper::instance()->error(message);
		}

	}
	else
	{
		QString message = QString("Colorimeter LoadCalibration Data Failed,Can Not Save Images");
		LoggingWrapper::instance()->error(message);
	}
}
