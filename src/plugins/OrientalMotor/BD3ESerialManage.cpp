// Copyright 2023 MLOptic

#include "BD3ESerialManage.h"
#include "MLSpdlog.h"
#include <utility>

BD3ESerialManage* BD3ESerialManage::m_instance = nullptr;
std::mutex BD3ESerialManage::m_bd3e_mutex;

BD3ESerialManage::BD3ESerialManage() {
    ML::MLSpdlog::instance()->info("BD3ESerialManage: create instance");
}

BD3ESerialManage* BD3ESerialManage::instance() {
    if (!m_instance) {
        static std::mutex mutex;
        std::unique_lock<std::mutex> locker(mutex);
        if (!m_instance) {
            m_instance = new BD3ESerialManage();
        }
    }
    return m_instance;
}

BD3ESerialManage::~BD3ESerialManage() {
    for (auto& it : m_SerialMap) {
        delete it.second;
    }
    std::map<std::string, BD3ESerial*>().swap(m_SerialMap);
    std::map<std::string, int>().swap(m_ConnectMap);
}

void BD3ESerialManage::createBD3ESerial(std::string port) {
    std::unique_lock<std::mutex> locker(m_bd3e_mutex);
    if (m_SerialMap.count(port) == 0) {
        BD3ESerial* new_serial = new BD3ESerial();
        m_SerialMap.insert(std::make_pair(port, new_serial));
        m_ConnectMap.insert(std::make_pair(port, 0));
    }
}

BD3ESerial* BD3ESerialManage::getBD3ESerial(std::string port) {
    std::unique_lock<std::mutex> locker(m_bd3e_mutex);
    for (auto it = m_SerialMap.begin(); it != m_SerialMap.end(); it++) {
        if (it->first == port) {
            return it->second;
        }
    }
    return nullptr;
}

void BD3ESerialManage::setConnection(std::string port) {
    std::unique_lock<std::mutex> locker(m_bd3e_mutex);
    std::map<std::string, int>::iterator iter = m_ConnectMap.find(port);
    if (iter != m_ConnectMap.end()) {
        int value = m_ConnectMap[port];
        m_ConnectMap[port] = value + 1;
    }
}

void BD3ESerialManage::closeConnection(std::string port) {
    std::unique_lock<std::mutex> locker(m_bd3e_mutex);
    std::map<std::string, int>::iterator iter = m_ConnectMap.find(port);
    if (iter != m_ConnectMap.end()) {
        int value = m_ConnectMap[port];
        m_ConnectMap[port] = value - 1;
    }
}

int BD3ESerialManage::getConnection(std::string port) {
    std::unique_lock<std::mutex> locker(m_bd3e_mutex);
    std::map<std::string, int>::iterator iter = m_ConnectMap.find(port);
    if (iter != m_ConnectMap.end()) {
        return m_ConnectMap[port];
    }
    return -1;
}
