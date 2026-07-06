#include "DUTMotionWidget.h"
#include "WaitWidget.h"
#include "OrientalMotorControl.h"
#include  "loggingwrapper.h"
#include <QtConcurrent>
#include <QMessageBox>

DUTMotionWidget::DUTMotionWidget(QString toolBoxName, QWidget *parent) : IToolBox(toolBoxName, parent)
{
    ui.setupUi(this);
    init();
}

DUTMotionWidget::~DUTMotionWidget()
{
}

void DUTMotionWidget::init()
{
    ui.label_status->setText("Not connected.");
    connect(&watcher, &QFutureWatcher<Result>::finished, this, &DUTMotionWidget::handleFinished);
    connect(ui.rotationline, &QLineEdit::returnPressed, this, &DUTMotionWidget::setRotationDegree);
    connect(ui.tiltline, &QLineEdit::returnPressed, this, &DUTMotionWidget::setTiltDegree);
    connect(ui.cantline, &QLineEdit::returnPressed, this, &DUTMotionWidget::setCantDegree);
    ui.rotationAlarm->setStyleSheet("background-color: rgb(0, 0, 255);");
    ui.tiltAlarm->setStyleSheet("background-color: rgb(0, 0, 255);");
    ui.tipAlarm->setStyleSheet("background-color: rgb(0, 0, 255);");
    //m_checkStateTimer = new QTimer(this);
    //connect(m_checkStateTimer, &QTimer::timeout, this, &DUTMotionWidget::on_checkStateTimer_timeout);
    //m_checkStateTimer->start(2000);

    ui.groupBox_7->hide();
}

void DUTMotionWidget::on_btn_connect_clicked()
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

void DUTMotionWidget::on_btn_disconnect_clicked()
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

void DUTMotionWidget::on_rotation_home_clicked()
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

void DUTMotionWidget::on_cant_home_clicked()
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

void DUTMotionWidget::on_tilt_home_clicked()
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

void DUTMotionWidget::on_rotation_refresh_clicked()
{
    if (!OrientalMotorControl::getInstance()->IsConnected())
    {
        LoggingWrapper::instance()->warn("module is not connected");
        return;
    }
    double degree = OrientalMotorControl::getInstance()->GetPosition(OrientalAxle::DZ);
    ui.degState_3->setText(QString::number(degree, 'f', 3));
}

void DUTMotionWidget::on_cant_refresh_clicked()
{
    if (!OrientalMotorControl::getInstance()->IsConnected())
    {
        LoggingWrapper::instance()->warn("module is not connected");
        return;
    }
    double degree = OrientalMotorControl::getInstance()->GetPosition(OrientalAxle::DY);
    ui.degState->setText(QString::number(degree, 'f', 3));
}

void DUTMotionWidget::on_tilt_refresh_clicked()
{
    if (!OrientalMotorControl::getInstance()->IsConnected())
    {
        LoggingWrapper::instance()->warn("module is not connected");
        return;
    }
    double degree = OrientalMotorControl::getInstance()->GetPosition(OrientalAxle::DX);
    ui.degState_2->setText(QString::number(degree, 'f', 3));
}

void DUTMotionWidget::on_rotation_stop_clicked()
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

void DUTMotionWidget::on_cant_stop_clicked()
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

void DUTMotionWidget::on_tilt_stop_clicked()
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

void DUTMotionWidget::handleFinished()
{
    setEnabled(true);
    if(watcher.result().success)
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

void DUTMotionWidget::setRotationDegree()
{
    if (!OrientalMotorControl::getInstance()->IsConnected())
    {
        LoggingWrapper::instance()->warn("module is not connected");
        return;
    }
    float degree = ui.rotationline->text().toFloat();
    //TODO:limit
    QFuture<Result> future = QtConcurrent::run([=]() {
            return OrientalMotorControl::getInstance()->MoveByDegreeSync(OrientalAxle::DZ, degree);
        });
    watcher.setFuture(future);
}

void DUTMotionWidget::setTiltDegree()
{
    if (!OrientalMotorControl::getInstance()->IsConnected())
    {
        LoggingWrapper::instance()->warn("module is not connected");
        return;
    }
    float degree = ui.tiltline->text().toFloat();
    //TODO:limit
    QFuture<Result> future = QtConcurrent::run([=]() {
            return OrientalMotorControl::getInstance()->MoveByDegreeSync(OrientalAxle::DX, degree);
        });
        watcher.setFuture(future);
}

void DUTMotionWidget::setCantDegree()
{
    if (!OrientalMotorControl::getInstance()->IsConnected())
    {
        LoggingWrapper::instance()->warn("module is not connected");
        return;
    }
    float degree = ui.cantline->text().toFloat();
    //TODO:limit
    QFuture<Result> future = QtConcurrent::run([=]() {
        return OrientalMotorControl::getInstance()->MoveByDegreeSync(OrientalAxle::DY, degree);
        });
    watcher.setFuture(future);
}

void DUTMotionWidget::on_checkStateTimer_timeout()
{
    return;
    //if (!OrientalMotorControl::getInstance()->IsConnected())
    //{
    //    return;
    //}
    int alarmCodeX= OrientalMotorControl::getInstance()->IsAlarm(OrientalAxle::DX);
    int alarmCodeY = OrientalMotorControl::getInstance()->IsAlarm(OrientalAxle::DY);
    int alarmCodeZ = OrientalMotorControl::getInstance()->IsAlarm(OrientalAxle::DZ);

    if (alarmCodeX < 0|| alarmCodeY < 0|| alarmCodeZ < 0)
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

void DUTMotionWidget::on_clearAlarm_clicked()
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

void DUTMotionWidget::setEnabled(bool enabled)
{
    ui.btn_connect->setEnabled(enabled);
    ui.btn_disconnect->setEnabled(enabled);
}