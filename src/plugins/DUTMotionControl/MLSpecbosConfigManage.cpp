#include "MLSpecbosConfigManage.h"

#include "ini.h"

using namespace ML::MLSpecbos;

ML::MLSpecbos::MLSpecbosConfigManage::MLSpecbosConfigManage() {
    m_calibrationInfo = new ML::MLSpecbos::CalibrationInfo();
}

ML::MLSpecbos::MLSpecbosConfigManage::~MLSpecbosConfigManage() {}

bool ML::MLSpecbos::MLSpecbosConfigManage::Init(std::string config_path) {
    m_configPath = config_path;
    std::map<std::string, std::map<std::string, std::string>> config;
    if (!readIniFile(config_path, config)) {
        return false;
    }
    std::string session = "ErrorThreshold";
    std::string X, Y, Z;
    X = config[session]["X"];
    Y = config[session]["Y"];
    Z = config[session]["Z"];
    m_errorThreshold.push_back(std::stod(X));
    m_errorThreshold.push_back(std::stod(Y));
    m_errorThreshold.push_back(std::stod(Z));

    for (int i = 1; i <= MAX_CALIBRATION_FILE_NUM; i++) {
        std::string template_mode, template_remark, mode, remark;
        template_mode = std::to_string(i) + "%23Mode";
        template_remark = std::to_string(i) + "%23Remark";

        mode = config["CalibrationFile"][template_mode];
        remark = config["CalibrationFile"][template_remark];
        memcpy(m_calibrationInfo->calibrationFiles[i - 1].cMode, mode.c_str(),
            sizeof(mode));
        memcpy(m_calibrationInfo->calibrationFiles[i - 1].cRemark, remark.c_str(),
            sizeof(remark));
    }
    m_iniConfig = config;
    return true;
}

SpecbosDeviceInfo ML::MLSpecbos::MLSpecbosConfigManage::GetSpecbosDeviceInfo() {
    return m_deviceInfo;
}

std::vector<double> ML::MLSpecbos::MLSpecbosConfigManage::GetErrorThreshold() {
    return m_errorThreshold;
}

bool ML::MLSpecbos::MLSpecbosConfigManage::SetErrorThreshold(
    std::vector<double> errorThreshold) {
    std::string X, Y, Z;
    X = std::to_string(errorThreshold[0]);
    Y = std::to_string(errorThreshold[1]);
    Z = std::to_string(errorThreshold[2]);
    m_iniConfig["ErrorThreshold"]["X"] = X;
    m_iniConfig["ErrorThreshold"]["Y"] = Y;
    m_iniConfig["ErrorThreshold"]["Z"] = Z;
    writeIniFile(m_configPath, m_iniConfig);
    return true;
}

CalibrationInfo* ML::MLSpecbos::MLSpecbosConfigManage::GetCalibrationInfo() {
    return m_calibrationInfo;
}

bool ML::MLSpecbos::MLSpecbosConfigManage::SetCalibrationInfo(
    CalibrationInfo* calibrationInfo) {
    m_calibrationInfo = calibrationInfo;
    std::string session2 = "CalibrationFile";
    for (int i = 1; i <= MAX_CALIBRATION_FILE_NUM; i++) {
        std::string template_mode, template_remark, mode, remark;
        template_mode = std::to_string(i) + "%23Mode";
        template_remark = std::to_string(i) + "%23Remark";

        mode = m_calibrationInfo->calibrationFiles[i - 1].cMode;
        remark = m_calibrationInfo->calibrationFiles[i - 1].cRemark;

        m_iniConfig[session2][template_mode] = mode;
        m_iniConfig[session2][template_remark] = remark;
    }
    if (!writeIniFile(m_configPath, m_iniConfig)) {
        return false;
    }
    return true;
}

int ML::MLSpecbos::MLSpecbosConfigManage::iniHandler(void* user, const char* section,
    const char* name,
    const char* value) {
    std::map<std::string, std::map<std::string, std::string>>* config =
        (std::map<std::string, std::map<std::string, std::string>>*)user;
    std::string key = std::string(name);
    std::string session = std::string(section);
    (*config)[session][key] = value;
    return 1;
}

bool ML::MLSpecbos::MLSpecbosConfigManage::readIniFile(
    const std::string& filename,
    std::map<std::string, std::map<std::string, std::string>>& config) {
    if (ini_parse(filename.c_str(), iniHandler, &config) < 0) {
        return false;
    }
    return true;
}

bool ML::MLSpecbos::MLSpecbosConfigManage::writeIniFile(
    const std::string& filename,
    const std::map<std::string, std::map<std::string, std::string>>& config) {
    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        return false;
    }

    std::string currentSection;
    for (const auto& pair : config) {
        std::string section = pair.first;
        if (section != currentSection) {
            outFile << "[" << section << "]" << std::endl;
            currentSection = section;
        }
        for (const auto& session : pair.second) {
            outFile << session.first << "=" << session.second << std::endl;
        }
    }

    outFile.close();
    return true;
}
