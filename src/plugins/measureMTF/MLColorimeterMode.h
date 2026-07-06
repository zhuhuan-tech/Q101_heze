#pragma once

#include <windows.h>
#include <QMap>
#include <QObject>
#include <opencv2\opencv.hpp>

#include "MLBinoBusinessManage.h"
#include "MLCamaraCommon.h"
#include "ML_addInInterface.h"
#include "pluginException.h"
#include "MLColorimeter.h"
#include "MLColorimeterConfig.h"
#include "MLColorimeterCommon.h"
#include "MLCameraParameters.h"
#include "MLColorimeterPlugin.h"
#include "MLTaskManager.h"

#include <QWaitCondition>

using namespace std;
using namespace ML::MLColorimeter;
using namespace ML::MLFilterWheel;
using namespace ML::CameraV2;
using namespace ML_Task;

class ML::MLColorimeter::MLMonoBusinessManage;

enum MLColorimeterCameraType{
    withColorimeter,
    singleCam
};
class MLColorimeterMode : public QObject
{
    Q_OBJECT

public:
    ~MLColorimeterMode();
    static MLColorimeterMode* Instance(QObject* parent = nullptr);
    Result Connect();
    Result ConnectAsync(int index=0);
    Result InitConfig();
    Result Disconnect();
    bool IsConnect();

    GrabberBase* GetCamera(int index=0);
    ActuatorBase* GetCameraMotion();
    MLBinoBusinessManage* GetBinocular();
    MLMonoBusinessManage* GetMonocular(int index=0);
    //MLMonoBusinessManage* GetSecondMonocular();
    ML_Task::MLTaskManager* GetCaptureTaskManager();

    ML::MLColorimeter::ModuleConfig GetModuleConfig(int index);
    ML::MLColorimeter::MotionConfig GetCameraMotionConfig();

    // ================ND XYZ Filter==================//
    Result SetNDFilter(QString nd);
    Result SetColorFilter(QString xyz);
    Result SetNDFilterAsyn(QString nd);
    Result SetXYZFilterAsyn(QString xyz);
    Result SetNDFilter(MLFilterEnum nd);
    Result SetColorFilter(MLFilterEnum xyz);
    QString GetNDFilter();
    QString GetXYZFilter();
    MLFilterEnum GetNDFilterEnum();
    MLFilterEnum GetXYZFilterEnum();
    Result ClearAlarmFilter();
    Result ClearAlarmRx();

    // ================Cylinder Filter==================//
    Result SetRXSync(RXCombination rx);
    Result SetRXAsync(RXCombination rx);
    Result SetCylinder(QString cylinder, int axis);
    RXCombination GetRX();

    // ================Focus Motion==================//
    Result SetVidSync(double vid);
    Result SetVidAsync(double vid);
    Result SetSphereSync(double sphere);
    Result SetSphereAsync(double sphere);
    Result SetFocusMotionPosAsync(double position);
    Result SetFocusMotionPosSync(double position);
    Result SetFocusMotionPosRelAsync(double position);
    Result SetFocusMotionPosRelSync(double position);
    double GetVid();
    double GetFocusMotionPos();
	// x:sphere y:vid z:motion pos
    cv::Point3f TransVidToFocusMotionSph(double vid);
    cv::Point3f TransSphToFocusMotionVid(double sphere);
    cv::Point3f TransFocusMotionToSphVid(double position);

    // ================Measure Camera==================//
    bool IsGrabbing(int index=0);
    Result StopGrabbing(bool stop,int index=0);
    Result SetExposureTime(double ms, int index=0);
    Result SetExposureTime(ExposureSetting exposure, int index=0);
    Result SetBitDepth(int bitDepth,int index=0);
    Result SetBitDepth(ML::CameraV2::MLPixelFormat bitDepth,int index=0);
    Result SetBinning(int binning,int index=0);
    Result SetBinning(ML::CameraV2::Binning binning, int index=0);
    Result SetBinningMode(const QString& binningMode, int index=0);
    Result SetBinningMode(ML::CameraV2::BinningMode binningMode, int index=0);
    Result SetBinningSelector(ML::CameraV2::BinningSelector binningSelector, int index=0);
    double GetExposureTime(int index=0);
    int GetBinning(int index=0);
    std::string GetBitDepth(int index=0);
    ML::CameraV2::BinningMode GetBinningMode(int index=0);
    Result GetImage(cv::Mat& image, int index=0);

    QString GetProcessPath();

    // ================Through Focus==================//
    //DiopterScan
    Result DiopterScanProcess(DiopterScanConfig config, std::string path = "", std::string prefix = "");
    double GetDiopterScanHResult();
    double GetDiopterScanVResult();
    double GetDiopterScanAVGResult();
    // CameraMotion Scan
    Result MotionScanProcess(FocusScanConfig config, std::string path);
    double GetMotionScanHResult();
    double GetMotionScanVResult();
    double GetMotionScanAVGResult();
    void StopThroughFocusProcess();


    //Capture Image
    Result LoadCalibrationData(const CalibrationConfig2& c_config, const CalibrationFlag& c_flag);

    // ================Other Methods==================//
    cv::Mat Flip_Rotation(cv::Mat srcImg,int index=0);
    ML::CameraV2::Binning TransIntToBinning(int bin);
    bool isDirExist(QString fullPath);
    bool UpdateFlipRotation(Flipping flip_x = Flipping::NonReverse, Flipping flip_y = Flipping::NonReverse, ML::MLColorimeter::Rotation rotate = ML::MLColorimeter::Rotation::R0,int index = 0);
    Result GetFixExposureTime(std::string name, double& time);
    bool UpdateFixExposureTimeConfig(const std::string& sourceDir);
    bool ReadFixExposureTime();

public:
    void notifyPause(bool isPause);
    // continue or stop
    void continueRun(bool isContinue);

private:
    Result initAfterConnect();
    bool isEnabled(std::string key);

private:
    static MLColorimeterMode* self;
    MLColorimeterMode(QObject* parent = nullptr);
    ML::MLColorimeter::MLBinoBusinessManage* m_bino = nullptr;
    ML_Task::MLTaskManager* m_taskManager = nullptr;

    QWaitCondition m_waitCondition;
    QMutex m_mutex;
    bool m_isStop = false;
    bool m_recipePause = false;

    bool m_IsConnected = false;
    const string ND_FILTER_KEY = "NDFilter";
    const string XYZ_FILTER_KEY = "ColorFilter";
    const string FOCUS_KEY = "CameraMotion";
    const string CAMERA_KEY = "VieCamera1";
    const string RX_KEY = "RXFilter";
    const QString CONFIG_PATH = ".\\config\\mlcolorimeter";
    const string FIXEXPOSURE_FILE = ".\\config\\exposureTime\\ET.csv";

    QString IMAGE_FILE_EXT = ".tif";

    std::map<std::string, double> m_FixExposureMap;

    //bool createResutCSVFile(QString cvFileName);

    //ML::MLColorimeter::FOVCrop getFOVCrop();
    //cv::Point2f getFOVCenter();

signals:
    void connectStatus(bool connected);
    void stopGrab(bool isStopGrab);
    void updateCameraImageSignal(const cv::Mat& image, MLColorimeterCameraType type=MLColorimeterCameraType::withColorimeter);
    //void updateSingleCameraImageSignal(const cv::Mat& image);
    void updateCameraGrayLevelSignal(int gray, MLColorimeterCameraType type = MLColorimeterCameraType::withColorimeter);
    //void updateSingleCameraGrayLevelSignal(int gray);
};
