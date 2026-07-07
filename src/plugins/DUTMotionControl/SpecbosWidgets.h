#pragma once

#include <QWidget>
#include "Result.h"
#include "itoolbox.h"
#include "ui_SpecbosWidgets.h"
#include "MLSpecbosLogic.h"
#include <QFutureWatcher>
#include <QObject>
#include <QTimer>

class SpecbosWidgets : public Core::IToolBox
{
	Q_OBJECT

public:
	SpecbosWidgets(QString toolboxName = "", QWidget* parent = nullptr);
	~SpecbosWidgets();

private:
	void init();
	void loadDeviceType();
	void loadCalibrationFile();

private slots:
	void on_btn_connect_clicked();
	void on_btn_disconnect_clicked();
	void on_btnCalibrate_clicked();

private:
	Ui::SpecbosWidgetsClass ui;
	bool isOpen;
	//std::unique_ptr<ML::MLSpecbos::MLSpecbosLogic> mlspecbos;
	ML::MLSpecbos::MeasurementConfig m_measurementConfig;
	ML::MLSpecbos::MeasurementResults* m_measurementResults;
	ML::MLSpecbos::CalibrationInfo* m_CalibrationInfo;
	QFutureWatcher<Result> watcher;
	QTimer* m_checkStateTimer;
};

