#include "BD3EMotorControl.h"
#include "MLSpdlog.h"

BD3EMotorControl* BD3EMotorControl::m_instance = nullptr;

BD3EMotorControl::BD3EMotorControl() {
    Init();
}

BD3EMotorControl::~BD3EMotorControl() {
    for (auto it = m_MotorMap.begin(); it != m_MotorMap.end(); ) {
        if (it->second != nullptr) {
            delete it->second;
        }
        it = m_MotorMap.erase(it);
    }
}

void BD3EMotorControl::Init() {
    std::map<int, BD3EConfiguation> configMap =
        BD3EMotorConfig::instance()->GetBD3EConfigMap();
    if (configMap.size() == 1) {
        Xmin = configMap[0].Min;
        Xmax = configMap[0].Max;
    }
    for (auto it = configMap.begin(); it != configMap.end(); ++it) {
        BD3ESerialManage::instance()->createBD3ESerial(it->second.port);
        BD3EMotor* motor = new BD3EMotor(it->second);
        m_MotorMap.insert(std::make_pair(it->first, motor));
    }
}

Result BD3EMotorControl::Connect() {
    Result ret;
    for (auto it = m_MotorMap.begin(); it != m_MotorMap.end(); ++it) {
        MLBD3EResult result = it->second->ML_OpenSerialSync();
        ret.success = result.code;
        ret.errorMsg = result.msg;
        if (!ret.success)
            return ret;
    }
    m_bIsOpen = true;
    return ret;
}

Result BD3EMotorControl::Disconnect() {
    Result ret;
    for (auto it = m_MotorMap.begin(); it != m_MotorMap.end(); ++it) {
        MLBD3EResult result = it->second->ML_CloseSerial();
        ret.success = result.code;
        ret.errorMsg = result.msg;
        if (!ret.success)
            return ret;
    }
    m_bIsOpen = false;
    return ret;
}

bool BD3EMotorControl::IsConnected() {
    return m_bIsOpen;
}

Result BD3EMotorControl::ServoOn(int type) {
    Result ret = JudgeOpen(type);
    if (!ret.success)
        return ret;

    ret = ClearAlarm();
    if (!ret.success)
        return ret;

    MLBD3EResult result = m_MotorMap[type]->ML_ServoOn();
    return Result(result.code, result.msg);
}

Result BD3EMotorControl::ServoOff(int type) {
    Result ret = JudgeOpen(type);
    if (!ret.success)
        return ret;

    MLBD3EResult result = m_MotorMap[type]->ML_ServoOff();
    return Result(result.code, result.msg);
}

Result BD3EMotorControl::ServoOnAll() {
    if (!IsConnected())
        return Result(false, "Motor is not connected.");

    Result ret;
    for (auto it = m_MotorMap.begin(); it != m_MotorMap.end(); ++it) {
        ret = ClearAlarm();
        if (!ret.success)
            return ret;

        MLBD3EResult result = it->second->ML_ServoOn();
        if (!result.code)
            return Result(result.code, result.msg);
    }
    return Result();
}

Result BD3EMotorControl::ServoOffAll() {
    if (!IsConnected())
        return Result(false, "Motor is not connected.");

    Result ret;
    for (auto it = m_MotorMap.begin(); it != m_MotorMap.end(); ++it) {
        MLBD3EResult result = it->second->ML_ServoOff();
        if (!result.code)
            return Result(result.code, result.msg);
    }
    return Result();
}

Result BD3EMotorControl::SetSpeed(int rpm) {
    if (!IsConnected())
        return Result(false, "Motor is not connected.");

    for (auto it = m_MotorMap.begin(); it != m_MotorMap.end(); ++it) {
        MLBD3EResult result = it->second->ML_SetSpeed(rpm);
        if (!result.code)
            return Result(result.code, result.msg);
    }
    return Result();
}

Result BD3EMotorControl::SetControlMode(uint16_t mode) {
    if (!IsConnected())
        return Result(false, "Motor is not connected.");

    for (auto it = m_MotorMap.begin(); it != m_MotorMap.end(); ++it) {
        MLBD3EResult result = it->second->ML_SetControlMode(mode);
        if (!result.code)
            return Result(result.code, result.msg);
    }
    return Result();
}

Result BD3EMotorControl::SetAccelTime(uint16_t ms) {
    if (!IsConnected())
        return Result(false, "Motor is not connected.");

    for (auto it = m_MotorMap.begin(); it != m_MotorMap.end(); ++it) {
        MLBD3EResult result = it->second->ML_SetAccelTime(ms);
        if (!result.code)
            return Result(result.code, result.msg);
    }
    return Result();
}

Result BD3EMotorControl::SetDecelTime(uint16_t ms) {
    if (!IsConnected())
        return Result(false, "Motor is not connected.");

    for (auto it = m_MotorMap.begin(); it != m_MotorMap.end(); ++it) {
        MLBD3EResult result = it->second->ML_SetDecelTime(ms);
        if (!result.code)
            return Result(result.code, result.msg);
    }
    return Result();
}

int BD3EMotorControl::GetSpeed(int type) {
    Result ret = JudgeOpen(type);
    if (!ret.success)
        return INT_MAX;
    return m_MotorMap[type]->ML_GetSpeed();
}

int BD3EMotorControl::GetCurrPulse(int type) {
    Result ret = JudgeOpen(type);
    if (!ret.success)
        return INT_MAX;
    return m_MotorMap[type]->ML_GetCurrPulse();
}

double BD3EMotorControl::GetCurrDegree(int type) {
    Result ret = JudgeOpen(type);
    if (!ret.success)
        return static_cast<double>(INT_MAX);
    return m_MotorMap[type]->ML_GetCurrDegree();
}

Result BD3EMotorControl::MoveByDegreeAsync(int type, double degree) {
    Result ret = JudgeOpen(type);
    if (!ret.success)
        return ret;

    ret = ClearAlarm();
    if (!ret.success)
        return ret;

    MLBD3EResult result = m_MotorMap[type]->ML_MoveByDegree(degree);
    return Result(result.code, result.msg);
}

Result BD3EMotorControl::MoveByDegreeSync(int type, double degree) {
    Result ret = JudgeOpen(type);
    if (!ret.success)
        return ret;

    ret = ClearAlarm();
    if (!ret.success)
        return ret;

    MLBD3EResult result = m_MotorMap[type]->ML_MoveByDegreeSync(degree);
    return Result(result.code, result.msg);
}

Result BD3EMotorControl::StopMove(int type) {
    Result ret = JudgeOpen(type);
    if (!ret.success)
        return ret;

    MLBD3EResult result = m_MotorMap[type]->ML_StopMove();
    return Result(result.code, result.msg);
}

Result BD3EMotorControl::StopMove() {
    if (!IsConnected())
        return Result(false, "Motor is not connected.");

    for (auto it = m_MotorMap.begin(); it != m_MotorMap.end(); ++it) {
        MLBD3EResult result = it->second->ML_StopMove();
        if (!result.code)
            return Result(result.code, result.msg);
    }
    return Result();
}

bool BD3EMotorControl::IsMoving(int type) {
    Result ret = JudgeOpen(type);
    if (!ret.success)
        return false;
    return m_MotorMap[type]->ML_IsMoving();
}

bool BD3EMotorControl::IsMoving() {
    for (auto it = m_MotorMap.begin(); it != m_MotorMap.end(); ++it) {
        Result ret = JudgeOpen(it->first);
        if (!ret.success)
            return false;
        if (it->second->ML_IsMoving())
            return true;
    }
    return false;
}

bool BD3EMotorControl::IsAlarm(int type) {
    Result ret = JudgeOpen(type);
    if (!ret.success)
        return false;
    return m_MotorMap[type]->ML_IsAlarm();
}

bool BD3EMotorControl::IsAlarm() {
    for (auto it = m_MotorMap.begin(); it != m_MotorMap.end(); ++it) {
        Result ret = JudgeOpen(it->first);
        if (!ret.success)
            return false;
        if (it->second->ML_IsAlarm())
            return true;
    }
    return false;
}

Result BD3EMotorControl::ClearAlarm() {
    Result ret;
    for (auto it = m_MotorMap.begin(); it != m_MotorMap.end(); ++it) {
        MLBD3EResult result = it->second->ML_ClearAlarm();
        ret.success = result.code;
        ret.errorMsg = result.msg;
        if (!ret.success)
            return ret;
    }
    return ret;
}

Result BD3EMotorControl::JudgeOpen(int type) {
    if (!m_MotorMap.count(type))
        return Result(false, "Motor map does not contain this type.");

    if (!m_bIsOpen)
        return Result(false, "Motor is not connected.");

    return Result();
}

BD3EMotorControl* BD3EMotorControl::getInstance() {
    if (!m_instance) {
        static std::mutex mutex;
        std::unique_lock<std::mutex> locker(mutex);
        if (!m_instance) {
            m_instance = new BD3EMotorControl();
        }
    }
    return m_instance;
}
