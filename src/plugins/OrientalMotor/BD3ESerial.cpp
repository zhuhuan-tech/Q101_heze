// Copyright 2023 MLOptic
#include "BD3ESerial.h"

#include <algorithm>
#include <utility>

#include "MLSpdlog.h"

BD3ESerial::BD3ESerial() {
    m_Serial = new serial::Serial();
    ML::MLSpdlog::instance()->info("BD3ESerial: create instance");
}

BD3ESerial::~BD3ESerial() {
    if (m_Serial != nullptr) {
        delete m_Serial;
        m_Serial = nullptr;
    }
}

void BD3ESerial::InitSerialPort(CORE::MLSerialInfo serial_info) {
    m_serialInfo = serial_info;
    serial::Timeout timeout = serial::Timeout(100, 300, 300, 300);
    m_Serial->setTimeout(timeout);

    serial::bytesize_t byte = static_cast<serial::bytesize_t>(serial_info.bytesize);
    serial::parity_t parity = transformToParity(serial_info.parity);
    serial::stopbits_t stopbit = transformToStopBit(serial_info.stopbits);
    serial::flowcontrol_t flowcontrol =
        transformToFlowcontrol(serial_info.flowcontrol);

    m_Serial->setBaudrate(serial_info.baudrate);
    m_Serial->setBytesize(byte);
    m_Serial->setParity(parity);
    m_Serial->setStopbits(stopbit);
    m_Serial->setFlowcontrol(flowcontrol);
    ML::MLSpdlog::instance()->info("BD3ESerial: initialize serial port");
}

bool BD3ESerial::Open(std::string port) {
    try {
        if (m_Serial == nullptr) {
            return false;
        }
        m_Serial->setPort(port);
        if (m_Serial->isOpen()) {
            ML::MLSpdlog::instance()->info("BD3ESerial: port " + port +
                                           " is already opened");
            return true;
        }
        m_Serial->open();
        if (m_Serial->isOpen()) {
            ML::MLSpdlog::instance()->info("BD3ESerial: open serial");
            return true;
        }
        ML::MLSpdlog::instance()->error("BD3ESerial: open serial failed");
        return false;
    } catch (const std::exception) {
        ML::MLSpdlog::instance()->error("BD3ESerial: open serial exception");
        return false;
    }
}

bool BD3ESerial::Close() {
    if (m_Serial == nullptr) {
        return false;
    }
    if (m_Serial->isOpen()) {
        m_Serial->close();
        if (m_Serial->isOpen()) {
            return false;
        }
        ML::MLSpdlog::instance()->info("BD3ESerial: close serial");
        return true;
    }
    return false;
}

bool BD3ESerial::IsOpen() {
    if (m_Serial == nullptr)
        return false;
    return m_Serial->isOpen();
}

int BD3ESerial::QueryStatus(int SlaveId) {
    uint8_t sID = static_cast<uint8_t>(SlaveId);
    uint8_t writeData[] = {sID, 0x03, 0xD0, 0x1F, 0x00, 0x01};

    std::vector<uint8_t> readBuffer = SendAndReceive(writeData, sizeof(writeData), 7);
    if (!readBuffer.empty()) {
        int value = ((readBuffer[3] << 8) | readBuffer[4]);
        return value;
    }
    return INT_MAX;
}

int BD3ESerial::ReadAlarmCode(int SlaveId) {
    uint8_t sID = static_cast<uint8_t>(SlaveId);
    uint8_t writeData[] = {sID, 0x03, 0xD0, 0x20, 0x00, 0x01};

    std::vector<uint8_t> readBuffer = SendAndReceive(writeData, sizeof(writeData), 7);
    if (!readBuffer.empty()) {
        int alarm_code = ((readBuffer[3] << 8) | readBuffer[4]);
        ML::MLSpdlog::instance()->info(
            "BD3ESerial: read alarm code for slave station " +
            std::to_string(SlaveId) + ", code = " + std::to_string(alarm_code));
        return alarm_code;
    }
    return INT_MAX;
}

bool BD3ESerial::IsMoving(int SlaveId) {
    int status = QueryStatus(SlaveId);
    if (status == INT_MAX) {
        return false;
    }
    return (status == 2);
}

bool BD3ESerial::IsAlarm(int SlaveId) {
    int status = QueryStatus(SlaveId);
    if (status == INT_MAX) {
        return false;
    }
    return (status == 3);
}

bool BD3ESerial::CheckSerial(int SlaveId) {
    uint8_t sID = static_cast<uint8_t>(SlaveId);
    uint8_t writeData[] = {sID, 0x03, 0xD0, 0x1F, 0x00, 0x01};
    std::vector<uint8_t> readBuffer = SendAndReceive(writeData, sizeof(writeData), 7);
    return !readBuffer.empty();
}

bool BD3ESerial::ServoOn(int SlaveId) {
    int val = ReadParam16(SlaveId, 0x6401);
    if (val == INT_MAX || val == 0) {
        ML::MLSpdlog::instance()->info(
            "BD3ESerial: enable comm DI for slave " + std::to_string(SlaveId));
        if (!WriteParam16(SlaveId, 0x6401, 1)) {
            return false;
        }
        Sleep(10);
    }
    if (!WriteParam16(SlaveId, 0x6402, 0x0001)) {
        ML::MLSpdlog::instance()->error(
            "BD3ESerial: servo on failed for slave " + std::to_string(SlaveId));
        return false;
    }
    ML::MLSpdlog::instance()->info(
        "BD3ESerial: servo on for slave " + std::to_string(SlaveId));
    return true;
}

bool BD3ESerial::ServoOff(int SlaveId) {
    if (!WriteParam16(SlaveId, 0x6402, 0x0000)) {
        ML::MLSpdlog::instance()->error(
            "BD3ESerial: servo off failed for slave " + std::to_string(SlaveId));
        return false;
    }
    ML::MLSpdlog::instance()->info(
        "BD3ESerial: servo off for slave " + std::to_string(SlaveId));
    return true;
}

bool BD3ESerial::StopMove(int SlaveId) {
    if (!WriteParam16(SlaveId, 0x600E, 1)) {
        ML::MLSpdlog::instance()->error(
            "BD3ESerial: stop move P6-00E=1 failed for slave " +
            std::to_string(SlaveId));
        return false;
    }
    if (!WriteParam16(SlaveId, 0x600E, 0)) {
        ML::MLSpdlog::instance()->error(
            "BD3ESerial: stop move recovery P6-00E=0 failed for slave " +
            std::to_string(SlaveId));
        return false;
    }
    ML::MLSpdlog::instance()->info(
        "BD3ESerial: stop move for slave " + std::to_string(SlaveId));
    return true;
}

bool BD3ESerial::ClearAlarm(int SlaveId) {
    if (IsAlarm(SlaveId)) {
        uint8_t sID = static_cast<uint8_t>(SlaveId);
        uint8_t high = static_cast<uint8_t>((0xC005 & 0xFF00) >> 8);
        uint8_t low = static_cast<uint8_t>(0xC005 & 0x00FF);
        uint8_t writeData[] = {sID, 0x06, high, low, 0x00, 0x01};

        bool ret = SendAndCheckCmd(writeData, 8);
        if (ret) {
            ML::MLSpdlog::instance()->info(
                "BD3ESerial: clear alarm for slave station " +
                std::to_string(SlaveId));
            return true;
        }
        ML::MLSpdlog::instance()->error(
            "BD3ESerial: clear alarm failed for slave station " +
            std::to_string(SlaveId));
        return false;
    }
    return true;
}

int BD3ESerial::GetCurrPosition(int SlaveId) {
    uint8_t sID = static_cast<uint8_t>(SlaveId);
    uint8_t writeData[] = {sID, 0x03, 0xD0, 0x23, 0x00, 0x02};

    std::vector<uint8_t> readBuffer = SendAndReceive(writeData, sizeof(writeData), 9);
    if (!readBuffer.empty()) {
        uint32_t lowWord = (readBuffer[3] << 8) | readBuffer[4];
        uint32_t highWord = (readBuffer[5] << 8) | readBuffer[6];
        uint32_t pos = (highWord << 16) | lowWord;
        ML::MLSpdlog::instance()->info(
            "BD3ESerial: get current position for slave station " +
            std::to_string(SlaveId) + ", pos = " + std::to_string(pos));
        return static_cast<int>(pos);
    }
    return INT_MAX;
}

int BD3ESerial::GetSpeed(int SlaveId) {
    uint8_t sID = static_cast<uint8_t>(SlaveId);
    uint8_t writeData[] = {sID, 0x03, 0xD0, 0x01, 0x00, 0x01};

    std::vector<uint8_t> readBuffer = SendAndReceive(writeData, sizeof(writeData), 7);
    if (!readBuffer.empty()) {
        int16_t speed = ((readBuffer[3] << 8) | readBuffer[4]);
        return static_cast<int>(speed);
    }
    return INT_MAX;
}

bool BD3ESerial::SetSpeed(int SlaveId, int rpm) {
    uint8_t sID = static_cast<uint8_t>(SlaveId);
    uint8_t high = static_cast<uint8_t>((rpm & 0xFF00) >> 8);
    uint8_t low = static_cast<uint8_t>(rpm & 0x00FF);
    uint8_t writeData[] = {sID, 0x06, 0x41, 0x02, high, low};

    bool ret = SendAndCheckCmd(writeData, 8);
    if (ret) {
        ML::MLSpdlog::instance()->info(
            "BD3ESerial: set speed " + std::to_string(rpm) +
            " rpm for slave " + std::to_string(SlaveId));
    } else {
        ML::MLSpdlog::instance()->error(
            "BD3ESerial: set speed failed for slave " + std::to_string(SlaveId));
    }
    return ret;
}

bool BD3ESerial::MoveToTarget(int SlaveId, int targetPulse) {
    if (!WriteParam32(SlaveId, 0x5305, static_cast<uint32_t>(targetPulse))) {
        ML::MLSpdlog::instance()->error(
            "BD3ESerial: MoveToTarget write P5-305 failed for slave " +
            std::to_string(SlaveId));
        return false;
    }
    if (!WriteParam16(SlaveId, 0x6004, 1)) {
        ML::MLSpdlog::instance()->error(
            "BD3ESerial: MoveToTarget write P6-004=1 failed for slave " +
            std::to_string(SlaveId));
        return false;
    }
    if (!WriteParam16(SlaveId, 0x6004, 0)) {
        ML::MLSpdlog::instance()->error(
            "BD3ESerial: MoveToTarget write P6-004=0 failed for slave " +
            std::to_string(SlaveId));
        return false;
    }
    ML::MLSpdlog::instance()->info(
        "BD3ESerial: move to target " + std::to_string(targetPulse) +
        " pulse for slave " + std::to_string(SlaveId));
    return true;
}

bool BD3ESerial::SetControlMode(int SlaveId, uint16_t mode) {
    return WriteParam16(SlaveId, 0x0101, mode);
}

bool BD3ESerial::SetAccelTime(int SlaveId, uint16_t ms) {
    return WriteParam16(SlaveId, 0x4103, ms);
}

bool BD3ESerial::SetDecelTime(int SlaveId, uint16_t ms) {
    return WriteParam16(SlaveId, 0x4104, ms);
}

bool BD3ESerial::WriteParam16(int SlaveId, uint16_t address, uint16_t value) {
    uint8_t sID = static_cast<uint8_t>(SlaveId);
    uint8_t addrHigh = static_cast<uint8_t>((address & 0xFF00) >> 8);
    uint8_t addrLow = static_cast<uint8_t>(address & 0x00FF);
    uint8_t valHigh = static_cast<uint8_t>((value & 0xFF00) >> 8);
    uint8_t valLow = static_cast<uint8_t>(value & 0x00FF);
    uint8_t writeData[] = {sID, 0x06, addrHigh, addrLow, valHigh, valLow};

    return SendAndCheckCmd(writeData, 8);
}

bool BD3ESerial::WriteParam32(int SlaveId, uint16_t address, uint32_t value) {
    uint8_t sID = static_cast<uint8_t>(SlaveId);
    uint8_t addrHigh = static_cast<uint8_t>((address & 0xFF00) >> 8);
    uint8_t addrLow = static_cast<uint8_t>(address & 0x00FF);
    uint16_t lowWord = static_cast<uint16_t>(value & 0xFFFF);
    uint16_t highWord = static_cast<uint16_t>((value >> 16) & 0xFFFF);
    uint8_t lowH = static_cast<uint8_t>((lowWord & 0xFF00) >> 8);
    uint8_t lowL = static_cast<uint8_t>(lowWord & 0x00FF);
    uint8_t highH = static_cast<uint8_t>((highWord & 0xFF00) >> 8);
    uint8_t highL = static_cast<uint8_t>(highWord & 0x00FF);
    uint8_t writeData[] = {sID, 0x10, addrHigh, addrLow,
                           0x00, 0x02, 0x04,
                           lowH, lowL, highH, highL};

    return SendAndCheckCmd(writeData, 8);
}

int BD3ESerial::ReadParam16(int SlaveId, uint16_t address) {
    uint8_t sID = static_cast<uint8_t>(SlaveId);
    uint8_t addrHigh = static_cast<uint8_t>((address & 0xFF00) >> 8);
    uint8_t addrLow = static_cast<uint8_t>(address & 0x00FF);
    uint8_t writeData[] = {sID, 0x03, addrHigh, addrLow, 0x00, 0x01};

    std::vector<uint8_t> readBuffer = SendAndReceive(writeData, sizeof(writeData), 7);
    if (!readBuffer.empty()) {
        int value = ((readBuffer[3] << 8) | readBuffer[4]);
        return value;
    }
    return INT_MAX;
}

int BD3ESerial::ReadParam32(int SlaveId, uint16_t address) {
    uint8_t sID = static_cast<uint8_t>(SlaveId);
    uint8_t addrHigh = static_cast<uint8_t>((address & 0xFF00) >> 8);
    uint8_t addrLow = static_cast<uint8_t>(address & 0x00FF);
    uint8_t writeData[] = {sID, 0x03, addrHigh, addrLow, 0x00, 0x02};

    std::vector<uint8_t> readBuffer = SendAndReceive(writeData, sizeof(writeData), 9);
    if (!readBuffer.empty()) {
        uint32_t lowWord = (readBuffer[3] << 8) | readBuffer[4];
        uint32_t highWord = (readBuffer[5] << 8) | readBuffer[6];
        uint32_t value = (highWord << 16) | lowWord;
        return static_cast<int>(value);
    }
    return INT_MAX;
}

bool BD3ESerial::WriteRegister(const std::vector<uint8_t>& data) {
    try {
        if (m_Serial == nullptr)
            return false;
        if (!m_Serial->isOpen())
            return false;
        if (data.empty()) {
            return false;
        }
        std::lock_guard<std::mutex> locker(mutex_);
        SetTXEnable(true);
        Sleep(1);
        size_t bytes_wrote = m_Serial->write(data);
        size_t waitMs = data.size() * 10 * 1000 / m_serialInfo.baudrate + 5;
        Sleep(static_cast<DWORD>(waitMs > 0 ? waitMs : 10));
        SetTXEnable(false);
        if (bytes_wrote == data.size()) {
            return true;
        }
        return false;
    } catch (const std::exception&) {
        return false;
    }
}

std::vector<uint8_t> BD3ESerial::ReadRegister(size_t size) {
    try {
        std::vector<uint8_t> read_buffer;
        size_t bytes_readed;
        std::lock_guard<std::mutex> locker(mutex_);
        bytes_readed = m_Serial->read(read_buffer, size);
        if (bytes_readed == size) {
            return read_buffer;
        } else {
            return std::vector<uint8_t>();
        }
    } catch (const std::exception&) {
        return std::vector<uint8_t>();
    }
}

uint16_t BD3ESerial::CRC16_Modbus(uint8_t data[], size_t data_size) {
    uint16_t CRC = 0xFFFF;
    for (size_t i = 0; i < data_size; i++) {
        CRC = CRC ^ data[i];
        for (int j = 0; j < 8; j++) {
            if (CRC & 1) {
                CRC >>= 1;
                CRC ^= 0xA001;
            } else {
                CRC >>= 1;
            }
        }
    }
    return CRC;
}

std::vector<uint8_t> BD3ESerial::ConvertCmd2(uint8_t data[], int dataLen) {
    uint16_t crc = CRC16_Modbus(data, static_cast<size_t>(dataLen));

    uint8_t crcLow = static_cast<uint8_t>(crc & 0x00FF);
    uint8_t crcHigh = static_cast<uint8_t>((crc & 0xFF00) >> 8);

    std::vector<uint8_t> cmdData;
    for (int i = 0; i < dataLen; i++) {
        cmdData.push_back(data[i]);
    }
    cmdData.push_back(crcLow);
    cmdData.push_back(crcHigh);

    return cmdData;
}

std::vector<uint8_t> BD3ESerial::SendCmd(uint8_t data[], int dataLen) {
    try {
        if (m_Serial == nullptr || !m_Serial->isOpen())
            return std::vector<uint8_t>();
        std::vector<uint8_t> cmdData = ConvertCmd2(data, dataLen);
        if (cmdData.empty())
            return std::vector<uint8_t>();
        std::lock_guard<std::mutex> locker(mutex_);
        Sleep(20);
        SetTXEnable(true);
        Sleep(1);
        m_Serial->flushOutput();
        size_t bytes_wrote = m_Serial->write(cmdData);
        size_t waitMs = cmdData.size() * 10 * 1000 / m_serialInfo.baudrate + 5;
        Sleep(static_cast<DWORD>(waitMs > 0 ? waitMs : 10));
        SetTXEnable(false);
        return (bytes_wrote == cmdData.size()) ? cmdData : std::vector<uint8_t>();
    } catch (const std::exception&) {
        return std::vector<uint8_t>();
    }
}

std::vector<uint8_t> BD3ESerial::CmdResult(uint8_t checkData[],
                                           size_t read_size) {
    try {
        if (m_Serial == nullptr || !m_Serial->isOpen())
            return std::vector<uint8_t>();
        SetTXEnable(false);
        Sleep(50);
        std::vector<uint8_t> read_buffer;
        size_t bytes_readed = m_Serial->read(read_buffer, read_size);
        if (bytes_readed != read_size)
            return std::vector<uint8_t>();
        uint8_t funcCode = *(checkData + 1);
        if (funcCode == 0x06) {
            for (size_t i = 0; i < 6; i++) {
                if (*(checkData + i) != read_buffer[i])
                    return std::vector<uint8_t>();
            }
            return read_buffer;
        } else if (funcCode == 0x10) {
            for (size_t i = 0; i < 6; i++) {
                if (*(checkData + i) != read_buffer[i])
                    return std::vector<uint8_t>();
            }
            return read_buffer;
        }
        return read_buffer;
    } catch (const std::exception&) {
        return std::vector<uint8_t>();
    }
}

std::vector<uint8_t> BD3ESerial::SendAndReceive(
    uint8_t data[], int dataLen, size_t readSize) {
    try {
        if (m_Serial == nullptr || !m_Serial->isOpen())
            return std::vector<uint8_t>();

        std::vector<uint8_t> cmdData = ConvertCmd2(data, dataLen);
        if (cmdData.empty())
            return std::vector<uint8_t>();

        std::lock_guard<std::mutex> locker(mutex_);
        Sleep(20);
        SetTXEnable(true);
        Sleep(1);
        m_Serial->flushOutput();
        size_t bytes_wrote = m_Serial->write(cmdData);
        size_t waitMs = cmdData.size() * 10 * 1000 / m_serialInfo.baudrate + 5;
        Sleep(static_cast<DWORD>(waitMs > 0 ? waitMs : 10));
        SetTXEnable(false);
        if (bytes_wrote != cmdData.size())
            return std::vector<uint8_t>();

        Sleep(50);
        std::vector<uint8_t> read_buffer;
        size_t bytes_readed = m_Serial->read(read_buffer, readSize);
        if (bytes_readed != readSize)
            return std::vector<uint8_t>();

        uint8_t funcCode = data[1];
        if (funcCode == 0x06 || funcCode == 0x10) {
            for (size_t i = 0; i < 6; i++) {
                if (data[i] != read_buffer[i])
                    return std::vector<uint8_t>();
            }
            return read_buffer;
        } else if (funcCode == 0x03) {
            uint16_t expectedCrc = CRC16_Modbus(
                read_buffer.data(), bytes_readed - 2);
            uint8_t crcLow = static_cast<uint8_t>(expectedCrc & 0x00FF);
            uint8_t crcHigh = static_cast<uint8_t>((expectedCrc & 0xFF00) >> 8);
            if (crcLow == read_buffer[bytes_readed - 2] &&
                crcHigh == read_buffer[bytes_readed - 1]) {
                return read_buffer;
            }
            return std::vector<uint8_t>();
        }
        return read_buffer;
    } catch (const std::exception&) {
        return std::vector<uint8_t>();
    }
}

bool BD3ESerial::SendAndCheckCmd(uint8_t data[], size_t read_size) {
    uint8_t funcCode = data[1];
    int dataLen = (funcCode == 0x10) ? 11 : 6;
    std::vector<uint8_t> readBuffer = SendAndReceive(data, dataLen, read_size);
    return !readBuffer.empty();
}

void BD3ESerial::SetTXEnable(bool enable) {
    if (m_Serial == nullptr)
        return;
    try {
        m_Serial->setRTS(enable);
    } catch (const std::exception&) {
    }
}

serial::parity_t BD3ESerial::transformToParity(std::string parity) {
    transform(parity.begin(), parity.end(), parity.begin(), ::tolower);
    if (parity.find("none") != std::string::npos) {
        return serial::parity_none;
    } else if (parity.find("odd") != std::string::npos) {
        return serial::parity_odd;
    } else if (parity.find("even") != std::string::npos) {
        return serial::parity_even;
    } else if (parity.find("mark") != std::string::npos) {
        return serial::parity_mark;
    } else if (parity.find("space") != std::string::npos) {
        return serial::parity_space;
    } else {
        return serial::parity_none;
    }
}

serial::stopbits_t BD3ESerial::transformToStopBit(std::string stopbit) {
    transform(stopbit.begin(), stopbit.end(), stopbit.begin(), ::tolower);
    if (stopbit.find("onestop") != std::string::npos) {
        return serial::stopbits_one;
    } else if (stopbit.find("twostop") != std::string::npos) {
        return serial::stopbits_two;
    } else if (stopbit.find("onepointfive") != std::string::npos) {
        return serial::stopbits_one_point_five;
    }
    return serial::stopbits_one;
}

serial::flowcontrol_t BD3ESerial::transformToFlowcontrol(
    std::string flowcontrol) {
    transform(flowcontrol.begin(), flowcontrol.end(), flowcontrol.begin(),
              ::tolower);
    if (flowcontrol.find("none") != std::string::npos) {
        return serial::flowcontrol_none;
    } else if (flowcontrol.find("software") != std::string::npos) {
        return serial::flowcontrol_software;
    } else if (flowcontrol.find("hardware") != std::string::npos) {
        return serial::flowcontrol_hardware;
    }
    return serial::flowcontrol_none;
}
