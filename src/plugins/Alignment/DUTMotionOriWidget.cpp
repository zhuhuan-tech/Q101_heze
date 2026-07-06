#include "DUTMotionOriWidget.h"
#include "OrientalMotorControl.h"
#include <QtConcurrent>
#include <QMessageBox>
#include <plugins/PrjCommon/metricsdata.h>
#include <plugins/Core/loggingwrapper.h>

DUTMotionOriWidget::DUTMotionOriWidget(QString toolboxName, QWidget* parent)
	: IToolBox(toolboxName, parent)
{
	ui.setupUi(this);
	init();
}

DUTMotionOriWidget::~DUTMotionOriWidget()
{}

void DUTMotionOriWidget::init()
{
    ui.label_status->setText("Not connected.");
    connect(&watcher, &QFutureWatcher<Result>::finished, this, &DUTMotionOriWidget::handleFinished);
    connect(ui.rotationline, &QLineEdit::returnPressed, this, &DUTMotionOriWidget::setRotationDegree);
    connect(ui.tiltline, &QLineEdit::returnPressed, this, &DUTMotionOriWidget::setTiltDegree);
    connect(ui.cantline, &QLineEdit::returnPressed, this, &DUTMotionOriWidget::setCantDegree);
    ui.rotationAlarm->setStyleSheet("background-color: rgb(0, 0, 255);");
    ui.tiltAlarm->setStyleSheet("background-color: rgb(0, 0, 255);");
    ui.tipAlarm->setStyleSheet("background-color: rgb(0, 0, 255);");
    //m_checkStateTimer = new QTimer(this);
    //connect(m_checkStateTimer, &QTimer::timeout, this, &DUTMotionOriWidget::on_checkStateTimer_timeout);
    //m_checkStateTimer->start(2000);

    ui.groupBox_7->hide();
}

void DUTMotionOriWidget::on_btn_connect_clicked()
{
    /*if (OrientalMotorControl::getInstance()->IsConnected())
    {
        ui.label_status->setText("Connected.");
        return;
    }*/
    setEnabled(false);
    //WaitWidget::instance()->startAnimation();
    //ui.label_status->setText(" Waiting for connection......");
    QFuture<Result> future = QtConcurrent::run([=]() { return OrientalMotorControl::getInstance()->Connect(); });
    watcher.setFuture(future);
}

void DUTMotionOriWidget::on_btn_disconnect_clicked()
{
    if (!OrientalMotorControl::getInstance()->IsConnected())
    {
        return;
    }
    Result ret = OrientalMotorControl::getInstance()->Disconnect();
    if (ret.success)
    {
        ui.label_status->setText("Disconnect Success.");
    }
}

void DUTMotionOriWidget::on_rotation_home_clicked()
{
    if (!OrientalMotorControl::getInstance()->IsConnected())
    {
        LoggingWrapper::instance()->warn("module is not connected");
        return;
    }
    QFuture<Result> future = QtConcurrent::run([=]() {
        return OrientalMotorControl::getInstance()->HomingSync(OrientalAxle::DZ);
        });
    watcher.setFuture(future);
}

void DUTMotionOriWidget::on_cant_home_clicked()
{
    if (!OrientalMotorControl::getInstance()->IsConnected())
    {
        LoggingWrapper::instance()->warn("module is not connected");
        return;
    }
    QFuture<Result> future = QtConcurrent::run([=]() {
        return OrientalMotorControl::getInstance()->HomingSync(OrientalAxle::DY);
        });
    watcher.setFuture(future);
}

void DUTMotionOriWidget::on_tilt_home_clicked()
{
    if (!OrientalMotorControl::getInstance()->IsConnected())
    {
        LoggingWrapper::instance()->warn("module is not connected");
        return;
    }
    QFuture<Result> future = QtConcurrent::run([=]() {
        return OrientalMotorControl::getInstance()->HomingSync(OrientalAxle::DX);
        });
    watcher.setFuture(future);
}

void DUTMotionOriWidget::on_rotation_refresh_clicked()
{
    if (!OrientalMotorControl::getInstance()->IsConnected())
    {
        LoggingWrapper::instance()->warn("module is not connected");
        return;
    }
    double degree = OrientalMotorControl::getInstance()->GetPosition(OrientalAxle::DZ);
    ui.degState_3->setText(QString::number(degree, 'f', 3));
}

void DUTMotionOriWidget::on_cant_refresh_clicked()
{
    if (!OrientalMotorControl::getInstance()->IsConnected())
    {
        LoggingWrapper::instance()->warn("module is not connected");
        return;
    }
    double degree = OrientalMotorControl::getInstance()->GetPosition(OrientalAxle::DY);
    ui.degState->setText(QString::number(degree, 'f', 3));
}

void DUTMotionOriWidget::on_tilt_refresh_clicked()
{
    if (!OrientalMotorControl::getInstance()->IsConnected())
    {
        LoggingWrapper::instance()->warn("module is not connected");
        return;
    }
    double degree = OrientalMotorControl::getInstance()->GetPosition(OrientalAxle::DX);
    ui.degState_2->setText(QString::number(degree, 'f', 3));
}

void DUTMotionOriWidget::on_rotation_stop_clicked()
{
    if (!OrientalMotorControl::getInstance()->IsConnected())
    {
        LoggingWrapper::instance()->warn("module is not connected");
        return;
    }
    Result ret = OrientalMotorControl::getInstance()->StopMove(OrientalAxle::DZ);
    on_rotation_refresh_clicked();
    if (!ret.success)
    {
        QMessageBox::warning(this, tr("Stop DZ Moving Failed"), QString::fromStdString(ret.errorMsg));
    }
}

void DUTMotionOriWidget::on_cant_stop_clicked()
{
    if (!OrientalMotorControl::getInstance()->IsConnected())
    {
        LoggingWrapper::instance()->warn("module is not connected");
        return;
    }
    Result ret = OrientalMotorControl::getInstance()->StopMove(OrientalAxle::DY);
    on_cant_refresh_clicked();
    if (!ret.success)
    {
        QMessageBox::warning(this, tr("Stop DY Moving Failed"), QString::fromStdString(ret.errorMsg));
    }
}

void DUTMotionOriWidget::on_tilt_stop_clicked()
{
    if (!OrientalMotorControl::getInstance()->IsConnected())
    {
        LoggingWrapper::instance()->warn("module is not connected");
        return;
    }
    Result ret = OrientalMotorControl::getInstance()->StopMove(OrientalAxle::DX);
    on_tilt_refresh_clicked();
    if (!ret.success)
    {
        QMessageBox::warning(this, tr("Stop DX Moving Failed"), QString::fromStdString(ret.errorMsg));
    }
}

void DUTMotionOriWidget::handleFinished()
{
    setEnabled(true);
    if (watcher.result().success)
    {
        ui.label_status->setText("Connected.");
        on_rotation_refresh_clicked();
        on_tilt_refresh_clicked();
        on_cant_refresh_clicked();
    }
    else
    {
        ui.label_status->setText("Connected Failed.");
    }
    //WaitWidget::instance()->stopAnimation();
}

void DUTMotionOriWidget::setRotationDegree()
{
    if (!OrientalMotorControl::getInstance()->IsConnected())
    {
        LoggingWrapper::instance()->warn("module is not connected");
        return;
    }
    float degree = ui.rotationline->text().toFloat();
    //limit
    int Zmin = OrientalMotorControl::getInstance()->getZLimitMin();
    int Zmax = OrientalMotorControl::getInstance()->getZLimitMax();
    if (degree < Zmin || degree > Zmax)
    {
        LoggingWrapper::instance()->warn("Z is over limit");
        return;
    }
    QFuture<Result> future = QtConcurrent::run([=]() {
        return OrientalMotorControl::getInstance()->MoveByDegreeSync(OrientalAxle::DZ, degree);
        });
    watcher.setFuture(future);
}

void DUTMotionOriWidget::setTiltDegree()
{
    if (!OrientalMotorControl::getInstance()->IsConnected())
    {
        LoggingWrapper::instance()->warn("module is not connected");
        return;
    }
    float degree = ui.tiltline->text().toFloat();
    //limit
    int Xmin = OrientalMotorControl::getInstance()->getXLimitMin();
    int Xmax = OrientalMotorControl::getInstance()->getXLimitMax();
    if (degree < Xmin || degree > Xmax)
    {
        LoggingWrapper::instance()->warn("X is over limit");
        return;
    }
    QFuture<Result> future = QtConcurrent::run([=]() {
        return OrientalMotorControl::getInstance()->MoveByDegreeSync(OrientalAxle::DX, degree);
        });
    watcher.setFuture(future);
}

void DUTMotionOriWidget::setCantDegree()
{
    if (!OrientalMotorControl::getInstance()->IsConnected())
    {
        LoggingWrapper::instance()->warn("module is not connected");
        return;
    }
    float degree = ui.cantline->text().toFloat();
    //limit
    int Ymin = OrientalMotorControl::getInstance()->getYLimitMin();
    int Ymax = OrientalMotorControl::getInstance()->getYLimitMax();
    if (degree < Ymin || degree > Ymax)
    {
        LoggingWrapper::instance()->warn("Y is over limit");
        return;
    }
    QFuture<Result> future = QtConcurrent::run([=]() {
        return OrientalMotorControl::getInstance()->MoveByDegreeSync(OrientalAxle::DY, degree);
        });
    watcher.setFuture(future);
}

void DUTMotionOriWidget::on_checkStateTimer_timeout()
{
    return;
    //if (!OrientalMotorControl::getInstance()->IsConnected())
    //{
    //    return;
    //}
    int alarmCodeX = OrientalMotorControl::getInstance()->IsAlarm(OrientalAxle::DX);
    int alarmCodeY = OrientalMotorControl::getInstance()->IsAlarm(OrientalAxle::DY);
    int alarmCodeZ = OrientalMotorControl::getInstance()->IsAlarm(OrientalAxle::DZ);

    if (alarmCodeX < 0 || alarmCodeY < 0 || alarmCodeZ < 0)
    {
        ui.rotationAlarm->setStyleSheet("background-color: rgb(0, 0, 255);");
        ui.tiltAlarm->setStyleSheet("background-color: rgb(0, 0, 255);");
        ui.tipAlarm->setStyleSheet("background-color: rgb(0, 0, 255);");
    }
    else
    {
        //m_isRotationAlarm = alarmCodeX;
        //m_isTiltAlarm = alarmCodeY;
        //m_isTipAlarm = alarmCodeZ;
        if (alarmCodeX)
        {
            ui.rotationAlarm->setStyleSheet("background-color: rgb(255, 0, 0);");
        }
        else
        {
            ui.rotationAlarm->setStyleSheet("background-color: rgb(0, 255, 0);");
        }

        if (alarmCodeY)
        {
            ui.tiltAlarm->setStyleSheet("background-color: rgb(255, 0, 0);");
        }
        else
        {
            ui.tiltAlarm->setStyleSheet("background-color: rgb(0, 255, 0);");
        }

        if (alarmCodeZ)
        {
            ui.tipAlarm->setStyleSheet("background-color: rgb(255, 0, 0);");
        }
        else
        {
            ui.tipAlarm->setStyleSheet("background-color: rgb(0, 255, 0);");
        }
    }

    //update pos status
    //onUpdateFilterStatus();
}

void DUTMotionOriWidget::on_clearAlarm_clicked()
{
    if (!OrientalMotorControl::getInstance()->IsConnected())
    {
        LoggingWrapper::instance()->warn("module is not connected");
        return;
    }
    Result ret = OrientalMotorControl::getInstance()->ClearAlarm();
    if (!ret.success)
        QMessageBox::warning(this, tr("Error"), "Clear Alarm Failed");
}

void DUTMotionOriWidget::setEnabled(bool enabled)
{
    ui.btn_connect->setEnabled(enabled);
    ui.btn_disconnect->setEnabled(enabled);
}
