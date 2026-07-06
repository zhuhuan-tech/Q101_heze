#include "pch.h"
#include "ML_RainbowCalculation.h"
#include"LogPlus.h"
#include <embed.h>
#include "ML_PythonUtl.h"
#include"IQMetricUtl.h"
#include <pybind11.h>
#include <numpy.h>
#include"ml_image_public.h"
namespace py = pybind11;
using namespace MLImageDetection;
using namespace MLIQMetrics;
using namespace cv;
using namespace std;



MLRainbowCalculation::MLRainbowCalculation()
{
}

MLRainbowCalculation::~MLRainbowCalculation()
{
}
void MLRainbowCalculation::setROILen(int len)
{
	m_ROILen = len;
}


vector<cv::Point>getRainbowCoordinate(py::array_t<int> xs,py::array_t<int> ys)
{
	vector<cv::Point>pts;
	py::buffer_info infox = xs.request();
	int* ptrx = static_cast<int*>(infox.ptr);
	py::buffer_info infoy = ys.request();
	int* ptry = static_cast<int*>(infoy.ptr);
	size_t size = infox.size;
	for (int i = 0; i < size; i++)
	{
		cv::Point pt0;
		pt0.x = ptrx[i];
		pt0.y = ptry[i];
		pts.push_back(pt0);
	}
	return pts;
}

py::array_t<unsigned short> cv_mat_uint16_1c_to_numpy(cv::Mat& input)
{
	// 确保输入的 Mat 是 10 位单通道图像
	assert(input.type() == CV_16UC1);  // 确保输入为16位无符号单通道图像

	// 创建 NumPy 数组，类型为 unsigned short (16位无符号)
	py::array_t<unsigned short> dst = py::array_t<unsigned short>({ input.rows, input.cols },
		reinterpret_cast<unsigned short*>(input.data));

	return dst;
}

RainbowInfo MLRainbowCalculation::rainbowDetection(cv::Mat img, double theta, double phi, double waveLen)
{
	LOG4CPLUS_INFO(LogPlus::getInstance()->logger, "rainbowDetection start");
	RainbowInfo re;
	try {
		PythonManager::instance();
		py::gil_scoped_acquire acquire;
		py::module_ sys = py::module_::import("sys");
		string pythonPath = "E:\\LiNing\\PICOprogram\\rainbow";
		pythonPath = IQMetricsParameters::pythonPath;
		sys.attr("path").attr("insert")(0, pythonPath);
		py::module_ test = py::module_::import("rainbow_detection");
		py::array img_np = cv_mat_uint16_1c_to_numpy(img);
		py::tuple result = test.attr("rainbowDetection1")(img_np, theta, phi, waveLen);
		bool flag = result[0].cast<bool>();
		py::array_t<int> xs = result[1].cast<py::array_t<int>>();
		py::array_t<int> ys = result[2].cast<py::array_t<int>>();
		vector<cv::Point>pts = getRainbowCoordinate(xs, ys);
		if (flag == true && pts.size() > 1)
		{
			re.rainbowFlag = true;
			re.rainbowPoints = pts;
			cv::Point2f cen = getPtsCenter(pts);
			re.rianbowCen = cen;
			re.rainbowRect = cv::Rect(cen.x - m_ROILen / 2, cen.y - m_ROILen / 2, m_ROILen, m_ROILen);
		}
		else
		{
			re.rainbowFlag = false;
		}
		LOG4CPLUS_INFO(LogPlus::getInstance()->logger, "rainbowDetection end");
		return re;
	}
	catch (py::error_already_set& e)
	{
		string errInfo = e.what();
		LOG4CPLUS_ERROR(LogPlus::getInstance()->logger, "processSingleRainbow");
		std::cout << "Error: " << e.what() << std::endl;
		re.rainbowFlag = false;
		re.detectionFlag = false;
		re.errMsg = errInfo;
		return re;
	}
	return re;
}
RainbowInfo MLRainbowCalculation::processSingleRainbow(std::string path)
{
	LOG4CPLUS_INFO(LogPlus::getInstance()->logger, "processSingleRainbow start");
	RainbowInfo re;
	try {
		PythonManager::instance();
		py::gil_scoped_acquire acquire;
		py::module_ sys = py::module_::import("sys");
		string pythonPath = "E:\\LiNing\\PICO program\\rainbow";
		pythonPath = IQMetricsParameters::pythonPath;
		//sys.attr("path").attr("insert")(0, pythonPath);
		py::list pathList = sys.attr("path");
		bool exists = false;
		for (auto item : pathList)
		{
			if (std::string(py::str(item)) == pythonPath)
			{
				exists = true;
				break;
			}
		}

		if (!exists)
		{
			pathList.attr("insert")(0, pythonPath);
		}


		py::module_ test = py::module_::import("rainbow_detection");
		py::tuple result = test.attr("process_single_image")(path);
		bool flag= result[0].cast<bool>();
		py::array_t<int> xs =result[1].cast<py::array_t<int>>();
		py::array_t<int> ys =result[2].cast<py::array_t<int>>();
		vector<cv::Point>pts=getRainbowCoordinate(xs, ys);
		if (flag == true&&pts.size()>1)
		{
			re.rainbowFlag = true;
			re.rainbowPoints = pts;
			cv::Point2f cen=getPtsCenter(pts);
			re.rianbowCen = cen;
			re.rainbowRect = cv::Rect(cen.x-m_ROILen/2,cen.y-m_ROILen/2,m_ROILen,m_ROILen);
		}
		else
		{
			re.rainbowFlag = false;
		}
		return re;
	}
	catch (py::error_already_set& e)
	{
		string errInfo = e.what();
		string message = "processSingleRainbow" + errInfo;
		LOG4CPLUS_ERROR(LogPlus::getInstance()->logger, message.c_str());
		//std::cout << "Error: " << e.what() << std::endl;
		re.rainbowFlag = false;
		re.detectionFlag = false;
		re.errMsg = errInfo;
		return re;
	}
	LOG4CPLUS_INFO(LogPlus::getInstance()->logger, "processSingleRainbow end");
	return re;
}

bool MLRainbowCalculation::processBatchRainbow(std::string path)
{
	LOG4CPLUS_INFO(LogPlus::getInstance()->logger, "processBatchRainbow start");
	try {
		PythonManager::instance();
		py::gil_scoped_acquire acquire;
		py::module_ sys = py::module_::import("sys");
		string pythonPath = "E:\\LiNing\\PICOprogram\\rainbow";
		pythonPath = IQMetricsParameters::pythonPath;
		//sys.attr("path").attr("insert")(0, pythonPath);
		py::list pathList = sys.attr("path");
		bool exists = false;
		for (auto item : pathList)
		{
			if (std::string(py::str(item)) == pythonPath)
			{
				exists = true;
				break;
			}
		}

		if (!exists)
		{
			pathList.attr("insert")(0, pythonPath);
		}
		py::module_ test = py::module_::import("rainbow_detection");
		bool  flag = test.attr("process_batch_image")(path).cast<bool>();
	}
	catch (py::error_already_set& e)
	{
		string errInfo = e.what();
		std::cout << "Error: " << e.what() << std::endl;
		string message= "processBatchRainbow:"+errInfo;
		LOG4CPLUS_ERROR(LogPlus::getInstance()->logger, message.c_str());
		return false;
	}
	LOG4CPLUS_INFO(LogPlus::getInstance()->logger, "processBatchRainbow end");
	return true;
}
cv::Point2f MLRainbowCalculation::getPtsCenter(const std::vector<cv::Point>& pts)
{
	cv::Point2f center(0, 0);
	if (pts.empty())
		return center;

	for (const auto& p : pts)
	{
		center.x += p.x;
		center.y += p.y;
	}

	center.x /= pts.size();
	center.y /= pts.size();

	return center;
}
void MLRainbowCalculation::collectRainbowData(std::string path)
{
	LOG4CPLUS_INFO(LogPlus::getInstance()->logger, "processBatchRainbow start");
	try {
		PythonManager::instance();
		py::gil_scoped_acquire acquire;
		py::module_ sys = py::module_::import("sys");
		string pythonPath = "E:\\LiNing\\PICOprogram\\rainbow";
		pythonPath = IQMetricsParameters::pythonPath;
		//sys.attr("path").attr("insert")(0, pythonPath);
		py::list pathList = sys.attr("path");
		bool exists = false;
		for (auto item : pathList)
		{
			if (std::string(py::str(item)) == pythonPath)
			{
				exists = true;
				break;
			}
		}

		if (!exists)
		{
			pathList.attr("insert")(0, pythonPath);
		}
		//std::cout << pythonPath << std::endl;
		//py::print(sys.attr("path"));
		py::module_ test = py::module_::import("rainbow_detection");
		string  message = test.attr("collectRainbowData")(path).cast<string>();
	}
	catch (py::error_already_set& e)
	{
		string errInfo = e.what();
		std::cout << "Error: " << e.what() << std::endl;
		string message = "processBatchRainbow:" + errInfo;
		LOG4CPLUS_ERROR(LogPlus::getInstance()->logger, message.c_str());
	//	return false;
	}
	LOG4CPLUS_INFO(LogPlus::getInstance()->logger, "processBatchRainbow end");
	//return true;
}
