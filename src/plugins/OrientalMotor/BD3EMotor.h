// Copyright 2023 MLOptic

#ifndef BD3E_MOTOR_H_
#define BD3E_MOTOR_H_

#include <mutex>
#include <string>

#include "ml.h"

#include "BD3ESerialManage.h"
#include "BD3EMotorConfig.h"

using namespace CORE;

enum class MLBD3EStatus {
    MLBD3EStatus_OK,
    MLBD3EStatus_Connecting,
    MLBD3EStatus_Close,
    MLBD3EStatus_Moving,
    MLBD3EStatus_Stationary,
    MLBD3EStatus_Alarm,
    MLBD3EStatus_Error,
    MLBD3EStatus_SerialException,
    MLBD3EStatus_Timeout,
    MLBD3EStatus_Unknown
};

struct MLBD3EResult {
    bool code = false;
    MLBD3EStatus status = MLBD3EStatus::MLBD3EStatus_Unknown;
    std::string msg = "";
};

class MLBD3EMotorCallback {
public:
    virtual void NotifyBD3EStatusChanged(
        const std::string object, MLBD3EStatus status) = 0;
};

class BD3EMotor {
public:
    BD3EMotor(const BD3EConfiguation& config);
    ~BD3EMotor();

    MLBD3EMotorCallback* _callback = nullptr;
    MLBD3EStatus m_status = MLBD3EStatus::MLBD3EStatus_Unknown;
    bool m_bIsOpen = false;
    std::string _msg;
    std::mutex _status_mutex;

    MLBD3EResult ML_OpenSerial(
        MLBD3EMotorCallback* cb = nullptr);
    MLBD3EResult ML_OpenSerialSync();
    MLBD3EResult ML_CloseSerial();

    bool ML_IsOpen();
    bool ML_IsMoving();
    bool ML_IsAlarm();

    MLBD3EResult ML_StopMove();
    MLBD3EResult ML_ClearAlarm();
    int  ML_ReadAlarmCode();

    MLBD3EResult ML_ServoOn();
    MLBD3EResult ML_ServoOff();

    MLBD3EResult ML_SetSpeed(int rpm);
    int  ML_GetSpeed();

    MLBD3EResult ML_SetControlMode(uint16_t mode);
    MLBD3EResult ML_SetAccelTime(uint16_t ms);
    MLBD3EResult ML_SetDecelTime(uint16_t ms);

    MLBD3EResult ML_MoveByDegree(double degree,
        MLBD3EMotorCallback* cb = nullptr);
    MLBD3EResult ML_MoveByDegreeSync(double degree);

    int  ML_GetCurrPulse();
    double ML_GetCurrDegree();

    MLBD3EStatus ML_GetStatus();
    MLBD3EResult ML_WaitForFinish(int timeout = 30000);

private:
    BD3EConfiguation m_config;
};

#endif  // BD3E_MOTOR_H_
