#pragma once
#ifdef IQMETRICS_EXPORTS
#define IQMETRICS_API __declspec(dllexport)
#else
#define IQMETRICS_API __declspec(dllimport)
#endif
#include <opencv2/core/base.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2\opencv.hpp>
#include<opencv2\highgui\highgui.hpp>
#include<opencv2\imgproc\imgproc.hpp>
#include <iostream>
namespace MLIQMetrics
{
    struct RainbowInfo
    {
        bool rainbowFlag = false;
        std::vector<cv::Point> rainbowPoints;
        cv::Rect rainbowRect;
        cv::Point2f rianbowCen;
        bool detectionFlag = true;
        std::string errMsg = "";
    };
    class IQMETRICS_API MLRainbowCalculation
    {
    public:
        MLRainbowCalculation();
        ~MLRainbowCalculation();
    public:
        void setROILen(int len);
        RainbowInfo rainbowDetection(cv::Mat img, double theta, double phi, double waveLen);
        RainbowInfo processSingleRainbow(std::string path);
        bool processBatchRainbow(std::string path);
        void collectRainbowData(std::string path);
    private:
        cv::Point2f getPtsCenter(const std::vector<cv::Point>& pts);
        int m_ROILen = 500;
    };
}

