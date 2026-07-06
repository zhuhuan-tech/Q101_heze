#include "LightAdjustWidget.h"
#include "LimitMove.h"
#include <QtConcurrent>
#include <QMessageBox>
#include <plugins/PrjCommon/metricsdata.h>

LightAdjustWidget::LightAdjustWidget(QString toolboxName, QWidget* parent)
	: IToolBox(toolboxName, parent)
{
	ui.setupUi(this);
	init();
}

LightAdjustWidget::~LightAdjustWidget()
{

}

void LightAdjustWidget::init()
{
	ui.label_status->setText("Not Connected");
	ui.lineEdit_wave_2->setText("400");
	initCombox();

}

void LightAdjustWidget::initCombox()
{
	ui.comboBox_type->addItem("1");
	ui.comboBox_type->addItem("2");
	ui.comboBox_type->addItem("3");

	ui.comboBox_type->setCurrentIndex(1);
}

void LightAdjustWidget::on_btn_connect_clicked()
{
	if (AAProcess::SpecSet::getInstance()->is_connect())
	{
		ui.label_status->setText("Connected");
	}
	else
	{
		QMessageBox::critical(this, "connect failed!", "", QMessageBox::Ok, QMessageBox::NoButton);
		ui.label_status->setText("Not Connected");
	}
	
}

void LightAdjustWidget::on_btn_disconnect_clicked()
{
	if (AAProcess::SpecSet::getInstance()->dis_connect())
	{
		ui.label_status->setText("Not Connected");
	}
	else
	{
		QMessageBox::critical(this, "disconnect failed!", "", QMessageBox::Ok, QMessageBox::NoButton);
	}
	
}

void LightAdjustWidget::on_btn_set_2_clicked()
{
	QString selectedType = ui.comboBox_type->currentText();
	bool ok;
	int type_value = selectedType.toInt(&ok);

	if (!AAProcess::SpecSet::getInstance()->set_grating(type_value))
	{
		QMessageBox::critical(this, "get cur wavelength failed.", "", QMessageBox::Ok, QMessageBox::NoButton);
		return;
	}
}

void LightAdjustWidget::on_btn_set_clicked()
{
	QString selectedType = ui.comboBox_type->currentText();
	bool ok;
	int type_value = selectedType.toInt(&ok);

	QString waveLength = ui.lineEdit_wave_2->text();
	float wave_value = waveLength.toFloat(&ok);
	if (!ok)
	{
		QMessageBox::critical(this, "Invalid Input", "The input is not a valid number.");
		return;
	}

	if (type_value == 1)
	{
		if (wave_value < 200 || wave_value > 600)
		{
			QMessageBox::critical(this, "Invalid WaveLength", "The WaveLength is 200~600.");
			return;
		}

	}
	else if (type_value == 2)
	{
		if (wave_value < 330 || wave_value > 1000)
		{
			QMessageBox::critical(this, "Invalid WaveLength", "The WaveLength is 330~1000.");
			return;
		}
	}
	else
	{
		if (wave_value < 800 || wave_value > 2400)
		{
			QMessageBox::critical(this, "Invalid WaveLength", "The WaveLength is 800~2400.");
			return;
		}
	}

	if (!AAProcess::SpecSet::getInstance()->set_wave(wave_value))
	{
		QMessageBox::critical(this, "move wavelength failed.", "", QMessageBox::Ok, QMessageBox::NoButton);
		return;
	}

}

