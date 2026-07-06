// Copyright 2025 MLOptic

#ifndef MLSPECBOSCONFIGMANAGE_H
#define MLSPECBOSCONFIGMANAGE_H

#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "MLSpecbosCommon.h"

namespace ML {
    namespace MLSpecbos {
        class MLSpecbosConfigManage {
        public:
            MLSpecbosConfigManage();
            ~MLSpecbosConfigManage();

            bool Init(std::string config_path);

            SpecbosDeviceInfo GetSpecbosDeviceInfo();

            std::vector<double> GetErrorThreshold();

            bool SetErrorThreshold(std::vector<double> errorThreshold);

            CalibrationInfo* GetCalibrationInfo();

            bool SetCalibrationInfo(CalibrationInfo* calibrationInfo);

            static int iniHandler(void* user, const char* section, const char* name,
                const char* value);

            bool readIniFile(const std::string& filename,
                std::map<std::string, std::map<std::string, std::string>>& config);

            bool writeIniFile(
                const std::string& filename,
                const std::map<std::string, std::map<std::string, std::string>>& config);

        private:
            std::string m_configPath;
            SpecbosDeviceInfo m_deviceInfo;
            std::vector<double> m_errorThreshold;
            CalibrationInfo* m_calibrationInfo;
            std::map<std::string, std::map<std::string, std::string>> m_iniConfig;
        };
    }  // namespace MLSpecbos
}  // namespace ML

#endif  // MLSPECBOSCONFIGMANAGE_H

