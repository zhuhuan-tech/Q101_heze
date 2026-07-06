#pragma once

#include <QWidget>
#include "Result.h"
#include "ui_DUTMotionOriWidget.h"
#include <QFutureWatcher>
#include <plugins/Core/itoolbox.h>
#include <QObject>
#include <QTimer>

//using namespace CORE;

class DUTMotionOriWidget : public Core::IToolBox
{
    Q_OBJECT

public:
	DUTMotionOriWidget(QString toolboxName = "", QWidget* parent = nullptr);
    ~DUTMotionOriWidget();

private:
    void init();
    void handleFinished();
    void setEnabled(bool enabled);

private slots:
    void on_btn_connect_clicked();
    void on_btn_disconnect_clicked();
    void on_cant_home_clicked();
    void on_tilt_home_clicked();
    void on_rotation_home_clicked();
    void on_cant_refresh_clicked();
    void on_tilt_refresh_clicked();
    void on_rotation_refresh_clicked();
    void on_cant_stop_clicked();
    void on_tilt_stop_clicked();
    void on_rotation_stop_clicked();
    void setRotationDegree();
    void setTiltDegree();
    void setCantDegree();
    void on_checkStateTimer_timeout();
    void on_clearAlarm_clicked();

private:
	Ui::DUTMotionOriWidgetClass ui;
    QFutureWatcher<Result> watcher;
    bool m_isRotationAlarm = false;
    bool m_isTiltAlarm = false;
    bool m_isTipAlarm = false;
    QTimer* m_checkStateTimer;

};

