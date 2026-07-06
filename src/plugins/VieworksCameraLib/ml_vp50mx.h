/************************************************************************/
/*
*
* Filename     :  ml_vp50mx.h
* date         :  2022.8.18
* Content      :  Camera operation interface
* author       :  jason.huang
*
*************************************************************************/
#ifndef ML_VP50MX_H
#define ML_VP50MX_H


#include <opencv2/opencv.hpp>
#include <PrjCommon/service/ml_camera.h>
#include "Mil.h"
#include <windows.h>
#include <mutex>
#include "VieworksCameraLib.h"
//#include "MLCamaraCommon.h"
#include "AutoResetEvent.h"

using namespace CORE;

/* User's processing function hook data structure. */
typedef struct
{
    MIL_ID  MilDigitizer;
    MIL_ID  MilImage;
    MIL_INT ProcessedImageCount;


    void* camera;

    cv::Mat Frame;

    void* Param;

} HookDataStruct;

class VIEWORKS_EXPORT ml_vp50mx : public CORE::MLCamera
{
public:
     ml_vp50mx();
     ~ml_vp50mx();

public:
    virtual bool InitializeConfigVariables(const char* configfile);

    //No interface display is required
    virtual CORE::MLResult Open();

    virtual CORE::MLResult OpenBySN(const char* SN);

    //Display in user-defined interface
    CORE::MLResult Open(HWND& wid);

    virtual void Close();

    virtual bool IsOpened();

    virtual CORE::MLCameraInfo GetCameraInfo();

    virtual CORE::MLResult GrabOne();

    virtual void StartGrabbingAsync();

    virtual void StopGrabbing();

    virtual bool IsGrabbing();

    virtual int GetWidth();

    virtual int GetHeight();

    virtual void SetFrameRate(double rate = 15.5);

    virtual double GetFrameRate();

    virtual void SetFormatType(MLPixelFormat format);

    virtual CORE::MLPixelFormat GetFormatType();

    virtual void SetMLExposureAuto();

    //virtual AutoExposureStatus SetMLExposureAutoDiscard2();

    //virtual void SetMLExposureAutoDiscard();

    virtual void SetExposureTime(double time = 10000);  //us, Valid range:1-500000

    virtual double GetExposureTime();

    virtual void SetExposureMode(MLExposureMode mode);

    virtual MLExposureMode GetExposureMode();

    virtual void GetExposureTimeRange(double& min, double& max);

    virtual void SetAdjustExposureTime(double time);

    virtual void SetGain(double value);

    virtual double GetGain();

    virtual void GetGainRange(double& min, double& max);

    virtual MLCameraState GetState();

    virtual Binning getBinning();

    virtual BinningMode GetBinningMode();

    virtual void SetBinning(Binning binning);

    virtual void SetBinningMode(BinningMode binningMode);

    virtual void SetBinning(Binning binning, BinningMode binningMode);
    virtual void SetFocusSyn(double vid) {};

    virtual void SetFocusAsy(double vid) {};

    virtual void AutoFocus() {};

    virtual void StopFocus() {};
    virtual void FlipX(bool flip);

    virtual void FlipY(bool flip);

    virtual void Rotate(bool rotate);

    virtual void Subscribe(MLCameraEvent event, MLCameraCallback* callback);

    virtual void Unsubscribe(MLCameraEvent event, MLCameraCallback* callback);

    virtual cv::Mat GetImage();

    virtual void StartCalcGrayLevel();

    virtual void StopCalcGrayLevel();

    virtual void SetTemperature(int temperature);

    virtual int GetTemperature();

    virtual void SetTriggerSource(TriggerSource source);

private:
    int ConversionCycleMultiple(int us);

    int AdjustTime(int init_time, int rate);
    
    void GrayLevelThread();
    
    cv::Mat GetNewImage();

    void NotifyCameraStatus(MLCameraState );

    void NotifyImageGrayLevel(int );

    //did't used, should delete?
    void SetPeltierOperationMode(std::string peltierOperationMode);
    //did't used, should delete?
    std::string GetPeltierOperationMode();

    void PrintMatroxError();

public:
    void NotifyImageCallBack();
    AutoResetEvent captureEvent;
    void HardwareArmOn();

    void ArmOn();
    void ArmOff();
    void SoftwareTrigger();

private:
    /* MIL variables */
    MIL_ID MilApplication;  /* MIL Application identifier.  */
    MIL_ID MilDisplay,
           MilSystem,       /* MIL System identifier.      */
           MilDigitizer,    /* MIL Digitizer identifier.   */
           MilImage;       /* MIL Image buffer identifier. */

    MIL_INT BufSizeX = 0;
    MIL_INT BufSizeY = 0;
    MIL_INT BufSizeBand = 0;
    MIL_UINT16* UserImagePtr = nullptr;
    HookDataStruct UserHookData;


private:
    bool m_isOpen;
    bool m_isGrab;
    MLPixelFormat m_pixelFormat;
    MLCameraCallback* camera_callback_;
    MLCameraState camera_state_;
    std::string m_exposureTimeMode;
    bool grabbing_callback_;
    bool graylevel_callback_;
    int rate_;
    int m_nMinExp = 1;
    int m_nMaxExp = 60000000;
    int max_num_;
    bool m_isGrayRun;
    bool m_GrayLevelThread;
    std::thread m_Thread_;
    int m_bitDepth;
    std::string autoExposureConfigPath;

    bool m_isFristIntoGetImage = true;
    cv::Mat imageBefore;
    int exposureTimeBefore = 0;
    double currentExposureTime = 0;

    
};

#endif