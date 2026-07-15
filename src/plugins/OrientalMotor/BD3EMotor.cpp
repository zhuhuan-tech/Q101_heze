// Copyright 2023 MLOptic
#include "BD3EMotor.h"

#include <thread>
#include <chrono>

#include "MLSpdlog.h"

BD3EMotor::BD3EMotor(const BD3EConfiguation& config)
    : m_config(config) {
}

BD3EMotor::~BD3EMotor() {
}

MLBD3EResult BD3EMotor::ML_OpenSerial(
    MLBD3EMotorCallback* cb) {
    MLBD3EResult ret;
    try {
        BD3ESerialManage::instance()
            ->getBD3ESerial(m_config.port)
            ->InitSerialPort(m_config.serial_info);
        bool rt = BD3ESerialManage::instance()
                      ->getBD3ESerial(m_config.port)
                      ->Open(m_config.port);
        if (rt) {
            std::lock_guard<std::mutex> locker(_status_mutex);
            m_bIsOpen = true;
            BD3ESerialManage::instance()->setConnection(m_config.port);
            m_status = MLBD3EStatus::MLBD3EStatus_OK;
            ret.code = rt;
            ret.status = m_status;
            ret.msg = _msg + " BD3EMotor ML_OpenSerial successfully";
            ML::MLSpdlog::instance()->info("BD3EMotor: " + ret.msg);
            if (cb != nullptr) {
                m_status = MLBD3EStatus::MLBD3EStatus_Connecting;
                ret.status = m_status;
                cb->NotifyBD3EStatusChanged(m_config.name, m_status);
            }
            return ret;
        }
        std::lock_guard<std::mutex> locker(_status_mutex);
        m_bIsOpen = rt;
        m_status = MLBD3EStatus::MLBD3EStatus_Error;
        ret.code = rt;
        ret.status = m_status;
        ret.msg = _msg + " BD3EMotor ML_OpenSerial failed";
        ML::MLSpdlog::instance()->error("BD3EMotor: " + ret.msg);
        return ret;
    } catch (const std::exception& e) {
        std::lock_guard<std::mutex> locker(_status_mutex);
        m_bIsOpen = false;
        m_status = MLBD3EStatus::MLBD3EStatus_SerialException;
        ret.code = false;
        ret.status = m_status;
        ret.msg = _msg + " BD3EMotor ML_OpenSerial exception: " + e.what();
        ML::MLSpdlog::instance()->error("BD3EMotor: " + ret.msg);
        return ret;
    }
}

MLBD3EResult BD3EMotor::ML_OpenSerialSync() {
    MLBD3EResult ret;
    try {
        BD3ESerialManage::instance()
            ->getBD3ESerial(m_config.port)
            ->InitSerialPort(m_config.serial_info);
        bool rt = BD3ESerialManage::instance()
                      ->getBD3ESerial(m_config.port)
                      ->Open(m_config.port);
        if (rt) {
            std::lock_guard<std::mutex> locker(_status_mutex);
            m_bIsOpen = true;
            BD3ESerialManage::instance()->setConnection(m_config.port);
            m_status = MLBD3EStatus::MLBD3EStatus_OK;
            ret.code = rt;
            ret.status = m_status;
            ret.msg = _msg + " BD3EMotor ML_OpenSerialSync successfully";
            ML::MLSpdlog::instance()->info("BD3EMotor: " + ret.msg);
            return ret;
        }
        std::lock_guard<std::mutex> locker(_status_mutex);
        m_bIsOpen = rt;
        m_status = MLBD3EStatus::MLBD3EStatus_Error;
        ret.code = rt;
        ret.status = m_status;
        ret.msg = _msg + " BD3EMotor ML_OpenSerialSync failed";
        ML::MLSpdlog::instance()->error("BD3EMotor: " + ret.msg);
        return ret;
    } catch (const std::exception& e) {
        std::lock_guard<std::mutex> locker(_status_mutex);
        m_bIsOpen = false;
        m_status = MLBD3EStatus::MLBD3EStatus_SerialException;
        ret.code = false;
        ret.status = m_status;
        ret.msg = _msg + " BD3EMotor ML_OpenSerialSync exception: " + e.what();
        ML::MLSpdlog::instance()->error("BD3EMotor: " + ret.msg);
        return ret;
    }
}

MLBD3EResult BD3EMotor::ML_CloseSerial() {
    MLBD3EResult ret;
    try {
        if (!ML_IsOpen()) {
            std::lock_guard<std::mutex> locker(_status_mutex);
            m_status = MLBD3EStatus::MLBD3EStatus_Error;
            ret.code = false;
            ret.status = m_status;
            ret.msg = _msg + " BD3EMotor ML_CloseSerial error, not opened";
            ML::MLSpdlog::instance()->error("BD3EMotor: " + ret.msg);
            return ret;
        }
        BD3ESerialManage::instance()->closeConnection(m_config.port);
        int connection =
            BD3ESerialManage::instance()->getConnection(m_config.port);
        if (connection == 0) {
            bool rt = BD3ESerialManage::instance()
                          ->getBD3ESerial(m_config.port)
                          ->Close();
            if (rt) {
                std::lock_guard<std::mutex> locker(_status_mutex);
                m_bIsOpen = false;
                m_status = MLBD3EStatus::MLBD3EStatus_Close;
                ret.code = rt;
                ret.status = m_status;
                ret.msg = _msg + " BD3EMotor ML_CloseSerial successfully";
                ML::MLSpdlog::instance()->info("BD3EMotor: " + ret.msg);
                return ret;
            }
            std::lock_guard<std::mutex> locker(_status_mutex);
            m_status = MLBD3EStatus::MLBD3EStatus_Error;
            ret.code = rt;
            ret.status = m_status;
            ret.msg = _msg + " BD3EMotor ML_CloseSerial error";
            ML::MLSpdlog::instance()->error("BD3EMotor: " + ret.msg);
            return ret;
        }
        std::lock_guard<std::mutex> locker(_status_mutex);
        m_bIsOpen = false;
        m_status = MLBD3EStatus::MLBD3EStatus_OK;
        ret.code = true;
        ret.status = m_status;
        ret.msg = _msg +
                  " BD3EMotor ML_CloseSerial not all drives off, "
                  "serial port cannot be turned off";
        ML::MLSpdlog::instance()->error("BD3EMotor: " + ret.msg);
        return ret;
    } catch (const std::exception& e) {
        std::lock_guard<std::mutex> locker(_status_mutex);
        m_bIsOpen = false;
        m_status = MLBD3EStatus::MLBD3EStatus_SerialException;
        ret.code = false;
        ret.status = m_status;
        ret.msg = _msg + " BD3EMotor ML_CloseSerial exception: " + e.what();
        ML::MLSpdlog::instance()->error("BD3EMotor: " + ret.msg);
        return ret;
    }
}

bool BD3EMotor::ML_IsOpen() {
    try {
        BD3ESerial* serial = BD3ESerialManage::instance()->getBD3ESerial(m_config.port);
        if (serial == nullptr)
            return false;
        bool isOpen = serial->IsOpen() &&
                      serial->CheckSerial(m_config.station) &&
                      m_bIsOpen;
        return isOpen;
    } catch (const std::exception&) {
        std::lock_guard<std::mutex> locker(_status_mutex);
        m_status = MLBD3EStatus::MLBD3EStatus_SerialException;
        return false;
    }
}

bool BD3EMotor::ML_IsMoving() {
    try {
        if (!ML_IsOpen()) {
            std::lock_guard<std::mutex> locker(_status_mutex);
            m_status = MLBD3EStatus::MLBD3EStatus_Error;
            return false;
        }
        bool isMoving = BD3ESerialManage::instance()
                            ->getBD3ESerial(m_config.port)
                            ->IsMoving(m_config.station);
        if (!isMoving) {
            std::lock_guard<std::mutex> locker(_status_mutex);
            m_status = MLBD3EStatus::MLBD3EStatus_Stationary;
            return false;
        }
        std::lock_guard<std::mutex> locker(_status_mutex);
        m_status = MLBD3EStatus::MLBD3EStatus_Moving;
        return true;
    } catch (const std::exception&) {
        std::lock_guard<std::mutex> locker(_status_mutex);
        m_status = MLBD3EStatus::MLBD3EStatus_SerialException;
        return false;
    }
}

bool BD3EMotor::ML_IsAlarm() {
    try {
        if (!ML_IsOpen()) {
            std::lock_guard<std::mutex> locker(_status_mutex);
            m_status = MLBD3EStatus::MLBD3EStatus_Error;
            return false;
        }
        bool isAlarm = BD3ESerialManage::instance()
                           ->getBD3ESerial(m_config.port)
                           ->IsAlarm(m_config.station);
        if (!isAlarm) {
            std::lock_guard<std::mutex> locker(_status_mutex);
            m_status = MLBD3EStatus::MLBD3EStatus_OK;
            return false;
        }
        std::lock_guard<std::mutex> locker(_status_mutex);
        m_status = MLBD3EStatus::MLBD3EStatus_Alarm;
        return true;
    } catch (const std::exception&) {
        std::lock_guard<std::mutex> locker(_status_mutex);
        m_status = MLBD3EStatus::MLBD3EStatus_SerialException;
        return false;
    }
}

MLBD3EResult BD3EMotor::ML_StopMove() {
    MLBD3EResult ret;
    try {
        if (!ML_IsOpen()) {
            std::lock_guard<std::mutex> locker(_status_mutex);
            m_status = MLBD3EStatus::MLBD3EStatus_Error;
            ret.code = false;
            ret.status = m_status;
            ret.msg = _msg + " BD3EMotor ML_StopMove error, not opened";
            ML::MLSpdlog::instance()->error("BD3EMotor: " + ret.msg);
            return ret;
        }
        bool rt = BD3ESerialManage::instance()
                      ->getBD3ESerial(m_config.port)
                      ->StopMove(m_config.station);
        if (rt) {
            std::lock_guard<std::mutex> locker(_status_mutex);
            m_status = MLBD3EStatus::MLBD3EStatus_OK;
            ret.code = true;
            ret.status = m_status;
            ret.msg = _msg + " BD3EMotor ML_StopMove successfully";
            ML::MLSpdlog::instance()->info("BD3EMotor: " + ret.msg);
            return ret;
        }
        std::lock_guard<std::mutex> locker(_status_mutex);
        m_status = MLBD3EStatus::MLBD3EStatus_Error;
        ret.code = false;
        ret.status = m_status;
        ret.msg = _msg + " BD3EMotor ML_StopMove error";
        ML::MLSpdlog::instance()->error("BD3EMotor: " + ret.msg);
        return ret;
    } catch (const std::exception& e) {
        std::lock_guard<std::mutex> locker(_status_mutex);
        m_status = MLBD3EStatus::MLBD3EStatus_SerialException;
        ret.code = false;
        ret.status = m_status;
        ret.msg = _msg + " BD3EMotor ML_StopMove exception: " + e.what();
        ML::MLSpdlog::instance()->error("BD3EMotor: " + ret.msg);
        return ret;
    }
}

MLBD3EResult BD3EMotor::ML_ClearAlarm() {
    MLBD3EResult ret;
    try {
        if (!ML_IsOpen()) {
            std::lock_guard<std::mutex> locker(_status_mutex);
            m_status = MLBD3EStatus::MLBD3EStatus_Error;
            ret.code = false;
            ret.status = m_status;
            ret.msg = _msg + " BD3EMotor ML_ClearAlarm error, not opened";
            ML::MLSpdlog::instance()->error("BD3EMotor: " + ret.msg);
            return ret;
        }
        bool rt = BD3ESerialManage::instance()
                      ->getBD3ESerial(m_config.port)
                      ->ClearAlarm(m_config.station);
        if (!rt) {
            std::lock_guard<std::mutex> locker(_status_mutex);
            m_status = MLBD3EStatus::MLBD3EStatus_Error;
            ret.code = false;
            ret.status = m_status;
            ret.msg = _msg + " BD3EMotor ML_ClearAlarm error";
            ML::MLSpdlog::instance()->error("BD3EMotor: " + ret.msg);
            return ret;
        }
        std::lock_guard<std::mutex> locker(_status_mutex);
        m_status = MLBD3EStatus::MLBD3EStatus_OK;
        ret.code = true;
        ret.status = m_status;
        ret.msg = _msg + " BD3EMotor ML_ClearAlarm successfully";
        ML::MLSpdlog::instance()->info("BD3EMotor: " + ret.msg);
        return ret;
    } catch (const std::exception& e) {
        std::lock_guard<std::mutex> locker(_status_mutex);
        m_status = MLBD3EStatus::MLBD3EStatus_SerialException;
        ret.code = false;
        ret.status = m_status;
        ret.msg = _msg + " BD3EMotor ML_ClearAlarm exception: " + e.what();
        ML::MLSpdlog::instance()->error("BD3EMotor: " + ret.msg);
        return ret;
    }
}

int BD3EMotor::ML_ReadAlarmCode() {
    try {
        if (!ML_IsOpen()) {
            std::lock_guard<std::mutex> locker(_status_mutex);
            m_status = MLBD3EStatus::MLBD3EStatus_Error;
            return INT_MAX;
        }
        int code = BD3ESerialManage::instance()
                       ->getBD3ESerial(m_config.port)
                       ->ReadAlarmCode(m_config.station);
        std::lock_guard<std::mutex> locker(_status_mutex);
        if (code != 0 && code != INT_MAX) {
            m_status = MLBD3EStatus::MLBD3EStatus_Alarm;
        } else {
            m_status = MLBD3EStatus::MLBD3EStatus_OK;
        }
        return code;
    } catch (const std::exception) {
        std::lock_guard<std::mutex> locker(_status_mutex);
        m_status = MLBD3EStatus::MLBD3EStatus_SerialException;
        return INT_MAX;
    }
}

MLBD3EResult BD3EMotor::ML_ServoOn() {
    MLBD3EResult ret;
    try {
        if (!ML_IsOpen()) {
            std::lock_guard<std::mutex> locker(_status_mutex);
            m_status = MLBD3EStatus::MLBD3EStatus_Error;
            ret.code = false;
            ret.status = m_status;
            ret.msg = _msg + " BD3EMotor ML_ServoOn error, not opened";
            ML::MLSpdlog::instance()->error("BD3EMotor: " + ret.msg);
            return ret;
        }
        bool rt = BD3ESerialManage::instance()
                      ->getBD3ESerial(m_config.port)
                      ->ServoOn(m_config.station);
        if (!rt) {
            std::lock_guard<std::mutex> locker(_status_mutex);
            m_status = MLBD3EStatus::MLBD3EStatus_Error;
            ret.code = false;
            ret.status = m_status;
            ret.msg = _msg + " BD3EMotor ML_ServoOn failed";
            ML::MLSpdlog::instance()->error("BD3EMotor: " + ret.msg);
            return ret;
        }
        std::lock_guard<std::mutex> locker(_status_mutex);
        m_status = MLBD3EStatus::MLBD3EStatus_OK;
        ret.code = true;
        ret.status = m_status;
        ret.msg = _msg + " BD3EMotor ML_ServoOn successfully";
        ML::MLSpdlog::instance()->info("BD3EMotor: " + ret.msg);
        return ret;
    } catch (const std::exception& e) {
        std::lock_guard<std::mutex> locker(_status_mutex);
        m_status = MLBD3EStatus::MLBD3EStatus_SerialException;
        ret.code = false;
        ret.status = m_status;
        ret.msg = _msg + " BD3EMotor ML_ServoOn exception: " + e.what();
        ML::MLSpdlog::instance()->error("BD3EMotor: " + ret.msg);
        return ret;
    }
}

MLBD3EResult BD3EMotor::ML_ServoOff() {
    MLBD3EResult ret;
    try {
        if (!ML_IsOpen()) {
            std::lock_guard<std::mutex> locker(_status_mutex);
            m_status = MLBD3EStatus::MLBD3EStatus_Error;
            ret.code = false;
            ret.status = m_status;
            ret.msg = _msg + " BD3EMotor ML_ServoOff error, not opened";
            ML::MLSpdlog::instance()->error("BD3EMotor: " + ret.msg);
            return ret;
        }
        bool rt = BD3ESerialManage::instance()
                      ->getBD3ESerial(m_config.port)
                      ->ServoOff(m_config.station);
        if (!rt) {
            std::lock_guard<std::mutex> locker(_status_mutex);
            m_status = MLBD3EStatus::MLBD3EStatus_Error;
            ret.code = false;
            ret.status = m_status;
            ret.msg = _msg + " BD3EMotor ML_ServoOff failed";
            ML::MLSpdlog::instance()->error("BD3EMotor: " + ret.msg);
            return ret;
        }
        std::lock_guard<std::mutex> locker(_status_mutex);
        m_status = MLBD3EStatus::MLBD3EStatus_OK;
        ret.code = true;
        ret.status = m_status;
        ret.msg = _msg + " BD3EMotor ML_ServoOff successfully";
        ML::MLSpdlog::instance()->info("BD3EMotor: " + ret.msg);
        return ret;
    } catch (const std::exception& e) {
        std::lock_guard<std::mutex> locker(_status_mutex);
        m_status = MLBD3EStatus::MLBD3EStatus_SerialException;
        ret.code = false;
        ret.status = m_status;
        ret.msg = _msg + " BD3EMotor ML_ServoOff exception: " + e.what();
        ML::MLSpdlog::instance()->error("BD3EMotor: " + ret.msg);
        return ret;
    }
}

MLBD3EResult BD3EMotor::ML_SetSpeed(int rpm) {
    MLBD3EResult ret;
    try {
        if (!ML_IsOpen()) {
            std::lock_guard<std::mutex> locker(_status_mutex);
            m_status = MLBD3EStatus::MLBD3EStatus_Error;
            ret.code = false;
            ret.status = m_status;
            ret.msg = _msg + " BD3EMotor ML_SetSpeed error, not opened";
            ML::MLSpdlog::instance()->error("BD3EMotor: " + ret.msg);
            return ret;
        }
        if (ML_IsAlarm()) {
            std::lock_guard<std::mutex> locker(_status_mutex);
            m_status = MLBD3EStatus::MLBD3EStatus_Alarm;
            ret.code = false;
            ret.status = m_status;
            ret.msg = _msg + " BD3EMotor ML_SetSpeed alarm";
            ML::MLSpdlog::instance()->error("BD3EMotor: " + ret.msg);
            return ret;
        }
        bool rt = BD3ESerialManage::instance()
                      ->getBD3ESerial(m_config.port)
                      ->SetSpeed(m_config.station, rpm);
        if (!rt) {
            std::lock_guard<std::mutex> locker(_status_mutex);
            m_status = MLBD3EStatus::MLBD3EStatus_Error;
            ret.code = false;
            ret.status = m_status;
            ret.msg = _msg + " BD3EMotor ML_SetSpeed error, set speed failed";
            ML::MLSpdlog::instance()->error("BD3EMotor: " + ret.msg);
            return ret;
        }
        std::lock_guard<std::mutex> locker(_status_mutex);
        m_status = MLBD3EStatus::MLBD3EStatus_OK;
        ret.code = true;
        ret.status = m_status;
        ret.msg = _msg + " BD3EMotor ML_SetSpeed successfully";
        ML::MLSpdlog::instance()->info("BD3EMotor: " + ret.msg);
        return ret;
    } catch (const std::exception& e) {
        std::lock_guard<std::mutex> locker(_status_mutex);
        m_status = MLBD3EStatus::MLBD3EStatus_SerialException;
        ret.code = false;
        ret.status = m_status;
        ret.msg = _msg + " BD3EMotor ML_SetSpeed exception: " + e.what();
        ML::MLSpdlog::instance()->error("BD3EMotor: " + ret.msg);
        return ret;
    }
}

MLBD3EResult BD3EMotor::ML_MoveByDegree(
    double degree, MLBD3EMotorCallback* cb) {
    MLBD3EResult ret;
    try {
        if (!ML_IsOpen()) {
            std::lock_guard<std::mutex> locker(_status_mutex);
            m_status = MLBD3EStatus::MLBD3EStatus_Error;
            ret.code = false;
            ret.status = m_status;
            ret.msg = _msg + " BD3EMotor ML_MoveByDegree error, not opened";
            ML::MLSpdlog::instance()->error("BD3EMotor: " + ret.msg);
            return ret;
        }
        if (ML_IsAlarm()) {
            std::lock_guard<std::mutex> locker(_status_mutex);
            m_status = MLBD3EStatus::MLBD3EStatus_Alarm;
            ret.code = false;
            ret.status = m_status;
            ret.msg = _msg + " BD3EMotor ML_MoveByDegree alarm";
            ML::MLSpdlog::instance()->error("BD3EMotor: " + ret.msg);
            return ret;
        }
        if (degree > m_config.Max || degree < m_config.Min) {
            std::lock_guard<std::mutex> locker(_status_mutex);
            m_status = MLBD3EStatus::MLBD3EStatus_Error;
            ret.code = false;
            ret.status = m_status;
            ret.msg = _msg +
                      " BD3EMotor ML_MoveByDegree error, degree out of range";
            ML::MLSpdlog::instance()->warn("BD3EMotor: " + ret.msg);
            return ret;
        }
        int targetPulse = static_cast<int>(
            static_cast<double>(m_config.PulsePerCycle) / 360.0 * degree);
        bool rt = BD3ESerialManage::instance()
                      ->getBD3ESerial(m_config.port)
                      ->MoveToTarget(m_config.station, targetPulse,
                                     m_config.MoveSpeed);
        if (!rt) {
            std::lock_guard<std::mutex> locker(_status_mutex);
            m_status = MLBD3EStatus::MLBD3EStatus_Error;
            ret.code = false;
            ret.status = m_status;
            ret.msg = _msg + " BD3EMotor ML_MoveByDegree move failed";
            ML::MLSpdlog::instance()->error("BD3EMotor: " + ret.msg);
            return ret;
        }
        std::lock_guard<std::mutex> locker(_status_mutex);
        m_status = MLBD3EStatus::MLBD3EStatus_OK;
        if (cb != nullptr) {
            m_status = MLBD3EStatus::MLBD3EStatus_Moving;
            cb->NotifyBD3EStatusChanged(m_config.name, m_status);
        }
        ret.code = true;
        ret.status = m_status;
        ret.msg = _msg + " BD3EMotor ML_MoveByDegree successfully";
        ML::MLSpdlog::instance()->info("BD3EMotor: " + ret.msg);
        return ret;
    } catch (const std::exception& e) {
        std::lock_guard<std::mutex> locker(_status_mutex);
        m_status = MLBD3EStatus::MLBD3EStatus_SerialException;
        ret.code = false;
        ret.status = m_status;
        ret.msg = _msg + " BD3EMotor ML_MoveByDegree exception: " + e.what();
        ML::MLSpdlog::instance()->error("BD3EMotor: " + ret.msg);
        return ret;
    }
}

MLBD3EResult BD3EMotor::ML_MoveByDegreeSync(double degree) {
    MLBD3EResult ret;
    try {
        if (!ML_IsOpen()) {
            std::lock_guard<std::mutex> locker(_status_mutex);
            m_status = MLBD3EStatus::MLBD3EStatus_Error;
            ret.code = false;
            ret.status = m_status;
            ret.msg = _msg + " BD3EMotor ML_MoveByDegreeSync error, not opened";
            ML::MLSpdlog::instance()->error("BD3EMotor: " + ret.msg);
            return ret;
        }
        if (ML_IsAlarm()) {
            std::lock_guard<std::mutex> locker(_status_mutex);
            m_status = MLBD3EStatus::MLBD3EStatus_Alarm;
            ret.code = false;
            ret.status = m_status;
            ret.msg = _msg + " BD3EMotor ML_MoveByDegreeSync alarm";
            ML::MLSpdlog::instance()->error("BD3EMotor: " + ret.msg);
            return ret;
        }
        if (degree > m_config.Max || degree < m_config.Min) {
            std::lock_guard<std::mutex> locker(_status_mutex);
            m_status = MLBD3EStatus::MLBD3EStatus_Error;
            ret.code = false;
            ret.status = m_status;
            ret.msg = _msg +
                      " BD3EMotor ML_MoveByDegreeSync error, degree out of range";
            ML::MLSpdlog::instance()->warn("BD3EMotor: " + ret.msg);
            return ret;
        }
        int targetPulse = static_cast<int>(
            static_cast<double>(m_config.PulsePerCycle) / 360.0 * degree);
        int speed = m_config.MoveSpeed;
        bool rt = BD3ESerialManage::instance()
                      ->getBD3ESerial(m_config.port)
                      ->MoveToTarget(m_config.station, targetPulse, speed);
        if (!rt) {
            std::lock_guard<std::mutex> locker(_status_mutex);
            m_status = MLBD3EStatus::MLBD3EStatus_Error;
            ret.code = false;
            ret.status = m_status;
            ret.msg = _msg + " BD3EMotor ML_MoveByDegreeSync move failed";
            ML::MLSpdlog::instance()->error("BD3EMotor: " + ret.msg);
            return ret;
        }
        int timeout = 30000;
        int count = 0;
        int precision = 20;
        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            count++;
            if (ML_IsAlarm()) {
                BD3ESerialManage::instance()
                    ->getBD3ESerial(m_config.port)
                    ->StopMove(m_config.station);
                std::lock_guard<std::mutex> locker(_status_mutex);
                m_status = MLBD3EStatus::MLBD3EStatus_Alarm;
                ret.code = false;
                ret.status = m_status;
                ret.msg = _msg + " BD3EMotor ML_MoveByDegreeSync alarm";
                ML::MLSpdlog::instance()->error("BD3EMotor: " + ret.msg);
                return ret;
            }
            int current = BD3ESerialManage::instance()
                              ->getBD3ESerial(m_config.port)
                              ->GetCurrPosition(m_config.station);
            if (current == INT_MAX) {
                BD3ESerialManage::instance()
                    ->getBD3ESerial(m_config.port)
                    ->StopMove(m_config.station);
                std::lock_guard<std::mutex> locker(_status_mutex);
                m_status = MLBD3EStatus::MLBD3EStatus_Error;
                ret.code = false;
                ret.status = m_status;
                ret.msg = _msg +
                          " BD3EMotor ML_MoveByDegreeSync position read error";
                ML::MLSpdlog::instance()->error("BD3EMotor: " + ret.msg);
                return ret;
            }
            if (std::abs(current - targetPulse) <= precision) {
                break;
            }
            if (count * 100 > timeout) {
                BD3ESerialManage::instance()
                    ->getBD3ESerial(m_config.port)
                    ->StopMove(m_config.station);
                std::lock_guard<std::mutex> locker(_status_mutex);
                m_status = MLBD3EStatus::MLBD3EStatus_Timeout;
                ret.code = false;
                ret.status = m_status;
                ret.msg = _msg + " BD3EMotor ML_MoveByDegreeSync timeout";
                ML::MLSpdlog::instance()->warn("BD3EMotor: " + ret.msg);
                return ret;
            }
        }
        if (!BD3ESerialManage::instance()
                 ->getBD3ESerial(m_config.port)
                 ->SetSpeed(m_config.station, 0)) {
            std::lock_guard<std::mutex> locker(_status_mutex);
            m_status = MLBD3EStatus::MLBD3EStatus_Error;
            ret.code = false;
            ret.status = m_status;
            ret.msg = _msg + " BD3EMotor ML_MoveByDegreeSync stop failed";
            ML::MLSpdlog::instance()->error("BD3EMotor: " + ret.msg);
            return ret;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        if (std::abs(ML_GetCurrDegree() - degree) > 0.3) {
            std::lock_guard<std::mutex> locker(_status_mutex);
            m_status = MLBD3EStatus::MLBD3EStatus_Error;
            ret.code = false;
            ret.status = m_status;
            ret.msg = _msg + " BD3EMotor ML_MoveByDegreeSync move to " +
                      std::to_string(degree) + " degree failed";
            ML::MLSpdlog::instance()->error("BD3EMotor: " + ret.msg);
            return ret;
        }
        std::lock_guard<std::mutex> locker(_status_mutex);
        m_status = MLBD3EStatus::MLBD3EStatus_OK;
        ret.code = true;
        ret.status = m_status;
        ret.msg = _msg + " BD3EMotor ML_MoveByDegreeSync successfully";
        ML::MLSpdlog::instance()->info("BD3EMotor: " + ret.msg);
        return ret;
    } catch (const std::exception& e) {
        std::lock_guard<std::mutex> locker(_status_mutex);
        m_status = MLBD3EStatus::MLBD3EStatus_SerialException;
        ret.code = false;
        ret.status = m_status;
        ret.msg = _msg + " BD3EMotor ML_MoveByDegreeSync exception: " + e.what();
        ML::MLSpdlog::instance()->error("BD3EMotor: " + ret.msg);
        return ret;
    }
}

int BD3EMotor::ML_GetSpeed() {
    try {
        if (!ML_IsOpen()) {
            std::lock_guard<std::mutex> locker(_status_mutex);
            m_status = MLBD3EStatus::MLBD3EStatus_Error;
            return INT_MAX;
        }
        int speed = BD3ESerialManage::instance()
                        ->getBD3ESerial(m_config.port)
                        ->GetSpeed(m_config.station);
        if (speed == INT_MAX) {
            std::lock_guard<std::mutex> locker(_status_mutex);
            m_status = MLBD3EStatus::MLBD3EStatus_Error;
            return INT_MAX;
        }
        std::lock_guard<std::mutex> locker(_status_mutex);
        m_status = MLBD3EStatus::MLBD3EStatus_OK;
        return speed;
    } catch (const std::exception&) {
        std::lock_guard<std::mutex> locker(_status_mutex);
        m_status = MLBD3EStatus::MLBD3EStatus_SerialException;
        return INT_MAX;
    }
}

int BD3EMotor::ML_GetCurrPulse() {
    try {
        if (!ML_IsOpen()) {
            std::lock_guard<std::mutex> locker(_status_mutex);
            m_status = MLBD3EStatus::MLBD3EStatus_Error;
            return INT_MAX;
        }
        if (BD3ESerialManage::instance()
                ->getBD3ESerial(m_config.port)
                ->IsAlarm(m_config.station)) {
            std::lock_guard<std::mutex> locker(_status_mutex);
            m_status = MLBD3EStatus::MLBD3EStatus_Alarm;
            return INT_MAX;
        }
        int pulse = BD3ESerialManage::instance()
                        ->getBD3ESerial(m_config.port)
                        ->GetCurrPosition(m_config.station);
        if (pulse == INT_MAX) {
            std::lock_guard<std::mutex> locker(_status_mutex);
            m_status = MLBD3EStatus::MLBD3EStatus_Error;
            return INT_MAX;
        }
        std::lock_guard<std::mutex> locker(_status_mutex);
        m_status = MLBD3EStatus::MLBD3EStatus_OK;
        return pulse;
    } catch (const std::exception&) {
        std::lock_guard<std::mutex> locker(_status_mutex);
        m_status = MLBD3EStatus::MLBD3EStatus_SerialException;
        return INT_MAX;
    }
}

double BD3EMotor::ML_GetCurrDegree() {
    try {
        if (!ML_IsOpen()) {
            std::lock_guard<std::mutex> locker(_status_mutex);
            m_status = MLBD3EStatus::MLBD3EStatus_Error;
            return static_cast<double>(INT_MAX);
        }
        if (BD3ESerialManage::instance()
                ->getBD3ESerial(m_config.port)
                ->IsAlarm(m_config.station)) {
            std::lock_guard<std::mutex> locker(_status_mutex);
            m_status = MLBD3EStatus::MLBD3EStatus_Alarm;
            return static_cast<double>(INT_MAX);
        }
        int pulse = BD3ESerialManage::instance()
                        ->getBD3ESerial(m_config.port)
                        ->GetCurrPosition(m_config.station);
        if (pulse == INT_MAX) {
            std::lock_guard<std::mutex> locker(_status_mutex);
            m_status = MLBD3EStatus::MLBD3EStatus_Error;
            return static_cast<double>(INT_MAX);
        }
        double deg = pulse * 360.0 / m_config.PulsePerCycle;
        std::lock_guard<std::mutex> locker(_status_mutex);
        m_status = MLBD3EStatus::MLBD3EStatus_OK;
        return deg;
    } catch (const std::exception&) {
        std::lock_guard<std::mutex> locker(_status_mutex);
        m_status = MLBD3EStatus::MLBD3EStatus_SerialException;
        return static_cast<double>(INT_MAX);
    }
}

MLBD3EStatus BD3EMotor::ML_GetStatus() {
    std::lock_guard<std::mutex> locker(_status_mutex);
    return m_status;
}

MLBD3EResult BD3EMotor::ML_WaitForFinish(int timeout) {
    MLBD3EResult ret;
    try {
        int count = 0;
        if (ML_IsOpen()) {
            while (ML_IsMoving()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                count++;
                if (count * 200 > timeout) {
                    std::lock_guard<std::mutex> locker(_status_mutex);
                    m_status = MLBD3EStatus::MLBD3EStatus_Timeout;
                    ret.code = false;
                    ret.status = m_status;
                    ret.msg = _msg + " BD3EMotor ML_WaitForFinish timeout";
                    ML::MLSpdlog::instance()->warn("BD3EMotor: " + ret.msg);
                    return ret;
                }
            }
            std::lock_guard<std::mutex> locker(_status_mutex);
            m_status = MLBD3EStatus::MLBD3EStatus_Stationary;
            ret.code = true;
            ret.status = m_status;
            ret.msg = _msg + " BD3EMotor ML_WaitForFinish successfully";
            ML::MLSpdlog::instance()->info("BD3EMotor: " + ret.msg);
            return ret;
        }
        std::lock_guard<std::mutex> locker(_status_mutex);
        m_status = MLBD3EStatus::MLBD3EStatus_Error;
        ret.code = false;
        ret.status = m_status;
        ret.msg = _msg + " BD3EMotor ML_WaitForFinish error, not opened";
        ML::MLSpdlog::instance()->error("BD3EMotor: " + ret.msg);
        return ret;
    } catch (const std::exception& e) {
        std::lock_guard<std::mutex> locker(_status_mutex);
        m_status = MLBD3EStatus::MLBD3EStatus_SerialException;
        ret.code = false;
        ret.status = m_status;
        ret.msg = _msg + " BD3EMotor ML_WaitForFinish exception: " + e.what();
        ML::MLSpdlog::instance()->error("BD3EMotor: " + ret.msg);
        return ret;
    }
}
