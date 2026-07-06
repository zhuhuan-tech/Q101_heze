#pragma once

#include <QWidget>
#include <QFutureWatcher>
#include "ui_XYZImageWidget.h"
#include "Result.h"

class XYZImageWidget : public QWidget
{
	Q_OBJECT

public:
	XYZImageWidget(QWidget *parent = nullptr);
	~XYZImageWidget();

private slots:
	void on_loadDataButton_clicked();
	void on_saveXYZImage_clicked();

private:
	Ui::XYZImageWidgetClass ui;
};

