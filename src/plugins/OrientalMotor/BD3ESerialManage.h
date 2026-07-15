// Copyright 2023 MLOptic

#pragma once

#include <map>
#include <string>

#include "BD3ESerial.h"

class BD3ESerialManage {
 public:
    static BD3ESerialManage* instance();
    ~BD3ESerialManage();

    void createBD3ESerial(std::string port);

    BD3ESerial* getBD3ESerial(std::string port);

    void setConnection(std::string port);
    void closeConnection(std::string port);
    int  getConnection(std::string port);

 private:
    BD3ESerialManage();

 private:
    std::map<std::string, BD3ESerial*> m_SerialMap;
    std::map<std::string, int> m_ConnectMap;
    static BD3ESerialManage* m_instance;
    static std::mutex m_bd3e_mutex;
};
