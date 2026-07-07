#include "MLSpecbosLogic.h"
#include <iostream>
#include "MLSpecbosConfigManage.h"
#include "loggingwrapper.h"
#include "jeti_core.h"
#include <mutex>

using namespace ML::MLSpecbos;

MLSpecbosLogic* MLSpecbosLogic::m_instance = nullptr;

MLSpecbosLogic* MLSpecbosLogic::getInstance()
{
    if (!m_instance)
    {
        static std::mutex mutex;
        std::unique_lock<std::mutex> locker(mutex);
        if (!m_instance)
        {
            m_instance = new MLSpecbosLogic();
        }
    }
    return m_instance;
}

MLSpecbosLogic::MLSpecbosLogic() {
    m_configManger = new MLSpecbosConfigManage();
    m_measurementResults = new MeasurementResults();
    m_CalibrationInfo = new CalibrationInfo();
    m_dtype = ML::MLSpecbos::DeviceType::Specbos_2501;
    LoggingWrapper::instance()->info("MLSpecbosLogic: create instance");
}

MLSpecbosLogic::~MLSpecbosLogic() {
    if (m_configManger != nullptr) {
        delete m_configManger;
        m_configManger = nullptr;
    }
    if (m_measurementResults != nullptr) {
        delete m_measurementResults;
        m_measurementResults = nullptr;
    }
    if (m_CalibrationInfo != nullptr) {
        delete m_CalibrationInfo;
        m_CalibrationInfo = nullptr;
    }
    LoggingWrapper::instance()->info("MLSpecbosLogic: destroy instance");
}

void MLSpecbosLogic::Init(std::string config_path) {
    m_configManger->Init(config_path);
    errorThreshold = m_configManger->GetErrorThreshold();
    m_configManger->SetErrorThreshold(errorThreshold);
    m_CalibrationInfo = m_configManger->GetCalibrationInfo();
    m_DeviceInfo = m_configManger->GetSpecbosDeviceInfo();
}

bool ML::MLSpecbos::MLSpecbosLogic::SetDeviceType(DeviceType dtype) {
    m_dtype = dtype;
    return true;
}

bool MLSpecbosLogic::Isopen() {
    if (m_DeviceInfo.dwDevice == NULL) {
        return false;
    }
    else {
        return true;
    }
}

bool MLSpecbosLogic::Open(std::string port) {
    int wComPosrNr = atoi(port.erase(0, 3).c_str());
    m_DeviceInfo.wPort = wComPosrNr;
    DWORD_PTR device;
    DWORD ret = JETI_OpenCOMDevice(m_DeviceInfo.wPort, m_DeviceInfo.dwBaudrate, &device);
    m_DeviceInfo.dwDevice = device;
    if (ret == JETI_SUCCESS) {
        // Close Laser
        if (JETI_GetLaserStat(m_DeviceInfo.dwDevice, &m_peripheralControl.boLaserStat) ==
            JETI_SUCCESS) {
            if (m_peripheralControl.boLaserStat == TRUE) {
                if (JETI_SetLaserStat(m_DeviceInfo.dwDevice, FALSE) == JETI_SUCCESS) {
                    LoggingWrapper::instance()->info("MLSpecbosLogic: switch off the internal pilot laser");
                }
            }
            JETI_GetLaserStat(m_DeviceInfo.dwDevice, &m_peripheralControl.boLaserStat);
        }

        // TintConf
        JETI_GetTintConf(m_DeviceInfo.dwDevice, &m_measurementConfig.fPrevTint,
            &m_measurementConfig.fConfTint);
        JETI_GetMinTintConf(m_DeviceInfo.dwDevice, &m_measurementConfig.fMinTint);
        JETI_GetMaxTintConf(m_DeviceInfo.dwDevice, &m_measurementConfig.fMaxTint);

        // Average
        JETI_GetAverConf(m_DeviceInfo.dwDevice, &m_measurementConfig.wPrevAver,
            &m_measurementConfig.wConfAver);

        // Pixel
        JETI_GetPixel(m_DeviceInfo.dwDevice, &m_DeviceInfo.dwPixelCount);

        JETI_GetWranConf(m_DeviceInfo.dwDevice, &m_measurementConfig.dwBegin,
            &m_measurementConfig.dwEnd, &m_measurementConfig.dwStep);

        //
        JETI_SetCalib(m_DeviceInfo.dwDevice, m_CalibrationInfo->bCurCalibNr);

        // Calibration file
        int ret = JETI_GetCalib(m_DeviceInfo.dwDevice, &m_CalibrationInfo->bCurCalibNr);
        ret = JETI_GetMeasHead(m_DeviceInfo.dwDevice, &m_CalibrationInfo->bHead);
        LoggingWrapper::instance()->info("MLSpecbosLogic: open device finish");
        return true;

    }
    else {
        LoggingWrapper::instance()->error("MLSpecbosLogic: open device failed");
        m_DeviceInfo.dwDevice = NULL;
        return false;
    }
}

bool MLSpecbosLogic::Close() {
    if (m_DeviceInfo.dwDevice) {
        // Close Laser
        if (JETI_GetLaserStat(m_DeviceInfo.dwDevice, &m_peripheralControl.boLaserStat) ==
            JETI_SUCCESS) {
            if (m_peripheralControl.boLaserStat == TRUE) {
                if (JETI_SetLaserStat(m_DeviceInfo.dwDevice, FALSE) == JETI_SUCCESS) {
                    LoggingWrapper::instance()->info("MLSpecbosLogic: switch off the internal pilot laser");
                }
            }
            JETI_GetLaserStat(m_DeviceInfo.dwDevice, &m_peripheralControl.boLaserStat);
        }

        DWORD ret = JETI_CloseDevice(m_DeviceInfo.dwDevice);
        if (ret == JETI_SUCCESS) {
            m_DeviceInfo.dwDevice = NULL;
            LoggingWrapper::instance()->info("MLSpecbosLogic: close device");
            return true;
        }
    }
    LoggingWrapper::instance()->error("MLSpecbosLogic: close device failed");
    return false;
}

bool MLSpecbosLogic::LaserOn() {
    if (!Isopen()) {
        LoggingWrapper::instance()->error("MLSpecbosLogic: not open");
        return false;
    }
    DWORD ret =
        JETI_GetLaserStat(m_DeviceInfo.dwDevice, &m_peripheralControl.boLaserStat);
    if (ret == JETI_SUCCESS) {
        // Need to switch on
        if (m_peripheralControl.boLaserStat == TRUE) {
            LoggingWrapper::instance()->info("MLSpecbosLogic: internal pilot laser already switch on");
            return true;
        }
        if (JETI_SetLaserStat(m_DeviceInfo.dwDevice, TRUE) == JETI_SUCCESS) {
            LoggingWrapper::instance()->info("MLSpecbosLogic: switch on the internal pilot laser");
            return true;
        }
    }
    LoggingWrapper::instance()->error("MLSpecbosLogic: switch on the internal pilot laser failed");
    return false;
}

bool MLSpecbosLogic::LaserOff() {
    if (!Isopen()) {
        LoggingWrapper::instance()->error("MLSpecbosLogic: not open");
        return false;
    }
    if (JETI_GetLaserStat(m_DeviceInfo.dwDevice, &m_peripheralControl.boLaserStat) ==
        JETI_SUCCESS) {
        if (m_peripheralControl.boLaserStat == FALSE) {
            LoggingWrapper::instance()->info("MLSpecbosLogic: internal pilot laser already switch off");
            return true;
        }
        if (JETI_SetLaserStat(m_DeviceInfo.dwDevice, FALSE) == JETI_SUCCESS) {
            LoggingWrapper::instance()->info("MLSpecbosLogic: switch off the internal pilot laser");
            return true;
        }
    }
    LoggingWrapper::instance()->error("MLSpecbosLogic: switch off the internal pilot laser failed");
    return false;
}

bool MLSpecbosLogic::SetAverage(WORD Average) {
    m_measurementConfig.wAver = Average;
    // This function sets the count of measurement scans for average calculation
    if (JETI_SetAverConf(m_DeviceInfo.dwDevice, m_measurementConfig.wAver) == JETI_SUCCESS) 
    {
        JETI_GetAverConf(m_DeviceInfo.dwDevice, &m_measurementConfig.wPrevAver,
            &m_measurementConfig.wConfAver);
        std::string wPrevAver_str = std::to_string(m_measurementConfig.wPrevAver);
        LoggingWrapper::instance()->info("MLSpecbosLogic: set average to: " + QString::fromStdString(wPrevAver_str));
        return true;
    }
    LoggingWrapper::instance()->info("MLSpecbosLogic: set average failed");
    return false;
}

bool MLSpecbosLogic::SetAutoTintConf(float fMaxTint) {
    if (!Isopen()) {
        LoggingWrapper::instance()->error("MLSpecbosLogic: not open");
        return false;
    }
    m_measurementConfig.fMaxTint = fMaxTint;
    m_measurementConfig.bFiexdTint = false;
    JETI_SetMaxTintConf(m_DeviceInfo.dwDevice, m_measurementConfig.fMaxTint);
    DWORD ret;
    if (m_dtype == ML::MLSpecbos::DeviceType::Specbos_1211) {
        // perform radiometric measurement and calculate radiometric values
        ret = JETI_SetFunctionConf(m_DeviceInfo.dwDevice, 12);
        ret = JETI_SetExposureConf(m_DeviceInfo.dwDevice, 1);
        if (ret != JETI_SUCCESS) {
            LoggingWrapper::instance()->error("MLSpecbosLogic: set auto integration time failed for specbos1211");
            return false;
        }
    }
    else if (m_dtype == ML::MLSpecbos::DeviceType::Specbos_2501) {
        // perform reference measurement (dark current corrected measurement), radiometric
        // values will calculated when fetched
        ret = JETI_SetFunctionConf(m_DeviceInfo.dwDevice, 3);
        // set tint to 0 -> means adapt integration time
        ret = JETI_SetTintConf(m_DeviceInfo.dwDevice, 0);
        if (ret != JETI_SUCCESS) {
            LoggingWrapper::instance()->error("MLSpecbosLogic: set auto integration time failed for soecbos2501");
            return false;
        }
    }
    else {
        LoggingWrapper::instance()->error("MLSpecbosLogic: not support device type");
        return false;
    }
    LoggingWrapper::instance()->info("MLSpecbosLogic: set auto integration time finish");
    return true;
}

bool MLSpecbosLogic::SetFixedTintConf(float fTint) {
    if (!Isopen()) {
        LoggingWrapper::instance()->error("MLSpecbosLogic: not open");
        return false;
    }
    m_measurementConfig.fTint = fTint;
    m_measurementConfig.bFiexdTint = true;
    DWORD ret;
    ret = JETI_SetSyncMode(m_DeviceInfo.dwDevice, 0);
    if (ret != JETI_SUCCESS) {
        LoggingWrapper::instance()->error("MLSpecbosLogic: set sync mode failed");
        return false;
    }

    if (m_dtype == ML::MLSpecbos::DeviceType::Specbos_1211) {
        // perform radiometric measurement and calculate radiometric values
        ret = JETI_SetFunctionConf(m_DeviceInfo.dwDevice, 12);
        // use configured fixed tint
        ret = JETI_SetExposureConf(m_DeviceInfo.dwDevice, 2);
        // no adaptation in case of under- or overexposure
        ret = JETI_SetAdaptConf(m_DeviceInfo.dwDevice, 0);
        // set tint to 60ms
        ret = JETI_SetTintConf(m_DeviceInfo.dwDevice, fTint);
        if (ret != JETI_SUCCESS) {
            LoggingWrapper::instance()->error("MLSpecbosLogic: set fixed integration time failed for specbos1211");
            return false;
        }
    }
    else if (m_dtype == ML::MLSpecbos::DeviceType::Specbos_2501) {
        // perform reference measurement (dark current corrected measurement), radiometric
        // values will calculated when fetched
        ret = JETI_SetFunctionConf(m_DeviceInfo.dwDevice, 3);
        // The functions JETI_SetExposureConf and JETI_SetAdaptConf are no longer required
        // for the specbos 2501, as the adaptation can also be configured here using the
        // function JETI_SetTintConf.
        ret = JETI_SetTintConf(m_DeviceInfo.dwDevice, fTint);
        if (ret != JETI_SUCCESS) {
            LoggingWrapper::instance()->error("MLSpecbosLogic: set fixed integration time failed for specbos2501");
            return false;
        }
    }
    else {
        LoggingWrapper::instance()->error("MLSpecbosLogic: not support device type");
        return false;
    }
    std::string fTint_str = std::to_string(fTint);
    LoggingWrapper::instance()->info("MLSpecbosLogic: set fixed integration time to " + QString::fromStdString(fTint_str));
    return true;
}

float MLSpecbosLogic::GetTintConf() {
    if (!Isopen()) {
        LoggingWrapper::instance()->error("MLSpecbosLogic: not open");
        return DBL_MAX;
    }
    float prevTint;
    float confTint;
    DWORD ret = JETI_GetTintConf(m_DeviceInfo.dwDevice, &prevTint, &confTint);
    if (ret != JETI_SUCCESS) {
        LoggingWrapper::instance()->error("MLSpecbosLogic: get integration time failed");
        return DBL_MAX;
    }
    std::string prevTint_str = std::to_string(prevTint);
    LoggingWrapper::instance()->info("MLSpecbosLogic: get integration time = " + QString::fromStdString(prevTint_str));
    return prevTint;
}

bool MLSpecbosLogic::SetSyncFreq(float hz) {
    if (!Isopen()) {
        LoggingWrapper::instance()->error("MLSpecbosLogic: not open");
        return false;
    }
    DWORD ret;
    ret = JETI_SetSyncMode(m_DeviceInfo.dwDevice, 1);
    if (ret != JETI_SUCCESS) {
        LoggingWrapper::instance()->error("MLSpecbosLogic: set sync mode failed");
        return false;
    }
    ret = JETI_SetSyncFreq(m_DeviceInfo.dwDevice, hz);
    if (ret != JETI_SUCCESS) {
        LoggingWrapper::instance()->error("MLSpecbosLogic: set sync freq failed");
        return false;
    }

    float freq;
    JETI_GetSyncFreq(m_DeviceInfo.dwDevice, &freq);
    std::string freq_str = std::to_string(freq);
    LoggingWrapper::instance()->info("MLSpecbosLogic: set sync freq to " + QString::fromStdString(freq_str) + " HZ");
    return true;
}

bool MLSpecbosLogic::SetSyncMode(BYTE syncMode) {
    if (!Isopen()) {
        LoggingWrapper::instance()->error("MLSpecbosLogic: not open");
        return false;
    }
    DWORD ret;
    ret = JETI_SetSyncMode(m_DeviceInfo.dwDevice, syncMode);
    if (ret != JETI_SUCCESS) {
        LoggingWrapper::instance()->error("MLSpecbosLogic: set sync mode failed");
        return false;
    }
    BYTE mode;
    JETI_GetSyncMode(m_DeviceInfo.dwDevice, &mode);
    std::string mode_str = std::to_string(mode);
    LoggingWrapper::instance()->info("MLSpecbosLogic: set sync mode to " + QString::fromStdString(mode_str));
    return true;
}

bool MLSpecbosLogic::SetCalibrationFileNumber(BYTE bCalibNr) {
    if (!Isopen()) {
        LoggingWrapper::instance()->error("MLSpecbosLogic: not open");
        return false;
    }
    if (bCalibNr < 1 || bCalibNr > 12) {
        m_lastError = "Invalid calibration file number.";
        LoggingWrapper::instance()->error("MLSpecbosLogic: invalid calibration file number");
        return false;
    }
    if (bCalibNr &&
        strcmp(m_CalibrationInfo->calibrationFiles[bCalibNr - 1].cMode, "") == 0) {
        m_lastError = "The corresponding calibration file does not exist.";
        LoggingWrapper::instance()->error("MLSpecbosLogic: calibration file does not exist");
        return false;
    }

    m_CalibrationInfo->bCurCalibNr = bCalibNr;

    if (JETI_SetCalib(m_DeviceInfo.dwDevice, m_CalibrationInfo->bCurCalibNr) != JETI_SUCCESS) 
    {
        LoggingWrapper::instance()->error("MLSpecbosLogic: invalid calibration file number");
        return false;
    }
    JETI_GetCalib(m_DeviceInfo.dwDevice, &bCalibNr);

    std::string bCalibNr_str = std::to_string(bCalibNr);
    LoggingWrapper::instance()->info("MLSpecbosLogic: calibration file number: " + QString::fromStdString(bCalibNr_str));
    return true;
}

bool MLSpecbosLogic::TakeMeasurement() {
    if (!Isopen()) {
        LoggingWrapper::instance()->error("MLSpecbosLogic: not open");
        return false;
    }
    if (m_measurementResults->boStatus) {
        LoggingWrapper::instance()->error("MLSpecbosLogic: The last measurement was not completed. New measurements cannot to be started");
        return false;
    }

    // switch off the internal pilot laser
    if (JETI_GetLaserStat(m_DeviceInfo.dwDevice, &m_peripheralControl.boLaserStat) != JETI_SUCCESS) 
    {
        LoggingWrapper::instance()->error("MLSpecbosLogic: get internal pilot laser failed");
        return false;
    }
    if (m_peripheralControl.boLaserStat == TRUE) {
        if (JETI_SetLaserStat(m_DeviceInfo.dwDevice, FALSE) == JETI_SUCCESS) 
        {
            LoggingWrapper::instance()->info("MLSpecbosLogic: switch off the internal pilot laser");
        }
        else 
        {
            LoggingWrapper::instance()->error("MLSpecbosLogic: switch on the internal pilot laser failed");
            return false;
        }
    }
    JETI_GetLaserStat(m_DeviceInfo.dwDevice, &m_peripheralControl.boLaserStat);

    // This function sets the wavelength range.
    if (JETI_SetWranConf(m_DeviceInfo.dwDevice, m_measurementConfig.dwBegin,
        m_measurementConfig.dwEnd,
        m_measurementConfig.dwStep) == JETI_SUCCESS) {
        JETI_GetWranConf(m_DeviceInfo.dwDevice, &m_measurementConfig.dwBegin,
            &m_measurementConfig.dwEnd, &m_measurementConfig.dwStep);
    }
    JETI_GetTintConf(m_DeviceInfo.dwDevice, &m_measurementConfig.fPrevTint,
        &m_measurementConfig.fConfTint);

    // measure
    m_measurementResults->boStatus = 1;
    DWORD ret = JETI_InitMeasure(m_DeviceInfo.dwDevice);
    if (ret == JETI_SUCCESS) {
        LoggingWrapper::instance()->info("MLSpecbosLogic: init measure");
        while (m_measurementResults->boStatus) {
            Sleep(100);
            if (JETI_MeasureStatusCore(m_DeviceInfo.dwDevice,
                &m_measurementResults->boStatus) != JETI_SUCCESS) {
                return false;
            }
        }
        JETI_GetTintConf(m_DeviceInfo.dwDevice, &m_measurementConfig.fPrevTint,
            &m_measurementConfig.fConfTint);
        LoggingWrapper::instance()->info("MLSpecbosLogic: take measurement finish");
        return true;
    }
    else {
        LoggingWrapper::instance()->error("MLSpecbosLogic: take measurement failed");
        return false;
    }
}

bool MLSpecbosLogic::TakeMeasurementAsy() {
    if (!Isopen()) {
        LoggingWrapper::instance()->error("MLSpecbosLogic: not open");
        return false;
    }
    if (m_measurementResults->boStatus) {
        LoggingWrapper::instance()->error(
            "MLSpecbosLogic: The last measurement was not completed. New measurements cannot to be started");
        return false;
    }

    // switch off the internal pilot laser
    if (JETI_GetLaserStat(m_DeviceInfo.dwDevice, &m_peripheralControl.boLaserStat) !=
        JETI_SUCCESS) {
        LoggingWrapper::instance()->error("MLSpecbosLogic: get internal pilot laser failed");
        return false;
    }
    if (m_peripheralControl.boLaserStat == TRUE) {
        if (JETI_SetLaserStat(m_DeviceInfo.dwDevice, FALSE) == JETI_SUCCESS) {
            LoggingWrapper::instance()->info("MLSpecbosLogic: switch off the internal pilot laser");
        }
        else {
            LoggingWrapper::instance()->error("MLSpecbosLogic: switch on the internal pilot laser failed");
            return false;
        }
    }
    JETI_GetLaserStat(m_DeviceInfo.dwDevice, &m_peripheralControl.boLaserStat);

    // This function sets the wavelength range.
    if (JETI_SetWranConf(m_DeviceInfo.dwDevice, m_measurementConfig.dwBegin,
        m_measurementConfig.dwEnd,
        m_measurementConfig.dwStep) == JETI_SUCCESS) {
        JETI_GetWranConf(m_DeviceInfo.dwDevice, &m_measurementConfig.dwBegin,
            &m_measurementConfig.dwEnd, &m_measurementConfig.dwStep);
    }
    JETI_GetTintConf(m_DeviceInfo.dwDevice, &m_measurementConfig.fPrevTint,
        &m_measurementConfig.fConfTint);

    // measure
    m_measurementResults->boStatus = 1;
    DWORD ret = JETI_InitMeasure(m_DeviceInfo.dwDevice);
    if (ret == JETI_SUCCESS) {
        LoggingWrapper::instance()->info("MLSpecbosLogic: init measure");
        return true;
    }
    else {
        LoggingWrapper::instance()->error("MLSpecbosLogic: take measurement failed");
        return false;
    }
}

bool MLSpecbosLogic::StopMeasure() {
    // In Process
    if (!Isopen()) {
        LoggingWrapper::instance()->error("MLSpecbosLogic: not open");
        return false;
    }
    if (m_measurementResults->boStatus) {
        DWORD ret = JETI_Break(m_DeviceInfo.dwDevice);
        if (ret == JETI_SUCCESS) {
            if (JETI_MeasureStatusCore(m_DeviceInfo.dwDevice,
                &m_measurementResults->boStatus) == JETI_SUCCESS) {
            }
            return true;
        }
        else {
            std::string ret_str = std::to_string(ret);
            LoggingWrapper::instance()->error("MLSpecbosLogic: stop measurement failed, error code: " + QString::fromStdString(ret_str));
            return false;
        }
    }
    else {
        return true;
    }
}

bool MLSpecbosLogic::GetXYZxy(float& X, float& Y, float& Z, float& x, float& y,
    float& lum) {
    X = m_measurementResults->fX;
    Y = m_measurementResults->fY;
    Z = m_measurementResults->fZ;
    x = m_measurementResults->fChromx;
    y = m_measurementResults->fChromy;
    lum = m_measurementResults->fPhoto;

    return true;
}

std::string MLSpecbosLogic::GetCalibrationFile(int num)
{
    std::string CalibrationFile_str;
    if (0 <= num && num <= MAX_CALIBRATION_FILE_NUM - 1)
    {
        CalibrationFile_str = m_CalibrationInfo->calibrationFiles[num].cRemark;
    }
    else
    {
        CalibrationFile_str = "";
        LoggingWrapper::instance()->error("MLSpecbosLogic: get calibration file failed, beyond max calibration fie num!");
    }

    return CalibrationFile_str;
}

MeasurementResults* MLSpecbosLogic::GetMeasurementResults() {
    if (m_DeviceInfo.dwDevice) {
        WORD ret;

        // FetchXYZ
        ret = JETI_FetchXYZ(m_DeviceInfo.dwDevice, &m_measurementResults->fX,
            &m_measurementResults->fY, &m_measurementResults->fZ);

        // FetchChromxy
        ret = JETI_FetchChromxy(m_DeviceInfo.dwDevice, &m_measurementResults->fChromx,
            &m_measurementResults->fChromy);

        // FetchChromuv
        ret = JETI_FetchChromuv(m_DeviceInfo.dwDevice, &m_measurementResults->fChromu,
            &m_measurementResults->fChromv);

        // FetchCCT ret 11,
        ret = JETI_CalcCCT(m_DeviceInfo.dwDevice, &m_measurementResults->fCCT);

        // GetWranConf
        ret = JETI_GetWranConf(m_DeviceInfo.dwDevice, &m_measurementConfig.dwBegin,
            &m_measurementConfig.dwEnd, &m_measurementConfig.dwStep);
        m_measurementResults->iCountOfSprad =
            (m_measurementConfig.dwEnd - m_measurementConfig.dwBegin) /
            m_measurementConfig.dwStep +
            1;

        // FetchSprad
        ret = JETI_FetchSprad(m_DeviceInfo.dwDevice, m_measurementResults->fSprad);

        if (JETI_FetchRadio(m_DeviceInfo.dwDevice, &m_measurementResults->fRadio) ==
            JETI_SUCCESS) {
        }

        if (JETI_FetchPhoto(m_DeviceInfo.dwDevice, &m_measurementResults->fPhoto) ==
            JETI_SUCCESS) {
        }
    }
    LoggingWrapper::instance()->info("MLSpecbosLogic: get measurement result");
    return m_measurementResults;
}

std::string MLSpecbosLogic::GetLastError() { return m_lastError; }

SpecbosDeviceInfo MLSpecbosLogic::GetDeviceInfo() { return m_DeviceInfo; }

MeasurementConfig MLSpecbosLogic::GetMeasurementConfig() { return m_measurementConfig; }

PeripheralControl MLSpecbosLogic::GetPeripheralControl() { return m_peripheralControl; }

CalibrationInfo* MLSpecbosLogic::GetCalibrationInfo() {
    JETI_GetCalib(m_DeviceInfo.dwDevice, &(m_CalibrationInfo->bCurCalibNr));
    JETI_GetMeasHead(m_DeviceInfo.dwDevice, &(m_CalibrationInfo->bHead));
    return m_CalibrationInfo;
}

void MLSpecbosLogic::ReCalAllValue(int begin, int end) {
    FLOAT fDWL, fPE;
    JETI_CalcAllValue(m_DeviceInfo.dwDevice, begin, end, &m_measurementResults->fRadio,
        &m_measurementResults->fPhoto, &m_measurementResults->fChromx,
        &m_measurementResults->fChromy, &m_measurementResults->fChromu,
        &m_measurementResults->fChromv, &fDWL, &fPE);

    JETI_FetchXYZ(m_DeviceInfo.dwDevice, &m_measurementResults->fX,
        &m_measurementResults->fY, &m_measurementResults->fZ);

    JETI_FetchCCT(m_DeviceInfo.dwDevice, &m_measurementResults->fCCT);
}

void MLSpecbosLogic::LoadCalibrationFile() {
    for (DWORD i = 0; i < MAX_CALIBRATION_FILE_NUM; i++) {
        m_CalibrationInfo->calibrationFiles[i].dwCalibNr = i + 1;
        int ret = JETI_ReadCalib(m_DeviceInfo.dwDevice, i + 1,
            m_CalibrationInfo->calibrationFiles[i].cMode,
            m_CalibrationInfo->calibrationFiles[i].cRemark,
            &m_CalibrationInfo->calibrationFiles[i].dwBegin,
            &m_CalibrationInfo->calibrationFiles[i].dwEnd,
            &m_CalibrationInfo->calibrationFiles[i].dwStep,
            &m_CalibrationInfo->calibrationFiles[i].dwTint,
            m_CalibrationInfo->calibrationFiles[i].dValue);

        std::string str = m_CalibrationInfo->calibrationFiles[i].cRemark;
        LoggingWrapper::instance()->info("MLSpecbosLogic: Remark: " + QString::fromStdString(str));

        if (ret == JETI_SUCCESS) {
            // appendOperatorInfo(tr("Get Calibration File: CaliNr:%1# Mode:%2 Remark:%3")
            //	.arg(m_CalibrationInfo.calibrationFiles[i].dwCalibNr)
            //	.arg(m_CalibrationInfo.calibrationFiles[i].cMode)
            //	.arg(m_CalibrationInfo.calibrationFiles[i].cRemark));
        }
        else if (ret == JETI_INVALID_CALIB) {
            char cRemark[100] = "-- not calibrated --";

            memcpy(m_CalibrationInfo->calibrationFiles[i].cRemark, cRemark,
                sizeof(cRemark));
        }
        else {
            m_lastError =
                "Get Calibration File Failed: CaliNr:" +
                std::to_string(m_CalibrationInfo->calibrationFiles[i].dwCalibNr) +
                "ret: " + std::to_string(ret);
        }
    }

    m_configManger->SetCalibrationInfo(m_CalibrationInfo);
}
