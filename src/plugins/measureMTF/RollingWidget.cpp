#include "RollingWidget.h"
#include "PrjCommon/WaitWidget.h"
#include "PrjCommon/PrjCommon.h"
#include <ImageRotationConfig.h>
#include <QtConcurrent/qtconcurrentrun.h>
#include <plugins/Core/loggingwrapper.h>
RollingWidget::RollingWidget(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);

    setQss(":/qss/uistyle.qss");
    initLiveImageRadio();
    on_btn_fold_clicked();
    connect(MLColorimeterMode::Instance(), SIGNAL(connectStatus(bool)), this, SLOT(connectStatus(bool)));
    connect(MLColorimeterMode::Instance(), SIGNAL(stopGrab(bool)), this, SLOT(stopGrabSlot(bool)));

    connect(this, SIGNAL(enableWidget(QWidget*, bool)), this, SLOT(setWidgetEnable(QWidget*, bool)));
    connect(this, SIGNAL(messageShow(QString)), this, SLOT(showConnectWarning(QString)));
    connect(&watcher, SIGNAL(finished()), this, SLOT(handleFinished()));
    connect(Core::PrjCommon::instance(), SIGNAL(connectAllDevicesSig()), this, SLOT(connectCameraSlot()));

    connect(Core::PrjCommon::instance(), SIGNAL(updateDutConfigSignal()), this, SLOT(updateDutConfigSlot()));
}

RollingWidget::~RollingWidget()
{}
void RollingWidget::setQss(QString filename)
{
    QFile file(filename);
    file.open(QFile::ReadOnly);
    QTextStream filetext(&file);
    QString stylesheet = filetext.readAll();
    this->setStyleSheet(stylesheet);
    file.close();
}
void RollingWidget::initLiveImageRadio()
{
    ui.radioBtn_liveImage->setChecked(false);
   // on_checkBox_SLBImgRotate_stateChanged(true);
}
void RollingWidget::NotifyCameraStateChanged(MLCameraState old_state, MLCameraState new_state)
{
    if (old_state != MLCameraState::kDisconnected && new_state == MLCameraState::kConnected)
    {
        on_disconnectCamera_clicked();
    }
}

void RollingWidget::NotifyCameraFrameReceived(cv::Mat img, MLPixelFormat format)
{
    if (ui.camView != nullptr)
    {
        cv::Mat matDest = img.clone();
        if (!img.empty() )//todo
        {
            Result ret/* = MLColorimeterMode::Instance()->imageRotation(img, matDest)*/;
            if (!ret.success)
            {
                LoggingWrapper::instance()->error(QString::fromStdString(ret.errorMsg));
            }
        }

        ui.camView->Enqueue(matDest);
        emit MLColorimeterMode::Instance()->updateCameraImageSignal(matDest.clone(),MLColorimeterCameraType::singleCam);
    }
}

void RollingWidget::NotifyCameraGrayLevel(int gray_level)
{
    emit MLColorimeterMode::Instance()->updateCameraGrayLevelSignal(gray_level, MLColorimeterCameraType::singleCam);

}

void RollingWidget::initUI()
{
    ui.btn_unfold->setVisible(false);

}
void RollingWidget::on_connectCamera_clicked()
{
    WaitWidget::instance()->startAnimation();
    QFuture<Result> future = QtConcurrent::run(this, &RollingWidget::connectCamera);
    watcher.setFuture(future);
}
Result RollingWidget::connectCamera()
{
    emit enableWidget(ui.connectCamera, false);
    emit enableWidget(ui.disconnectCamera, false);

    Result ret = MLColorimeterMode::Instance()->ConnectAsync();
    if (!ret.success)
    {
        emit messageShow(QString::fromStdString(ret.errorMsg));
        emit enableWidget(ui.connectCamera, true);
        emit enableWidget(ui.disconnectCamera, true);
        return ret;
    }
    emit enableWidget(ui.connectCamera, true);
    emit enableWidget(ui.disconnectCamera, true);
    return ret;
}
void RollingWidget::on_disconnectCamera_clicked()
{

}

void RollingWidget::connectCameraSlot()
{
}

void RollingWidget::on_btn_unfold_clicked()
{
    ui.btn_fold->setVisible(true);
    ui.btn_unfold->setVisible(false);
    ui.widget2->setVisible(true);
}

void RollingWidget::on_btn_fold_clicked()
{
    ui.btn_unfold->setVisible(true);
    ui.btn_fold->setVisible(false);
    ui.widget2->setVisible(false);
}
