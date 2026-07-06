#pragma once
#include <iostream>
#include <Windows.h>
#include <opencv2/opencv.hpp>
#include "LogPlus.h"
#include "ReadWriteConfig.h"

using namespace std;

#ifdef AE_EXPORTS
#define AE_API __declspec(dllexport)
#else
#define AE_API __declspec(dllimport)
#endif

typedef enum AE_API AEStatus
{
    AE_normal,
    AE_max_iterations_reached,
    AE_too_dark,
    AE_too_bright,
    AE_single_acquisition_error,
    AE_continuous_acquisition_error,
    AE_stop_acquisition_error,
    AE_time_out
};

typedef enum AE_API GrayCalculationMode
{
    average,
    weighted_average,
};

typedef enum AE_API GrayStatus
{
	gray_target,
	gray_normal,
	gray_near_normal_min,
	gray_near_normal_max,
	gray_large,
	gray_small,
	too_dark,
	too_bright
};


class AE_API AutoExposureAlgorithm
{
public:
	AutoExposureAlgorithm(std::string cameraNameFromJson, std::string autoExposurePath, std::string cameraNamePrint, int initTime = 2000000, bool isManualInitTime = false);
	~AutoExposureAlgorithm();

public:
    template <class T>
    AEStatus capture_ae(T* cameraPtr);

	bool IsUseROI();
	map<string, vector<double>> GetROIS();
private:
	// Corrects the exposure time (et) to be in the range of ET_Min and ET_Max, and sets the flags respectively
	void correct_et_for_max_min(int& ET, int ET_Max, int ET_Min);

	// Corrects the exposure time (et) based on the fps
	int correct_et_for_fps(int ET, int fps);

	// Scales the exposure time based on current exposure time, current gray level and the target gray level
	int adjust_et_lin(int ET, double GL);

	// decreases the exposure time exponentially
	int adjust_et_exp_dec(int ET, double GL, bool isNearLinear);

	//Exponentially increasing exposure time
	int adjust_et_exp_inc(int ET, double GL, bool isNearLinear);

	// Gets the single gray level of the image based on current algorithm
	double get_Single_gl(cv::Mat img, bool isCalculateEntireImage, int pixelTotalNumber = 0);

	// Gets the Multiple gray level of the image based on current algorithm
	double get_Multiple_gl(cv::Mat img, bool isUseROI, bool isCalculateEntireImage, int pixelTotalNumber, GrayCalculationMode grayCalculationMode = average);

	// capture an image with fixed exposure time ET
	template <class T>
	cv::Mat getImage_for_ae(T* cameraPtr, int ET);

	// current gray status
	GrayStatus GetGrayStatus(cv::Mat img, int ET, double GL);
private:
	//Calculate the average grayscale and weighted average grayscale of all ROIs
	double GrayModeCalculate(GrayCalculationMode grayCalculationMode, vector<double> grayValues);

	//max and min value Correction
	int MaxAndMinCorrection(int value, int rate);

	//init all params
	void initParams();

	//update initTime to config
	void updateInitTimeToConfig(int initTime);

	//update max/min time to config
	void updateMaxMinTimeToConfig(int minTime, int maxTime);

	template <class T>
	AEStatus capture_ae_recurive(T* cameraPtr, int ET, int iterations);
private:
	std::string m_cameraNameFromJson;
	std::string m_autoExposurePath;
	std::string m_cameraNamePrint;
	int maunalInitTime;
	bool isManualInitTime;

private:
	ReadWriteConfig m_ReadWriteConfig;
	AEConfig m_AEConfig;

private:
	int m_currentExposureTime;
	double m_currentGrayValue;
	//bool m_ET_Max_Flag;
	//bool m_ET_Min_Flag;
	int m_etMax;
	int m_etMin;
	int m_fps;
	int totalIterations;
	int currentIterations;
private:
	bool m_isResetFpsParam = true;
	bool m_isResetIterationParam = true;
	double dynamic_range;
	double dark_level;
	int bit_depth;

private:
	int timeBefore = -1;
	int imageGrayMax;
	double belowNormalRangeStep;
	double aboveNormalRangeStep;
	double m_tmp;
	double n_tmp;
	int times_tmp;
	int exposureTimeTmp;
	double grayValueTmp;
	double rangeMax;
	double rangeMin;
	double target_max;
	double target_min;
	map<string, vector<double>> rois;
	bool isUseROIs = false;
	int pixelTotalNumber;
	bool isCalculateEntireROIGray;
	bool isUseWeightedAverage;
	GrayCalculationMode grayCalculationMode;
};

template <class T>
inline AEStatus AutoExposureAlgorithm::capture_ae(T* cameraPtr)
{
	string cameraName = "[--AutoExposure:" + m_cameraNamePrint + "--]";
	string msg;
	msg = "================================ " + m_cameraNamePrint + ": start auto exposure================================";
	LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());

	try
	{
        //cameraPtr->GrabOne();

        //cameraPtr->GetImage();

		initParams();
	}
	catch (const std::exception& e)
	{
		throw runtime_error(e.what());
	}

	msg = m_cameraNamePrint + ": Successfully initialized parameters.";
	LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());

	return capture_ae_recurive(cameraPtr, m_currentExposureTime, currentIterations);
}


template<class T>
inline cv::Mat AutoExposureAlgorithm::getImage_for_ae(T* cameraPtr, int ET)
{
	cameraPtr->SetExposureTime(ET);
	string msg = m_cameraNamePrint + ": Set the exposure time to " + to_string(ET);
	LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());
	Sleep(1000);

	msg = m_cameraNamePrint + ": start to grab one ...";
	LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());
	cameraPtr->GrabOne();
	
	msg = m_cameraNamePrint + ": start to get image...";
	LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());
	return cameraPtr->GetImage();
}

template <class T>
inline AEStatus AutoExposureAlgorithm::capture_ae_recurive(T* cameraPtr, int ET, int iterations)
{
	string cameraName = "[--AutoExposure:" + m_cameraNamePrint + "--]";
	string msg;
	if (iterations == totalIterations)
	{
		msg = "======================================== " + m_cameraNamePrint + ": Auto exposure end. Status:reach max iteration=========================================";
		LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());
		return AEStatus::AE_max_iterations_reached;
	}

	msg = m_cameraNamePrint + ": Start the " + to_string(++iterations) + " iteration";
	LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());

	correct_et_for_max_min(ET, m_etMax, m_etMin);
	m_currentExposureTime = correct_et_for_fps(ET, m_fps);

	cv::Mat imageNewTmp_;
	getImage_for_ae(cameraPtr, m_currentExposureTime).copyTo(imageNewTmp_);

	m_currentGrayValue = get_Multiple_gl(imageNewTmp_, isUseROIs, isCalculateEntireROIGray, pixelTotalNumber, grayCalculationMode);

	GrayStatus grayStatus = GetGrayStatus(imageNewTmp_, m_currentExposureTime, m_currentGrayValue);

	switch (grayStatus)
	{
	case GrayStatus::gray_target:
		updateInitTimeToConfig(m_currentExposureTime);
		LOG4CPLUS_INFO(LogPlus::getInstance()->logger, "******************The grayscale value enters the target range******************");
		msg = "======================================== " + m_cameraNamePrint + ": Auto exposure end. Status:normal=========================================";
		LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());
		return AEStatus::AE_normal;
	case GrayStatus::gray_normal:
		LOG4CPLUS_INFO(LogPlus::getInstance()->logger, "******************The grayscale value enters the linear range******************");
		m_currentExposureTime = adjust_et_lin(m_currentExposureTime, m_currentGrayValue);
		return capture_ae_recurive(cameraPtr, m_currentExposureTime, iterations);
	case GrayStatus::too_bright:
		updateInitTimeToConfig(m_currentExposureTime);
		msg = "======================================== " + m_cameraNamePrint + ": Auto exposure end. Status:too bright=========================================";
		LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());
		return AEStatus::AE_too_bright;
	case GrayStatus::too_dark:
		updateInitTimeToConfig(m_currentExposureTime);
		msg = "======================================== " + m_cameraNamePrint + ": Auto exposure end. Status:too dark=========================================";
		LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());
		return AEStatus::AE_too_dark;
	case GrayStatus::gray_large:
		m_currentExposureTime = adjust_et_exp_dec(m_currentExposureTime , m_currentGrayValue, false);
		return capture_ae_recurive(cameraPtr, m_currentExposureTime, iterations);
	case GrayStatus::gray_small:
		m_currentExposureTime = adjust_et_exp_inc(m_currentExposureTime, m_currentGrayValue, false);
		return capture_ae_recurive(cameraPtr, m_currentExposureTime, iterations);
	case GrayStatus::gray_near_normal_min:
		m_currentExposureTime = adjust_et_exp_inc(m_currentExposureTime, m_currentGrayValue, false);
		return capture_ae_recurive(cameraPtr, m_currentExposureTime, iterations);
	case GrayStatus::gray_near_normal_max:
		m_currentExposureTime = adjust_et_exp_dec(m_currentExposureTime, m_currentGrayValue, false);
		return capture_ae_recurive(cameraPtr, m_currentExposureTime, iterations);
	}
}