// Copyright 2026 MLOptic
#pragma once

#include <Windows.h>
#include <serial.h>
#include "ml.h"

#include <map>
#include <mutex>
#include <string>
#include <vector>

using namespace CORE;

class BD3ESerial {
 public:
    BD3ESerial();
    ~BD3ESerial();

    void InitSerialPort(CORE::MLSerialInfo serial_info = CORE::MLSerialInfo());

    bool Open(std::string port);
    bool Close();
    bool IsOpen();

    bool IsMoving(int SlaveId);
    bool IsAlarm(int SlaveId);
    bool CheckSerial(int SlaveId);

    bool ServoOn(int SlaveId);
    bool ServoOff(int SlaveId);
    bool StopMove(int SlaveId);
    bool ClearAlarm(int SlaveId);
    int  ReadAlarmCode(int SlaveId);

    int  GetCurrPosition(int SlaveId);
    int  GetSpeed(int SlaveId);
    bool SetSpeed(int SlaveId, int rpm);
    bool MoveToTarget(int SlaveId, int targetPulse, int rpm);

    bool WriteParam16(int SlaveId, uint16_t address, uint16_t value);
    bool WriteParam32(int SlaveId, uint16_t address, uint32_t value);
    int  ReadParam16(int SlaveId, uint16_t address);
    int  ReadParam32(int SlaveId, uint16_t address);

    bool WriteRegister(const std::vector<uint8_t>& data);
    std::vector<uint8_t> ReadRegister(size_t size);

 private:
    int  QueryStatus(int SlaveId);

    uint16_t CRC16_Modbus(uint8_t data[], size_t data_size);
    std::vector<uint8_t> ConvertCmd2(uint8_t data[], int dataLen);

    std::vector<uint8_t> SendCmd(uint8_t data[], int dataLen);
    std::vector<uint8_t> CmdResult(uint8_t checkData[], size_t read_size);
    std::vector<uint8_t> SendAndReceive(uint8_t data[], int dataLen, size_t readSize);
    bool SendAndCheckCmd(uint8_t data[], size_t read_size);

    void SetTXEnable(bool enable);

    serial::parity_t transformToParity(std::string parity);
    serial::stopbits_t transformToStopBit(std::string stopbit);
    serial::flowcontrol_t transformToFlowcontrol(std::string flowcontrol);

 private:
    serial::Serial* m_Serial;
    CORE::MLSerialInfo m_serialInfo;
    std::mutex mutex_;
};
