#include "BD3EMotorConfig.h"
#include <mutex>
#include <fstream>
#include <QMessageBox>
#include "loggingwrapper.h"

BD3EMotorConfig* BD3EMotorConfig::m_instance = nullptr;

BD3EMotorConfig::BD3EMotorConfig() {
    Result ret = ReadConfig();
    if (!ret.success) {
        QMessageBox::warning(NULL, "Error",
            QString::fromStdString(ret.errorMsg), QMessageBox::Yes);
    }
}

BD3EMotorConfig::~BD3EMotorConfig() {
}

BD3EMotorConfig* BD3EMotorConfig::instance() {
    if (!m_instance) {
        static std::mutex mutex;
        std::unique_lock<std::mutex> locker(mutex);
        if (!m_instance) {
            m_instance = new BD3EMotorConfig();
        }
    }
    return m_instance;
}

Result BD3EMotorConfig::ReadConfig() {
    string message;
    try {
        m_BD3EConfigMap.clear();
        json m_JsonControl;
        std::ifstream ifs(m_ConfigPath.c_str(), std::ifstream::in);
        if (ifs.fail()) {
            message = "BD3EMotorConfig: load " + m_ConfigPath + " failed";
            LoggingWrapper::instance()->error(
                QString::fromStdString(message));
            return Result(false, message);
        }
        ifs >> m_JsonControl;

        if (m_JsonControl.contains("BD3EMotorMap")) {
            for (auto& it : m_JsonControl["BD3EMotorMap"].items()) {
                BD3EConfiguation config;
                std::string key = it.value()["Key"].get<string>();
                config.type = it.value()["Type"].get<string>();
                config.name = it.value()["Name"].get<string>();
                config.protocol = it.value()["Protocol"].get<string>();
                config.motorType = it.value()["MotorType"].get<string>();
                config.port = it.value()["Port"].get<string>();
                config.station = it.value()["Station"].get<int>();
                config.serial_info.baudrate =
                    it.value()["SerialInfo"]["BaudRate"].get<int>();
                config.serial_info.bytesize =
                    it.value()["SerialInfo"]["ByteSize"].get<int>();
                config.serial_info.parity =
                    it.value()["SerialInfo"]["Parity"].get<string>();
                config.serial_info.stopbits =
                    it.value()["SerialInfo"]["StopBit"].get<string>();
                config.serial_info.flowcontrol =
                    it.value()["SerialInfo"]["FlowControl"].get<string>();
                config.PulsePerCycle =
                    it.value()["PulsePerCycle"].get<int>();
                config.MoveSpeed =
                    it.value()["MoveSpeed"].get<int>();
                config.Min = it.value()["Min"].get<int>();
                config.Max = it.value()["Max"].get<int>();
                config.ZeroPulse = it.value()["ZeroPulse"].get<int>();

                int type;
                if (key == "X")
                    type = 0;
                else if (key == "Y")
                    type = 1;
                else
                    type = 2;
                m_BD3EConfigMap[type] = config;
            }
        }
        return Result();
    } catch (const std::exception e) {
        std::string ex = e.what();
        message = "BD3EMotorConfig: read config exception, " + ex;
        LoggingWrapper::instance()->error(QString::fromStdString(message));
        return Result(false, message);
    }
}

BD3EConfiguation BD3EMotorConfig::GetBD3EConfig(int type) {
    if (m_BD3EConfigMap.count(type)) {
        return m_BD3EConfigMap[type];
    } else {
        return BD3EConfiguation();
    }
}

std::map<int, BD3EConfiguation> BD3EMotorConfig::GetBD3EConfigMap() {
    return m_BD3EConfigMap;
}
