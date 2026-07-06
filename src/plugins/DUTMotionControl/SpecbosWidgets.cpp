#include "SpecbosWidgets.h"
#include "loggingwrapper.h"
#include "MLSpecbosCommon.h"
#include <QtConcurrent>
#include <QMessageBox>

SpecbosWidgets::SpecbosWidgets(QString toolBoxName, QWidget* parent) : IToolBox(toolBoxName, parent)
{
	ui.setupUi(this);
	init();
}

void SpecbosWidgets::init()
{
	ui.label_status->setText("Not connected.");
	isOpen = false;
	mlspecbos = std::make_unique<ML::MLSpecbos::MLSpecbosLogic>();
}

void SpecbosWidgets::loadDeviceType()
{
	int device_type;
	if ("specbos2501" == ui.deviceType->currentText())
	{
		device_type = 4;
	}
	else
	{
		device_type = 4;
	}
	mlspecbos->SetDeviceType(ML::MLSpecbos::DeviceType(device_type));
}

void SpecbosWidgets::loadCalibrationFile()
{
	mlspecbos->LoadCalibrationFile();
	for (int num = 0; num < MAX_CALIBRATION_FILE_NUM; num++)
	{
		std::string calibrationFileName = mlspecbos->GetCalibrationFile(num);
		ui.comboBox_file->addItem(QString::fromStdString(calibrationFileName), QVariant(num + 1));
	}

}

void SpecbosWidgets::on_btn_connect_clicked()
{
	if (!isOpen)
	{
		mlspecbos->Init("E:\\project\\MLSpecbos\\jeticonfig.ini");
		std::string COM_str = ui.lineEdit_COM->text().toStdString();
		if (mlspecbos->Open("COM" + COM_str))
		{
			isOpen = true;
			ui.label_status->setText("connected.");

			loadDeviceType();
			loadCalibrationFile();
		}
		else
		{
			LoggingWrapper::instance()->warn("Specbos connected failed.");
		}
	}
}

void SpecbosWidgets::on_btn_disconnect_clicked()
{
	if (isOpen)
	{
		if (mlspecbos->Close())
		{
			ui.label_status->setText("Not connected.");
			isOpen = false;
		}
		else
		{
			LoggingWrapper::instance()->warn("Specbos close device failed.");
		}

	}
}

void SpecbosWidgets::on_btnCalibrate_clicked()
{
	if (!isOpen)
	{
		LoggingWrapper::instance()->error("Specbos is not connected.");
		return;
	}

	if (ui.comboBox_file->currentText() == "-- not calibrated --")
	{
		LoggingWrapper::instance()->error("Specbos file is not calibrated");
		return;
	}
	else
	{
		int fileNum = ui.comboBox_file->currentData().toInt();
		if (!mlspecbos->SetCalibrationFileNumber(fileNum))
		{
			return;
		}
	}

	if (!mlspecbos->SetAverage(ui.lineEdit_average->text().toInt()))
	{
		return;
	}
	if (!mlspecbos->SetSyncFreq(ui.lineEdit_freq->text().toFloat()))
	{
		return;
	}
	if (!mlspecbos->SetFixedTintConf(ui.lineEdit_fTint->text().toFloat()))
	{
		return;
	}
	
	mlspecbos->TakeMeasurement();

	m_measurementResults = mlspecbos->GetMeasurementResults();

	ui.label_chromx->setText(QString::number(m_measurementResults->fChromx, 'f', 3));
	ui.label_chromy->setText(QString::number(m_measurementResults->fChromy, 'f', 3));
	ui.label_Y->setText(QString::number(m_measurementResults->fY, 'f', 3));
}

SpecbosWidgets::~SpecbosWidgets()
{}

