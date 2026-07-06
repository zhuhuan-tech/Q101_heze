#include "AutoExposureAlgorithm.h"
mutex m_mutex1;
AutoExposureAlgorithm::AutoExposureAlgorithm(std::string cameraNameFromJson, std::string autoExposurePath, std::string cameraNamePrint, int initTime, bool isManualInitTime)
{
    m_cameraNameFromJson = cameraNameFromJson;
    m_autoExposurePath = autoExposurePath;
    m_cameraNamePrint = cameraNamePrint;
    maunalInitTime = initTime;
    this->isManualInitTime = isManualInitTime;

    m_AEConfig = m_ReadWriteConfig.ReadAEConfig(m_autoExposurePath);
}

AutoExposureAlgorithm::~AutoExposureAlgorithm()
{

}

bool AutoExposureAlgorithm::IsUseROI()
{
    return isUseROIs;
}

map<string, vector<double>> AutoExposureAlgorithm::GetROIS()
{
    return rois;
}

void AutoExposureAlgorithm::correct_et_for_max_min(int& ET, int ET_Max, int ET_Min)
{
    string msg;
    if (ET > ET_Max)
    {
        ET = ET_Max;

        msg = m_cameraNamePrint + ": Exposure time exceeds the maximum value, correct the exposure time to the current maximum value: " + to_string(ET);
    }
    else if (ET < ET_Min)
    {
        ET = ET_Min;

        msg = m_cameraNamePrint + ": Exposure time exceeds the minimum value, correct the exposure time to the current minimum value: " + to_string(ET);
    }
    else
    {
        msg = m_cameraNamePrint + ": Exposure time between maximum and minimum values: " + to_string(ET);
    }

    LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());
}

int AutoExposureAlgorithm::correct_et_for_fps(int ET, int fps)
{
    string msg;
    msg = m_cameraNamePrint + ": current exposure time: " + to_string(ET);
    LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());

    if (ET >= timeBefore)
    {
        if (ET <= m_etMin)
        {
            if (timeBefore == m_etMin)
            {
                timeBefore = (int)(1000000 / fps) + timeBefore;
            }
            else
            {
                timeBefore = m_etMin;
            }
            msg = m_cameraNamePrint + ": correct et for fps:" + to_string(timeBefore);
            LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());
            return timeBefore;
        }

        if (ET >= m_etMax)
        {
            timeBefore = m_etMax;

            msg = m_cameraNamePrint + ": correct et for fps:" + to_string(timeBefore);
            LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());
            return timeBefore;
        }

        int remainder = ET % (int)(1000000 / fps);
        if (remainder != 0)
        {
            ET = ET - remainder;
        }
        if (timeBefore == ET)
        {
            timeBefore = timeBefore + (int)(1000000 / fps);
            ET = timeBefore;
        }
        else
        {
            timeBefore = ET;
        }

        msg = m_cameraNamePrint + ": correct et for fps:" + to_string(timeBefore);
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());
        return ET;
    }
    else
    {
        if (ET <= m_etMin)
        {
            timeBefore = m_etMin;

            msg = m_cameraNamePrint + ": correct et for fps:" + to_string(timeBefore);
            LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());
            return timeBefore;
        }

        if (ET >= m_etMax)
        {
            timeBefore = m_etMax;

            msg = m_cameraNamePrint + ": correct et for fps:" + to_string(timeBefore);
            LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());
            return timeBefore;
        }

        int remainder = ET % (int)(1000000 / fps);
        if (remainder != 0)
        {
            ET = ET - remainder;
        }
        if (timeBefore == ET)
        {
            timeBefore = timeBefore - (int)(1000000 / fps);
            ET = timeBefore;
        }
        else
        {
            timeBefore = ET;
        }

        msg = m_cameraNamePrint + ": correct et for fps:" + to_string(timeBefore);
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());
        return ET;
    }
}

int AutoExposureAlgorithm::adjust_et_lin(int ET, double GL)
{
    string msg = m_cameraNamePrint + ": current exposure time: " + to_string(ET);
    LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());

    double cal_time = ET * (dynamic_range * imageGrayMax - dark_level) / (GL - dark_level);

    msg = m_cameraNamePrint + ": Calculate target exposure time: " + to_string(cal_time);
    LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());

    return cal_time;
}

int AutoExposureAlgorithm::adjust_et_exp_dec(int ET, double GL, bool isNearLinear)
{
    if (isNearLinear)
    {
        times_tmp++;
        if (times_tmp < 2)
        {
            exposureTimeTmp = ET;
            grayValueTmp = GL;
            ET = ET / 1.15;
        }
        m_tmp = belowNormalRangeStep;
    }
    else
    {
        ET = ET / n_tmp;
        n_tmp++;
        m_tmp = belowNormalRangeStep;
        times_tmp = 0;
    }

    if (times_tmp > 1)
    {
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, "Approaching the upper limit of the linear interval, start line fitting.");
        /*double k = (double)(grayValueTmp - currentGray) / (exposureTimeTmp - currentExposureTime);
        double b = (double)(grayValueTmp - k * exposureTimeTmp);*/

        int newExposureTime = ((double)((rangeMin + rangeMax) * imageGrayMax) / 2.1 -
            (exposureTimeTmp * GL - ET * grayValueTmp) /
            (exposureTimeTmp - ET)) /
            ((grayValueTmp - GL) / (exposureTimeTmp - ET));

        if (newExposureTime < 0)
        {
            if (exposureTimeTmp < ET)
            {
                newExposureTime = ET * 1.14;
            }
            if (exposureTimeTmp > ET)
            {
                newExposureTime = ET / 1.15;
            }
        }
        if (m_etMax < newExposureTime)
            newExposureTime = m_etMax;

        exposureTimeTmp = ET;
        grayValueTmp = GL;
        ET = newExposureTime;
    }

    string msg = m_cameraNamePrint + ": The exposure time after reduction is: " + to_string(ET);
    LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());
    return ET;
}

int AutoExposureAlgorithm::adjust_et_exp_inc(int ET, double GL, bool isNearLinear)
{
    if (isNearLinear)
    {
        times_tmp++;
        if (times_tmp < 2)
        {
            exposureTimeTmp = ET;
            grayValueTmp = GL;
            ET = ET * 1.14;
        }
        n_tmp = aboveNormalRangeStep;
    }
    else
    {
        ET = ET * m_tmp;
        m_tmp++;
        n_tmp = aboveNormalRangeStep;
        times_tmp = 0;
    }

    if (times_tmp > 1)
    {
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, "Approaching the lower limit of the linear interval, start line fitting.");
        /*double k = (double)(grayValueTmp - currentGray) / (exposureTimeTmp - currentExposureTime);
        double b = (double)(grayValueTmp - k * exposureTimeTmp);*/

        int newExposureTime = ((double)((rangeMin + rangeMax) * imageGrayMax) / 2.1 -
            (exposureTimeTmp * GL - ET * grayValueTmp) /
            (exposureTimeTmp - ET)) /
            ((grayValueTmp - GL) / (exposureTimeTmp - ET));

        if (newExposureTime < 0)
        {
            if (exposureTimeTmp < ET)
            {
                newExposureTime = ET * 1.14;
            }
            if (exposureTimeTmp > ET)
            {
                newExposureTime = ET / 1.15;
            }
        }
        if (m_etMax < newExposureTime)
            newExposureTime = m_etMax;

        exposureTimeTmp = ET;
        grayValueTmp = GL;
        ET = newExposureTime;
    }

    string msg = m_cameraNamePrint + ": The exposure time after raising is: " + to_string(ET);
    LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());

    return ET;
}

double AutoExposureAlgorithm::get_Single_gl(cv::Mat img, bool isCalculateEntireImage, int pixelTotalNumber)
{
    if (img.empty())
    {
        return 0;
    }

    if (isCalculateEntireImage)
    {
        return cv::mean(img)[0];
    }

    int max_value = 255;
    if (img.type() == CV_8U)
    {
        max_value = 255;
    }
    else if (img.type() == CV_16U)
    {
        max_value = 4095;
    }
    int channels[] = { 0 };
    const int histSize[] = { max_value + 1 };

    float range[] = { 0, max_value + 1 };
    const float* ranges[] = { range };

    cv::Mat hist;
    cv::calcHist(&img, 1, channels, cv::Mat(), hist, 1, histSize, ranges, true, false);

    int sum = 0;
    int tmpSum = 0;
    std::map<int, int> GraySum;
    int grayNum = 0;

    string msg;
    msg = m_cameraNamePrint + ": Start to calculate the sum of the first " + to_string(pixelTotalNumber) + " pixel values";
    LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());
    for (int gray = max_value; gray > -1; gray--)
    {
        float bin_val = hist.at<float>(gray);
        if (bin_val != 0)
        {
            sum = bin_val + sum;
            GraySum[gray] = bin_val;
            if (sum >= pixelTotalNumber)
            {
                grayNum = grayNum + (pixelTotalNumber - tmpSum) * gray;
                break;
            }
            grayNum = bin_val * gray + grayNum;
            tmpSum = sum;
        }
    }
    msg = m_cameraNamePrint + ": Calculate the gray value:" + to_string(grayNum) + ", pixelTotalNumber:" + to_string(pixelTotalNumber);
    LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());
    double aveGray = (double)grayNum / (double)pixelTotalNumber;
    msg = m_cameraNamePrint + ": current gray value: " + to_string(aveGray);
    LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());
    return aveGray;
}

double AutoExposureAlgorithm::get_Multiple_gl(cv::Mat img, bool isUseROI, bool isCalculateEntireImage, int pixelTotalNumber, GrayCalculationMode grayCalculationMode)
{
    if (img.empty())
    {
        return 0;
    }
    if (isUseROI)
    {
        vector<double> grayValue;
        int i = 0;
        for (auto it = rois.begin(); it != rois.end(); it++)
        {
            cv::Mat roiImage;
            if ((it->second[1] + it->second[3]) > img.cols || (it->second[2] + it->second[4]) > img.rows)
            {
                string msg = m_cameraNamePrint + ": ROI(x+width): " + to_string(it->second[1] + it->second[3])
                    + ", " + "ROI(y+height): " + to_string(it->second[2] + it->second[4]) + ", rawImage(width, height): ("
                    + to_string(img.cols) + ", " + to_string(img.rows) + ")";
                LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());
                throw std::runtime_error("[AutoExposure]ROI out of range! " + msg);
            }

            cv::Rect roi(it->second[1], it->second[2], it->second[3], it->second[4]);
            roiImage = img(roi);
            double gray = get_Single_gl(roiImage, isCalculateEntireImage, pixelTotalNumber);
            string msg = m_cameraNamePrint + ": Calculate the gray value of ROI" + to_string(i + 1) + ": " + to_string(gray);
            LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());
            grayValue.push_back(gray);
            i++;
        }
        return GrayModeCalculate(grayCalculationMode, grayValue);
    }
    else
    {
        return get_Single_gl(img, isCalculateEntireImage, pixelTotalNumber);
    }
}

void AutoExposureAlgorithm::initParams()
{
    currentIterations = 0;
    if (m_AEConfig.AE_Config.find(m_cameraNameFromJson) == m_AEConfig.AE_Config.end())
    {
        string msg = m_cameraNamePrint + ": The Name field in the Autoexposure.json file is inconsistent with the external input";
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());

        throw runtime_error(msg);
    }

    bit_depth = m_AEConfig.AE_Config[m_cameraNameFromJson].fixedParameters.bit_depth;
    imageGrayMax = bit_depth;
    totalIterations = m_AEConfig.AE_Config[m_cameraNameFromJson].iterationStepLength.totalIterations;
    dynamic_range = m_AEConfig.AE_Config[m_cameraNameFromJson].fixedParameters.dynamic_range;
    dark_level = m_AEConfig.AE_Config[m_cameraNameFromJson].fixedParameters.dark_level;
    belowNormalRangeStep = m_AEConfig.AE_Config[m_cameraNameFromJson].iterationStepLength.belowNormalRange;
    aboveNormalRangeStep = m_AEConfig.AE_Config[m_cameraNameFromJson].iterationStepLength.aboveNormalRange;
    rangeMax = m_AEConfig.AE_Config[m_cameraNameFromJson].judgmentMechanism.rangeMax;
    rangeMin = m_AEConfig.AE_Config[m_cameraNameFromJson].judgmentMechanism.rangeMin;
    target_max = m_AEConfig.AE_Config[m_cameraNameFromJson].judgmentMechanism.target_max;
    target_min = m_AEConfig.AE_Config[m_cameraNameFromJson].judgmentMechanism.target_min;
    m_etMax = m_AEConfig.AE_Config[m_cameraNameFromJson].judgmentMechanism.max_time;
    m_etMin = m_AEConfig.AE_Config[m_cameraNameFromJson].judgmentMechanism.min_time;
    m_fps = m_AEConfig.AE_Config[m_cameraNameFromJson].initialParameters.rate;
    m_etMax = MaxAndMinCorrection(m_etMax, m_fps);
    m_etMin = MaxAndMinCorrection(m_etMin, m_fps);
    updateMaxMinTimeToConfig(m_etMin, m_etMax);
    m_ReadWriteConfig.WriteAEConfig(m_AEConfig, m_autoExposurePath);

    m_currentExposureTime = m_AEConfig.AE_Config[m_cameraNameFromJson].initialParameters.InitialTime;
    if (isManualInitTime)
        m_currentExposureTime = maunalInitTime;

    isUseROIs = m_AEConfig.AE_Config[m_cameraNameFromJson].isUseROIs.isUseROIs;
    isCalculateEntireROIGray = m_AEConfig.AE_Config[m_cameraNameFromJson].isUseROIs.isCalculateEntireROIGray;
    pixelTotalNumber = m_AEConfig.AE_Config[m_cameraNameFromJson].isUseROIs.pixelTotalNumber;
    isUseWeightedAverage = m_AEConfig.AE_Config[m_cameraNameFromJson].isUseROIs.isUseWeightedAverage;
    grayCalculationMode = GrayCalculationMode::average;
    if (isUseWeightedAverage) grayCalculationMode = GrayCalculationMode::weighted_average;
    rois.clear();
    for (int i = 0; i < m_AEConfig.AE_Config[m_cameraNameFromJson].isUseROIs.rois.size(); i++)
    {
        string roiName = "ROI" + to_string(i + 1);
        vector<double> roiInfo;
        double weight;
        int pointX, pointY, width, height;
        weight = m_AEConfig.AE_Config[m_cameraNameFromJson].isUseROIs.rois[roiName].weight;
        pointX = m_AEConfig.AE_Config[m_cameraNameFromJson].isUseROIs.rois[roiName].x;
        pointY = m_AEConfig.AE_Config[m_cameraNameFromJson].isUseROIs.rois[roiName].y;
        width = m_AEConfig.AE_Config[m_cameraNameFromJson].isUseROIs.rois[roiName].width;
        height = m_AEConfig.AE_Config[m_cameraNameFromJson].isUseROIs.rois[roiName].height;

        roiInfo.push_back(weight);
        roiInfo.push_back(pointX);
        roiInfo.push_back(pointY);
        roiInfo.push_back(width);
        roiInfo.push_back(height);
        rois.insert(pair<string, vector<double>>(roiName, roiInfo));
    }

    m_isResetFpsParam = true;
    if (m_isResetFpsParam)
    {
        timeBefore = -1;
        m_isResetFpsParam = false;
    }

    m_isResetIterationParam = true;
    if (m_isResetIterationParam)
    {
        m_tmp = belowNormalRangeStep;
        n_tmp = aboveNormalRangeStep;
        //totalIteration_tmp = 0;
        times_tmp = 0;
        exposureTimeTmp = 0;
        grayValueTmp = 0;
        m_isResetIterationParam = false;
    }
}

void AutoExposureAlgorithm::updateInitTimeToConfig(int initTime)
{
    m_mutex1.lock();
    m_AEConfig = m_ReadWriteConfig.ReadAEConfig(m_autoExposurePath);
    m_AEConfig.AE_Config[m_cameraNameFromJson].initialParameters.InitialTime = initTime;
    m_ReadWriteConfig.WriteAEConfig(m_AEConfig, m_autoExposurePath);
    /*string msg = m_cameraNamePrint + ": update exposure time " + to_string(initTime) + " to config";
    LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());*/
    m_mutex1.unlock();
}

void AutoExposureAlgorithm::updateMaxMinTimeToConfig(int minTime, int maxTime)
{
    m_mutex1.lock();
    m_AEConfig = m_ReadWriteConfig.ReadAEConfig(m_autoExposurePath);
    m_AEConfig.AE_Config[m_cameraNameFromJson].judgmentMechanism.min_time = minTime;
    m_AEConfig.AE_Config[m_cameraNameFromJson].judgmentMechanism.max_time = maxTime;
    m_ReadWriteConfig.WriteAEConfig(m_AEConfig, m_autoExposurePath);
    m_mutex1.unlock();
}

GrayStatus AutoExposureAlgorithm::GetGrayStatus(cv::Mat img, int ET, double GL)
{
    string msg = m_cameraNamePrint + ": Start to get gray status...";
    LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());

    if (GL < (rangeMin * imageGrayMax))
    {
        msg = m_cameraNamePrint + ": current exposure time: " + to_string(ET) + ", current gray value: " + to_string(GL) + "--[Green]--";
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());

        if (ET == m_etMax)
        {
            msg = m_cameraNamePrint + ": exposure time reaches maximum: " + to_string(m_etMax);
            LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());
            return too_dark;
        }
        if (GL >= (rangeMin * imageGrayMax * 0.7))
        {
            msg = m_cameraNamePrint + ": gray value small, need to increase exposure time";
            LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());

            return gray_near_normal_min;
        }

        msg = m_cameraNamePrint + ": gray value small, need to increase exposure time";
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());

        return GrayStatus::gray_small;
    }
    else if ((GL >= (rangeMin * imageGrayMax)) && (GL <= (rangeMax * imageGrayMax)))
    {
        if ((GL >= (target_min * imageGrayMax)) && (GL <= (target_max * imageGrayMax)))
        {
            msg = m_cameraNamePrint + ": current exposure time: " + to_string(ET) + ", current gray value: " + to_string(GL) + "--[Blue]--";
            LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());
            return GrayStatus::gray_target;
        }

        if ((GL < (target_min * imageGrayMax)) && (ET == m_etMax))
        {
            msg = m_cameraNamePrint + ": current exposure time: " + to_string(ET) + ", current gray value: " + to_string(GL) + "--[Lower Gray]--";
            LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());

            msg = m_cameraNamePrint + ": exposure time reaches maximum: " + to_string(m_etMax);
            LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());

            return too_dark;
        }

        if ((GL > (target_max * imageGrayMax)) && (ET == m_etMin))
        {
            msg = m_cameraNamePrint + ": current exposure time: " + to_string(ET) + ", current gray value: " + to_string(GL) + "--[High Gray]--";
            LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());

            msg = m_cameraNamePrint + ": exposure time reaches minimum: " + to_string(m_etMin);
            LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());

            return too_bright;
        }

        if ((GL < (target_min * imageGrayMax)))
        {
            msg = m_cameraNamePrint + ": current exposure time: " + to_string(ET) + ", current gray value: " + to_string(GL) + "--[Lower Gray]--";
            LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());
        }

        if ((GL > (target_max * imageGrayMax)))
        {
            msg = m_cameraNamePrint + ": current exposure time: " + to_string(ET) + ", current gray value: " + to_string(GL) + "--[High Gray]--";
            LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());
        }

        return GrayStatus::gray_normal;
    }
    else if (GL > (rangeMax * imageGrayMax))
    {
        msg = m_cameraNamePrint + ": current exposure time: " + to_string(ET) + ", current gray value: " + to_string(GL) + "--[Red]--";
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());

        if (ET == m_etMin)
        {
            msg = m_cameraNamePrint + ": exposure time reaches minimum: " + to_string(m_etMin);
            LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());

            return too_bright;
        }
        if (GL < imageGrayMax)
        {
            msg = m_cameraNamePrint + ": gray value large, need to reduce exposure time";
            LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());

            return gray_near_normal_max;
        }

        msg = m_cameraNamePrint + ": gray value large, need to reduce exposure time";
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());

        return GrayStatus::gray_large;
    }
}

double AutoExposureAlgorithm::GrayModeCalculate(GrayCalculationMode grayCalculationMode, vector<double> grayValues)
{
    string msg;
    double grayvalue = 0;
    switch (grayCalculationMode)
    {
    case GrayCalculationMode::average:
        for (int i = 0; i < grayValues.size(); i++)
            grayvalue += grayValues[i];
        grayvalue = grayvalue / (double)grayValues.size();

        msg = m_cameraNamePrint + ": current average gray value: " + to_string(grayvalue);
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());

        break;
    case GrayCalculationMode::weighted_average:
        map<string, vector<double>>::iterator it = rois.begin();
        for (int i = 0; i < grayValues.size(); i++)
        {
            grayvalue += (grayValues[i] * it->second[0]);
            it++;
        }

        msg = m_cameraNamePrint + ": current weighted average gray value: " + to_string(grayvalue);
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());

        break;
    }
    
    return grayvalue;
}

int AutoExposureAlgorithm::MaxAndMinCorrection(int value, int rate)
{
    if (value <= (int)(1000000 / rate))
    {
        return (int)(1000000 / rate);
    }

    int remainder = value % (int)(1000000 / rate);
    if (remainder != 0)
    {
        value = value - remainder;
    }
    return value;
}
