#pragma once

#include <QWidget>
#include "ui_LightAdjustWidget.h"
#include "Result.h"
#include <QFutureWatcher>
#include <plugins/Core/itoolbox.h>


class LightAdjustWidget : public Core::IToolBox
{
	Q_OBJECT

public:
	LightAdjustWidget(QString toolboxName = "", QWidget* parent = nullptr);
	~LightAdjustWidget();

private slots:
	void on_btn_connect_clicked();
	void on_btn_disconnect_clicked();
	void on_btn_set_clicked();
	void on_btn_set_2_clicked();

private:
	void init();
	void initCombox();

private:
	Ui::LightAdjustWidgetClass ui;
	QFutureWatcher<Result> watcher;
};

