// Copyright 2025 MLOptic

#include <string>
#include <vector>

#include "MLSpecbosCommon.h"
#include "dutmotioncontrol_global.h"

namespace ML {
    namespace MLSpecbos {
        class MLSpecbosConfigManage;
        class DUTMOTIONCONTROL_EXPORT MLSpecbosLogic {
        public:
            MLSpecbosLogic();

            ~MLSpecbosLogic();

            void Init(std::string config_path);

            bool SetDeviceType(DeviceType dtype);

            // check whether specbos is open
            bool Isopen();

            // open specbos
            bool Open(std::string port);

            // close specbos
            bool Close();

            // This function switches on the internal pilot laser of a specbos device.
            bool LaserOn();

            // This function switches off the internal pilot laser of a specbos device.
            bool LaserOff();

            // This function sets the count of measurement scans for average calculation
            bool SetAverage(WORD Average);

            // This function sets the maximum integration time which will be used for the next
            // auto-adapted measurement.
            // Allowed values are between 1000 and 60000 ms for specbos devices and between 400
            // and 6000 ms for spectraval devices.
            bool SetAutoTintConf(float fMaxTint = 60000);

            // This function sets the integration time for the next measurement.
            // The maximum value for integration time is 60000.0 ms.
            // The minimum value can be obtained by the function JETI_GetMinTintConf.
            bool SetFixedTintConf(float fTint);

            // This function gets the integration time configuration.
            float GetTintConf();

            // This function sets the sync frequency in [Hz].
            bool SetSyncFreq(float hz);

            // This function sets the sync mode.
            // The function JETI_SetSyncFreq must be used to set the sync frequency.
            bool SetSyncMode(BYTE syncMode);

            //
            bool SetCalibrationFileNumber(BYTE bCalibNr);

            // start a measure
            bool TakeMeasurement();

            bool TakeMeasurementAsy();

            // This function cancels an initiated measurement.
            bool StopMeasure();

            // get measurement result of X, Y, Z, x, y
            bool GetXYZxy(float& X, float& Y, float& Z, float& x, float& y, float& lum);

            MeasurementResults* GetMeasurementResults();

            std::string GetLastError();

            SpecbosDeviceInfo GetDeviceInfo();

            MeasurementConfig GetMeasurementConfig();

            PeripheralControl GetPeripheralControl();

            CalibrationInfo* GetCalibrationInfo();

            void ReCalAllValue(int begin, int end);

            void LoadCalibrationFile();

        private:
            MLSpecbosConfigManage* m_configManger;
            SpecbosDeviceInfo m_DeviceInfo;
            CalibrationInfo* m_CalibrationInfo;
            PeripheralControl m_peripheralControl;
            MeasurementConfig m_measurementConfig;
            MeasurementResults* m_measurementResults;
            std::vector<double> errorThreshold;

            std::string m_lastError;
            DeviceType m_dtype;
        };
    }  // namespace MLSpecbos
}  // namespace ML
