#pragma once
#include "Result.h"
#include "mltaskmanager_global.h"
#include "TaskEventCallback.h"

class MLTASKMANAGER_EXPORT CaliDataCallback : public ProcessEventCallback {
 public:
    CaliDataCallback();

    ~CaliDataCallback();

    void started(std::string) override;

    void finished(ProcessorData& data) override;

    void error(std::string) override;

    Result saveProcessedData(CalibrationData data, SaveDataMeta saveconfig);

};