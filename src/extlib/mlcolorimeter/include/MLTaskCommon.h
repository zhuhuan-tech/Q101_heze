#pragma once
#include <QObject>
#include "MLColorimeterCommon.h"
#include "MLProcessorCommon.h"
#include "AECommon.h"
using namespace ML::MLColorimeter;
using namespace ML::MLFilterWheel;
using namespace ML::CameraV2;
using namespace ML::AECommon;

namespace ML_Task {
struct ExposureCheckParam {
    bool isExposureCheck = false;
    int topN = 1000;
    int threadCount = 4;
    int minGray = 0;
    double over_ratio = 0.9;
    double low_ratio = 0.6;
};


struct ROIExposureParam {
    bool isUseROIs;
    int pixelCount;
    std::map<std::string, ROIS> rois;
};

struct ImageCaptureConfig {
    std::string aperture;
    std::string lightSource;
    Binning binning;
    MLFilterEnum ndFilter;
    std::map<::MLFilterEnum, ExposureSetting> colorFilterToExposureMap;
    CalibrationFlag calibrationFlag;
    SaveDataMeta saveDataMeta;
    bool isSaveCali = false;
    bool isSaveRaw = false;
    bool is_undistort_method = false;
    bool is_use_darklist = false;
    HDRConfig hdrConfig;
    DUT_Type dutType = DUT_Type::Left;
    ExposureCheckParam exposureCheckParam;
    ROIExposureParam roiExposureParam;
};

struct ImageAlgoMetaData {
    int binning;
    std::string dutId;
    std::string eyeboxId;
    std::string lightSource;  //"R/G/B/W"
    std::string imageType;    //"Solid/Checkerboard/XHI/Flare/Ghost/Grid"
    std::string imageName;
    cv::Mat image;
};
Q_DECLARE_METATYPE(ImageAlgoMetaData)

}  // namespace ML_Task