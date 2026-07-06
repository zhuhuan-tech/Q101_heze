#include "MLColorimeterMode.h"
#include "MLColorimeterHelp.h"
#include "IQMetrics/ML_CrossMTF.h"
#include "ml_multiCrossHairDetection.h"
#include "IQMetricUtl.h"
#include "Core/loggingwrapper.h"
#include "PrjCommon/metricsdata.h"
#include <QThread>
#include <QtConcurrent/qtconcurrentrun.h>
#include <QFutureSynchronizer>
#include <QMessageBox>
#include <fstream>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;
using namespace MLImageDetection;

MLColorimeterMode* MLColorimeterMode::self = nullptr;

MLColorimeterMode::MLColorimeterMode(QObject* parent) : QObject(parent)
{
    qRegisterMetaType<cv::Mat>("cv::Mat");
    m_bino = new MLBinoBusinessManage(this);
    Result res = InitConfig();
    if (!res.success)
    {
        QMessageBox::warning(NULL, "Error", QString::fromStdString(res.errorMsg), QMessageBox::Yes);
    }
}

MLColorimeterMode::~MLColorimeterMode()
{
    if (m_bino != nullptr)
    {
        m_bino->ML_DisconnectModules();
        delete m_bino;
        m_bino = nullptr;
    }
    if (m_taskManager != nullptr)
    {
        delete m_taskManager;
        m_taskManager = nullptr;
    }
}

MLColorimeterMode* MLColorimeterMode::Instance(QObject* parent)
{
    if (!self)
    {
        static QMutex mutex;
        QMutexLocker locker(&mutex);
        if (!self)
        {
            self = new MLColorimeterMode(parent);
        }
    }
    return self;
}

Result MLColorimeterMode::Connect()
{
    try
    {
        int startTime = QDateTime::currentMSecsSinceEpoch();
        LoggingWrapper::instance()->info("colorimeter connect start.");
        m_IsConnected = false;
        Result ret;
        if (m_bino->ML_IsModulesConnect())
        {
            LoggingWrapper::instance()->info("colorimeter is connected.");
            m_IsConnected = true;
            return Result();
        }

        ret = m_bino->ML_ConnectModules();
		if (!ret.success)
		{
            return Result(false, "colorimeter module connection failed. " + ret.errorMsg);
		}
		//m_bino->ML_WaitForMovingStop();
		//QThread::msleep(3000);
		//m_bino->ML_WaitForMovingStop();
		m_IsConnected = true;
        //m_taskManager = new MLTaskManager(m_bino);
		ret = initAfterConnect();
        if (!ret.success)
        {
            return ret;
        }

        emit connectStatus(true);

        int takeTime = QDateTime::currentMSecsSinceEpoch() - startTime;
        LoggingWrapper::instance()->debug(QString("colorimeter connect time: %1 ms.").arg(takeTime));
        return Result();
    }
    catch (exception e)
    {
        return Result(false, e.what());
    }
}

Result MLColorimeterMode::ConnectAsync(int index) {
    Result res;
    QList<QFuture<Result>> futures;
    QFutureSynchronizer<Result> synchronizer;
    MLMonoBusinessManage* mono = GetMonocular(index);
    if (mono->ML_IsModuleConnect())
    {
        LoggingWrapper::instance()->info("colorimeter is connected.");
        m_IsConnected = true;
        return Result();
    }
    res = mono->ML_CreateModule();
    if (!res.success)
    {
        return res;
    }
    futures.append(QtConcurrent::run([=]() { return mono->ConnectCamera(); }));
    futures.append(QtConcurrent::run([=]() { return mono->ConnectFilterWheel(); }));
    futures.append(QtConcurrent::run([=]() { return mono->ConnectMotion(); }));
    futures.append(QtConcurrent::run([=]() { return mono->ConnectRXFilterWheel(); }));
    futures.append(QtConcurrent::run([=]() { return mono->ConnectReflector(); }));
    futures.append(QtConcurrent::run([=]() { return mono->ConnectSpectrometer(); }));
    futures.append(QtConcurrent::run([=]() { return mono->ConnectSpecbos(); }));

    for (auto& future : futures) {
        synchronizer.addFuture(future);
    }
    synchronizer.waitForFinished();
    std::string errmsg;
    m_IsConnected = true;
    for (const auto& future : futures) {
        Result res = future.result();
        if (!res.success) {
            m_IsConnected = false;
            errmsg = res.errorMsg;
            break;
        }
    }

    if (!m_IsConnected)
    {
        return Result(false, "colorimeter module connection failed. " + errmsg);
    }

    //m_taskManager = new MLTaskManager(m_bino);
    Result ret = initAfterConnect();
    if (!ret.success)
    {
        return ret;
    }

    emit connectStatus(true);
    return Result();
}

Result MLColorimeterMode::InitConfig()
{
    try {
        if (m_bino->ML_GetModulesIDList().size() == 0) {
            static QMutex mutex;
            QMutexLocker locker(&mutex);

            if (m_bino->ML_GetModulesIDList().size() == 0) {
                QDir dir(CONFIG_PATH);
                if (!dir.exists())
                {
                    LoggingWrapper::instance()->warn("MLColorimeterMode: Directory does not exist:" + CONFIG_PATH);
                    return Result(false, "MLColorimeterMode: Directory does not exist:" + CONFIG_PATH.toStdString());
                }

                Result ret;
                QStringList subDirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
                for (const QString& subDir : subDirs)
                {
                    QString subDirAbs = dir.absoluteFilePath(subDir);
                    ret = m_bino->ML_AddModule(subDirAbs.toStdString().c_str());

                    if (!ret.success)
                    {
                        return Result(false, "MLColorimeterMode: Init colorimeter config failed, " + ret.errorMsg);
                    }
                }
            }
        }
        m_taskManager = new MLTaskManager(m_bino);
        if (!m_taskManager)
        {
            return Result{ false,"InitConfig error,task manager is nullptr" };
        }
        return Result();
    }
    catch (exception e)
    {
        return Result(false, QString("colorimeter init config error, %1").arg(e.what()).toStdString());
    }
}

Result MLColorimeterMode::Disconnect()
{
    if(!IsConnect())
        return Result();
    Result ret = m_bino->ML_DisconnectModules();
    if (!ret.success)
    {
        return Result(false, "colorimeter disconnect failed, " + ret.errorMsg);
    }
    m_IsConnected = false;
    emit connectStatus(false);
    if (m_taskManager != nullptr)
    {
        delete m_taskManager;
        m_taskManager = nullptr;
    }
    return Result();
}

bool MLColorimeterMode::IsConnect()
{
    return m_IsConnected;
}

MLMonoBusinessManage* MLColorimeterMode::GetMonocular(int index)
{
    std::vector<int> ids = m_bino->ML_GetModulesIDList();
    if ((ids.size() == 0)|| ids.size()<(index+1)) {
        return nullptr;
    }
    return m_bino->ML_GetModuleByID(ids.at(index));
}

//MLMonoBusinessManage* MLColorimeterMode::GetSecondMonocular()
//{
//    std::vector<int> ids = m_bino->ML_GetModulesIDList();
//    if ((ids.size() == 0)||ids.size()<2) {
//        return nullptr;
//    }
//    return m_bino->ML_GetModuleByID(ids.at(0));
//}

GrabberBase* MLColorimeterMode::GetCamera(int index)
{
    if (GetMonocular(index) == nullptr)
    {
        return nullptr;
    }
    MLMonoBusinessManage* monocular = GetMonocular(index);
    GrabberBase* grabberBase = static_cast<GrabberBase*>(monocular->ML_GetOneModuleByName(CAMERA_KEY));
    return grabberBase;
}

ActuatorBase* MLColorimeterMode::GetCameraMotion()//0
{
    if (GetMonocular() == nullptr)
    {
        return nullptr;
    }

    MLMonoBusinessManage* monocular = GetMonocular();
    ActuatorBase* base = static_cast<ActuatorBase*>(monocular->ML_GetOneModuleByName(FOCUS_KEY));
    return base;
}

MLBinoBusinessManage* MLColorimeterMode::GetBinocular()
{
    return m_bino;
}

ML_Task::MLTaskManager* MLColorimeterMode::GetCaptureTaskManager()
{
    return m_taskManager;
}

//ND XYZ Filter Interface
Result MLColorimeterMode::SetNDFilter(QString nd)
{
    MLFilterEnum ndEnum = MLColorimeterHelp::instance()->TransStrToFilterEnum(nd.toStdString());
    return SetNDFilter(ndEnum);
}

Result MLColorimeterMode::SetNDFilter(MLFilterEnum nd)
{
    if (!IsConnect())
    {
        return Result(false, "Set nd filter failed, colorimeter is not connected.");
    }
    Result ret = GetMonocular()->ML_MoveND_XYZFilterByEnumSync(ND_FILTER_KEY, nd);
    if (!ret.success)
    {
        return Result(false, "Set nd filter failed." + ret.errorMsg);
    }
    return Result();
}

Result MLColorimeterMode::SetColorFilter(QString xyz)
{
    MLFilterEnum xyzEnum = MLColorimeterHelp::instance()->TransStrToFilterEnum(xyz.toStdString());
    return SetColorFilter(xyzEnum);
}

Result MLColorimeterMode::SetColorFilter(MLFilterEnum xyzEnum)
{
    if (!IsConnect())
    {
        return Result(false, "Set XYZ filter failed, colorimeter is not connected.");
    }
    Result ret = GetMonocular()->ML_MoveND_XYZFilterByEnumSync(XYZ_FILTER_KEY, xyzEnum);
    if (!ret.success)
    {
        return Result(false, "Set XYZ filter failed." + ret.errorMsg);
    }
    return Result();
}

Result MLColorimeterMode::SetNDFilterAsyn(QString nd)
{
    if (!IsConnect())
    {
        return Result(false, "Set ND Filter ASYN failed, colorimeter is not connected.");
    }

    MLFilterEnum ndEnum = MLColorimeterHelp::instance()->TransStrToFilterEnum(nd.toStdString());
    Result ret = GetMonocular()->ML_MoveND_XYZFilterByEnumAsync(ND_FILTER_KEY, ndEnum);
    if (!ret.success)
    {
        return Result(false, "Asyn set nd filter failed." + ret.errorMsg);
    }
    return Result();
}

Result MLColorimeterMode::SetXYZFilterAsyn(QString xyz)
{
    if (!IsConnect())
    {
        return Result(false, "Set XYZ Filter ASYN failed, colorimeter is not connected.");
    }

    MLFilterEnum xyzEnum = MLColorimeterHelp::instance()->TransStrToFilterEnum(xyz.toStdString());
    Result ret = GetMonocular()->ML_MoveND_XYZFilterByEnumAsync(XYZ_FILTER_KEY, xyzEnum);
    if (!ret.success)
    {
        return Result(false, "Asyn set XYZ filter failed." + ret.errorMsg);
    }
    return Result();
}

QString MLColorimeterMode::GetNDFilter()
{
    std::string nd = MLColorimeterHelp::instance()->TransFilterEnumToStr(GetNDFilterEnum());
    return QString::fromStdString(nd);
}

MLFilterEnum MLColorimeterMode::GetNDFilterEnum()
{
    if (IsConnect())
    {
        MLFilterEnum ndEnum = GetMonocular()->ML_GetND_XYZFilterChannel(ND_FILTER_KEY);
        return ndEnum;
    }
    return MLFilterEnum();
}

QString MLColorimeterMode::GetXYZFilter()
{
    std::string xyz = MLColorimeterHelp::instance()->TransFilterEnumToStr(GetXYZFilterEnum());
    return QString::fromStdString(xyz);
}

MLFilterEnum MLColorimeterMode::GetXYZFilterEnum()
{
    if (IsConnect())
    {
        MLFilterEnum xyzEnum = GetMonocular()->ML_GetND_XYZFilterChannel(XYZ_FILTER_KEY);
        return xyzEnum;
    }
    return MLFilterEnum();
}

Result MLColorimeterMode::ClearAlarmFilter()
{
    if (!IsConnect())
    {
        return Result(false, "Motion filter clear alarm failed, colorimeter is not connected.");
    }

    MLMonoBusinessManage* monocular = GetMonocular();
    if (isEnabled(ND_FILTER_KEY))
    {
        FilterWheelBase* grabberBaseND =
            static_cast<FilterWheelBase*>(monocular->ML_GetOneModuleByName(ND_FILTER_KEY));
        pluginException ex = grabberBaseND->ML_ClearAlarm();
        if (!ex.getStatusFlag())
        {
            std::string message = "ND filter clear alarm error, " + ex.getExceptionMsg();
            LoggingWrapper::instance()->error(QString::fromStdString(message));
            return Result(false, message);
        }
    }

    if (isEnabled(XYZ_FILTER_KEY))
    {
        FilterWheelBase* grabberBaseXYZ =
            static_cast<FilterWheelBase*>(monocular->ML_GetOneModuleByName(XYZ_FILTER_KEY));
        pluginException ex = grabberBaseXYZ->ML_ClearAlarm();
        if (!ex.getStatusFlag())
        {
            std::string message = "XYZ filter clear alarm error, " + ex.getExceptionMsg();
            LoggingWrapper::instance()->error(QString::fromStdString(message));
            return Result(false, message);
        }
    }

    Result ret = ClearAlarmRx();
    return ret;
}

//RX Interface
Result MLColorimeterMode::SetRXAsync(RXCombination rx)
{
    if (!IsConnect())
    {
        return Result(false, "Set RX async failed, colorimeter is not connected.");
    }
    Result ret = GetMonocular()->ML_SetRXAsync(rx);
    if (!ret.success)
    {
        return Result(false, "Set RX async failed." + ret.errorMsg);
    }
    return Result();
}

Result MLColorimeterMode::SetRXSync(RXCombination rx)
{
    if (!IsConnect())
    {
        return Result(false, "Set RX sync failed, colorimeter is not connected.");
    }
    Result ret = GetMonocular()->ML_SetRXSync(rx);
    if (!ret.success)
    {
        return Result(false, "Set RX sync failed." + ret.errorMsg);
    }
    return Result();
}

Result MLColorimeterMode::SetCylinder(QString cylinder, int axis)
{
    if (!IsConnect())
    {
        return Result(false, "Set Cylinder failed, colorimeter is not connected.");
    }
    Result ret = GetMonocular()->ML_MoveRXFilterByNameSync(cylinder.toStdString(), axis);
    if (!ret.success)
    {
        return Result(false, "Set Cylinder failed." + ret.errorMsg);
    }
    return Result();
}

ML::MLColorimeter::RXCombination MLColorimeterMode::GetRX()
{
    if (!IsConnect())
    {
        return RXCombination();
    }
    return GetMonocular()->ML_GetRX();
}

Result MLColorimeterMode::ClearAlarmRx()
{
    if (!IsConnect())
    {
        return Result(false, "Cylindry mirror clear alarm failed, colorimeter is not connected.");
    }

    if (!isEnabled(RX_KEY))
    {
        return Result();
    }

    MLMonoBusinessManage* monocular = GetMonocular();
    RxFilterWheelBase* grabberBase = static_cast<RxFilterWheelBase*>(monocular->ML_GetOneModuleByName(RX_KEY));
    pluginException ex = grabberBase->ML_ClearAlarm();
    if (!ex.getStatusFlag())
    {
        std::string message = "Cylindry mirror revolution clear alarm error, " + ex.getExceptionMsg();
        LoggingWrapper::instance()->error(QString::fromStdString(message));
        return Result(false, message);
    }
    ex = grabberBase->ML_ClearAxisAlarm();
    if (!ex.getStatusFlag())
    {
        std::string message = "Cylindry mirror rotation clear alarm error, " + ex.getExceptionMsg();
        LoggingWrapper::instance()->error(QString::fromStdString(message));
        return Result(false, message);
    }
    return Result();
}

//Focus Motion Interface
Result MLColorimeterMode::SetFocusMotionPosAsync(double position)
{
    if (!IsConnect())
    {
        return Result(false, "Set focus ASYN failed, colorimeter is not connected.");
    }

    Result ret = GetMonocular()->ML_SetPositionAbsAsync(FOCUS_KEY, position);

    if (!ret.success)
    {
        return Result(false, "Asyn set focus failed." + ret.errorMsg);
    }
    return Result();
}

Result MLColorimeterMode::SetFocusMotionPosSync(double position)
{
    if (!IsConnect())
    {
        return Result(false, "Set focus ASYN failed, colorimeter is not connected.");
    }

    Result ret = GetMonocular()->ML_SetPositionAbsSync(FOCUS_KEY, position);

    if (!ret.success)
    {
        return Result(false, "Sync set focus failed." + ret.errorMsg);
    }
    return Result();
}

Result MLColorimeterMode::SetFocusMotionPosRelAsync(double position)
{
    if (!IsConnect())
    {
        return Result(false, "Set focus ASYN failed, colorimeter is not connected.");
    }

    Result ret = GetMonocular()->ML_SetPositionAbsAsync(FOCUS_KEY, position);

    if (!ret.success)
    {
        return Result(false, "Asyn set focus failed." + ret.errorMsg);
    }
    return Result();
}

Result MLColorimeterMode::SetFocusMotionPosRelSync(double position)
{
    if (!IsConnect())
    {
        return Result(false, "Set focus ASYN failed, colorimeter is not connected.");
    }

    Result ret = GetMonocular()->ML_SetPositionAbsSync(FOCUS_KEY, position);

    if (!ret.success)
    {
        return Result(false, "Sync set focus failed." + ret.errorMsg);
    }
    return Result();
}

Result MLColorimeterMode::SetVidSync(double vid)
{
    if (!IsConnect())
    {
        return Result(false, "Set vid ASYN failed, colorimeter is not connected.");
    }

    Result ret = GetMonocular()->ML_SetVIDAsync(vid);

    if (!ret.success)
    {
        return Result(false, "Sync set vid failed." + ret.errorMsg);
    }
    return Result();
}

Result MLColorimeterMode::SetVidAsync(double vid)
{
    if (!IsConnect())
    {
        return Result(false, "Set vid ASYN failed, colorimeter is not connected.");
    }

    Result ret = GetMonocular()->ML_SetVIDAsync(vid);

    if (!ret.success)
    {
        return Result(false, "Asyn set vid failed." + ret.errorMsg);
    }
    return Result();
}

Result MLColorimeterMode::SetSphereSync(double sphere)
{
    if (!IsConnect())
    {
        return Result(false, "Set sphere sync failed, colorimeter is not connected.");
    }

    Result ret = GetMonocular()->ML_SetSphericalSync(sphere);

    if (!ret.success)
    {
        return Result(false, "Set sphere sync failed," + ret.errorMsg);
    }
    return Result();
}

Result MLColorimeterMode::SetSphereAsync(double sphere)
{
    if (!IsConnect())
    {
        return Result(false, "Set sphere async failed, colorimeter is not connected.");
    }

    Result ret = GetMonocular()->ML_SetSphericalSync(sphere);

    if (!ret.success)
    {
        return Result(false, "Set sphere async failed," + ret.errorMsg);
    }
    return Result();
}

double MLColorimeterMode::GetVid()
{
    if (!IsConnect())
    {
        return DBL_MAX;
    }

    return GetMonocular()->ML_GetVID();
}

double MLColorimeterMode::GetFocusMotionPos()
{
    if (!IsConnect())
    {
        return DBL_MAX;
    }

    return GetMonocular()->ML_GetMotionPosition(FOCUS_KEY);
}

cv::Point3f MLColorimeterMode::TransVidToFocusMotionSph(double vid)
{
    MotionConfig motionConfig = GetCameraMotionConfig();
    if (vid == 0)
    {
        return cv::Point3f(FLT_MAX, FLT_MAX, FLT_MAX);
    }
    double motionPos;
    if (motionConfig.IsReverse)
    {
        motionPos = motionConfig.FocalLength * motionConfig.FocalLength /
            (vid - motionConfig.FocalPlanesObjectSpace) +
            motionConfig.InfinityPosition;
    }
    else
    {
        motionPos = -(motionConfig.FocalLength * motionConfig.FocalLength) /
            (vid + motionConfig.FocalPlanesObjectSpace) +
            motionConfig.InfinityPosition;
    }
    double sphere = 1000 / vid;
    return cv::Point3f(sphere, vid, motionPos);
}

cv::Point3f MLColorimeterMode::TransSphToFocusMotionVid(double sphere)
{
    MotionConfig motionConfig = GetCameraMotionConfig();
	if (sphere == 0)
    {
        return cv::Point3f(sphere, FLT_MAX, motionConfig.InfinityPosition);
    }
    double vid = 1000 / sphere;
    double motionPos;
    if (motionConfig.IsReverse)
    {
        motionPos = motionConfig.FocalLength * motionConfig.FocalLength /
            (vid - motionConfig.FocalPlanesObjectSpace) +
            motionConfig.InfinityPosition;
    }
    else
    {
        motionPos = -(motionConfig.FocalLength * motionConfig.FocalLength) /
            (vid + motionConfig.FocalPlanesObjectSpace) +
            motionConfig.InfinityPosition;
    }
    return cv::Point3f(sphere, vid, motionPos);
}

cv::Point3f MLColorimeterMode::TransFocusMotionToSphVid(double position)
{
    MotionConfig motionConfig = GetCameraMotionConfig();
	if (abs(position - motionConfig.InfinityPosition) < 0.01)
	{
		return cv::Point3f(0.0, FLT_MAX, position);
	}
    double vid;
    if (motionConfig.IsReverse)
    {
        vid = -motionConfig.FocalLength * motionConfig.FocalLength /
            (motionConfig.InfinityPosition - position) +
            motionConfig.FocalPlanesObjectSpace;
    }
    else
    {
        vid = motionConfig.FocalLength * motionConfig.FocalLength /
            (motionConfig.InfinityPosition - position) -
            motionConfig.FocalPlanesObjectSpace;
    }
    double sphere = 1000 / vid;
    return cv::Point3f(sphere, vid, position);
}

//Camera Interface
bool MLColorimeterMode::IsGrabbing(int index)
{
    GrabberBase* grabberBase = GetCamera(index);
    if (grabberBase == nullptr || !grabberBase->IsOpened())
    {
        return false;
    }
    return grabberBase->IsGrabbing();
}

Result MLColorimeterMode::StopGrabbing(bool stop,int index)
{
    if (!IsConnect())
    {
        return Result(false, "Stop grabbing failed, colorimeter is not connected.");
    }

    GrabberBase* grabberBase = GetCamera(index);
    if (grabberBase == nullptr || !grabberBase->IsOpened())
    {
        return Result(false, "Measure camera stop/start grabbing failed, camera is not connected.");
    }

    if (stop)
    {
        grabberBase->StopGrabbing();
    }
    else
    {
        grabberBase->StartGrabbingAsync();
    }

    emit stopGrab(stop);
    return Result();
}

Result MLColorimeterMode::SetExposureTime(double ms, int index)
{
    if (!IsConnect())
    {
        return Result(false, "Set exposure time failed, colorimeter is not connected.");
    }

    GrabberBase* grabberBase = GetCamera(index);
    grabberBase->SetExposureTime(ms * 1000.0);
    return Result();
}

Result MLColorimeterMode::SetExposureTime(ExposureSetting exposure, int index)
{
    if (!IsConnect())
    {
        return Result(false, "Set exposure failed, colorimeter is not connected.");
    }

    Result ret;
    MLMonoBusinessManage* mono = GetMonocular(index);
    ML::CameraV2::MLPixelFormat curFormat = mono->ML_GetPixelFormat();
    if (exposure.Mode == ExposureMode::Auto)
    {
        if (curFormat != MLPixelFormat::MLMono12)
        {
            ret = mono->ML_SetPixelFormat(MLPixelFormat::MLMono12);
            if (!ret.success)
                return ret;
        }
        ret = mono->ML_SetExposure(exposure);
        if (!ret.success)
            return ret;
        ret = mono->ML_SetPixelFormat(curFormat);
        if (!ret.success)
            return ret;
    }
    else
    {
        ret = mono->ML_SetExposure(exposure);
    }
    if (!ret.success)
    {
        return Result(false, "Set exposure failed." + ret.errorMsg);
    }
    return Result();
}

Result MLColorimeterMode::SetBitDepth(int bitDepth,int index)
{
    if (!IsConnect())
    {
        return Result(false, "Set bit depth failed, colorimeter is not connected.");
    }

    MLPixelFormat format;
    if (bitDepth == 8)
    {
        format = ML::CameraV2::MLPixelFormat::MLMono8;
    }
    else if (bitDepth == 10)
    {
        format = ML::CameraV2::MLPixelFormat::MLMono10;
    }
    else if (bitDepth == 12)
    {
        format = ML::CameraV2::MLPixelFormat::MLMono12;
    }
    else if (bitDepth == 16)
    {
        format = ML::CameraV2::MLPixelFormat::MLMono12;
    }
    Result ret = SetBitDepth(format,index);
    return ret;
}

Result MLColorimeterMode::SetBitDepth(ML::CameraV2::MLPixelFormat format, int index)
{
    if (!IsConnect())
    {
        return Result(false, "Set bit depth failed, colorimeter is not connected.");
    }
    Result ret = GetMonocular(index)->ML_SetPixelFormat(format);
    if (!ret.success)
    {
        return Result(false, QString("Set bit depth failed.").toStdString() + ret.errorMsg);
    }
    return Result();
}

Result MLColorimeterMode::SetBinning(int binning, int index)
{
    ML::CameraV2::Binning bin;
    if (binning == 1)
    {
        bin = ML::CameraV2::Binning::ONE_BY_ONE;
    }
    else if (binning == 2)
    {
        bin = ML::CameraV2::Binning::TWO_BY_TWO;
    }
    else if (binning == 4)
    {
        bin = ML::CameraV2::Binning::FOUR_BY_FOUR;
    }
    else if (binning == 8)
    {
        bin = ML::CameraV2::Binning::EIGHT_BY_EIGHT;
    }

    Result ret = SetBinning(bin,index);
    return ret;
}

Result MLColorimeterMode::SetBinning(Binning binning, int index)
{
    if (!IsConnect())
    {
        return Result(false, "Set binning failed, colorimeter is not connected.");
    }

    Result ret = GetMonocular(index)->ML_SetBinning(binning);
    if (!ret.success)
    {
        return Result(false, QString("Set binning failed.").toStdString() + ret.errorMsg);
    }
    return Result();
}

Result MLColorimeterMode::SetBinningMode(const QString& binningMode, int index)
{
    ML::CameraV2::BinningMode mode = ML::CameraV2::BinningMode::SUM;
    if (binningMode.toLower() == "average")
    {
        mode = ML::CameraV2::BinningMode::AVERAGE;
    }

    Result ret = SetBinningMode(mode,index);
    return ret;
}

Result MLColorimeterMode::SetBinningMode(BinningMode binningMode,int index)
{
    if (!IsConnect())
    {
        return Result(false, "Set binning mode failed, colorimeter is not connected.");
    }

    Result ret = GetMonocular(index)->ML_SetBinningMode(binningMode);
    if (!ret.success)
    {
        return Result(false, QString("Set binning mode failed.").toStdString() + ret.errorMsg);
    }
    return Result();
}

Result MLColorimeterMode::SetBinningSelector(ML::CameraV2::BinningSelector binningSelector, int index)
{
    if (!IsConnect())
    {
        return Result(false, "Set binning mode failed, colorimeter is not connected.");
    }

    Result ret = GetMonocular(index)->ML_SetBinningSelector(binningSelector);
    if (!ret.success)
    {
        return Result(false, QString("Set binning selector failed.").toStdString() + ret.errorMsg);
    }
    return Result();
}

double MLColorimeterMode::GetExposureTime(int index)
{
    if (IsConnect()) {
        return GetCamera(index)->GetExposureTime() / 1000.0;
    }
    return 0.0;
}

int MLColorimeterMode::GetBinning(int index)
{
    MLMonoBusinessManage* monocular = GetMonocular(index);
    if (monocular == nullptr)
    {
        return 0;
    }
    Binning bin = monocular->ML_GetBinning();

    int binInt = 1;
    switch (bin)
    {
    case ML::CameraV2::Binning::ONE_BY_ONE:
        binInt = 1;
        break;
    case ML::CameraV2::Binning::TWO_BY_TWO:
        binInt = 2;
        break;
    case ML::CameraV2::Binning::FOUR_BY_FOUR:
        binInt = 4;
        break;
    case ML::CameraV2::Binning::EIGHT_BY_EIGHT:
        binInt = 8;
        break;
    default:
        break;
    }

    return binInt;
}

ML::CameraV2::Binning MLColorimeterMode::TransIntToBinning(int bin)
{
    if (bin == 1)
        return ML::CameraV2::Binning::ONE_BY_ONE;
    else if (bin == 2)
        return ML::CameraV2::Binning::TWO_BY_TWO;
    else if (bin == 4)
        return ML::CameraV2::Binning::FOUR_BY_FOUR;
    else if (bin == 8)
        return ML::CameraV2::Binning::EIGHT_BY_EIGHT;
    else
        return ML::CameraV2::Binning::ONE_BY_ONE;
}

std::string MLColorimeterMode::GetBitDepth(int index)
{
    if (IsConnect()) {
        return MLColorimeterHelp::instance()->TransMLPixelFormatToStr(GetMonocular(index)->ML_GetPixelFormat());
    }
    return "";
}

BinningMode MLColorimeterMode::GetBinningMode(int index)
{
    MLMonoBusinessManage* monocular = GetMonocular(index);
    if (monocular == nullptr)
    {
        return BinningMode::AVERAGE;
    }
    return monocular->ML_GetBinningMode();
}

Result MLColorimeterMode::GetImage(cv::Mat &image, int index)
{
    if (!IsConnect())
    {
        return Result(false, "GetImage failed, colorimeter is not connected.");
    }
    Result ret = GetMonocular(index)->ML_CaptureImageSync();
    if (!ret.success)
    {
        return Result(false, "GetImage failed," + ret.errorMsg);
    }
    image = GetMonocular(index)->ML_GetImage();
}

Result MLColorimeterMode::DiopterScanProcess(DiopterScanConfig config, std::string path, std::string prefix)//0
{
    if (!IsConnect())
    {
        return Result(false, "DiopterScanProcess failed, colorimeter is not connected.");
    }
    cv::Mat image;
    Result ret = GetImage(image);
    if (!ret.success)
    {
        return Result(false, "DiopterScanProcess failed," + ret.errorMsg);
    }
 //   int offset;
	//MLIQMetrics::MLCrossMTF mlc;
	//MLIQMetrics::CenterCrossHairMTFROIRe algo_ret = mlc.getCenterCrossMTFROICenter(image, offset);
    int binNum = GetBinning();
    MultiCrossHairDetection md;
    md.setBinNum(binNum);
    MultiCrossHairRe algo_ret = md.getMuliCrossHairCenter(image, MLIQMetrics::IQMetricsParameters::crossBinNum / binNum, false);
    if (!algo_ret.flag)
    {
        std::string save_folder = path;
        std::string sn = MetricsData::instance()->getDutBarCode().toStdString();
        if (isDirExist(QString::fromStdString(save_folder)))
        {
            std::string path = save_folder + sn + "_through_focus_init.tif";
            cv::imwrite(path, image);
        }
        return Result(false, algo_ret.errMsg);
    }

    config.ROIs = algo_ret.rectVec;
    ret = GetMonocular()->ML_DiopterScan(FOCUS_KEY, config);
    if (!ret.success)
    {
        return Result(false, "DiopterScanProcess failed," + ret.errorMsg);
    }
    if (!path.empty())
    {
        ret = GetMonocular()->ML_SaveDiopterScanResult(path, config.StoreResultImg, prefix);
    }
    return ret;
}

Result MLColorimeterMode::MotionScanProcess(FocusScanConfig config, std::string path)//0
{
    if (!IsConnect())
    {
        return Result(false, "MotionScanProcess failed, colorimeter is not connected.");
    }
    cv::Mat image;
    Result ret = GetImage(image);
    if (!ret.success)
    {
        return Result(false, "MotionScanProcess failed," + ret.errorMsg);
    }

    int binNum = GetBinning();
    MultiCrossHairDetection md;
    md.setBinNum(binNum);
    MultiCrossHairRe algo_ret = md.getMuliCrossHairCenter(image, MLIQMetrics::IQMetricsParameters::crossBinNum / binNum, false);
    if (!algo_ret.flag)
    {
        std::string save_folder = path;
        std::string sn = MetricsData::instance()->getDutBarCode().toStdString();
        if (isDirExist(QString::fromStdString(save_folder)))
        {
            std::string path = save_folder + sn + "_through_focus_init.tif";
            cv::imwrite(path, image);
        }
        return Result(false, algo_ret.errMsg);
    }

    config.ROIs = algo_ret.rectVec;
    ret = GetMonocular()->ML_FocusScan(FOCUS_KEY, config);
    if (!ret.success)
    {
        return Result(false, "MotionScanProcess failed," + ret.errorMsg);
    }
    if (!path.empty())
    {
        ret = GetMonocular()->ML_SaveFocusScanResult(path);
    }
    return ret;
}


double MLColorimeterMode::GetDiopterScanHResult()
{
    return GetMonocular()->ML_GetDiopterScanResult_H().x;
}

double MLColorimeterMode::GetDiopterScanVResult()
{
    return GetMonocular()->ML_GetDiopterScanResult_V().x;
}

double MLColorimeterMode::GetDiopterScanAVGResult()
{
    cv::Point3f retH = GetMonocular()->ML_GetDiopterScanResult_H();
    cv::Point3f retV = GetMonocular()->ML_GetDiopterScanResult_V();
    return TransFocusMotionToSphVid((TransSphToFocusMotionVid(retH.x).z + TransSphToFocusMotionVid(retV.x).z) / 2.0).x;
}

double MLColorimeterMode::GetMotionScanHResult()
{
    std::vector<cv::Point3f> roisRe = GetMonocular()->ML_GetFocusScanResult();
    if (roisRe.size() == 4)
    {
        return TransFocusMotionToSphVid((roisRe[0].x + roisRe[1].x) / 2.0).x;
    }
    return -1;
}

double MLColorimeterMode::GetMotionScanVResult()
{
    std::vector<cv::Point3f> roisRe = GetMonocular()->ML_GetFocusScanResult();
    if (roisRe.size() == 4)
    {
        return TransFocusMotionToSphVid((roisRe[2].x + roisRe[3].x) / 2.0).x;
    }
    return -1;
}

double MLColorimeterMode::GetMotionScanAVGResult()
{
    std::vector<cv::Point3f> roisRe = GetMonocular()->ML_GetFocusScanResult();
    if (roisRe.size() == 4)
    {
        return TransFocusMotionToSphVid((roisRe[0].x + roisRe[1].x + roisRe[2].x + roisRe[3].x) / 4.0).x;
    }
    return -1;
}

void MLColorimeterMode::StopThroughFocusProcess()
{
    GetMonocular()->StopThroughFocus();
}

ML::MLColorimeter::MotionConfig MLColorimeterMode::GetCameraMotionConfig()
{
    return GetMonocular()->ML_GetBusinessManageConfig()->GetModuleInfo().MotionConfig_Map[FOCUS_KEY];
}

ML::MLColorimeter::ModuleConfig MLColorimeterMode::GetModuleConfig(int index)
{
    return GetMonocular(index)->ML_GetBusinessManageConfig()->GetModuleInfo();
}

Result MLColorimeterMode::LoadCalibrationData(const CalibrationConfig2& c_config, const CalibrationFlag& c_flag)
{
    struct FlagProcessPair {
        bool flag;
        ProcessType process;
    };

    const FlagProcessPair flags[] = {
        { c_flag.Dark_Flag, ProcessType::Dark },
        { c_flag.FFC_Flag, ProcessType::FFC },
        { c_flag.Distortion_Flag, ProcessType::Distortion },
        { c_flag.ColorShift_Flag, ProcessType::ColorShift },
        { c_flag.FourColor_Flag, ProcessType::FourColor },
        { c_flag.Exposure_Flag, ProcessType::Exposure },
        { c_flag.Flip_Rotate_Flag, ProcessType::Flip_Rotate },
        { c_flag.FOVCrop_Flag, ProcessType::FOVCrop },
        { c_flag.Luminance_Flag, ProcessType::Luminance }
    };

    Result ret;
    for (const auto& fp : flags) {
        if (fp.flag) {
            ret = m_taskManager->LoadCalibrationModule(fp.process, c_config);
            if (!ret.success)
                return ret;
        }
    }
    return ret;
}

cv::Mat MLColorimeterMode::Flip_Rotation(cv::Mat srcImg,int index)
{
    ML::MLColorimeter::ProcessPathConfig ppconfig;
    ppconfig.InputPath = CONFIG_PATH.toStdString();
    std::string outPath =
        GetMonocular(index)->ML_GetBusinessManageConfig()->ProcessPath("Flip_Rotate", ppconfig);
    bool ok = MLColorimeterHelp::instance()->ReadFlip_RotateJson(
        (ppconfig.InputPath + outPath).c_str(), "Flip_Rotate");
    ML::MLColorimeter::Flipping FlipX = MLColorimeterHelp::instance()->GetFlippingX();
    ML::MLColorimeter::Flipping FlipY = MLColorimeterHelp::instance()->GetFlippingY();
    ML::MLColorimeter::Rotation Rotate = MLColorimeterHelp::instance()->GetRotation();

    if (FlipX == ML::MLColorimeter::Flipping::ReverseX) {
        srcImg = MLColorimeterHelp::instance()->Flip(srcImg, FlipX);
    }
    if (FlipY == ML::MLColorimeter::Flipping::ReverseY) {
        srcImg = MLColorimeterHelp::instance()->Flip(srcImg, FlipY);
    }
    if (Rotate != ML::MLColorimeter::Rotation::R0) {
        srcImg = MLColorimeterHelp::instance()->Rotation(srcImg, Rotate);
    }
    return srcImg;
}

bool MLColorimeterMode::UpdateFlipRotation(Flipping flip_x, Flipping flip_y, Rotation rotate,int index)
{
    ML::MLColorimeter::ProcessPathConfig ppconfig;
    ppconfig.InputPath = GetMonocular(index)->ML_GetConfigPath();
    std::string outPath =
        GetMonocular(index)->ML_GetBusinessManageConfig()->ProcessPath("Flip_Rotate", ppconfig);
    return MLColorimeterHelp::instance()->WriteFlip_RotateToJson(
        (ppconfig.InputPath + "\\" + outPath).c_str(), "Flip_Rotate", flip_x, flip_y, rotate);
}

QString MLColorimeterMode::GetProcessPath()
{
    return CONFIG_PATH + "\\EYE1";
}

bool MLColorimeterMode::isEnabled(std::string key)//0
{
    if (!IsConnect())
    {
        return false;
    }

    bool enabled = false;
    ML::MLColorimeter::ModuleConfig config = GetMonocular()->ML_GetBusinessManageConfig()->GetModuleInfo();
    if (ND_FILTER_KEY == key || XYZ_FILTER_KEY == key)
    {
        if (config.NDFilterConfig_Map.find(key) != config.NDFilterConfig_Map.end())
        {
            enabled = config.NDFilterConfig_Map[key].enable;
        }
    }
    else if (FOCUS_KEY == key)
    {
        if (config.MotionConfig_Map.find(key) != config.MotionConfig_Map.end())
        {
            enabled = config.MotionConfig_Map[key].Enable;
        }
    }
    else if (CAMERA_KEY == key)
    {
        enabled = config.CameraConfig.Enable;
    }
    else if (RX_KEY == key)
    {
        enabled = config.RXFilterConfig.enable;
    }
    return enabled;
}

Result MLColorimeterMode::initAfterConnect()
{
    Result ret = ClearAlarmFilter();
    if (!ret.success)
    {
        LoggingWrapper::instance()->warn("Init after connect error, " + QString::fromStdString(ret.errorMsg));
    }
    return ret;
}

bool MLColorimeterMode::isDirExist(QString fullPath)
{
    if (fullPath.isEmpty())
    {
        return true;
    }
    QDir dir(fullPath);
    if (dir.exists())
    {
        return true;
    }
    else
    {
        bool ok = dir.mkpath(fullPath);
        return ok;
    }
}

void MLColorimeterMode::notifyPause(bool isPause)
{
    m_recipePause = isPause;
    if (!isPause)
    {
        m_waitCondition.wakeOne();
    }
}

void MLColorimeterMode::continueRun(bool isContinue)
{
    m_isStop = !isContinue;
    m_waitCondition.wakeOne();

    if (!isContinue)
    {
        QThread::msleep(500);
        m_recipePause = false;
    }
}

bool MLColorimeterMode::UpdateFixExposureTimeConfig(const std::string& sourceDir)
{
	fs::path sourceFilePath = fs::path(sourceDir) / "metaData.csv";
	if (fs::exists(sourceFilePath) && fs::is_regular_file(sourceFilePath)) 
    {
		return fs::copy_file(sourceFilePath, FIXEXPOSURE_FILE, fs::copy_options::overwrite_existing);
	}
	return false;
}

bool MLColorimeterMode::ReadFixExposureTime()
{
	try {
        m_FixExposureMap.clear();
		std::ifstream file(FIXEXPOSURE_FILE);
        if (!file.is_open()) {
            LoggingWrapper::instance()->error(QString("MLColorimeterMode: %1 does not exist or cannot be opened.")
                .arg(QString::fromStdString(FIXEXPOSURE_FILE)), false);
            return false;
        }
		std::string line;
		std::string token;
		std::vector<std::string> headers;

		if (std::getline(file, line)) {
			std::istringstream ss(line);
			while (std::getline(ss, token, ',')) {
				headers.push_back(token);
			}
		}

		while (std::getline(file, line)) {
			std::istringstream ss(line);
			std::vector<std::string> values;
			while (std::getline(ss, token, ',')) {
				values.push_back(token);
			}

			if (values.size() > 1) {
				std::string imageName = values[0];
				double exposureTime = std::stod(values[4]);
				m_FixExposureMap[imageName] = exposureTime;
			}
		}
		return true;
	}
	catch (std::exception e)
	{
		LoggingWrapper::instance()->error("MLColorimeterMode: read fix exposure time error," + QString::fromStdString(e.what()), false);
		return false;
	}
}

Result MLColorimeterMode::GetFixExposureTime(std::string name, double& time)
{
    if (!m_FixExposureMap.count(name))
    {
        bool ret = ReadFixExposureTime();
        if (!ret)
        {
            LoggingWrapper::instance()->error(QString::fromStdString("Get Fix exposure time error."), false);
            return Result(false, "Get Fix exposure time error.");
        }
    }

    if (!m_FixExposureMap.count(name))
    {
        LoggingWrapper::instance()->error(QString::fromStdString("Fix exposure time map is not contain " + name), false);
        return Result(false, "Fix exposure time map is not contain " + name);
    }

    time = m_FixExposureMap[name];
    return Result();
}