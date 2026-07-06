#pragma once
#include <QWidget>
#include "Result.h"
#include "itoolbox.h"
#include "ui_DUTMotionWidget.h"
#include <QFutureWatcher>
#include <QObject>
#include <QTimer>

class DUTMotionWidget : public Core::IToolBox
{
    Q_OBJECT

  public:
    DUTMotionWidget(QString toolboxName = "", QWidget *parent = nullptr);
    ~DUTMotionWidget();

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
    Ui::DUTMotionWidgetClass ui;
    QFutureWatcher<Result> watcher;
    bool m_isRotationAlarm = false;
    bool m_isTiltAlarm = false;
    bool m_isTipAlarm = false;
    QTimer *m_checkStateTimer;
};
