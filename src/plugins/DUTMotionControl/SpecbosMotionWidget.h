#pragma once
#include <QWidget>
#include "Result.h"
#include "itoolbox.h"
#include "ui_SpecbosMotionWidget.h"
#include <QFutureWatcher>
#include <QObject>
#include <QTimer>

class SpecbosMotionWidget  : public Core::IToolBox
{
	Q_OBJECT

public:
	SpecbosMotionWidget(QString toolboxName = "", QWidget* parent = nullptr);
	~SpecbosMotionWidget();

private:
	void init();
	void handleFinished();
	void setEnabled(bool enabled);

private slots:
	void on_btn_connect_clicked();
	void on_btn_disconnect_clicked();
	void on_refresh_clicked();
	void on_stop_clicked();
	void setRotationDegree();

private:
	Ui::SpecbosMotionWidgetClass ui;
	QFutureWatcher<Result> watcher;
};

