#include "ml_vp50mx.h"
#include "LogPlus.h"
#include "helper.h"
#include <direct.h>
#include "ml_vp50mxSystem.h"
#include "MLFrame.h"
#include <json.hpp>
#include <iostream>
#include <fstream>

#define GetCurrentDir _getcwd
std::string get_current_dir() {
    char buff[FILENAME_MAX]; //create string buffer to hold path
    GetCurrentDir(buff, FILENAME_MAX);
    string current_working_dir(buff);
    return current_working_dir;
}

/* Default image dimensions. */
#define DEFAULT_IMAGE_SIZE_X       640
#define DEFAULT_IMAGE_SIZE_Y       480
#define DEFAULT_IMAGE_SIZE_BAND      1
#define DEFAULT_TIMEOUT      10000000

static void AutoExposureCallBack(int exposure_time, double gray, int current_step, int total_step, string msg);
/* User's processing function prototype. */
MIL_INT MFTYPE ProcessingFunction(MIL_INT HookType, MIL_ID HookId, void* HookDataPtr);
mutex m_nMutex;

ml_vp50mx::ml_vp50mx()
{
    m_isOpen = false;
    m_isGrab = false;
    camera_state_ = MLCameraState::kDisconnected;
    camera_callback_ = NULL;
    grabbing_callback_ = false;
    graylevel_callback_ = false;
    m_isGrayRun = false;
    m_GrayLevelThread = false;
    m_exposureTimeMode = "mlAuto";
    m_isImageUpdate = false;
    m_isFristIntoGetImage = true;
    autoExposureConfigPath = "";
    LOG4CPLUS_INFO(LogPlus::getInstance()->logger, "Create Camera Instance");
}

ml_vp50mx::~ml_vp50mx()
{
    Close();
    if(UserImagePtr)
        SAFE_DELETE_ARRAY(UserImagePtr);
}

MLResult ml_vp50mx::Open(HWND& wid)
{
    MLResult result;
    if (m_isOpen)
    {
        result.code =(int)MLStatus:: ML_STATUS_OK;
        result.msg = "The camera connected!";
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, result.msg.c_str());


        this->NotifyCameraStatus(MLCameraState::kConnected);

        return result;
    }

    /* Allocate a MIL application. */
    MappAlloc(M_NULL, M_DEFAULT, &MilApplication);

    /* Allocate a MIL system. */
    MsysAlloc(M_DEFAULT, MIL_TEXT("M_DEFAULT"), M_DEFAULT, M_DEFAULT, &MilSystem);

    /* Allocate a MIL display. */
    MdispAlloc(MilSystem, M_DEFAULT, MIL_TEXT("M_DEFAULT"), M_WINDOWED, &MilDisplay);

    /* Allocate a MIL digitizer if supported and sets the target image size. */
    if (MsysInquire(MilSystem, M_DIGITIZER_NUM, M_NULL) > 0)
    {
        MdigAlloc(MilSystem, M_DEFAULT, MIL_TEXT("M_DEFAULT"), M_DEFAULT, &MilDigitizer);
        MdigInquire(MilDigitizer, M_SIZE_X, &BufSizeX);
        MdigInquire(MilDigitizer, M_SIZE_Y, &BufSizeY);
        MdigInquire(MilDigitizer, M_SIZE_BAND, &BufSizeBand);

    }
    else
    {
        MilDigitizer = M_NULL;
        BufSizeX = DEFAULT_IMAGE_SIZE_X;
        BufSizeY = DEFAULT_IMAGE_SIZE_Y;
        BufSizeBand = DEFAULT_IMAGE_SIZE_BAND;
    }

    /* Allocate a MIL buffer. */
    MbufAllocColor(MilSystem, BufSizeBand, BufSizeX, BufSizeY, 16 + M_UNSIGNED,
        (MilDigitizer ? M_IMAGE + M_DISP + M_GRAB : M_IMAGE + M_DISP), &MilImage);
    /* Clear the buffer */
   // MbufClear(MilImage, 0);
    MbufClear(MilImage, M_RGB888(0, 0, 0));

    /* Select the MIL buffer to be displayed in the user-specified window */
    MdispSelectWindow(MilDisplay, MilImage, (MIL_WINDOW_HANDLE)wid);
    
   
    if (MilDigitizer)
    {
        
        // The following code can be used to control feature values.
        MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("PixelFormat"), M_TYPE_STRING, MIL_TEXT("Mono12"));

        // Grab continuously.
        //MdigGrabContinuous(MilDigitizer, MilImage);

        // Initialize the user's processing function data structure.
        UserHookData.MilDigitizer = MilDigitizer;
        UserHookData.MilImage = MilImage;
        UserHookData.ProcessedImageCount = 0;
        UserHookData.camera = this;
        
        SetFormatType(MLPixelFormat::kMLMono12);
        UserImagePtr = new MIL_UINT16[GetWidth() * GetHeight()];
    }
    m_isOpen = true;


    result.code = (int)MLStatus::ML_STATUS_OK;
    result.msg = "The camera connects successfully!";
    LOG4CPLUS_INFO(LogPlus::getInstance()->logger, result.msg.c_str());

    this->NotifyCameraStatus(MLCameraState::kConnected);

    return result;
}

MLResult ml_vp50mx::Open()
{
    MLResult result;
    if (m_isOpen)
    {
        result.code = (int)MLStatus::ML_STATUS_OK;
        result.msg = "The camera connected!";
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, result.msg.c_str());
        this->NotifyCameraStatus(MLCameraState::kConnected);
        return result;
    }

    /* Allocate a MIL application. */
    MappAlloc(M_NULL, M_DEFAULT, &MilApplication);

    /* Allocate a MIL system. */
    MsysAlloc(M_DEFAULT, MIL_TEXT("M_DEFAULT"), M_DEFAULT, M_DEFAULT, &MilSystem);

    /* Allocate a MIL display. */
    MdispAlloc(MilSystem, M_DEFAULT, MIL_TEXT("M_DEFAULT"), M_WINDOWED, &MilDisplay);


    /* Allocate a MIL digitizer if supported and sets the target image size. */
    if (MsysInquire(MilSystem, M_DIGITIZER_NUM, M_NULL) > 0)
    {
        MdigAlloc(MilSystem, M_DEFAULT, MIL_TEXT("M_DEFAULT"), M_DEFAULT, &MilDigitizer);
        MdigInquire(MilDigitizer, M_SIZE_X, &BufSizeX);
        MdigInquire(MilDigitizer, M_SIZE_Y, &BufSizeY);
        MdigInquire(MilDigitizer, M_SIZE_BAND, &BufSizeBand);

    }
    else
    {
        MilDigitizer = M_NULL;
        BufSizeX = DEFAULT_IMAGE_SIZE_X;
        BufSizeY = DEFAULT_IMAGE_SIZE_Y;
        BufSizeBand = DEFAULT_IMAGE_SIZE_BAND;
    }

    /* Allocate a MIL buffer. */
    MbufAllocColor(MilSystem, BufSizeBand, BufSizeX, BufSizeY, 16 + M_UNSIGNED,
        (MilDigitizer ? M_IMAGE + M_DISP + M_GRAB : M_IMAGE + M_DISP), &MilImage);
    /* Clear the buffer */
    MbufClear(MilImage, 0);
    //MbufClear(MilImage, M_RGB888(0, 0, 0));

    /* Select the MIL buffer to be displayed in the user-specified window */
    //MdispSelectWindow(MilDisplay, MilImage, (MIL_WINDOW_HANDLE)wid);   
    
    if (MilDigitizer)
    {
        SetFormatType(MLPixelFormat::kMLMono12);
        UserImagePtr = new MIL_UINT16[GetWidth() * GetHeight()];

    }
    m_isOpen = true;
    GetWidth();
    GetHeight();

    result.code = (int)MLStatus::ML_STATUS_OK;
    result.msg = "The camera connects successfully!";
    LOG4CPLUS_INFO(LogPlus::getInstance()->logger, result.msg.c_str());

    this->NotifyCameraStatus(MLCameraState::kConnected);

    return result;
}

MLResult ml_vp50mx::OpenBySN(const char* SN)
{
    string sn = "[--Camera: " + serial_number_ + "]--";
    int dev = atoi(SN);
    ml_vp50mxSystem::GetInstance().OpenConnection();
    MilApplication = ml_vp50mxSystem::GetInstance().ml_vp.MilApplication;
    MilSystem = ml_vp50mxSystem::GetInstance().ml_vp.MilSystem;

    LOG4CPLUS_INFO(LogPlus::getInstance()->logger, sn.c_str() << "MilApplication = " << MilApplication);
    LOG4CPLUS_INFO(LogPlus::getInstance()->logger, sn.c_str() << "MilSystem = " << MilSystem);

    MLResult result;
    if (m_isOpen)
    {
        result.code = (int)MLStatus::ML_STATUS_OK;
        result.msg = "The camera is already connected!";
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, sn.c_str() << result.msg.c_str());
        this->NotifyCameraStatus(MLCameraState::kConnected);
        return result;
    }
    
    MdispAlloc(MilSystem, M_DEFAULT, MIL_TEXT("M_DEFAULT"), M_WINDOWED, &MilDisplay);
    /* Allocate a MIL digitizer if supported and sets the target image size. */
    if (MsysInquire(MilSystem, M_DIGITIZER_NUM, M_NULL) > 0)
    {
        MdigAlloc(MilSystem, dev, MIL_TEXT("M_DEFAULT"), M_DEFAULT, &MilDigitizer);
        MdigInquire(MilDigitizer, M_SIZE_X, &BufSizeX);
        MdigInquire(MilDigitizer, M_SIZE_Y, &BufSizeY);
        MdigInquire(MilDigitizer, M_SIZE_BAND, &BufSizeBand);
    }
    else
    {
        MilDigitizer = M_NULL;
        BufSizeX = DEFAULT_IMAGE_SIZE_X;
        BufSizeY = DEFAULT_IMAGE_SIZE_Y;
        BufSizeBand = DEFAULT_IMAGE_SIZE_BAND;
    }

    /* Allocate a MIL buffer. */
    MbufAllocColor(MilSystem, BufSizeBand, BufSizeX, BufSizeY, 8 + M_UNSIGNED,
        (MilDigitizer ? M_IMAGE + M_GRAB: M_IMAGE), &MilImage);//829

    MbufClear(MilImage, 0);//829


    
    m_isOpen = true; 

    if (MilDigitizer)
    {
        //SetFormatType(MLPixelFormat::MLMono12);
        //GetFormatType();
        // 829
        MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("PixelFormat"), M_TYPE_STRING, MIL_TEXT("Mono8"));
        //MbufAllocColor(MilSystem, BufSizeBand, BufSizeX, BufSizeY, 8 + M_UNSIGNED,
        //    (MilDigitizer ? M_IMAGE + M_DISP + M_GRAB : M_IMAGE + M_DISP), &MilImage);

        ///* Clear the buffer */
        //MbufClear(MilImage, 0);
        //MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("BinningVertical"), M_TYPE_STRING, MIL_TEXT("X4"));
        //MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("BinningVertical"), M_TYPE_STRING, MIL_TEXT("X4")); UserHookData.MilDigitizer = MilDigitizer;
        UserHookData.MilImage = MilImage;
        UserHookData.ProcessedImageCount = 0;
        UserHookData.camera = this;
        UserImagePtr = new MIL_UINT16[GetWidth() * GetHeight()];
    }
    else
    {
        m_isOpen = false;
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, sn.c_str() << " open failed");
        this->NotifyCameraStatus(MLCameraState::kDisconnected);
        return MLResult{ 0, serial_number_ + " camera open failed"};
    }

    result.code =(int)CORE::MLStatus:: ML_STATUS_OK;
    result.msg = "The camera connects successfully!";
    LOG4CPLUS_INFO(LogPlus::getInstance()->logger, sn.c_str() << result.msg.c_str());
    this->NotifyCameraStatus(MLCameraState::kConnected);
    currentExposureTime = GetExposureTime();
    string emsg = "current exposure time: " + to_string(currentExposureTime);
    LOG4CPLUS_INFO(LogPlus::getInstance()->logger, emsg.c_str());

    return result;
}

void ml_vp50mx::Close()
{
    if (m_isOpen)
    {
        MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("TriggerMode"), M_TYPE_STRING, MIL_TEXT("Off"));

        MdigProcess(MilDigitizer, &MilImage, 1,
            M_STOP, M_DEFAULT, ProcessingFunction, &UserHookData);

        /* Remove the MIL buffer from the display. */
        //MdispSelect(MilDisplay, M_NULL);

        /* Free allocated objects. */
        MbufFree(MilImage);
        if (MilDigitizer)
            MdigFree(MilDigitizer);
        MdispFree(MilDisplay);
        //MsysFree(MilSystem);
        //MappFree(MilApplication);     
        ml_vp50mxSystem::GetInstance().CloseConnection();
        m_isOpen = false;
        m_isGrab = false;

        std::string msg = "The camera is turned off!";
        string sn = "[--Camera: " + serial_number_ + "]--";
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, sn.c_str() << msg.c_str());

        this->NotifyCameraStatus(MLCameraState::kDisconnected);
    }
}

bool ml_vp50mx::IsOpened()
{
    return m_isOpen;
}

MLCameraInfo ml_vp50mx::GetCameraInfo()
{
    MLCameraInfo info;
    MIL_STRING CameraVendor;
    MIL_STRING CameraModel;
    MIL_STRING CameraSerialNumber;
    MIL_STRING CameraUserName;
    MIL_STRING CameraScanType;
    if (IsOpened())
    {
        MdigInquireFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("DeviceVendorName"), M_TYPE_STRING, CameraVendor);
        MdigInquireFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("DeviceModelName"), M_TYPE_STRING, CameraModel);
        MdigInquireFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("DeviceID"), M_TYPE_STRING, CameraSerialNumber);
        MdigInquireFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("DeviceUserID"), M_TYPE_STRING, CameraUserName);
        MdigInquireFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("DeviceScanType"), M_TYPE_STRING, CameraScanType);
    }
    //TODO:MIL_STRING switch to string
    /*info.CameraVendor  = CameraVendor;
    info.CameraModel = CameraModel;
    info.CameraSerialNumber = CameraSerialNumber;
    info.CameraUserName = CameraUserName;
    info.CameraScanType = CameraScanType;*/
    return MLCameraInfo{};
}

MLResult ml_vp50mx::GrabOne()
{
    string sn = "[--Camera: " + serial_number_ + "]--";
    if (IsOpened())
    {
        try
        {
            LOG4CPLUS_INFO(LogPlus::getInstance()->logger, sn.c_str() << "Grab One");

            if (m_isGrab)
            {
                std::string msg = "The camera is grabbing!";

                LOG4CPLUS_INFO(LogPlus::getInstance()->logger, sn.c_str() << msg.c_str());
                return MLResult{ (int)MLStatus::ML_STATUS_OK, "The camera is grabbing!" };
            }
            LOG4CPLUS_INFO(LogPlus::getInstance()->logger, sn.c_str() << "Arm On start");

            ArmOn();
            LOG4CPLUS_INFO(LogPlus::getInstance()->logger, sn.c_str() << "SoftwareTrigger start");

            SoftwareTrigger();
            LOG4CPLUS_INFO(LogPlus::getInstance()->logger, sn.c_str() << "Armoff start");

            ArmOff();

            return MLResult{ (int)MLStatus::ML_STATUS_OK, "The camera grabs one Successfully!" };
        }
        catch (const std::exception& e)
        {
            LOG4CPLUS_INFO(LogPlus::getInstance()->logger, e.what());
        }
    }
    else
    {
        LOG4CPLUS_ERROR(LogPlus::getInstance()->logger, sn.c_str() << " is not opened");
        return MLResult{ (int)MLStatus::ML_STATUS_CAMERA_ERROR, serial_number_ + " is not opened" };
    }
}

void ml_vp50mx::StartGrabbingAsync()
{
    Sleep(500);
    if (!m_isGrab && IsOpened())
    {
        MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("TriggerMode"), M_TYPE_STRING, MIL_TEXT("Off"));

        // Sets the maximum amount of time to wait for MdigProcess to complete the current grab ( M_STOP) or all the queued grabs ( M_STOP+ M_WAIT), before generating an error.
        MdigControl(MilDigitizer, M_PROCESS_TIMEOUT, -1);
        /* Grab continuously. */
        // MdigGrabContinuous(MilDigitizer, MilImage);
        /* Start the processing. The processing function is called with every frame grabbed. */
        MdigProcess(MilDigitizer, &MilImage, 1,
            M_START, M_DEFAULT, ProcessingFunction, &UserHookData);

        m_isGrab = true;
        std::string msg = "The camera is grabbing!";
        string sn = "[--Camera: " + serial_number_ + "]--";
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, sn.c_str() << msg.c_str());
        this->NotifyCameraStatus(MLCameraState::kIsGrabbing);
    }
}

void ml_vp50mx::StopGrabbing()
{
    if (m_isGrab && IsOpened())
    {
        /* Stop continuous grab. */
        //MdigHalt(MilDigitizer);
        /* Stop the processing. */
        MdigProcess(MilDigitizer, &MilImage, 1,
            M_STOP, M_DEFAULT, ProcessingFunction, &UserHookData);

        m_isGrab = false;
        std::string msg = "The camera is stopping grab!";
        string sn = "[--Camera: " + serial_number_ + "]--";
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, sn.c_str() << msg.c_str());

        this->NotifyCameraStatus(MLCameraState::kIsOpen);
    }
    Sleep(500);
}

bool ml_vp50mx::IsGrabbing()
{
    return m_isGrab;
}

int ml_vp50mx::GetWidth()
{
    if (IsOpened())
    {
        MIL_INT64 Width = 0;
        MdigInquireFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("Width"), M_TYPE_INT64, &Width);
        return Width;
    } 
}

int ml_vp50mx::GetHeight()
{
    if (IsOpened())
    {
        MIL_INT64 Height = 0;
        MdigInquireFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("Height"), M_TYPE_INT64, &Height);
        return Height;
    }
}

void ml_vp50mx::SetFrameRate(double rate)
{
    if (IsOpened())
    {
        // The following code can be used to control feature values.
        MIL_DOUBLE AcquisitionFrameRate = rate;
        MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("AcquisitionFrameRate"), M_TYPE_DOUBLE, &AcquisitionFrameRate);
        std::string msg = "The frame rate set successfully!!";
        string sn = "[--Camera: " + serial_number_ + "]--";
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, sn.c_str() << msg.c_str());
    }
}

double ml_vp50mx::GetFrameRate()
{
    if (IsOpened())
    {
        // The following code can be used to inquire feature values.
        MIL_DOUBLE AcquisitionFrameRate = 0;
        MdigInquireFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("AcquisitionFrameRate"), M_TYPE_DOUBLE, &AcquisitionFrameRate);
        std::string msg = "Get frame rate successfully, rate: " + to_string(AcquisitionFrameRate);
        string sn = "[--Camera: " + serial_number_ + "]--";
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, sn.c_str() << msg.c_str());
        return AcquisitionFrameRate;
    }
    else
    {
        return 0;
    }
}

void ml_vp50mx::SetFormatType(MLPixelFormat format)
{
    bool wasGrabbing = m_isGrab;
    StopGrabbing();
    string pf = " ";
    if (IsOpened() && !m_isGrab)
    {
        switch (format)
        {
        case MLPixelFormat::kMLMono8:
            // The following code can be used to control feature values.
            MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("PixelFormat"), M_TYPE_STRING, MIL_TEXT("Mono8"));            
            pf = "Mono8";
            m_bitDepth = 8;
            break;
        case MLPixelFormat::kMLMono10:
            MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("PixelFormat"), M_TYPE_STRING, MIL_TEXT("Mono10"));
            pf = "Mono10";
            m_bitDepth = 10;
            break;
        case MLPixelFormat::kMLMono12:
            MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("PixelFormat"), M_TYPE_STRING, MIL_TEXT("Mono12"));
            pf = "Mono12";
            m_bitDepth = 12;
            break;
        default:
            MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("PixelFormat"), M_TYPE_STRING, MIL_TEXT("Mono12"));
            pf = "Mono12";
            break;
        }
        std::string msg = "Set frame format " + pf + " successfully!!";
        string sn = "[--Camera: " + serial_number_ + "]--";
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, sn.c_str() << msg.c_str());

    }
    MbufFree(MilImage);
	if (format == MLPixelFormat::kMLMono8)
	{
		/* Allocate a MIL buffer. */
		MbufAllocColor(MilSystem, BufSizeBand, BufSizeX, BufSizeY, 8 + M_UNSIGNED,
			(MilDigitizer ? M_IMAGE + M_DISP + M_GRAB : M_IMAGE + M_DISP), &MilImage);
	}
	else
	{
		/* Allocate a MIL buffer. */
		MbufAllocColor(MilSystem, BufSizeBand, BufSizeX, BufSizeY, 16 + M_UNSIGNED,
			(MilDigitizer ? M_IMAGE + M_DISP + M_GRAB : M_IMAGE + M_DISP), &MilImage);
	}

    MbufClear(MilImage, 0);

    UserHookData.MilImage = MilImage;

    if (wasGrabbing)
        StartGrabbingAsync();
}

MLPixelFormat ml_vp50mx::GetFormatType()
{
    if (IsOpened())
    {
        // The following code can be used to inquire feature values.
        MIL_STRING PixelFormat;
        MdigInquireFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("PixelFormat"), M_TYPE_STRING, PixelFormat);
        if (PixelFormat == MIL_TEXT("Mono8"))
        { 
            m_pixelFormat = MLPixelFormat::kMLMono8;
        }
        else if (PixelFormat == MIL_TEXT("Mono10"))
        {
            m_pixelFormat = MLPixelFormat::kMLMono10;
        }
        else if (PixelFormat == MIL_TEXT("Mono12"))
        {
            m_pixelFormat = MLPixelFormat::kMLMono12;
        }
        else
        {
            std::string msg = "Unknown PixelFormat!!";
            string sn = "[--Camera: " + serial_number_ + "]--";
            LOG4CPLUS_INFO(LogPlus::getInstance()->logger, sn.c_str() << msg.c_str()<< PixelFormat);
            throw runtime_error("Unknown PixelFormat!!");
        }
        std::string msg = "Get frame format successfully!!";
        string sn = "[--Camera: " + serial_number_ + "]--";
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, sn.c_str() << msg.c_str() << " PixelFormat = " << PixelFormat);

        return m_pixelFormat;
    }
    else
    {
        std::string msg = "Camera Closed!!";
        string sn = "[--Camera: " + serial_number_ + "]--";
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, sn.c_str() << msg.c_str());
    }
}

void ml_vp50mx::SetMLExposureAuto()
{
    //if (IsOpened())
    //{
    //    AEStatus_ = AutoExposureStatus::AE_NONE;

    //    string currentPath = "Camera ID: " + serial_number_ + ", current AE configurtion path: " + autoExposureConfigPath;
    //    LOG4CPLUS_INFO(LogPlus::getInstance()->logger, currentPath.c_str());
    //    AutoExposureAlgorithm autoExposure(cameraName_, autoExposureConfigPath, serial_number_, init_Time_, isSetInitTime);

    //    /*if (autoExposure.IsUseROI())
    //        roiForAE = autoExposure.GetROIS();
    //    else
    //        roiForAE.clear();*/

    //        //AEStatus AEStatus = autoExposure.AutoExposureAlgorithm1<ml_vp50mx>(this);
    //    AEStatus AEStatus;
    //    try
    //    {
    //        AEStatus = autoExposure.capture_ae<ml_vp50mx>(this);
    //        if (AEStatus == AEStatus::AE_max_iterations_reached)
    //        {
    //            AEStatus_ = AutoExposureStatus::AE_max_iterations_reached_;
    //            m_AEendStatus = "Max iterations reached";
    //            return ;
    //        }
    //        else if (AEStatus == AEStatus::AE_normal)
    //        {
    //            AEStatus_ = AutoExposureStatus::AE_normal_;
    //            m_AEendStatus = "normal";
    //            return ;
    //        }
    //        else if (AEStatus == AEStatus::AE_too_bright)
    //        {
    //            AEStatus_ = AutoExposureStatus::AE_too_bright_;
    //            m_AEendStatus = "Too bright";
    //            return ;
    //        }
    //        else if (AEStatus == AEStatus::AE_too_dark)
    //        {
    //            AEStatus_ = AutoExposureStatus::AE_too_dark_;
    //            m_AEendStatus = "Too dark";
    //            return ;
    //        }
    //        else if (AEStatus == AEStatus::AE_single_acquisition_error)
    //        {
    //            AEStatus_ = AutoExposureStatus::AE_single_acquisition_error;
    //            m_AEendStatus = "Single acquisition error";
    //            return ;
    //        }
    //        else if (AEStatus == AEStatus::AE_time_out)
    //        {
    //            AEStatus_ = AutoExposureStatus::AE_time_out;
    //            m_AEendStatus = "Time out";
    //            return ;
    //        }
    //    }
    //    catch (const std::exception& e)
    //    {
    //        AEStatus_ = AutoExposureStatus::AE_NONE;
    //        m_AEendStatus = "AE_NONE";
    //        return AutoExposureStatus::AE_NONE;
    //    }
    //}
}

void ml_vp50mx::SetExposureTime(double time)
{
    if (IsOpened())
    {
        // The following code can be used to control feature values.
        exposureTimeBefore = currentExposureTime;
        //GetNewImage().copyTo(imageBefore);

        //time < m_nMinExp ? time = m_nMinExp : time;
        //time > m_nMaxExp ? time = m_nMaxExp : time;

        //currentExposureTime = time;

        MIL_DOUBLE ExposureTime = time;
        MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("ExposureTime"), M_TYPE_DOUBLE, &ExposureTime);
        std::string msg = "Set exposure time successfully!! " + to_string(time) + " microseconds";
        string sn = "[--Camera: " + serial_number_ + "]--";
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, sn.c_str() << msg.c_str());

        double exposureTime = GetExposureTime();
        std::string msg1 = "Get current expsoure time:" + to_string(exposureTime) + " microseconds";
        //LOG4CPLUS_INFO(LogPlus::getInstance()->logger, sn.c_str() << msg1.c_str());
    }
}

double ml_vp50mx::GetExposureTime()
{
    if (IsOpened())
    {
        // The following code can be used to inquire feature values.
        MIL_DOUBLE ExposureTime = 0;
        MdigInquireFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("ExposureTime"), M_TYPE_DOUBLE, &ExposureTime);
        string sn = "[--Camera: " + serial_number_ + "]--";
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, sn.c_str() << "Get exposure time : " << ExposureTime << " microseconds");
        return ExposureTime;
    }
    else
    {
        return 0;
    }
}

void ml_vp50mx::SetExposureMode(MLExposureMode mode)
{
    switch (mode)
    {
    case MLExposureMode::kContinuous:
        m_exposureTimeMode = "Continuous";
        break;
    case MLExposureMode::kOff:
        m_exposureTimeMode = "Off";
        break;
    case MLExposureMode::kOnce:
        m_exposureTimeMode = "Once";
        break;
    case MLExposureMode::kMLAuto:
        m_exposureTimeMode = "mlAuto";
        break;
    case MLExposureMode::kMLAuto2:
        m_exposureTimeMode = "mlAuto2";
        break;
    default:
        break;
    }
}

MLExposureMode ml_vp50mx::GetExposureMode()
{
    return MLExposureMode();
}

void ml_vp50mx::GetExposureTimeRange(double& min, double& max)
{
    min = m_nMinExp;
    max = m_nMaxExp;
}

void ml_vp50mx::SetAdjustExposureTime(double time)
{
    if (IsOpened())
    {
        double exposure_time = AdjustTime(time, rate_);
        //double exposure_time = time;
        SetExposureTime(exposure_time);

        std::string message = "AdjustTime :" + std::to_string(exposure_time);
        string sn = "[--Camera: " + serial_number_ + "]--";
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, sn.c_str() << message.c_str());
    }
}

void ml_vp50mx::SetGain(double value)
{

}

double ml_vp50mx::GetGain()
{
    return 0.0;
}

void ml_vp50mx::GetGainRange(double& min, double& max)
{
}

MLCameraState ml_vp50mx::GetState()
{
    return camera_state_;
}

Binning ml_vp50mx::getBinning()
{
    if (IsOpened())
    {
        MIL_STRING binning_str;
        Binning binning;
        MdigInquireFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("BinningVertical"), M_TYPE_STRING, binning_str);
        std::string peltierOperationMode;
        if (binning_str == MIL_TEXT("X1"))
        {
            binning = Binning::ONE_BY_ONE;
        }
        else if (binning_str == MIL_TEXT("X2"))
        {
            binning = Binning::TWO_BY_TWO;
        }
        else if (binning_str == MIL_TEXT("X4"))
        {
            binning = Binning::FOUR_BY_FOUR;
        }
        else
        {
            std::string msg = "Unknown Binning";
            string sn = "[--Camera: " + serial_number_ + "]--";
            LOG4CPLUS_INFO(LogPlus::getInstance()->logger, sn.c_str() << msg.c_str());
            throw runtime_error("Unknown Binning");
        }
        std::string msg = "Get binning successfully!!";
        string sn = "[--Camera: " + serial_number_ + "]--";
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, sn.c_str() << msg.c_str() << " binning = " << binning_str);
        return binning;
    }
}

BinningMode ml_vp50mx::GetBinningMode()
{
    if (IsOpened())
    {
        MIL_STRING binning_mode_str;
        BinningMode binning_mode;
        MdigInquireFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("BinningVerticalMode"), M_TYPE_STRING, binning_mode_str);
        std::string peltierOperationMode;
        if (binning_mode_str == MIL_TEXT("Average"))
        {
            binning_mode = BinningMode::AVERAGE;
        }
        else if (binning_mode_str == MIL_TEXT("Sum"))
        {
            binning_mode = BinningMode::SUM;
        }
        else
        {
            std::string msg = "Unknown BinningMode";
            string sn = "[--Camera: " + serial_number_ + "]--";
            LOG4CPLUS_INFO(LogPlus::getInstance()->logger, sn.c_str() << msg.c_str());
            throw runtime_error("Unknown BinningMode");
        }
        std::string msg = "Get binning mode successfully!!";
        string sn = "[--Camera: " + serial_number_ + "]--";
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, sn.c_str() << msg.c_str() << " binning mode = " << binning_mode_str);
        return binning_mode;
    }
}

void ml_vp50mx::SetBinning(Binning binning)
{
    if (IsOpened())
    {
        bool wasGrabbing = m_isGrab;
        StopGrabbing();

        string binning_str = " ";
        switch (binning)
        {
        case Binning::ONE_BY_ONE:
            MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("BinningVertical"), M_TYPE_STRING, MIL_TEXT("X1"));
			MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("BinningHorizontal"), M_TYPE_STRING, MIL_TEXT("X1"));
			binning_str = "X1";
            break;
        case Binning::TWO_BY_TWO:
            MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("BinningVertical"), M_TYPE_STRING, MIL_TEXT("X2"));
            MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("BinningHorizontal"), M_TYPE_STRING, MIL_TEXT("X2")); 
            binning_str = "X2";
            break;
        case Binning::FOUR_BY_FOUR:
            MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("BinningVertical"), M_TYPE_STRING, MIL_TEXT("X4"));
            MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("BinningHorizontal"), M_TYPE_STRING, MIL_TEXT("X4")); 
            binning_str = "X4";
            break;
        default:
            MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("BinningVertical"), M_TYPE_STRING, MIL_TEXT("X1"));
            MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("BinningHorizontal"), M_TYPE_STRING, MIL_TEXT("X1")); 
            binning_str = "X1";
            break;
        }
        std::string msg = "Set binning " + binning_str + " successfully!!";
        string sn = "[--Camera: " + serial_number_ + "]--";
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, sn.c_str() << msg.c_str());
        MbufFree(MilImage);
        if (UserImagePtr)
            SAFE_DELETE_ARRAY(UserImagePtr);

        MdigInquire(MilDigitizer, M_SIZE_X, &BufSizeX);
        MdigInquire(MilDigitizer, M_SIZE_Y, &BufSizeY);
        MdigInquire(MilDigitizer, M_SIZE_BAND, &BufSizeBand);
		if (GetFormatType() == MLPixelFormat::kMLMono8)
		{
			/* Allocate a MIL buffer. */
			MbufAllocColor(MilSystem, BufSizeBand, BufSizeX, BufSizeY, 8 + M_UNSIGNED,
				(MilDigitizer ? M_IMAGE + M_DISP + M_GRAB : M_IMAGE + M_DISP), &MilImage);
		}
		else
		{
			/* Allocate a MIL buffer. */
			MbufAllocColor(MilSystem, BufSizeBand, BufSizeX, BufSizeY, 16 + M_UNSIGNED,
				(MilDigitizer ? M_IMAGE + M_DISP + M_GRAB : M_IMAGE + M_DISP), &MilImage);
		}

		MbufClear(MilImage, 0);

        UserHookData.MilImage = MilImage;
        UserImagePtr = new MIL_UINT16[GetWidth() * GetHeight()];
        std::string msg2 = "resize binning buffer";
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, sn.c_str() << msg2.c_str());

        if (wasGrabbing)
            StartGrabbingAsync();
    }
}

void ml_vp50mx::SetBinningMode(BinningMode binningMode)
{
    if (IsOpened())
    {
        bool wasGrabbing = m_isGrab;
        StopGrabbing();

        string binningmode_str = " ";
        switch (binningMode)
        {
        case BinningMode::AVERAGE:
            MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("BinningVerticalMode"), M_TYPE_STRING, MIL_TEXT("Average"));
            binningmode_str = "Average";
            break;
        case BinningMode::SUM:
            MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("BinningVerticalMode"), M_TYPE_STRING, MIL_TEXT("Sum"));
            binningmode_str = "Sum";
            break;
        default:
            MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("BinningVerticalMode"), M_TYPE_STRING, MIL_TEXT("Average"));
            binningmode_str = "Average";
            break;
        }
        std::string msg = "Set binning mode " + binningmode_str + " successfully!!";
        string sn = "[--Camera: " + serial_number_ + "]--";
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, sn.c_str() << msg.c_str());
        if (wasGrabbing)
            StartGrabbingAsync();
    }
}

void ml_vp50mx::SetBinning(Binning binning, BinningMode binningMode)
{
    if (IsOpened())
    {
        bool wasGrabbing = m_isGrab;
        StopGrabbing();
        //mode
        string binningmode_str = " ";
        switch (binningMode)
        {
        case BinningMode::AVERAGE:
            MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("BinningVerticalMode"), M_TYPE_STRING, MIL_TEXT("Average"));
            binningmode_str = "Average";
            break;
        case BinningMode::SUM:
            MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("BinningVerticalMode"), M_TYPE_STRING, MIL_TEXT("Sum"));
            binningmode_str = "Sum";
            break;
        default:
            MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("BinningVerticalMode"), M_TYPE_STRING, MIL_TEXT("Average"));
            binningmode_str = "Average";
            break;
        }
        std::string msg = "Set binning mode " + binningmode_str + " successfully!!";
        string sn = "[--Camera: " + serial_number_ + "]--";
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, sn.c_str() << msg.c_str());
        //binning;
        string binning_str = " ";
        switch (binning)
        {
        case Binning::ONE_BY_ONE:
            MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("BinningVertical"), M_TYPE_STRING, MIL_TEXT("X1"));
            MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("BinningHorizontal"), M_TYPE_STRING, MIL_TEXT("X1"));
            binning_str = "X1";
            break;
        case Binning::TWO_BY_TWO:
            MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("BinningVertical"), M_TYPE_STRING, MIL_TEXT("X2"));
            MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("BinningHorizontal"), M_TYPE_STRING, MIL_TEXT("X2"));
            binning_str = "X2";
            break;
        case Binning::FOUR_BY_FOUR:
            MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("BinningVertical"), M_TYPE_STRING, MIL_TEXT("X4"));
            MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("BinningHorizontal"), M_TYPE_STRING, MIL_TEXT("X4"));
            binning_str = "X4";
            break;
        default:
            MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("BinningVertical"), M_TYPE_STRING, MIL_TEXT("X1"));
            MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("BinningHorizontal"), M_TYPE_STRING, MIL_TEXT("X1"));
            binning_str = "X1";
            break;
        }
        msg = "Set binning " + binning_str + " successfully!!";
        sn = "[--Camera: " + serial_number_ + "]--";
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, sn.c_str() << msg.c_str());
        MbufFree(MilImage);
        if (UserImagePtr)
            SAFE_DELETE_ARRAY(UserImagePtr);

        MdigInquire(MilDigitizer, M_SIZE_X, &BufSizeX);
        MdigInquire(MilDigitizer, M_SIZE_Y, &BufSizeY);
        MdigInquire(MilDigitizer, M_SIZE_BAND, &BufSizeBand);
        if (GetFormatType() == MLPixelFormat::kMLMono8)
        {
            /* Allocate a MIL buffer. */
            MbufAllocColor(MilSystem, BufSizeBand, BufSizeX, BufSizeY, 8 + M_UNSIGNED,
                (MilDigitizer ? M_IMAGE + M_DISP + M_GRAB : M_IMAGE + M_DISP), &MilImage);
        }
        else
        {
            /* Allocate a MIL buffer. */
            MbufAllocColor(MilSystem, BufSizeBand, BufSizeX, BufSizeY, 16 + M_UNSIGNED,
                (MilDigitizer ? M_IMAGE + M_DISP + M_GRAB : M_IMAGE + M_DISP), &MilImage);
        }

        MbufClear(MilImage, 0);

        UserHookData.MilImage = MilImage;
        UserImagePtr = new MIL_UINT16[GetWidth() * GetHeight()];
        std::string msg2 = "resize binning buffer";
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, sn.c_str() << msg2.c_str());


        if (wasGrabbing)
            StartGrabbingAsync();
    }

}

void ml_vp50mx::FlipX(bool flip)
{
}

void ml_vp50mx::FlipY(bool flip)
{
}

void ml_vp50mx::Rotate(bool rotate)
{
}

void ml_vp50mx::Subscribe(MLCameraEvent event, MLCameraCallback* callback)
{
    if (camera_callback_ == nullptr || callback == camera_callback_)
    {
        if (camera_callback_ == nullptr)
        {
            camera_callback_ = callback;
        }
        std::string camera_event;
        switch (event)
        {
        case CORE::MLCameraEvent::kFrameReceived:
            StartGrabbingAsync();
            grabbing_callback_ = true;
            camera_event = "kFrameReceived";
            break;
        case CORE::MLCameraEvent::kGrayLevel:
            StartCalcGrayLevel();
            graylevel_callback_ = true;
            camera_event = "GrayLevelReceived";
            break;
        default:
            break;
        }
        /*std::string message = "Camera subscribe the camera callback: " + camera_event;
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, message.c_str());*/
    }
}

void ml_vp50mx::Unsubscribe(MLCameraEvent event, MLCameraCallback* callback)
{
    if (camera_callback_ != nullptr)
    {
        std::string camera_event;
        switch (event)
        {
        case CORE::MLCameraEvent::kFrameReceived:
            StopGrabbing();
            grabbing_callback_ = false;
            camera_event = "kFrameReceived";
            break;
        case CORE::MLCameraEvent::kGrayLevel:
            StopCalcGrayLevel();
            graylevel_callback_ = false;
            camera_event = "GrayLevelReceived";
            break;
        default:
            break;
        }

        if (grabbing_callback_ == false && graylevel_callback_ == false)
        {
            camera_callback_ = NULL;
        }

        /* std::string message = "Camera ID: " + GetCameraInfo().id + ", unsubscribe the camera callback: " + camera_event;
         LOG4CPLUS_INFO(LogPlus::getInstance()->logger, message.c_str());*/
    }
}

cv::Mat ml_vp50mx::GetNewImage()
{
    if (IsOpened())
    {
        string cameraName = "[--GetImage:" + serial_number_ + "]--";
        if (m_isFristIntoGetImage && GetFrameRate() > 1)
        {
            Sleep(200);
        }
        else if (m_isFristIntoGetImage && GetFrameRate() <= 1)
        {
            int time = 1 / GetFrameRate();
            Sleep((time + 1) * 1000);
            Sleep(200);
        }
        if (m_isFristIntoGetImage)
        {
            GetNewImage().copyTo(imageBefore);
            exposureTimeBefore = GetExposureTime();
            currentExposureTime = exposureTimeBefore;
            m_isFristIntoGetImage = false;
        }

        int times = 0;

        string msg;

        int exposureTimeNew = currentExposureTime;
        msg = cameraName + "Old exposure time: " + to_string(exposureTimeBefore) +
            ", new exposure time: " + to_string(exposureTimeNew);
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());

        if ((exposureTimeBefore != exposureTimeNew) && (abs(exposureTimeBefore - exposureTimeNew) > 100))
        {
            msg = cameraName + "Start to compare the mean difference between new "
                "and old images under different exposure times";
            LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());
            while (true)
            {
                Sleep(250);

                cv::Mat imageNew;
                GetNewImage().copyTo(imageNew);
                double meanValue = fabs(cv::mean(imageBefore)[0] - cv::mean(imageNew)[0]);
                msg = cameraName + "The difference between the mean value of the old image and the new image is " +
                    to_string(meanValue);
                LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());

                if (meanValue > 2)
                {
                    int times_ = 0;
                    int times__ = 0;
                    double frameRate = GetFrameRate();
                    msg = cameraName + "The mean difference is normal, and it needs to be verified according to the frame "
                        "rate. The current frame rate is " +
                        to_string(frameRate);
                    LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());
                    while (true)
                    {
                        cv::Mat imgTmp;
                        GetNewImage().copyTo(imgTmp);
                        double meanValue_ = fabs(cv::mean(imageNew)[0] - cv::mean(imgTmp)[0]);
                        msg = cameraName + "The mean difference between two images at the same exposure time is " +
                            to_string(meanValue_);
                        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());

                        if (meanValue_ < 2 && frameRate > 1)
                        {
                            msg = cameraName + "The frame rate and mean value are normal, and the latest image is returned";
                            LOG4CPLUS_INFO(
                                LogPlus::getInstance()->logger,
                                msg.c_str());
                            GetNewImage().copyTo(imageBefore);
                            exposureTimeBefore = exposureTimeNew;
                            return imageBefore;
                        }
                        else if (meanValue_ < 2 && frameRate < 1)
                        {
                            msg = cameraName + "The mean value is normal, but the frame rate is too small. It needs to "
                                "obtain the image again for verification. The current iteration number is " +
                                to_string(times__ + 1);
                            LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());
                            if (times__ == 3)
                            {
                                msg = cameraName + "The mean difference "
                                    "between two images at the same exposure time is still very small, it is "
                                    "normal, and the latest image is returned";
                                LOG4CPLUS_INFO(LogPlus::getInstance()->logger,
                                    msg.c_str());
                                GetNewImage().copyTo(imageBefore);
                                exposureTimeBefore = exposureTimeNew;
                                return imageBefore;
                            }
                            GetNewImage().copyTo(imageNew);
                            times__++;
                            Sleep(500);
                        }
                        else
                        {
                            msg = cameraName + "The mean difference between two images at the same exposure time is very "
                                "large, need to obtain the image again for comparison. The current number of iterations "
                                "is " +
                                to_string(times_ + 1);
                            LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());
                            if (times_ == 5)
                            {
                                msg = cameraName + "The maximum number of iterations has been reached. The "
                                    "current returned image may not be a new image";
                                LOG4CPLUS_INFO(LogPlus::getInstance()->logger,
                                    msg.c_str());
                                GetNewImage().copyTo(imageBefore);
                                exposureTimeBefore = exposureTimeNew;
                                return imageBefore;
                            }
                            GetNewImage().copyTo(imageNew);
                            times_++;
                            Sleep(700);
                        }
                    }
                }
                if (times == (exposureTimeNew / 250000 + 2))
                {
                    msg = cameraName + "The maximum number of iterations has been reached, but the mean difference between the new and old images is still small, and the returned image may still be the old image";
                    LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());
                    exposureTimeBefore = exposureTimeNew;
                    GetNewImage().copyTo(imageBefore);
                    return imageBefore;
                }
                times++;
                msg = cameraName + "The mean difference between the new and old images under different exposure times is "
                    "too small, and the new image may not be obtained, and will be acquired again. The current iteration "
                    "number is " +
                    to_string(times);
                LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());
            }
        }
        else
        {
            msg = cameraName + "The old exposure time is the same as the new exposure time, and the image is returned directly";
            LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());

            GetNewImage().copyTo(imageBefore);
            Sleep(100);
            GetNewImage().copyTo(imageBefore);
            Sleep(100);
            GetNewImage().copyTo(imageBefore);
            if (GetFrameRate() < 1)
            {
                Sleep(300);
                GetNewImage().copyTo(imageBefore);
            }
            if (GetFrameRate() < 0.5)
            {
                Sleep(300);
                GetNewImage().copyTo(imageBefore);
            }
            return imageBefore;
        }
    }
}

cv::Mat ml_vp50mx::GetImage()
{
    if (IsOpened())
    {
        try
        {
            LOG4CPLUS_INFO(LogPlus::getInstance()->logger, "GetImage");

            if (!UserImagePtr)
            {
                LOG4CPLUS_INFO(LogPlus::getInstance()->logger, "Image buffer is null");
                throw runtime_error("Image buffer is null");
            }

            MLPixelFormat bit_depth = GetFormatType();
            cv::Mat imageNew;
            if (bit_depth == MLPixelFormat::kMLMono8)
            {
                m_nMutex.lock();
                imageNew = cv::Mat(GetHeight(), GetWidth(), CV_8UC1, UserImagePtr).clone();
                m_nMutex.unlock();
            }
            else if (bit_depth == MLPixelFormat::kMLMono10 || bit_depth == MLPixelFormat::kMLMono12)
            {
                m_nMutex.lock();
                imageNew = cv::Mat(GetHeight(), GetWidth(), CV_16UC1, UserImagePtr).clone();
                m_nMutex.unlock();
            }

            return imageNew;
        }
        catch (const std::exception& e)
        {
            LOG4CPLUS_INFO(LogPlus::getInstance()->logger, "exception msg:" << e.what());
        }
    }
}

void ml_vp50mx::NotifyCameraStatus(MLCameraState state)
{
    if (camera_callback_ != nullptr && camera_callback_)
    {
        camera_callback_->NotifyCameraStateChanged(camera_state_, state);

        camera_state_ = state;
    }
}

void ml_vp50mx::StartCalcGrayLevel()
{
    m_isGrayRun = true;
    if (m_isGrayRun)
    {
        m_GrayLevelThread = true;
        //The thread will repeatedly call getImage to update the image, 
        //which will cause the image obtained by getImage to be overwritten and need to be modified
        m_Thread_ = std::thread(&ml_vp50mx::GrayLevelThread, this);
    }
}

void ml_vp50mx::StopCalcGrayLevel()
{
    if (m_GrayLevelThread)
    {
        m_isGrayRun = false;
        m_GrayLevelThread = false;
        //m_Thread_.join();
        m_Thread_.join();
    }
}

int ml_vp50mx::AdjustTime(int init_time, int rate)
{
    if (rate > 0) {
        int times = init_time * rate / 1e6;
        if (times < 1)
        {
            init_time = ConversionCycleMultiple(init_time);
            return init_time;
        }
        int time_ad = double(times) / rate * 1e6;
        time_ad = ConversionCycleMultiple(time_ad);
        return round(time_ad);
    }
    else
        return init_time;
}


bool ml_vp50mx::InitializeConfigVariables(const char* configfile)
{
    max_num_ = 2000;

    std::string path = configfile;
    autoExposureConfigPath = path + "\\AutoExposureConfig.json";

    using json = nlohmann::ordered_json;
    json jsonConfig;
    string sn = "[--Camera: " + serial_number_ + "]--";
    std::ifstream ifs(autoExposureConfigPath.c_str(), std::fstream::in);
    if (ifs.fail())
    {
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, sn.c_str() << " Unable to open AEConfig File.");
        throw std::runtime_error(sn + " Unable to open AEConfig File.");
    }
    ifs >> jsonConfig;
    ifs.close();

    for (auto& it : jsonConfig["AutoExposure"].items())
    {
        if (it.value()["SerialNumber"].get<string>() == serial_number_)
        {
            rate_ = it.value()["InitialParameters"]["rate"].get<int>();
            m_nMinExp = it.value()["JudgmentMechanism"]["min_time"].get<int>();
            m_nMaxExp = it.value()["JudgmentMechanism"]["max_time"].get<int>();

        }
    }

    return true;
}

void ml_vp50mx::GrayLevelThread()
{
    while (m_GrayLevelThread)
    {
        if (m_isGrayRun)
        {
            int sum = 0;
            cv::Mat img;
           // GetNewImage().copyTo(img);
            GetImage().copyTo(img);
            if (img.empty())
            {
                continue;
            }

            if (img.type() == CV_8UC1)
            {
                int gray = Helper::GetGrayValue2(img, max_num_);

                this->NotifyImageGrayLevel(gray);
            }
            else if (img.type() == CV_16UC1)
            {

                //int gray = Helper::GreaterNValue(img, max_num_);
                int gray = Helper::GetGrayValue2(img, max_num_);

                this->NotifyImageGrayLevel(gray);

            }
        }

        Sleep(400);

    }
}

int ml_vp50mx::ConversionCycleMultiple(int us)
{
    if (us <= (int)(1000000 / rate_))
    {
        return (int)(1000000 / rate_);
    }

    int remainder = us % (int)(1000000 / rate_);
    if (remainder != 0)
    {
        us = us - remainder;
    }
    return us;
}

void ml_vp50mx::NotifyImageCallBack()
{
    MbufGet2d(MilImage, 0, 0, GetWidth(), GetHeight(), UserImagePtr);

   // cv::Mat img = GetNewImage();
    cv::Mat img = GetImage();

    MLFrame frame(img, m_pixelFormat);

    if (camera_callback_ != nullptr)
    {
        camera_callback_->NotifyCameraFrameReceived(&frame, UserImagePtr);
    }
}
void ml_vp50mx::ArmOn()
{
    string sn = "[--Camera: " + serial_number_ + "]--";
    LOG4CPLUS_INFO(LogPlus::getInstance()->logger, sn.c_str() << "Arm On");
    // Sets the maximum time to wait for a frame before generating an error.
    MdigControl(MilDigitizer, M_GRAB_TIMEOUT, int(m_nMaxExp / 1000 + 1000));

    //-----------software trigger---------------
    MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("TriggerSelector"), M_TYPE_STRING, MIL_TEXT("ExposureStart"));
    MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("TriggerMode"), M_TYPE_STRING, MIL_TEXT("On"));
    MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("TriggerSource"), M_TYPE_STRING, MIL_TEXT("Software"));
    MdigControlFeature(MilDigitizer, M_FEATURE_EXECUTE, MIL_TEXT("AcquisitionStart"), M_DEFAULT, M_NULL);

    MdigProcess(MilDigitizer, &MilImage, 1,
        M_START, M_DEFAULT, ProcessingFunction, &UserHookData);
}
void ml_vp50mx::ArmOff()
{
    string sn = "[--Camera: " + serial_number_ + "]--";
    LOG4CPLUS_INFO(LogPlus::getInstance()->logger, sn.c_str() << "Arm Off");

    MdigProcess(MilDigitizer, &MilImage, 1,
        M_STOP, M_DEFAULT, ProcessingFunction, &UserHookData);

    MdigControlFeature(MilDigitizer, M_FEATURE_EXECUTE, MIL_TEXT("AcquisitionStop"), M_DEFAULT, M_NULL);
    MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("TriggerMode"), M_TYPE_STRING, MIL_TEXT("Off"));
}

void ml_vp50mx::HardwareArmOn()
{
    string sn = "[--Camera: " + serial_number_ + "]--";
    LOG4CPLUS_INFO(LogPlus::getInstance()->logger, sn.c_str() << "Hardware Arm On");
    // Sets the maximum time to wait for a frame before generating an error.
    MdigControl(MilDigitizer, M_GRAB_TIMEOUT, int(m_nMaxExp / 1000 + 1000));

    //-----------hardware trigger---------------
    MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("TriggerSelector"), M_TYPE_STRING, MIL_TEXT("ExposureStart"));
    MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("TriggerMode"), M_TYPE_STRING, MIL_TEXT("On"));
    MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("TriggerSource"), M_TYPE_STRING, MIL_TEXT("Line1"));//modify line

    MdigControlFeature(MilDigitizer, M_FEATURE_EXECUTE, MIL_TEXT("AcquisitionStart"), M_DEFAULT, M_NULL);

    MdigProcess(MilDigitizer, &MilImage, 1,
        M_START, M_DEFAULT, ProcessingFunction, &UserHookData);
}



void ml_vp50mx::SoftwareTrigger()
{
    string sn = "[--Camera: " + serial_number_ + "]--";

    LOG4CPLUS_INFO(LogPlus::getInstance()->logger, sn.c_str() << "Software Trigger");

    captureEvent.Reset();

    LOG4CPLUS_DEBUG(LogPlus::getInstance()->logger, "start TriggerSoftware");
    MdigControlFeature(MilDigitizer, M_FEATURE_EXECUTE, MIL_TEXT("TriggerSoftware"), M_DEFAULT, M_NULL);

    captureEvent.WaitOne();

    if (GetFormatType() == MLPixelFormat::kMLRGB24)
    {
        std::string msg = "Unknown MLPixelFormat MLRGB24!";
        LOG4CPLUS_ERROR(LogPlus::getInstance()->logger, sn.c_str() << msg.c_str());
    }
    else
    {
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, sn.c_str() << "start get data from buffers");
        MbufGet2d(UserHookData.MilImage, 0, 0, GetWidth(), GetHeight(), UserImagePtr);
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, sn.c_str() << "end get data from buffers");
    }

    this->PrintMatroxError();


    std::string msg = "The camera grabs one Successfully!";
    LOG4CPLUS_INFO(LogPlus::getInstance()->logger, sn.c_str() << msg.c_str());
}

void ml_vp50mx::NotifyImageGrayLevel(int value)
{
    if (camera_callback_ != NULL && graylevel_callback_)
    {
        //camera_callback_->NotifyCameraGrayLevel(value * 255 / 4095);
        camera_callback_->NotifyCameraGrayLevel(value);
        int gray = value * 255 / pow(2, m_bitDepth);
        //camera_callback_->NotifyCameraGrayLevel(value*255/pow(2,m_bitDepth));
        //int y = 0;
    }
}

void ml_vp50mx::SetTemperature(int temperature)
{
    if (IsOpened())
    {
        MIL_INT64 TargetTemperature = temperature;
        MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("TargetTemperature"), M_TYPE_INT64, &TargetTemperature);
        std::string msg = "Set Temperature ";
        string sn = "[--Camera: " + serial_number_ + "]--";
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, sn.c_str() << msg.c_str() << temperature << " successfully!!");
    }
}

int ml_vp50mx::GetTemperature()
{
    if (IsOpened())
    {
        MIL_INT64 TargetTemperature = 0;
        MdigInquireFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("TargetTemperature"), M_TYPE_INT64, &TargetTemperature);
        std::string msg = "Get Temperature, temperature = ";
        string sn = "[--Camera: " + serial_number_ + "]--";
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, sn.c_str() << msg.c_str() << TargetTemperature);
        return TargetTemperature;
    }
}

void ml_vp50mx::SetTriggerSource(CORE:: TriggerSource source)
{
    string sn = "[--Camera: " + serial_number_ + "]--";
    if (IsOpened())
    {
        if (source == TriggerSource::Software)
        {
            MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("TriggerMode"), M_TYPE_STRING, MIL_TEXT("On"));
            MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("TriggerSource"), M_TYPE_STRING, MIL_TEXT("Software"));
            LOG4CPLUS_INFO(LogPlus::getInstance()->logger, sn.c_str() << "Set Trigger Source on Software");
            return;
        }
        else if (source == TriggerSource::Hardware)
        {
            MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("TriggerMode"), M_TYPE_STRING, MIL_TEXT("On"));
            MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("TriggerSource"), M_TYPE_STRING, MIL_TEXT("CXPin"));
            LOG4CPLUS_INFO(LogPlus::getInstance()->logger, sn.c_str() << "Set Trigger Source on Hardware");
            return;
        }
        else if (source == TriggerSource::None)
        {
            MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("TriggerMode"), M_TYPE_STRING, MIL_TEXT("Off"));
            return;
        }
    }
    LOG4CPLUS_INFO(LogPlus::getInstance()->logger, sn.c_str() << "Set Trigger Source Failed");
}

void ml_vp50mx::SetPeltierOperationMode(std::string peltierOperationMode)
{
    if (IsOpened())
    {
        if (peltierOperationMode == "On")
        {
            MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("PeltierOperationMode"), M_TYPE_STRING, MIL_TEXT("On"));
        }
        else
            MdigControlFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("PeltierOperationMode"), M_TYPE_STRING, MIL_TEXT("Off"));
    }
}

std::string ml_vp50mx::GetPeltierOperationMode()
{
    if (IsOpened())
    {
        MIL_STRING PeltierOperationMode;
        MdigInquireFeature(MilDigitizer, M_FEATURE_VALUE, MIL_TEXT("PeltierOperationMode"), M_TYPE_STRING, PeltierOperationMode);
        std::string peltierOperationMode;
        if (PeltierOperationMode == MIL_TEXT("On"))
            peltierOperationMode = "On";
        else
            peltierOperationMode = "Off";
        return peltierOperationMode;
    }
}

void ml_vp50mx::PrintMatroxError()
{
    if (MappGetError(M_DEFAULT, M_CURRENT, M_NULL) != 0)
    {
        MIL_STRING ErrorMessageFunction;
        MIL_STRING ErrorMessage;
        MIL_STRING ErrorSubMessage1;
        MIL_STRING_STREAM ErrorStream;

        MappGetError(M_DEFAULT, M_MESSAGE + M_CURRENT_FCT, ErrorMessageFunction);
        MappGetError(M_DEFAULT, M_MESSAGE + M_CURRENT, ErrorMessage);
        MappGetError(M_DEFAULT, M_MESSAGE + M_CURRENT_SUB_1, ErrorSubMessage1);

        ErrorStream << MIL_TEXT("Error  Function: ") << ErrorMessageFunction <<MIL_TEXT(" msg: ") << ErrorMessage << MIL_TEXT(" ") << ErrorSubMessage1 << MIL_TEXT("\n");
        
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, ErrorStream.str().c_str());
    }
}

MIL_INT MFTYPE ProcessingFunction(MIL_INT HookType, MIL_ID HookId, void* HookDataPtr)
{
    HookDataStruct* UserHookDataPtr = (HookDataStruct*)HookDataPtr;

    ml_vp50mx* camera = (ml_vp50mx*)UserHookDataPtr->camera;

    MIL_ID ModifiedBufferId;

    ///* Retrieve the MIL_ID of the grabbed buffer. */
    MdigGetHookInfo(HookId, M_MODIFIED_BUFFER + M_BUFFER_ID, &ModifiedBufferId);

    /* Increment the frame counter. */
    UserHookDataPtr->ProcessedImageCount++;

    LOG4CPLUS_DEBUG(LogPlus::getInstance()->logger, "ProcessingFunction image count : " << UserHookDataPtr->ProcessedImageCount);

    /* Execute the processing and update the display. */
    MbufCopy(ModifiedBufferId, UserHookDataPtr->MilImage);
    //MdigGrab(UserHookDataPtr->MilDigitizer, UserHookDataPtr->MilImage);

    camera->NotifyImageCallBack();
    camera->m_isImageUpdate = true;

    camera->captureEvent.Set();

    return 0;
}

void AutoExposureCallBack(int exposure_time, double gray, int current_step, int total_step, string msg)
{
    string msg1 = "current exposure:" + to_string(exposure_time) + ", gray value:" + to_string(gray) + ", current step:" + to_string(current_step) + ", total step:" + to_string(total_step) + ". " + msg;
    LOG4CPLUS_INFO(LogPlus::getInstance()->logger, "Camera:" << msg1.c_str());
}