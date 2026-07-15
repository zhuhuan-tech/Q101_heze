#pragma once
#include "json.hpp"
#include "ml.h"
#include <QObject>

using json = nlohmann::ordered_json;
using namespace std;

struct BD3EConfiguation {
    std::string type = "BD3E";
    std::string name = "BD3E Motor";
    std::string protocol = "modbus/serial";
    std::string motorType = "BD3E";
    std::string port;
    int station = 1;
    CORE::MLSerialInfo serial_info;
    int PulsePerCycle = 10000;
    int MoveSpeed = 300;
    int Min = -360;
    int Max = 360;
};

class BD3EMotorConfig {
  public:
    BD3EMotorConfig();
    ~BD3EMotorConfig();
    static BD3EMotorConfig* instance();

  public:
    Result ReadConfig();
    BD3EConfiguation GetBD3EConfig(int type);
    std::map<int, BD3EConfiguation> GetBD3EConfigMap();

  private:
    static BD3EMotorConfig* m_instance;
    std::string m_ConfigPath = "./config/device/BD3EMotorConfig.json";
    std::map<int, BD3EConfiguation> m_BD3EConfigMap;
};
