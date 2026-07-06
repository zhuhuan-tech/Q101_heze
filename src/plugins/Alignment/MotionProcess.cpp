#include <QMessageBox>
#include <QCoreApplication>
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <corecrt_math_defines.h>
#include <cmath>
#include "MotionProcess.h"
#include "Core/loggingwrapper.h"
#include "piMotionActor/Motion3DModel.h"
#include "OrientalMotor/OrientalMotorControl.h"
#include "CollimatedLightTube/CollimatedLightTubeMode.h"
#include "CollimatedLightTube/CollimatedConfig.h"
#include "PLCControl/PLCController.h"
#include "Alignment/CameraModel.h"
#include "piMotionActor/configItem.h"
#include "MLKeyenceLRF/MLKeyenceCL.h"
#include "piMotionActor/Motion2DModel.h"
#include "dutTypeConfig.h"
//#include "PolarizerControl/ThorlabsMode.h"
#include <QDir>
#include "LimitMove.h"
#include "ImageDetection/FiducialDetect.h"

namespace AAProcess
{
	MotionProcess& MotionProcess::getInstance() {
		static MotionProcess instance;
		return instance;
	}

	MotionProcess::MotionProcess(QObject* parent)
		: QObject(parent),
		m_isTreeSystemRun(false),
		m_bIsAutoFiducial(true)
	{
		InitConfig();

		PrintModulePosition(ModuleName::DutModuleXYZ, ModuleName::ImagingModuleXYZ);
	}

	MotionProcess::~MotionProcess()
	{
	}
	std::string MotionProcess::ConnectMVCamera() {

		std::string msg;
		if (!CameraModel::GetInstance()->isConnected())
		{
			QString cameraSn = ConfigItem::instance()->getMVCameraSn();

			if (cameraSn.isEmpty())
			{
				msg = "MV camera connection failed, SN is empty.";
				return msg;
			}

			Result ret = CameraModel::GetInstance()->connect(cameraSn.toStdString().c_str());
			if (!ret.success)
			{
				CameraModel::GetInstance()->disConnect();
				QThread::msleep(1000);

				ret = CameraModel::GetInstance()->connect(cameraSn.toStdString().c_str());
				if (!ret.success)
				{
					return ret.errorMsg;
				}
			}

			CameraModel::GetInstance()->SubscribeCameraEvent(MLCameraEvent::kFrameReceived);
			CameraModel::GetInstance()->SubscribeCameraEvent(CORE::MLCameraEvent::kGrayLevel);
		}
		return "";
	}

	std::string MotionProcess::ConnectMeasureCameraMotionModule()
	{
		// Measure 3D move/tilt station
		Result result;
		std::string msg;
		//if (!Motion2DModel::getInstance(ACS2DCameraTilt)->Motion2DisConnected())
		{
			QString plcIp = ConfigItem::instance()->getMotion2DIpAndOther(PLC2D);

			result = Motion2DModel::getInstance(PLC2D)->connectAndInit(plcIp);
			if (!result.success)
			{
				msg = "Measure plc 2D tilt motion connect failed, " + result.errorMsg;
				return msg;
			}
		}
		//先将转轮移动到安全位置
		Motion2DModel::getInstance(PLC2D)->Motion2DMoveAbs(270, 97);

		while (CheckModuleIsMoving(ModuleName::ImagingModuleDxDy))
		{
			QCoreApplication::processEvents();
			_sleep(100);
		}

		//if (!Motion3DModel::getInstance(withCamera)->Motion3DisConnected())
		{
			QString mcIp = ConfigItem::instance()->getMotion3DIpAndOther(withCamera);
			result = Motion3DModel::getInstance(withCamera)->connectAndInit(mcIp);
			if (!result.success)
			{
				msg = "Measure camera 3D move motion connect failed, " + result.errorMsg;
				return msg;
			}
		}
		

		return "";
	}

	std::string MotionProcess::ConnectDutMotionModule()
	{
		// DUT 3D move/tilt station
		Result result;
		std::string msg;
		//if (!Motion3DModel::getInstance(withDUT)->Motion3DisConnected())
		{
			QString dutIp = ConfigItem::instance()->getMotion3DIpAndOther(withDUT);

			result = Motion3DModel::getInstance(withDUT)->connectAndInit(dutIp);
			if (!result.success)
			{
				msg = "DUT 3D move motion connect failed, " + result.errorMsg;
				return msg;
			}
		}
		//if (!OrientalMotorControl::getInstance()->IsConnected())
		{
			result = OrientalMotorControl::getInstance()->Connect();
			if (!result.success)
			{
				msg = "DUT 3D tilt motion connect failed, " + result.errorMsg;
				return msg;
			}
		}
		return "";
	}

	std::string MotionProcess::ConnectProjectorMotionModule()
	{
		// Projector tilt station
		Result result;
		std::string msg;
		//if (!Motion2DModel::getInstance(ACS2DPro)->GetIsConnected())
		{
			QString ipEtc = ConfigItem::instance()->getMotion2DIpAndOther(ACS2DPro);
			result = Motion2DModel::getInstance(ACS2DPro)->connectAndInit(ipEtc);
			if (!result.success)
			{
				msg = "Projector 2D tilt motion connect failed, " + result.errorMsg;
				return msg;
			}
		}

		return "";
	}

	std::string MotionProcess::ConnectKeyence()
	{
		// Keyence
		std::string msg;
		if (!MLKeyenceCL::MakeRangeFinder()->IsConnected())
		{
			KeyenceDviceInfo keyenceType = ConfigItem::instance()->getKeyenceType();
			bool ret = MLKeyenceCL::MakeRangeFinder()->Connect(keyenceType.type.toStdString().c_str(), keyenceType.ip.toStdString().c_str(), keyenceType.port, keyenceType.id);
			if (!ret)
			{
				msg = "Keyence Confocal Distance Sensor connection failed.";
				return msg;
			}
		}

		return "";
	}

	std::string MotionProcess::ConnectCollimator()
	{
		Result result;
		std::string msg;
		if (!CollimatedLightTubeMode::GetInstance()->IsConnected())
		{
			std::string collimateSn = CollimatedConfig::instance()->GetCollimatorSn();
			result = CollimatedLightTubeMode::GetInstance()->Connect(collimateSn.c_str());
			if (!result.success)
			{
				msg = "Collimate connection failed." + result.errorMsg;
				return msg;
			}
		}
		return "";
	}

	std::string MotionProcess::ConnectPLC()
	{
		bool result;
		std::string msg;
		if (!PLCController::instance()->IsConnected())
		{
			result = PLCController::instance()->Connect();
			if (!result)
			{
				msg = "PLC connection failed.";
				return msg;
			}
		}

		return "";
	}

	std::string MotionProcess::ConnectSpec()
	{
		bool result;
		std::string msg;
		result = SpecSet::getInstance()->is_connect();
		if (!result)
		{
			msg = "Spectrometer connection failed.";
			return msg;
		}

		return "";
	}

	//std::string MotionProcess::ConnectPolarizer()
	//{
	//	if (!ThorlabsMode::instance()->IsConnected())
	//	{
	//		Result res = ThorlabsMode::instance()->Connect();
	//		if (!res.success)
	//		{
	//			return "Polarizer connection failed.";
	//		}
	//		//double angle = ConfigItem::instance()->getThorlabsAngle();
	//		//res = ThorlabsMode::instance()->AbsMoveSync(angle);
	//		//if (!res.success)
	//		//{
	//		//	return "Polarizer abs move " + to_string(angle) + " error";
	//		//}
	//	}
	//	return "";
	//}

	std::string MotionProcess::LoadDUT()
	{
		PrintLog(LogType::Normal, "[LoadDUT]");

		std::string msg = CheckModuleConnectStatus(ModuleName::DutModuleXYZ, ModuleName::ImagingModuleXYZ);
		if (msg != "")
			return msg;

		double loadx = m_processConfigInfo.offsetRoatate.loadPos[currentDutName].dutModule.x;
		double loady = m_processConfigInfo.offsetRoatate.loadPos[currentDutName].dutModule.y;
		double loadz = m_processConfigInfo.offsetRoatate.loadPos[currentDutName].dutModule.z;

		CORE::ML_Point3D currentPos = Motion3DModel::getInstance(motion3DType::withDUT)->getPosition(); //um
		/*if (loadz < currentPos.z / 1000.0)
		{
			return PrintLog(LogType::Error, "Load motor Z value calibration error", !m_isTreeSystemRun);
		}*/
		msg = LimitMove::getInstance()->motion3DMoveAbsAsync(cv::Point3f(currentPos.x / 1000.0, currentPos.y / 1000.0, loadz), withDUT);
		if (!msg.empty())
			return msg;
		while (CheckModuleIsMoving(ModuleName::DutModuleXYZ))
		{
			QCoreApplication::processEvents();
			_sleep(100);
		}

		msg = LimitMove::getInstance()->motion3DMoveAbsAsync(cv::Point3f(loadx, loady, loadz), withDUT);
		if (!msg.empty())
			return msg;

		while (CheckModuleIsMoving(ModuleName::DutModuleXYZ))
		{
			QCoreApplication::processEvents();
			_sleep(100);
		}

		PrintLog(LogType::Normal, "dut motor loading end.");
		PrintModulePosition(ModuleName::DutModuleXYZ);

		double imagingFixedPosX = m_processConfigInfo.offsetRoatate.imagingFixedPos[currentDutName].x;
		double imagingFixedPosY = m_processConfigInfo.offsetRoatate.imagingFixedPos[currentDutName].y;
		double imagingFixedPosZ = m_processConfigInfo.offsetRoatate.imagingFixedPos[currentDutName].z;
		msg = LimitMove::getInstance()->motion3DMoveAbsAsync(cv::Point3f(imagingFixedPosX,
			imagingFixedPosY, imagingFixedPosZ), withCamera);
		if (!msg.empty())
			return msg;
		while (CheckModuleIsMoving(ModuleName::ImagingModuleXYZ))
		{
			QCoreApplication::processEvents();
			_sleep(100);
		}

		PrintLog(LogType::Normal, "imaging module loading end.");
		PrintModulePosition(ModuleName::ImagingModuleXYZ);

		//adjust projection module tiptilt
		/*double projectionTiptiltX = m_processConfigInfo.offsetRoatate.projectionTiptilt[currentDutName].x;
		double projectionTiptiltY = m_processConfigInfo.offsetRoatate.projectionTiptilt[currentDutName].y;
		Motion2DModel::getInstance(motion2DType::ACS2DPro)->Motion2DMoveAbsAsync(projectionTiptiltX, projectionTiptiltY);*/
		/*while (CheckModuleIsMoving(ModuleName::ProjectionDxDy))
		{
			QCoreApplication::processEvents();
			_sleep(100);
		}
		PrintLog(LogType::Normal, "projection module adjust tiptilt end.");
		PrintModulePosition(ModuleName::ProjectionDxDy);*/

		//adjust imaging module tiptilt
		double imgingTiptiltX = m_processConfigInfo.offsetRoatate.imagingTiptilt[currentDutName].x;
		double imgingTiptiltY = m_processConfigInfo.offsetRoatate.imagingTiptilt[currentDutName].y;
		Motion2DModel::getInstance(motion2DType::PLC2D)->Motion2DMoveAbsAsync(imgingTiptiltX, imgingTiptiltY);

		while (CheckModuleIsMoving(ModuleName::ImagingModuleDxDy))
		{
			QCoreApplication::processEvents();
			_sleep(100);
		}
		//PrintLog(LogType::Normal, "projection module adjust tiptilt end.");
		PrintLog(LogType::Normal, "imaging module adjust tiptilt end.");

		PrintModulePosition(ModuleName::ImagingModuleDxDy);

		return "";
	}

	std::string MotionProcess::DutParallelAdjustment()
	{
		PrintLog(LogType::Normal, "[DutParallelAdjustment]");

		std::string msg = CheckModuleConnectStatus(ModuleName::DutModuleXYZ, ModuleName::DutModuleDxDyDz, ModuleName::Collimator,
			ModuleName::PLC);
		if (msg != "")
			return msg;

		cv::Point3f parallelAdjust;
		parallelAdjust.x = m_processConfigInfo.offsetRoatate.parallelAdjustmentPos[currentDutName].adjustPos.x;
		parallelAdjust.y = m_processConfigInfo.offsetRoatate.parallelAdjustmentPos[currentDutName].adjustPos.y;
		parallelAdjust.z = m_processConfigInfo.offsetRoatate.parallelAdjustmentPos[currentDutName].adjustPos.z;

		msg = LimitMove::getInstance()->motion3DMoveAbsAsync(cv::Point3f(parallelAdjust.x, parallelAdjust.y, parallelAdjust.z), withDUT);
		if (!msg.empty())
			return msg;

		Result res = PLCController::instance()->collimatorLight(true);
		if (!res.success)
			return PrintLog(LogType::Error, res.errorMsg, !m_isTreeSystemRun);

		while (CheckModuleIsMoving(ModuleName::DutModuleXYZ))
		{
			QCoreApplication::processEvents();
			_sleep(100);
		}
		PrintLog(LogType::Normal, "dut motor move to parallel adjustment position end.");
		PrintModulePosition(ModuleName::DutModuleXYZ);
		Sleep(1000);

		MLCollimatorConfig collimatorConfig = CollimatedConfig::instance()->GetLevelingConfig();
		int adjustTimes = collimatorConfig.AdjustTimes;
		int angleRange = collimatorConfig.AngleRange;
		for (int i = 0; i < adjustTimes; i++)
		{
			if (m_collimatorDeltaX == "NULL" || m_collimatorDeltaY == "NULL")
			{
				PLCController::instance()->collimatorLight(false);
				return PrintLog(LogType::Error, "Unable to measure XY offset value, please manually adjust until XY offset value is displayed!", !m_isTreeSystemRun);
			}
			if (fabs(m_collimatorDeltaX.toDouble()) <= angleRange && fabs(m_collimatorDeltaY.toDouble()) <= angleRange)
			{
				PLCController::instance()->collimatorLight(false);
				PrintLog(LogType::Normal, "Collimator angleX: " + m_collimatorDeltaX.toStdString() + ", angleY: " + m_collimatorDeltaY.toStdString());
				PrintLog(LogType::Normal, "Collimator parallel adjustment successful!", false);
				return "";
			}

			msg = LimitMove::getInstance()->orientalMoveRel(cv::Point3f(m_collimatorDeltaX.toDouble() / 3600.0,
				m_collimatorDeltaY.toDouble() / 3600.0, 0));
			if (!msg.empty())
				return PrintLog(LogType::Error, msg, !m_isTreeSystemRun);

			while (CheckModuleIsMoving(ModuleName::DutModuleDxDyDz))
			{
				QCoreApplication::processEvents();
				_sleep(100);
			}
			Sleep(500);
		}

		PLCController::instance()->collimatorLight(false);
		return PrintLog(LogType::Error, "Reaching the maximum number of adjustments!", !m_isTreeSystemRun);
	}

	std::string MotionProcess::IsAutoIdentifyFiducial(bool isAuto)
	{
		m_bIsAutoFiducial = isAuto;
		return std::string();
	}

	std::string MotionProcess::FindFiducial()
	{
		PrintLog(LogType::Normal, "[FindFiducial]");

		PrintLog(LogType::Normal, "Starting the first find fiducial.");
		std::string msg = FindFiducialToCalculate();
		if (msg != "")
			return PrintLog(LogType::Error, msg, !m_isTreeSystemRun);

		// Restore the state without angles
		double dutDZ_before = OrientalMotorControl::getInstance()->GetPosition(OrientalAxle::DZ);
		PrintLog(LogType::Normal, "Rotation angle before dut: " + to_string(dutDZ_before));

		PrintLog(LogType::Normal, "Restore the state without angles");
		
		//配置文件第一个fid作为origin点，则角度-180，否则+180
		int compensation = -180;
		if (m_dutConfigInfoMap[currentDutName].fiducialOffset_[1].x == 0 && m_dutConfigInfoMap[currentDutName].fiducialOffset_[1].y == 0)
			compensation = -180;
		else
			compensation = 180;

		msg = LimitMove::getInstance()->orientalMoveRel(OrientalAxle::DZ, -(dutOffsetRotate.rotate * 180.0f / CV_PI + compensation));
		if (!msg.empty())
			return PrintLog(LogType::Error, msg, !m_isTreeSystemRun);
		while (CheckModuleIsMoving(ModuleName::DutModuleDxDyDz))
		{
			QCoreApplication::processEvents();
			_sleep(100);
		}
		_sleep(1000);
		double dutDZ_Current = OrientalMotorControl::getInstance()->GetPosition(OrientalAxle::DZ);
		PrintLog(LogType::Normal, "dut current rotation angle: " + to_string(dutDZ_Current));

		//safe verification
		double safeAngleMax = m_processConfigInfo.offsetRoatate.anticollision[currentDutName].dutMotorAngleMax;
		double safeAngleMin = m_processConfigInfo.offsetRoatate.anticollision[currentDutName].dutMotorAngleMin;
		if (dutDZ_Current > safeAngleMax || dutDZ_Current < safeAngleMin)
		{
			msg = "current dut motor angle: " + to_string(dutDZ_Current) + ", exceeded the safe range [" +
				to_string(safeAngleMin) + ", " + to_string(safeAngleMax) + "]";
			return PrintLog(LogType::Error, msg, !m_isTreeSystemRun);
		}

		setSavePosition("", "\n");

		PrintLog(LogType::Normal, "Starting the second find fiducial.");
		msg = FindFiducialToCalculate();
		if (msg != "")
			return PrintLog(LogType::Error, msg, !m_isTreeSystemRun);

		return "";
	}

	cv::Point2f MotionProcess::DutAnyPointToMVCenterCoordinate(cv::Point2f offsetFromOrigin, OffsetRotate offsetRotate)
	{
		// Step1: 把offset的设计偏移封装成向量 

		Eigen::Vector2d m(offsetFromOrigin.x, offsetFromOrigin.y);

		// Step2: 按当前旋转角度旋转 

		Eigen::Rotation2Dd R(offsetRotate.rotate);
		Eigen::Vector2d m_rot = R * m;

		// Step3: 加上整体偏移（origin -> 中心） 

		Eigen::Vector2d m_abs = m_rot + Eigen::Vector2d(offsetRotate.offset.x, offsetRotate.offset.y);

		// Step4: 电机要走反向位移(正负要验证) 

		return cv::Point2f(m_abs.x(), m_abs.y());
	}

	std::string MotionProcess::AutoIdentifyFiducial(cv::Point2f& center, cv::Point3f fiducialAcs)
	{
		//take fiducial image
		PrintLog(LogType::Normal, "Start moving fiducial into the MV");
		//Result ret = Motion3DModel::getInstance(motion3DType::withDUT)->Motion3DMoveAbsAsync(fiducialAcs.x * 1000,
		//	fiducialAcs.y * 1000, fiducialAcs.z * 1000);
		//if (!ret.success)
		//	return ret.errorMsg;
		
		//std::string res = LimitMove::getInstance()->motion3DMoveAbsAsync(cv::Point3f(fiducialAcs.x, fiducialAcs.y, fiducialAcs.z), withDUT);
		std::string res = LimitMove::getInstance()->motion3DMoveAbsAsync(cv::Point3f(fiducialAcs.x, fiducialAcs.y, fiducialAcs.z), withCamera);
		if (!res.empty())
			return res;
		while (CheckModuleIsMoving(ModuleName::ImagingModuleXYZ))
		{
			QCoreApplication::processEvents();
			_sleep(100);
		}

		//由于MV对焦原因，z轴需要再移动2.44mm
		res = LimitMove::getInstance()->motion3DMoveRel(cv::Point3f(0, 0, 2.44), withCamera);
		if (!res.empty())
			return PrintLog(LogType::Error, res, !m_isTreeSystemRun);
		while (CheckModuleIsMoving(ModuleName::ImagingModuleXYZ))
		{
			QCoreApplication::processEvents();
			_sleep(100);
		}

		PrintModulePosition(ModuleName::ImagingModuleXYZ);
		Sleep(1000);
		PrintLog(LogType::Normal, "Obtain fiducial image");
		PLCController::instance()->coaxialLight(true);
		Sleep(1000);
		if (m_is_auto_exposure)
		{
			CameraModel::GetInstance()->SetMLExposureAuto();
		}
		CameraModel::GetInstance()->StopGrabbing();
		CameraModel::GetInstance()->GrabOne();
		cv::Mat imgFid = CameraModel::GetInstance()->GetImage();
		PLCController::instance()->coaxialLight(false);
		CameraModel::GetInstance()->StartGrabbing();

		//拍完照，z轴需要回正2.44mm
		res = LimitMove::getInstance()->motion3DMoveRel(cv::Point3f(0, 0, -2.44), withCamera);
		if (!res.empty())
			return PrintLog(LogType::Error, res, !m_isTreeSystemRun);
		while (CheckModuleIsMoving(ModuleName::ImagingModuleXYZ))
		{
			QCoreApplication::processEvents();
			_sleep(100);
		}

		if (imgFid.empty())
			return PrintLog(LogType::Error, "Fiducial image acquisition failed!", !m_isTreeSystemRun);

		PrintLog(LogType::Normal, "Find fiduical center");
		cv::Vec3f fidCenter = ObtainCenterByDetectedCircle(imgFid);
		if (fidCenter == cv::Vec3f(0, 0, 0))
			return PrintLog(LogType::Error, "Fiducial recognition failed!", !m_isTreeSystemRun);
		center = cv::Point2f(fidCenter[0], fidCenter[1]);
		PrintLog(LogType::Normal, "fiduical center pixel coordinates: " + to_string(center.x) + ", " + to_string(center.y));
		return "";
	}

	std::string MotionProcess::ManualIdentifyFiducial(cv::Point2f& center, cv::Point3f fiducialAcs, bool isAuto)
	{
		//take fiducial image
		PrintLog(LogType::Normal, "Start moving fiducial into the MV");
		//Result ret = Motion3DModel::getInstance(motion3DType::withDUT)->Motion3DMoveAbsAsync(fiducialAcs.x * 1000,
		//	fiducialAcs.y * 1000, fiducialAcs.z * 1000);
		//if (!ret.success)
		//	return ret.errorMsg;

		//std::string res = LimitMove::getInstance()->motion3DMoveAbsAsync(cv::Point3f(fiducialAcs.x, fiducialAcs.y, fiducialAcs.z), withDUT);
		std::string res = LimitMove::getInstance()->motion3DMoveAbsAsync(cv::Point3f(fiducialAcs.x, fiducialAcs.y, fiducialAcs.z), withCamera);
		if (!res.empty())
			return res;
		while (CheckModuleIsMoving(ModuleName::ImagingModuleXYZ))
		{
			QCoreApplication::processEvents();
			_sleep(100);
		}
		PrintModulePosition(ModuleName::ImagingModuleXYZ);

		//由于MV对焦原因，z轴需要再移动2.44mm
		res = LimitMove::getInstance()->motion3DMoveRel(cv::Point3f(0, 0, 2.44), withCamera);
		if (!res.empty())
			return PrintLog(LogType::Error, res, !m_isTreeSystemRun);
		while (CheckModuleIsMoving(ModuleName::ImagingModuleXYZ))
		{
			QCoreApplication::processEvents();
			_sleep(100);
		}

		Sleep(1000);

#if 0
		CameraModel::GetInstance()->SetMLExposureAuto();
		Sleep(1500);

		if (isAuto)
		{
			emit messageBoxSignal("Please select fiducial manually, because fiducial can't be recognized by image.");
		}
		else
		{
			emit messageBoxSignal("Please select fiducial manually then click the confirm button.");
		}
		Result result = waitPause(true);
		if (!result.success)
		{
			return result.errorMsg;
		}
		if (m_fiducialPixel == cv::Point2f())
		{
			return "Manual calibrate fiducial error, fiducial pixel map is null.";
		}
		center = m_fiducialPixel;
#endif
		PrintLog(LogType::Normal, "Obtain fiducial image");
		PLCController::instance()->coaxialLight(true);
		if (m_is_auto_exposure)
		{
			CameraModel::GetInstance()->SetMLExposureAuto();
		}
		CameraModel::GetInstance()->StopGrabbing();
		CameraModel::GetInstance()->GrabOne();
		cv::Mat imgFid = CameraModel::GetInstance()->GetImage();
		PLCController::instance()->coaxialLight(false);
		CameraModel::GetInstance()->StartGrabbing();

		//拍完照，z轴需要回正2.44mm
		res = LimitMove::getInstance()->motion3DMoveRel(cv::Point3f(0, 0, -2.44), withCamera);
		if (!res.empty())
			return PrintLog(LogType::Error, res, !m_isTreeSystemRun);
		while (CheckModuleIsMoving(ModuleName::ImagingModuleXYZ))
		{
			QCoreApplication::processEvents();
			_sleep(100);
		}

		if (imgFid.empty())
			return PrintLog(LogType::Error, "Fiducial image acquisition failed!", !m_isTreeSystemRun);

		cv::imwrite(QDateTime::currentDateTime().toString("hhmmsszzz").toStdString() + "cv.tif", imgFid);

		QImage image = matToQImageCopy(imgFid);
		if (image.isNull())
			return PrintLog(LogType::Error, "Fiducial image conversion to QImage failed!", !m_isTreeSystemRun);
		image.save(QDateTime::currentDateTime().toString("hhmmsszzz") + "qt.tif");
		QPointF fiducialPos = emit onSignalgetFiducialPos(image);
		if (qFuzzyCompare(fiducialPos.x(), 0) || qFuzzyCompare(fiducialPos.y(), 0))
		{
			return "find fiducial failed!";
		}
		center = cv::Point2f(fiducialPos.x(), fiducialPos.y());
		PrintLog(LogType::Normal, "fiduical center pixel coordinates: " + to_string(center.x) + ", " + to_string(center.y));
		return "";
	}

	std::string MotionProcess::Ranging()
	{
		PrintLog(LogType::Normal, "[Ranging]");

		std::string msg = CheckModuleConnectStatus(ModuleName::DutModuleXYZ, ModuleName::keyence, ModuleName::PLC);
		if (msg != "")
			return msg;

		cv::Point3f keyence;
		keyence.x = m_processConfigInfo.offsetRoatate.rangingPos[currentDutName].rangingPos.x;
		keyence.y = m_processConfigInfo.offsetRoatate.rangingPos[currentDutName].rangingPos.y;
		keyence.z = m_processConfigInfo.offsetRoatate.rangingPos[currentDutName].rangingPos.z;

		msg = LimitMove::getInstance()->motion3DMoveAbsAsync(cv::Point3f(keyence.x, keyence.y, keyence.z), withDUT);
		if (!msg.empty())
			return msg;

		while (CheckModuleIsMoving(ModuleName::DutModuleXYZ))
		{
			QCoreApplication::processEvents();
			_sleep(100);
		}
		PrintLog(LogType::Normal, "dut motor move to ranging position end.");
		PrintModulePosition(ModuleName::DutModuleXYZ);

		auto res = PLCController::instance()->keyenceLight(true);
		if (!res.success)
			return PrintLog(LogType::Error, res.errorMsg, !m_isTreeSystemRun);
		_sleep(1000);

		KeyenceInfo info = ConfigItem::instance()->getKeyenceInfo();
		currentRangingPos = info.keyenceZeroPos - MLKeyenceCL::MakeRangeFinder()->GetPosition();
		if (-9999 == currentRangingPos)
			return PrintLog(LogType::Error, "Ranging failed!", !m_isTreeSystemRun);
		PrintLog(LogType::Normal, "current ranging: " + to_string(currentRangingPos));

		double safeRangMax = m_processConfigInfo.offsetRoatate.anticollision[currentDutName].rangingMax;
		double safeRangMin = m_processConfigInfo.offsetRoatate.anticollision[currentDutName].rangingMin;
		if (safeRangMin > currentRangingPos || safeRangMax < currentRangingPos)
		{
			std::string msg = "current ranging: " + to_string(currentRangingPos) + ", exceeding the safe range[" + to_string(safeRangMin) +
				", " + to_string(safeRangMax) + "]";
			return PrintLog(LogType::Error, msg, !m_isTreeSystemRun);
		}

		PLCController::instance()->keyenceLight(false);

		CORE::ML_Point3D currentPos = Motion3DModel::getInstance(motion3DType::withDUT)->getPosition(); //um
		double eyeBoxCenterKeyenceValue = m_processConfigInfo.offsetRoatate.eyeBoxCenterKeyenceValue[currentDutName];
		double heightDifference = currentRangingPos - eyeBoxCenterKeyenceValue;
		msg = LimitMove::getInstance()->motion3DMoveRel(cv::Point3f(0, 0, -heightDifference), withDUT);
		if (!msg.empty())
			return PrintLog(LogType::Error, msg, !m_isTreeSystemRun);
		while (CheckModuleIsMoving(ModuleName::DutModuleXYZ))
		{
			QCoreApplication::processEvents();
			_sleep(100);
		}
		PrintLog(LogType::Normal, "start adjust dut motor height.");
		PrintModulePosition(ModuleName::DutModuleXYZ);

		currentRangingPos -= heightDifference;
		PrintLog(LogType::Normal, "current ranging: " + to_string(currentRangingPos));

		return "";
	}

	std::string MotionProcess::InitConfig()
	{
		PrintLog(LogType::Normal, "[InitConfig]");

		if (!dutConfig::GetInstance().Load(DUT_FILE_NAME.c_str()))
		{
			return PrintLog(LogType::Error, "read dutconfig.json failed!", true);
		}
		if (!processConfig::GetInstance().Load(PROCESS_FILE_NAME.c_str()))
		{
			return PrintLog(LogType::Error, "read processConfig.json failed!", true);
		}
		if (!slbConfig::GetInstance().Load(SLB_FILE_NAME.c_str()))
		{
			return PrintLog(LogType::Error, "read slbConfig.json failed!", true);
		}

		m_dutConfigInfoMap = dutConfig::GetInstance().GetDutConfigInfo();
		m_processConfigInfo = processConfig::GetInstance().GetProcessConfigInfo();
		m_slbConfigInfo = slbConfig::GetInstance().GetSlbConfigInfo();

		connect(CollimatedLightTubeMode::GetInstance(), &CollimatedLightTubeMode::updateCollimatorAngle, this, [&](QString deltaX, QString deltaY) {
			m_collimatorDeltaX = deltaX;
			m_collimatorDeltaY = deltaY;
			});

		return "";
	}

	std::string MotionProcess::EntrancePupilAlignment()
	{
		PrintLog(LogType::Normal, "[EntrancePupilAlignment]");

		std::string msg = CheckModuleConnectStatus(ModuleName::DutModuleXYZ, ModuleName::DutModuleDxDyDz, ModuleName::ProjectionDxDy,
			ModuleName::ImagingModuleXYZ, ModuleName::ImagingModuleDxDy);
		if (msg != "")
			return msg;

		cv::Point2f inputOffset(m_dutConfigInfoMap[currentDutName].inputgratingOffset_.x,
			m_dutConfigInfoMap[currentDutName].inputgratingOffset_.y);

		//Move input to MV center absolute coordinates
		cv::Point2f inputAbs = DutAnyPointToMVCenterCoordinate(inputOffset, dutOffsetRotate);
		setSavePosition("inputAbsCoorInMVCenter", inputAbs);

		/*if (currentRangingPos == -9999)
			return PrintLog(LogType::Error, "Please measure the distance first!", !m_isTreeSystemRun);
		double HeightDifference = currentRangingPos - m_processConfigInfo.offsetRoatate.projectionKeyenceValue[currentDutName];*/

		PrintLog(LogType::Normal, "Start moving the entrance pupil to the center of the MV");

		CORE::ML_Point3D currentPos = Motion3DModel::getInstance(motion3DType::withDUT)->getPosition(); //um
		msg = LimitMove::getInstance()->motion3DMoveAbsAsync(cv::Point3f(inputAbs.x, inputAbs.y, currentPos.z / 1000.0), withDUT);
		if (!msg.empty())
			return PrintLog(LogType::Error, msg, !m_isTreeSystemRun);

		while (CheckModuleIsMoving(ModuleName::DutModuleXYZ))
		{
			QCoreApplication::processEvents();
			_sleep(100);
		}
		PrintModulePosition(ModuleName::DutModuleXYZ);

		PrintLog(LogType::Normal, "Start moving the entrance pupil from the center of the MV to align with the exit pupil of the projection module.");

		//adjust Z to walkoffDistance
		//msg = LimitMove::getInstance()->motion3DMoveRel(cv::Point3f(0, 0, -HeightDifference), withDUT);
		//if (!msg.empty())
		//	return PrintLog(LogType::Error, msg, !m_isTreeSystemRun);
		//while (CheckModuleIsMoving(ModuleName::DutModuleXYZ))
		//{
		//	QCoreApplication::processEvents();
		//	_sleep(100);
		//}
		//PrintModulePosition(ModuleName::DutModuleXYZ);
		////update ranging pos
		//currentRangingPos -= HeightDifference;

		// adjust x, y
		double offsetx = m_processConfigInfo.offsetRoatate.projectionOffsetRelativeToMV[currentDutName].x;
		double offsety = m_processConfigInfo.offsetRoatate.projectionOffsetRelativeToMV[currentDutName].y;

		//safe verification
		double safePosXMax = m_processConfigInfo.offsetRoatate.anticollision[currentDutName].inputAlignmentMotorPosXMax;
		double safePosXMin = m_processConfigInfo.offsetRoatate.anticollision[currentDutName].inputAlignmentMotorPosXMin;
		double safePosYMax = m_processConfigInfo.offsetRoatate.anticollision[currentDutName].inputAlignmentMotorPosYMax;
		double safePosYMin = m_processConfigInfo.offsetRoatate.anticollision[currentDutName].inputAlignmentMotorPosYMin;
		CORE::ML_Point3D currentPos1 = Motion3DModel::getInstance(motion3DType::withDUT)->getPosition(); //um
		if (currentPos1.y / 1000.0 + offsety > safePosYMax || currentPos1.y / 1000.0 + offsety < safePosYMin)
		{
			msg = "Y motor is about to be moved to " + to_string(currentPos1.y / 1000.0 + offsety) +
				", which has exceeded the safe range[" + to_string(safePosYMin) + ", " + to_string(safePosYMax) + "]";
			return PrintLog(LogType::Error, msg, !m_isTreeSystemRun);
		}
		if (currentPos1.x / 1000.0 + offsetx > safePosXMax || currentPos1.x / 1000.0 + offsetx < safePosXMin)
		{
			msg = "X motor is about to be moved to " + to_string(currentPos1.x / 1000.0 + offsetx) +
				", which has exceeded the safe range[" + to_string(safePosXMin) + ", " + to_string(safePosXMax) + "]";
			return PrintLog(LogType::Error, msg, !m_isTreeSystemRun);
		}

		msg = LimitMove::getInstance()->motion3DMoveRel(cv::Point3f(0, offsety, 0), withDUT);
		if (!msg.empty())
			return PrintLog(LogType::Error, msg, !m_isTreeSystemRun);
		while (CheckModuleIsMoving(ModuleName::DutModuleXYZ))
		{
			QCoreApplication::processEvents();
			_sleep(100);
		}

		msg = LimitMove::getInstance()->motion3DMoveRel(cv::Point3f(offsetx, 0, 0), withDUT);
		if (!msg.empty())
			return PrintLog(LogType::Error, msg, !m_isTreeSystemRun);
		while (CheckModuleIsMoving(ModuleName::DutModuleXYZ))
		{
			QCoreApplication::processEvents();
			_sleep(100);
		}
		PrintModulePosition(ModuleName::DutModuleXYZ);

		//Restore imaging module height
		PrintLog(LogType::Normal, "Restore imaging module height");
		CORE::ML_Point3D currentImagingPos = Motion3DModel::getInstance(motion3DType::withCamera)->getPosition(); //um
		double imagingFixedPosZ = m_processConfigInfo.offsetRoatate.imagingFixedPos[currentDutName].z;
		msg = LimitMove::getInstance()->motion3DMoveAbsAsync(cv::Point3f(currentImagingPos.x / 1000.0, currentImagingPos.y / 1000.0, imagingFixedPosZ), withCamera);
		if (!msg.empty())
			return PrintLog(LogType::Error, msg, !m_isTreeSystemRun);
		while (CheckModuleIsMoving(ModuleName::ImagingModuleXYZ))
		{
			QCoreApplication::processEvents();
			_sleep(100);
		}
		PrintModulePosition(ModuleName::ImagingModuleXYZ);

		return "";
	}

	std::string MotionProcess::PrintLog(LogType logType, std::string logMsg, bool isPopUpEnable)
	{
		switch (logType)
		{
		case LogType::Normal:
			LoggingWrapper::instance()->info(QString::fromStdString(logMsg));
			if (isPopUpEnable)
				QMessageBox::information(NULL, "Info", QString::fromStdString(logMsg), QMessageBox::Yes);
			break;
		case LogType::Warn:
			LoggingWrapper::instance()->warn(QString::fromStdString(logMsg));
			if (isPopUpEnable)
				QMessageBox::warning(NULL, "Warning", QString::fromStdString(logMsg), QMessageBox::Yes);
			break;
		case LogType::Error:
			LoggingWrapper::instance()->error(QString::fromStdString(logMsg));
			if (isPopUpEnable)
				QMessageBox::warning(NULL, "Error", QString::fromStdString(logMsg), QMessageBox::Yes);
			break;
		}

		return logMsg;
	}
	std::string MotionProcess::FindFiducialToCalculate()
	{
		PrintLog(LogType::Normal, "[FindFiducialToCalculate]");

		std::string msg = CheckModuleConnectStatus(ModuleName::DutModuleXYZ, ModuleName::ImagingModuleXYZ, ModuleName::MV);
		if (msg != "")
			return msg;

		//adjust imaging module height to fiducial height
		PrintLog(LogType::Normal, "adjust imaging module height to fiducial height");
		double fiducialZMotorPos = m_dutConfigInfoMap[currentDutName].fiducialZMotorPos;
		CORE::ML_Point3D currentPos = Motion3DModel::getInstance(motion3DType::withCamera)->getPosition(); //um
		msg = LimitMove::getInstance()->motion3DMoveAbsAsync(cv::Point3f(currentPos.x / 1000.0, currentPos.y / 1000.0, fiducialZMotorPos), withCamera);
		if (!msg.empty())
			return PrintLog(LogType::Error, msg, !m_isTreeSystemRun);
		while (CheckModuleIsMoving(ModuleName::ImagingModuleXYZ))
		{
			QCoreApplication::processEvents();
			_sleep(100);
		}
		PrintModulePosition(ModuleName::ImagingModuleXYZ);

		std::vector<cv::Point3f> fiducialMotorPosition;

		cv::Point3f originAbsCoordinates(m_dutConfigInfoMap[currentDutName].originPointMotorPositionInMV_.x,
			m_dutConfigInfoMap[currentDutName].originPointMotorPositionInMV_.y, 0);

		//fiducial1, 2 absolute coordinates
		CORE::ML_Point3D currentDutPos = Motion3DModel::getInstance(motion3DType::withDUT)->getPosition(); //um
		cv::Point3f fid1(originAbsCoordinates.x - m_dutConfigInfoMap[currentDutName].fiducialOffset_[1].x,
			originAbsCoordinates.y - m_dutConfigInfoMap[currentDutName].fiducialOffset_[1].y, currentDutPos.z / 1000.0);
		fiducialMotorPosition.push_back(fid1);

		cv::Point3f fid2(originAbsCoordinates.x - m_dutConfigInfoMap[currentDutName].fiducialOffset_[2].x,
			originAbsCoordinates.y - m_dutConfigInfoMap[currentDutName].fiducialOffset_[2].y, currentDutPos.z / 1000.0);
		fiducialMotorPosition.push_back(fid2);

		std::vector <cv::Point2f> fiducialCenter;
		for (int i = 0; i < fiducialMotorPosition.size(); ++i)
		{
			PrintLog(LogType::Normal, "start to find fiducial " + to_string(i + 1));

			cv::Point2f center;
			if (m_bIsAutoFiducial)
			{
				msg = AutoIdentifyFiducial(center, fiducialMotorPosition[i]);
				if (!msg.empty())
				{
					QMessageBox::StandardButton result = QMessageBox::No;
					QMetaObject::invokeMethod(qApp, [&]() {
						result = QMessageBox::question(
							nullptr,
							"Fiducial Error",
							"Auto find fiducial error,\nPlease confirm whether to search manually.",
							QMessageBox::Yes | QMessageBox::No
						);
						}, Qt::BlockingQueuedConnection);

					if (result == QMessageBox::Yes) {
						msg = ManualIdentifyFiducial(center, fiducialMotorPosition[i], true);
						if (!msg.empty())
							return msg;
					}
					else {
						return msg;
					}
				}
			}
			else
			{
				msg = ManualIdentifyFiducial(center, fiducialMotorPosition[i]);
				if (!msg.empty())
					return msg;
			}
			fiducialCenter.push_back(center);
		}

		PrintLog(LogType::Normal, "Calculate the absolute coordinates of the fiducial motor at the center of the MV");
		// Calculate the absolute coordinates of fiducial moving to the center of MV
		std::vector<cv::Point2f> pixelCoor = { cv::Point2f(fiducialCenter[0]) , cv::Point2f(fiducialCenter[1]) };
		std::vector<cv::Point3f> motorCoor = { fid1 , fid2 };
		int width, height;
		ConfigItem::instance()->getCameraResolution(width, height);
		cv::Point2f mvCenter(width / 2.0, height / 2.0);

		setSavePosition("pixelCoor1", pixelCoor[0]);
		setSavePosition("pixelCoor2", pixelCoor[1]);
		setSavePosition("motorCoor1", fid1);
		setSavePosition("motorCoor2", fid2);
		setSavePosition("mvCenter", mvCenter);
		std::vector<cv::Point2f> mvCenterAbsCoor = CalculateFidAbsPosInMVCenter(pixelCoor, motorCoor, mvCenter);
		setSavePosition("mvCenterAbsCoor1", mvCenterAbsCoor[0]);
		setSavePosition("mvCenterAbsCoor2", mvCenterAbsCoor[1]);
		PrintLog(LogType::Normal, "Fiducial 1 in MV center motor absolute coordinates: " + to_string(mvCenterAbsCoor[0].x) + ", " + to_string(mvCenterAbsCoor[0].y));
		PrintLog(LogType::Normal, "Fiducial 2 in MV center motor absolute coordinates: " + to_string(mvCenterAbsCoor[1].x) + ", " + to_string(mvCenterAbsCoor[1].y));

		// calculate dut rotate and offset
		PrintLog(LogType::Normal, "Calculate dut rotate and offset");
		std::vector<cv::Point2f> fidOffsetFromOrigin = { cv::Point2f(m_dutConfigInfoMap[currentDutName].fiducialOffset_[1].x,
			m_dutConfigInfoMap[currentDutName].fiducialOffset_[1].y), cv::Point2f(m_dutConfigInfoMap[currentDutName].fiducialOffset_[2].x,
			m_dutConfigInfoMap[currentDutName].fiducialOffset_[2].y) };
		dutOffsetRotate = CalculateOffsetAndRotate(mvCenterAbsCoor, fidOffsetFromOrigin);

		//配置文件第一个fid作为origin点，则角度-180，否则+180
		int compensation = -180;
		if (fidOffsetFromOrigin[0].x == 0 && fidOffsetFromOrigin[0].y == 0)
			compensation = -180;
		else
			compensation = 180;

		PrintLog(LogType::Normal, "dut angle: " + to_string(dutOffsetRotate.rotate * 180.0 / M_PI + compensation) + ", dut offset(" +
			to_string(dutOffsetRotate.offset.x) + ", " + to_string(dutOffsetRotate.offset.y) + ")");
		setSavePosition("fidOffsetFromOrigin", fidOffsetFromOrigin[0]);
		setSavePosition("fidOffsetFromOrigin", fidOffsetFromOrigin[1]);
		setSavePosition("dutOffset", dutOffsetRotate.offset);
		setSavePosition("dutRotate", dutOffsetRotate.rotate * 180.0 / M_PI + compensation);

		PrintLog(LogType::Normal, "dut offset: " + to_string(dutOffsetRotate.offset.x) + ", " + to_string(dutOffsetRotate.offset.y));
		PrintLog(LogType::Normal, "dut angle: " + to_string(dutOffsetRotate.rotate * 180.0 / M_PI + compensation));

		return "";
	}
	std::string MotionProcess::EyeboxScanning(int eyeBoxIndex)
	{
		PrintLog(LogType::Normal, "[EyeboxScanning], eyeBox " + to_string(eyeBoxIndex));

		if (!m_dutConfigInfoMap[currentDutName].outputgratingOffset_.count(eyeBoxIndex))
			return PrintLog(LogType::Error, "No eyebox"+ std::to_string(eyeBoxIndex) + " information in the map, Please re - enter the eyebox scanning list.", !m_isTreeSystemRun);

		std::string msg = CheckModuleConnectStatus(ModuleName::ImagingModuleXYZ);
		if (msg != "")
			return msg;

		if (currentRangingPos == -9999)
			return PrintLog(LogType::Error, "Please measure the distance first!", !m_isTreeSystemRun);

		CORE::ML_Point3D currentPos = Motion3DModel::getInstance(motion3DType::withCamera)->getPosition(); //um

		PrintLog(LogType::Normal, "Calculate the motor movement height based on eyeRelief");
		double eyeBoxCenter_currentEyeBoxDifference = m_dutConfigInfoMap[currentDutName].outputgratingOffset_[5].z -
			m_dutConfigInfoMap[currentDutName].outputgratingOffset_[eyeBoxIndex].z;
		double currentEyeBoxNeedHeight = m_processConfigInfo.offsetRoatate.eyeBoxCenterKeyenceValue[currentDutName] - eyeBoxCenter_currentEyeBoxDifference;
		double heightDifference = currentRangingPos - currentEyeBoxNeedHeight;
		double imgingAbsZPos = currentPos.z / 1000.0 + heightDifference; //mm
		setSavePosition("imgingAbsZPos", imgingAbsZPos);

		PrintLog(LogType::Normal, "Calculate the absolute coordinates from the entrance pupil of the imaging module to the center of the MV");
		double eyeBoxInitPosX = m_processConfigInfo.offsetRoatate.imagingFixedPos[currentDutName].x; //mm
		double eyeBoxInitPosY = m_processConfigInfo.offsetRoatate.imagingFixedPos[currentDutName].y;
		double imagingOffsetRelativeToMVX = m_processConfigInfo.offsetRoatate.imagingOffsetRelativeToMV[currentDutName].x;
		double imagingOffsetRelativeToMVY = m_processConfigInfo.offsetRoatate.imagingOffsetRelativeToMV[currentDutName].y;
		//Absolute coordinates of the entrance pupil of the imaging module at the center of the MV
		double absCoorInMVCenterX = eyeBoxInitPosX - imagingOffsetRelativeToMVX;
		double absCoorInMVCenterY = eyeBoxInitPosY - imagingOffsetRelativeToMVY;
		setSavePosition("absCoorInMVCenterX", absCoorInMVCenterX);
		setSavePosition("absCoorInMVCenterY", absCoorInMVCenterY);
		PrintLog(LogType::Normal, "The absolute coordinates of the imaging module at the center of the MV (" + to_string(absCoorInMVCenterX) +
			", " + to_string(absCoorInMVCenterY) + ")");

		PrintLog(LogType::Normal, "Calculate the offset from eyeBox " + to_string(eyeBoxIndex) + " to MV center");

		//calculate eyebox point offset relative to MV
		cv::Point2f inputOffset(m_dutConfigInfoMap[currentDutName].inputgratingOffset_.x,
			m_dutConfigInfoMap[currentDutName].inputgratingOffset_.y);
		//cv::Point2f inputAbs = DutAnyPointToMVCenterCoordinate(inputOffset, dutOffsetRotate);
		cv::Point2f outputOffset(m_dutConfigInfoMap[currentDutName].outputgratingOffset_[eyeBoxIndex].x,
			m_dutConfigInfoMap[currentDutName].outputgratingOffset_[eyeBoxIndex].y);
		//cv::Point2f outputAbs = DutAnyPointToMVCenterCoordinate(outputOffset, dutOffsetRotate);
		Eigen::Vector2d deltaP(outputOffset.x - inputOffset.x, outputOffset.y - inputOffset.y);
		//When the input is at the center of the MV, the offset of eyeBox relative to the center of the MV
		Eigen::Rotation2Dd rot(dutOffsetRotate.rotate);
		Eigen::Vector2d eyeBoxOffset_ = rot * deltaP;
		//input平移后，eyeBox相对于MV中心偏移 

		double p_offsetx = m_processConfigInfo.offsetRoatate.projectionOffsetRelativeToMV[currentDutName].x;
		double p_offsety = m_processConfigInfo.offsetRoatate.projectionOffsetRelativeToMV[currentDutName].y;
		cv::Point2f eyeBoxOffsetRelativeToMV(-eyeBoxOffset_.x() + p_offsetx, -eyeBoxOffset_.y() + p_offsety);

		setSavePosition("eyeBoxOffsetRelativeToMVX", eyeBoxOffsetRelativeToMV.x);
		setSavePosition("eyeBoxOffsetRelativeToMVY", eyeBoxOffsetRelativeToMV.y);

		double absCoorInEyeBoxX = absCoorInMVCenterX + eyeBoxOffsetRelativeToMV.x;
		double absCoorInEyeBoxY = absCoorInMVCenterY + eyeBoxOffsetRelativeToMV.y;
		setSavePosition("absCoorInEyeBoxX", absCoorInEyeBoxX);
		setSavePosition("absCoorInEyeboxY", absCoorInEyeBoxY);
		PrintLog(LogType::Normal, "Absolute coordinates from the entrance pupil of the imaging module to eyeBox " + to_string(eyeBoxIndex) + "("
			+ to_string(absCoorInEyeBoxX) + ", " + to_string(absCoorInEyeBoxY) + ", " + to_string(imgingAbsZPos) + ")");

		//Motion3DModel::getInstance(withCamera)->Motion3DMoveAbsAsync(absCoorInEyeBoxX * 1000, absCoorInEyeBoxY * 1000, imgingAbsZPos * 1000);
		msg = LimitMove::getInstance()->motion3DMoveAbsAsync(cv::Point3f(absCoorInEyeBoxX, absCoorInEyeBoxY, imgingAbsZPos), withCamera);
		if (!msg.empty())
			return PrintLog(LogType::Error, msg, !m_isTreeSystemRun);

		while (CheckModuleIsMoving(ModuleName::ImagingModuleXYZ))
		{
			QCoreApplication::processEvents();
			_sleep(100);
		}
		PrintModulePosition(ModuleName::ImagingModuleXYZ);

		currentRangingPos -= heightDifference;

		return "";
	}
	void MotionProcess::setTreeSystemRun(bool isRun)
	{
		m_isTreeSystemRun = isRun;
	}
	std::string MotionProcess::CheckModuleConnectStatus(ModuleName moduleName)
	{
		switch (moduleName)
		{
		case ModuleName::DutModuleXYZ:
			if (!Motion3DModel::getInstance(motion3DType::withDUT)->Motion3DisConnected())
				return PrintLog(LogType::Error, "Dut module XYZ motor not connected!", !m_isTreeSystemRun);
			break;
		case ModuleName::DutModuleDxDyDz:
			if (!OrientalMotorControl::getInstance()->IsConnected())
				return PrintLog(LogType::Error, "Dut module dxdydz motor not connected!", !m_isTreeSystemRun);
			break;
		case ModuleName::ImagingModuleXYZ:
			if (!Motion3DModel::getInstance(motion3DType::withCamera)->Motion3DisConnected())
				return PrintLog(LogType::Error, "Imaging module XYZ motor not connected!", !m_isTreeSystemRun);
			break;
		case ModuleName::Collimator:
			if (!CollimatedLightTubeMode::GetInstance()->IsConnected())
				return PrintLog(LogType::Error, "Collimate not connected!", !m_isTreeSystemRun);
			break;
		case ModuleName::PLC:
			if (!PLCController::instance()->IsConnected())
				return PrintLog(LogType::Error, "PLC not connected!", !m_isTreeSystemRun);
			break;
		case ModuleName::MV:
			if (!CameraModel::GetInstance()->isConnected())
				return PrintLog(LogType::Error, "MV not connected!", !m_isTreeSystemRun);
			break;
		case ModuleName::keyence:
			if (!MLKeyenceCL::MakeRangeFinder()->IsConnected())
				return PrintLog(LogType::Error, "Keyence not connected!", !m_isTreeSystemRun);
			break;
		case ModuleName::ProjectionDxDy:
			if (!Motion2DModel::getInstance(ACS2DPro)->GetIsConnected())
				return PrintLog(LogType::Error, "Projection module dx, dy motor not connected!", !m_isTreeSystemRun);
			break;
		case ModuleName::ImagingModuleDxDy:
			if (!Motion2DModel::getInstance(ACS2DCameraTilt)->GetIsConnected())
				return PrintLog(LogType::Error, "Imaging module dx, dy motor not connected!", !m_isTreeSystemRun);
			break;
		}

		return "";
	}
	void MotionProcess::PrintModulePosition(ModuleName moduleName)
	{
		std::string posMsg;
		switch (moduleName)
		{
		case ModuleName::DutModuleXYZ:
		{
			_sleep(1000);
			CORE::ML_Point3D pos = Motion3DModel::getInstance(motion3DType::withDUT)->getPosition();
			posMsg = "Current dut module xyz position [" + std::to_string(pos.x / 1000.0) + ", " + std::to_string(pos.y / 1000.0) +
				", " + std::to_string(pos.z / 1000.0) + "]";
			PrintLog(LogType::Normal, posMsg);
			break;
		}
		case ModuleName::ImagingModuleXYZ:
		{
			_sleep(1000);
			CORE::ML_Point3D pos = Motion3DModel::getInstance(motion3DType::withCamera)->getPosition();
			posMsg = "Current imaging module xyz position [" + std::to_string(pos.x / 1000.0) + ", " + std::to_string(pos.y / 1000.0) +
				", " + std::to_string(pos.z / 1000.0) + "]";
			PrintLog(LogType::Normal, posMsg);
			break;
		}
		case ModuleName::ProjectionDxDy:
		{
			_sleep(1000);
			float dx, dy;
			Motion2DModel::getInstance(ACS2DPro)->getPosition(dx, dy);
			posMsg = "Current projection module dx,dy position [" + std::to_string(dx) + ", " + std::to_string(dy) + "]";
			PrintLog(LogType::Normal, posMsg);
			break;
		}
		case ModuleName::ImagingModuleDxDy:
		{
			_sleep(1000);
			float dx, dy;
			Motion2DModel::getInstance(ACS2DCameraTilt)->getPosition(dx, dy);
			posMsg = "Current imaging module dx,dy position [" + std::to_string(dx) + ", " + std::to_string(dy) + "]";
			PrintLog(LogType::Normal, posMsg);
			break;
		}
		}
	}
	bool MotionProcess::CheckModuleIsMoving(ModuleName moduleName)
	{
		switch (moduleName)
		{
		case ModuleName::DutModuleXYZ:
			return Motion3DModel::getInstance(motion3DType::withDUT)->is3DMoving();
		case ModuleName::ImagingModuleXYZ:
			return Motion3DModel::getInstance(motion3DType::withCamera)->is3DMoving();
		case ModuleName::ProjectionDxDy:
			return Motion2DModel::getInstance(ACS2DPro)->is2DMoving();
		case ModuleName::ImagingModuleDxDy:
			return Motion2DModel::getInstance(ACS2DCameraTilt)->is2DMoving();
		case ModuleName::DutModuleDxDyDz:
			return OrientalMotorControl::getInstance()->IsMoving();
		case ModuleName::PLC:
			return Motion2DModel::getInstance(PLC2D)->is2DMoving();
		}
	}

	void MotionProcess::StopModuleMove(ModuleName moduleName)
	{
		Result ret;
		switch (moduleName)
		{
		case ModuleName::DutModuleXYZ:
		{
			ret = Motion3DModel::getInstance(motion3DType::withDUT)->Motion3DMoveStop();
			if (ret.success)
				PrintLog(LogType::Normal, "Tree System forced termination, dut module xyz stop move.");
			else
				PrintLog(LogType::Error, "Tree System forced termination, dut module xyz stop move error," + ret.errorMsg);
			break;
		}
		case ModuleName::ImagingModuleXYZ:
		{
			ret = Motion3DModel::getInstance(motion3DType::withCamera)->Motion3DMoveStop();
			if (ret.success)
				PrintLog(LogType::Normal, "Tree System forced termination, imaging module xyz stop move.");
			else
				PrintLog(LogType::Error, "Tree System forced termination, imaging module xyz stop move error," + ret.errorMsg);
			break;
		}
		case ModuleName::ProjectionDxDy:
		{
			ret = Motion2DModel::getInstance(ACS2DPro)->Motion2DMoveStop();
			if (ret.success)
				PrintLog(LogType::Normal, "Tree System forced termination, projector module dxdy stop move.");
			else
				PrintLog(LogType::Error, "Tree System forced termination, projector module dxdy stop move error," + ret.errorMsg);
			break;
		}
		case ModuleName::ImagingModuleDxDy:
		{
			ret = Motion2DModel::getInstance(ACS2DCameraTilt)->Motion2DMoveStop();
			if (ret.success)
				PrintLog(LogType::Normal, "Tree System forced termination, imaging module dxdy stop move.");
			else
				PrintLog(LogType::Error, "Tree System forced termination, imaging module dxdy stop move error," + ret.errorMsg);
			break;
		}
		case ModuleName::DutModuleDxDyDz:
		{
			ret = OrientalMotorControl::getInstance()->StopMove();
			if (ret.success)
				PrintLog(LogType::Normal, "Tree System forced termination, dut module dxdydz stop move.");
			else
				PrintLog(LogType::Error, "Tree System forced termination, dut module dxdydz stop move error," + ret.errorMsg);
			break;
		}
		}
	}

	cv::Vec3f MotionProcess::ObtainCenterByDetectedCircle(cv::Mat& fidImg)
	{
		if (fidImg.empty()) {
			PrintLog(LogType::Error, "Fiducial image is NULL!", !m_isTreeSystemRun);
			return cv::Vec3f();
		}
#if 0
		cv::Mat gray;
		if (fidImg.channels() == 3)
			cv::cvtColor(fidImg, gray, cv::COLOR_BGR2GRAY);
		else
			gray = fidImg.clone();

		cv::Mat blurred;
		GaussianBlur(gray, blurred, Size(3, 3), 2);

		//cv::threshold

		cv::Mat edges;
		cv::Canny(blurred, edges, 25, 80);

		vector<Vec3f> circles;
		HoughCircles(edges, circles, cv::HOUGH_GRADIENT, 1, 2000, 80, 20, 130, 200);

		// 在原图上绘制检测到的圆
		for (size_t i = 0; i < circles.size(); ++i) {
			circle(fidImg, Point(circles[i][0], circles[i][1]), circles[i][2], Scalar(0, 255, 0), 3, 8);
			circle(fidImg, Point(circles[i][0], circles[i][1]), 10, Scalar(250, 0, 0), -1, 8);
		}

		if (circles.size() != 1)
		{
			PrintLog(LogType::Error, "Unable to detect circle!", !m_isTreeSystemRun);
			return cv::Vec3f();
		}

		return circles[0];
#endif
		MLImageDetection::FiducialDetect fidDetector;
		MLImageDetection::FiducialRe res = fidDetector.getFiducialCoordinate(fidImg);

		if (m_bSaveFiducialImage)
		{
			QString ct = QDateTime::currentDateTime().toString("HHmmsszzz");
			QString strSavePath = QString(m_strFiducialRootDir + "/" + m_dutSeq + "/fiducial/");
			QDir dir(strSavePath);
			if (!dir.exists())
			{
				dir.mkpath(strSavePath);
			}
			QString orignalPath = strSavePath + "fiducial_original_" + ct + (res.flag ? "OK.tif" : "NG.tif");
			cv::imwrite(orignalPath.toStdString(), fidImg);
			if (res.flag)
			{
				QString resultPath = strSavePath + "fiducial_result_" + ct + (res.flag ? "OK.png" : "NG.png");
				cv::imwrite(resultPath.toStdString(), res.imgdraw);
			}
		}

		if (!res.flag)
		{
			PrintLog(LogType::Error, "Fiducial recognition failed, " + res.errMsg, !m_isTreeSystemRun);
			return cv::Vec3f();
		}
		return cv::Vec3f(res.loc.x, res.loc.y, 0);
	}

	std::vector<cv::Point2f> MotionProcess::CalculateFidAbsPosInMVCenter(std::vector<cv::Point2f> pixelCoor, std::vector<cv::Point3f> motorCoor, cv::Point2f mvCenter)
	{
		//输入两个fid在MV中心附近的像素坐标，两个fid在MV中心附近的电机绝对坐标 

		vector<cv::Point2f> fiducialMotorLocation;
		for (int i = 0; i < pixelCoor.size(); i++)
		{
			float pixelSize, magnification;
			ConfigItem::instance()->getCameraCoefficient(pixelSize, magnification);

			//Fiducial offset from MV center (mm)
			cv::Point2f mvCenterOffset;
			cv::Point2f cameraOffset = mvCenter - pixelCoor[i];
			mvCenterOffset = cameraOffset * pixelSize / magnification / 1000.0;
			PrintLog(LogType::Normal, "Fiducial_" + std::to_string(i) + " distance mv center offset (" + to_string(mvCenterOffset.x)
				+ to_string(mvCenterOffset.y) + ")");

			//Calculate the absolute coordinates of moving fiducial to the center of MV
			cv::Point3f fiducialMotor;
			fiducialMotor.x = motorCoor[i].x + mvCenterOffset.y;
			fiducialMotor.y = motorCoor[i].y - mvCenterOffset.x;
			fiducialMotor.z = motorCoor[i].z;
			PrintLog(LogType::Normal, "Calculate the absolute coordinates of Fiducial_" + std::to_string(i) + " moving to the center of MV ("
				+ to_string(fiducialMotor.x) + to_string(fiducialMotor.y) + to_string(fiducialMotor.z) + ")");

			cv::Point2f point;
			point.x = fiducialMotor.x;
			point.y = fiducialMotor.y;
			fiducialMotorLocation.push_back(point);
		}
		return fiducialMotorLocation;
	}

	OffsetRotate MotionProcess::CalculateOffsetAndRotate(std::vector<cv::Point2f> actual, std::vector<cv::Point2f> design)
	{
		OffsetRotate result;

		if (actual.size() != 2 || design.size() != 2)
		{
			throw std::runtime_error("Two fiducials are needed for calculation!");
		}

		Eigen::Vector2d v_design(design[1].x - design[0].x, design[1].y - design[0].y);
		Eigen::Vector2d v_actual(actual[1].x - actual[0].x, actual[1].y - actual[0].y);

		double theta = std::atan2(v_actual.y(), v_actual.x()) - std::atan2(v_design.y(), v_design.x());
		result.rotate = theta;

		Eigen::Rotation2Dd rot(theta);

		if (design[0].x == 0 && design[0].y == 0)
		{
			Eigen::Vector2d fid1_design(design[1].x, design[1].y);
			Eigen::Vector2d fid1_rotated = rot * fid1_design;
			result.offset.x = actual[1].x - fid1_rotated.x();
			result.offset.y = actual[1].y - fid1_rotated.y();
		}
		else
		{
			Eigen::Vector2d fid1_design(design[0].x, design[0].y);
			Eigen::Vector2d fid1_rotated = rot * fid1_design;
			result.offset.x = actual[0].x - fid1_rotated.x();
			result.offset.y = actual[0].y - fid1_rotated.y();
		}

		result.result = true;

		return result;
	}

	cv::Point3f MotionProcess::RestoreOriginalPointToMVCenterCoordinate(cv::Point2f point, OffsetRotate offsetRotate)
	{
		double restoreAngle = -offsetRotate.rotate;

		Eigen::Vector2d measure_original(point.x, point.y);

		Eigen::Vector2d offset(offsetRotate.offset.x, offsetRotate.offset.y);

		Eigen::Vector2d move = offset + measure_original;

		return cv::Point3f(move.x(), move.y(), restoreAngle);
	}

	void MotionProcess::continueRun(bool isContinue, cv::Point2f pixel)
	{
		if (isContinue)
		{
			m_fiducialPixel = pixel;
		}

		if (!isContinue)
		{
			Motion3DModel::getInstance(withCamera)->Motion3DMoveStop();
			Motion3DModel::getInstance(withDUT)->Motion3DMoveStop();

			//AlignUtils::fiducialLight(false);
		}

		m_waitPause.continueRun(isContinue);
		emit notifyStopSignal(!isContinue);
	}

	void MotionProcess::notifyPause(bool isPause)
	{
		m_waitPause.notifyPause(isPause);
	}

	bool MotionProcess::isStop()
	{
		return m_waitPause.isStop();
	}

	std::string MotionProcess::GetDutTypeName(std::string eye_type, std::string eye_relief, std::string& dut_name)
	{
		std::string state = eye_type;
		std::string msg ="";
		if (msg != "")
			return msg;

		std::map<std::string, std::string> state_name_map;
		if (!DutTypeConfig::GetInstance().Load(SENSOR_DUT_TYPE.c_str()))
			return "dut type config file load error.";
		state_name_map = DutTypeConfig::GetInstance().GetDutTypeConfig();
		if (!state_name_map.count(state))
			return "dut type config file not contain " + state + " state";
		if (state_name_map[state] == "None")
			return state + " holder state is None.";

		dut_name = state_name_map[state] + "_" + eye_relief;
		currentDutName = dut_name;

		return "";
	}

	std::string MotionProcess::SetIsSaveFiducialImage(bool isSave, QString rootDir, QString dutSeq)
	{
		m_bSaveFiducialImage = isSave;
		m_strFiducialRootDir = rootDir;
		m_dutSeq = dutSeq;
		return "";
	}

	std::string MotionProcess::ImagingMove(float x, float y)
	{
		PrintLog(LogType::Normal, "[ImagingMoving]");

		std::string msg;

		if (x > 8 || x < -8 || y > 8 || y < -8)
		{
			msg = "postion is over limit";
			return msg;
		}

		CORE::ML_Point3D currentImagingPos = Motion3DModel::getInstance(motion3DType::withCamera)->getPosition();

		msg = LimitMove::getInstance()->motion3DMoveAbsAsync(cv::Point3f(currentImagingPos.x/1000.0 + x, currentImagingPos.y/1000.0 + y, currentImagingPos.z / 1000.0), withCamera);
		if (!msg.empty())
			return msg;
		while (CheckModuleIsMoving(ModuleName::ImagingModuleXYZ))
		{
			QCoreApplication::processEvents();
			_sleep(100);
		}
		PrintLog(LogType::Normal, "image motor move to position end.");
		return "";
	}

	std::string MotionProcess::LoadImaging()
	{
		PrintLog(LogType::Normal, "[LoadImaging]");
		//三轴PI电机第一次连接归零后不能立刻动
		_sleep(5000);
		//先把3轴电机移动到安全距离
		std::string msg = CheckModuleConnectStatus(ModuleName::ImagingModuleXYZ);
		//if (msg != "")
		//	return msg;
		//double safePosX = -60;
		//double safePosY = 0;
		//double safePosZ = 30.5;
		//msg = LimitMove::getInstance()->moveAbsToSafePos(cv::Point3f(safePosX, safePosY, safePosZ),withCamera);

		//if (!msg.empty())
		//	return msg;
		//while (CheckModuleIsMoving(ModuleName::ImagingModuleXYZ))
		//{
		//	QCoreApplication::processEvents();
		//	_sleep(100);
		//}

		//adjust plc module tiptilt
		double imgingTiptiltX = m_processConfigInfo.offsetRoatate.imagingTiptilt[currentDutName].x;
		double imgingTiptiltY = m_processConfigInfo.offsetRoatate.imagingTiptilt[currentDutName].y;
		Motion2DModel::getInstance(motion2DType::PLC2D)->Motion2DMoveAbsAsync(imgingTiptiltX, imgingTiptiltY);
		//while (CheckModuleIsMoving(ModuleName::PLC))
		//{
		//	QCoreApplication::processEvents();
		//	_sleep(100);
		//}
		PrintLog(LogType::Normal, "plc module adjust tiptilt end.");

		msg = CheckModuleConnectStatus(ModuleName::ImagingModuleXYZ);
		if (msg != "")
			return msg;

		double imagingFixedPosX = m_processConfigInfo.offsetRoatate.imagingFixedPos[currentDutName].x;
		double imagingFixedPosY = m_processConfigInfo.offsetRoatate.imagingFixedPos[currentDutName].y;
		double imagingFixedPosZ = m_processConfigInfo.offsetRoatate.imagingFixedPos[currentDutName].z;
		msg = LimitMove::getInstance()->motion3DMoveAbsAsync(cv::Point3f(imagingFixedPosX,
			imagingFixedPosY, imagingFixedPosZ), withCamera);

		if (!msg.empty())
			return msg;
		while (CheckModuleIsMoving(ModuleName::ImagingModuleXYZ))
		{
			QCoreApplication::processEvents();
			_sleep(100);
		}

		PrintLog(LogType::Normal, "imaging module loading end.");
		PrintModulePosition(ModuleName::ImagingModuleXYZ);

		return "";
	}

	std::string MotionProcess::ImagingRanging()
	{
		PrintLog(LogType::Normal, "[ImagingRanging]");

		std::string msg = CheckModuleConnectStatus(ModuleName::ImagingModuleXYZ, ModuleName::keyence, ModuleName::PLC);
		if (msg != "")
			return msg;

		cv::Point3f keyence;
		keyence.x = m_processConfigInfo.offsetRoatate.rangingPos[currentDutName].rangingPos.x;
		keyence.y = m_processConfigInfo.offsetRoatate.rangingPos[currentDutName].rangingPos.y;
		keyence.z = m_processConfigInfo.offsetRoatate.rangingPos[currentDutName].rangingPos.z;

		CORE::ML_Point3D currentImagingPos = Motion3DModel::getInstance(motion3DType::withCamera)->getPosition();

		//防止碰撞先移动xy
		msg = LimitMove::getInstance()->motion3DMoveAbsAsync(cv::Point3f(keyence.x, keyence.y, currentImagingPos.z/1000.0), withCamera);
		if (!msg.empty())
			return msg;
		while (CheckModuleIsMoving(ModuleName::ImagingModuleXYZ))
		{
			QCoreApplication::processEvents();
			_sleep(100);
		}

		msg = LimitMove::getInstance()->motion3DMoveAbsAsync(cv::Point3f(keyence.x, keyence.y, keyence.z), withCamera);
		if (!msg.empty())
			return msg;
		while (CheckModuleIsMoving(ModuleName::ImagingModuleXYZ))
		{
			QCoreApplication::processEvents();
			_sleep(100);
		}

		PrintLog(LogType::Normal, "image motor move to ranging position end.");
		PrintModulePosition(ModuleName::ImagingModuleXYZ);

		auto res = PLCController::instance()->keyenceLight(true);
		if (!res.success)
			return PrintLog(LogType::Error, res.errorMsg, !m_isTreeSystemRun);
		_sleep(1000);

		KeyenceInfo info = ConfigItem::instance()->getKeyenceInfo();
		currentRangingPos = info.keyenceZeroPos - MLKeyenceCL::MakeRangeFinder()->GetPosition();
		if (-9999 == currentRangingPos)
			return PrintLog(LogType::Error, "Ranging failed!", !m_isTreeSystemRun);
		PrintLog(LogType::Normal, "current ranging: " + to_string(currentRangingPos));

		double safeRangMax = m_processConfigInfo.offsetRoatate.anticollision[currentDutName].rangingMax;
		double safeRangMin = m_processConfigInfo.offsetRoatate.anticollision[currentDutName].rangingMin;
		if (safeRangMin > currentRangingPos || safeRangMax < currentRangingPos)
		{
			std::string msg = "current ranging: " + to_string(currentRangingPos) + ", exceeding the safe range[" + to_string(safeRangMin) +
				", " + to_string(safeRangMax) + "]";
			return PrintLog(LogType::Error, msg, !m_isTreeSystemRun);
		}

		PLCController::instance()->keyenceLight(false);

		//CORE::ML_Point3D currentPos = Motion3DModel::getInstance(motion3DType::withDUT)->getPosition(); //um
		double eyeBoxCenterKeyenceValue = m_processConfigInfo.offsetRoatate.eyeBoxCenterKeyenceValue[currentDutName];
		double distanceDifference = currentRangingPos - eyeBoxCenterKeyenceValue;
		msg = LimitMove::getInstance()->motion3DMoveRel(cv::Point3f(0, 0, -distanceDifference), withCamera);
		if (!msg.empty())
			return PrintLog(LogType::Error, msg, !m_isTreeSystemRun);
		while (CheckModuleIsMoving(ModuleName::ImagingModuleXYZ))
		{
			QCoreApplication::processEvents();
			_sleep(100);
		}
		PrintLog(LogType::Normal, "start adjust image motor distance.");
		PrintModulePosition(ModuleName::ImagingModuleXYZ);

		//由于keyence测距原因，z轴需要再移动2mm
		msg = LimitMove::getInstance()->motion3DMoveRel(cv::Point3f(0, 0, 2), withCamera);
		if (!msg.empty())
			return PrintLog(LogType::Error, msg, !m_isTreeSystemRun);
		while (CheckModuleIsMoving(ModuleName::ImagingModuleXYZ))
		{
			QCoreApplication::processEvents();
			_sleep(100);
		}

		currentRangingPos = currentRangingPos - distanceDifference + 2;
		PrintLog(LogType::Normal, "current ranging: " + to_string(currentRangingPos));

		return "";
	}

	std::string MotionProcess::FindImagingFiducial()
	{
		PrintLog(LogType::Normal, "[FindImagingFiducial]");

		PrintLog(LogType::Normal, "Starting the first find fiducial.");
		std::string msg = FindImagingFiducialToCalculate();
		if (msg != "")
			return PrintLog(LogType::Error, msg, !m_isTreeSystemRun);

		//PrintLog(LogType::Normal, "Starting the second find fiducial.");
		//msg = FindImagingFiducialToCalculate();
		//if (msg != "")
		//	return PrintLog(LogType::Error, msg, !m_isTreeSystemRun);

		return "";
	}
	std::string MotionProcess::FindImagingFiducialToCalculate()
	{
		PrintLog(LogType::Normal, "[FindFiducialToCalculate]");

		std::string msg = CheckModuleConnectStatus(ModuleName::ImagingModuleXYZ, ModuleName::MV);
		if (msg != "")
			return msg;

		//adjust imaging module height to fiducial height
		//PrintLog(LogType::Normal, "adjust imaging module height to fiducial height");
		//double fiducialZMotorPos = m_dutConfigInfoMap[currentDutName].fiducialZMotorPos;
		//CORE::ML_Point3D currentPos = Motion3DModel::getInstance(motion3DType::withCamera)->getPosition(); //um
		//msg = LimitMove::getInstance()->motion3DMoveAbsAsync(cv::Point3f(currentPos.x / 1000.0, currentPos.y / 1000.0, fiducialZMotorPos), withCamera);
		//if (!msg.empty())
		//	return PrintLog(LogType::Error, msg, !m_isTreeSystemRun);
		//while (CheckModuleIsMoving(ModuleName::ImagingModuleXYZ))
		//{
		//	QCoreApplication::processEvents();
		//	_sleep(100);
		//}
		PrintModulePosition(ModuleName::ImagingModuleXYZ);

		std::vector<cv::Point3f> fiducialMotorPosition;

		cv::Point3f originAbsCoordinates(m_dutConfigInfoMap[currentDutName].originPointMotorPositionInMV_.x,
			m_dutConfigInfoMap[currentDutName].originPointMotorPositionInMV_.y, 0);

		//fiducial1, 2 absolute coordinates
		CORE::ML_Point3D currentImagingPos = Motion3DModel::getInstance(motion3DType::withCamera)->getPosition(); //um
		cv::Point3f fid1(originAbsCoordinates.x - m_dutConfigInfoMap[currentDutName].fiducialOffset_[1].x,
			originAbsCoordinates.y - m_dutConfigInfoMap[currentDutName].fiducialOffset_[1].y, currentImagingPos.z / 1000.0);
		fiducialMotorPosition.push_back(fid1);

		cv::Point3f fid2(originAbsCoordinates.x - m_dutConfigInfoMap[currentDutName].fiducialOffset_[2].x,
			originAbsCoordinates.y - m_dutConfigInfoMap[currentDutName].fiducialOffset_[2].y, currentImagingPos.z / 1000.0);
		fiducialMotorPosition.push_back(fid2);

		std::vector <cv::Point2f> fiducialCenter;
		for (int i = 0; i < fiducialMotorPosition.size(); ++i)
		{
			PrintLog(LogType::Normal, "start to find fiducial " + to_string(i + 1));

			cv::Point2f center;
			if (m_bIsAutoFiducial)
			{
				msg = AutoIdentifyFiducial(center, fiducialMotorPosition[i]);
				if (!msg.empty())
				{
					QMessageBox::StandardButton result = QMessageBox::No;
					QMetaObject::invokeMethod(qApp, [&]() {
						result = QMessageBox::question(
							nullptr,
							"Fiducial Error",
							"Auto find fiducial error,\nPlease confirm whether to search manually.",
							QMessageBox::Yes | QMessageBox::No
						);
						}, Qt::BlockingQueuedConnection);

					if (result == QMessageBox::Yes) {
						msg = ManualIdentifyFiducial(center, fiducialMotorPosition[i], true);
						if (!msg.empty())
							return msg;
					}
					else {
						return msg;
					}
				}
			}
			else
			{
				msg = ManualIdentifyFiducial(center, fiducialMotorPosition[i]);
				if (!msg.empty())
					return msg;
			}
			fiducialCenter.push_back(center);
		}

		PrintLog(LogType::Normal, "Calculate the absolute coordinates of the fiducial motor at the center of the MV");
		// Calculate the absolute coordinates of fiducial moving to the center of MV
		std::vector<cv::Point2f> pixelCoor = { cv::Point2f(fiducialCenter[0]) , cv::Point2f(fiducialCenter[1]) };
		std::vector<cv::Point3f> motorCoor = { fid1 , fid2 };
		int width, height;
		ConfigItem::instance()->getCameraResolution(width, height);
		cv::Point2f mvCenter(width / 2.0, height / 2.0);

		setSavePosition("pixelCoor1", pixelCoor[0]);
		setSavePosition("pixelCoor2", pixelCoor[1]);
		setSavePosition("motorCoor1", fid1);
		setSavePosition("motorCoor2", fid2);
		setSavePosition("mvCenter", mvCenter);
		std::vector<cv::Point2f> mvCenterAbsCoor = CalculateFidAbsPosInMVCenter(pixelCoor, motorCoor, mvCenter);
		setSavePosition("mvCenterAbsCoor1", mvCenterAbsCoor[0]);
		setSavePosition("mvCenterAbsCoor2", mvCenterAbsCoor[1]);
		PrintLog(LogType::Normal, "Fiducial 1 in MV center motor absolute coordinates: " + to_string(mvCenterAbsCoor[0].x) + ", " + to_string(mvCenterAbsCoor[0].y));
		PrintLog(LogType::Normal, "Fiducial 2 in MV center motor absolute coordinates: " + to_string(mvCenterAbsCoor[1].x) + ", " + to_string(mvCenterAbsCoor[1].y));
		// calculate dut rotate and offset
		PrintLog(LogType::Normal, "Calculate dut rotate and offset");
		std::vector<cv::Point2f> fidOffsetFromOrigin = { cv::Point2f(fid1.x,fid1.y), cv::Point2f(fid2.x,fid2.y) };
		//std::vector<cv::Point2f> fidOffsetFromOrigin = { cv::Point2f(-27.1 ,-30.4), cv::Point2f(-48.15 ,-18.65) };

		dutOffsetRotate = CalculateOffsetAndRotate(mvCenterAbsCoor, fidOffsetFromOrigin);

		PrintLog(LogType::Normal,  "dut offset(" +
			to_string(dutOffsetRotate.offset.x) + ", " + to_string(dutOffsetRotate.offset.y) + ")");
		setSavePosition("fidOffsetFromOrigin", fidOffsetFromOrigin[0]);
		setSavePosition("fidOffsetFromOrigin", fidOffsetFromOrigin[1]);
		setSavePosition("dutOffset", dutOffsetRotate.offset);

		PrintLog(LogType::Normal, "dut offset: " + to_string(dutOffsetRotate.offset.x) + ", " + to_string(dutOffsetRotate.offset.y));

		return "";
	}

	std::string MotionProcess::EntranceAlignment()
	{
		PrintLog(LogType::Normal, "[EntranceAlignment]");

		std::string msg = CheckModuleConnectStatus(ModuleName::ImagingModuleXYZ);
		if (msg != "")
			return msg;

		cv::Point2f inputOffset(m_dutConfigInfoMap[currentDutName].inputgratingOffset_.x,
			m_dutConfigInfoMap[currentDutName].inputgratingOffset_.y);

		//cv::Point2f inputOffset(-32.58, -10.07);

		//Move input to MV center absolute coordinates
		cv::Point2f inputAbs = DutAnyPointToMVCenterCoordinate(inputOffset, dutOffsetRotate);
		setSavePosition("inputAbsCoorInMVCenter", inputAbs);

		PrintLog(LogType::Normal, "Start moving MV to the center of the DUT");

		CORE::ML_Point3D currentPos = Motion3DModel::getInstance(motion3DType::withCamera)->getPosition(); //um
		msg = LimitMove::getInstance()->motion3DMoveAbsAsync(cv::Point3f(currentPos.x / 1000.0 + inputAbs.x, currentPos.y / 1000.0 + inputAbs.y, currentPos.z / 1000.0), withCamera);
		if (!msg.empty())
			return PrintLog(LogType::Error, msg, !m_isTreeSystemRun);

		while (CheckModuleIsMoving(ModuleName::ImagingModuleXYZ))
		{
			QCoreApplication::processEvents();
			_sleep(100);
		}
		PrintModulePosition(ModuleName::ImagingModuleXYZ);

		PrintLog(LogType::Normal, "Start moving the entrance pupil from the center of the MV to align with the exit pupil of the projection module.");

		// adjust x, y
		double offsetx = m_processConfigInfo.offsetRoatate.projectionOffsetRelativeToMV[currentDutName].x;
		double offsety = m_processConfigInfo.offsetRoatate.projectionOffsetRelativeToMV[currentDutName].y;

		//safe verification
		double safePosXMax = m_processConfigInfo.offsetRoatate.anticollision[currentDutName].inputAlignmentMotorPosXMax;
		double safePosXMin = m_processConfigInfo.offsetRoatate.anticollision[currentDutName].inputAlignmentMotorPosXMin;
		double safePosYMax = m_processConfigInfo.offsetRoatate.anticollision[currentDutName].inputAlignmentMotorPosYMax;
		double safePosYMin = m_processConfigInfo.offsetRoatate.anticollision[currentDutName].inputAlignmentMotorPosYMin;
		CORE::ML_Point3D currentPos1 = Motion3DModel::getInstance(motion3DType::withCamera)->getPosition(); //um
		if (currentPos1.y / 1000.0 + offsety > safePosYMax || currentPos1.y / 1000.0 + offsety < safePosYMin)
		{
			msg = "Y motor is about to be moved to " + to_string(currentPos1.y / 1000.0 + offsety) +
				", which has exceeded the safe range[" + to_string(safePosYMin) + ", " + to_string(safePosYMax) + "]";
			return PrintLog(LogType::Error, msg, !m_isTreeSystemRun);
		}
		if (currentPos1.x / 1000.0 + offsetx > safePosXMax || currentPos1.x / 1000.0 + offsetx < safePosXMin)
		{
			msg = "X motor is about to be moved to " + to_string(currentPos1.x / 1000.0 + offsetx) +
				", which has exceeded the safe range[" + to_string(safePosXMin) + ", " + to_string(safePosXMax) + "]";
			return PrintLog(LogType::Error, msg, !m_isTreeSystemRun);
		}

		msg = LimitMove::getInstance()->motion3DMoveRel(cv::Point3f(0, offsety, 0), withCamera);
		if (!msg.empty())
			return PrintLog(LogType::Error, msg, !m_isTreeSystemRun);
		while (CheckModuleIsMoving(ModuleName::ImagingModuleXYZ))
		{
			QCoreApplication::processEvents();
			_sleep(100);
		}

		msg = LimitMove::getInstance()->motion3DMoveRel(cv::Point3f(offsetx, 0, 0), withCamera);
		if (!msg.empty())
			return PrintLog(LogType::Error, msg, !m_isTreeSystemRun);
		while (CheckModuleIsMoving(ModuleName::ImagingModuleXYZ))
		{
			QCoreApplication::processEvents();
			_sleep(100);
		}
		PrintModulePosition(ModuleName::ImagingModuleXYZ);

		CORE::ML_Point3D currentImagingPos = Motion3DModel::getInstance(motion3DType::withCamera)->getPosition();

		return "";
	}

	std::string MotionProcess::Imaging_Rotary_Compensation(double cant, double pant)
	{
		//旋转中心在tiptilt上为固定值,这个是基恩士的定标值
		double rotaryCenter_x = -122.554;
		double rotaryCenter_y = -38.39;

		CORE::ML_Point3D currentImagingPos = Motion3DModel::getInstance(motion3DType::withCamera)->getPosition();

		//换算成基恩士的中心坐标
		double kenyence_center_x = currentImagingPos.x / 1000.0 - 58.06;
		double kenyence_center_y = currentImagingPos.y / 1000.0 + 50.21;

		double rx = kenyence_center_x - rotaryCenter_x;
		double ry = rotaryCenter_y - kenyence_center_y;
		double rz = 0.0;



		// 原始点固定为 0,0,0
		double x0 = 0.0;
		double y0 = 0.0;
		double z0 = 0.0;

		double eyeCenter_new_x = 0.0;
		double eyeCenter_new_y = 0.0;
		double eyeCenter_new_z = 0.0;

		cant = cant * PI / 180.0;
		pant = pant * PI / 180.0;

		double cos_cant = cos(cant);  
		double sin_cant = sin(cant);
		double cos_pant = cos(pant);
		double sin_pant = sin(pant);

		double x1 = (x0 - rx) * cos_cant + (z0 - rz) * sin_cant + rx;
		double y1 = ((x0 - rx) * sin_cant - (z0 - rz) * cos_cant) * sin_pant
			+ (y0 - ry) * cos_pant + ry;
		double z1 = ((rx - x0) * sin_cant + (z0 - rz) * cos_cant) * cos_pant
			+ (y0 - ry) * sin_pant + rz;


		string msg;
		msg = LimitMove::getInstance()->motion3DMoveRel(cv::Point3f(-x1, 0, 0), withCamera);
		if (!msg.empty())
			return PrintLog(LogType::Error, msg, !m_isTreeSystemRun);
		while (CheckModuleIsMoving(ModuleName::ImagingModuleXYZ))
		{
			QCoreApplication::processEvents();
			_sleep(100);
		}

		msg = LimitMove::getInstance()->motion3DMoveRel(cv::Point3f(0, y1, 0), withCamera);
		if (!msg.empty())
			return PrintLog(LogType::Error, msg, !m_isTreeSystemRun);
		while (CheckModuleIsMoving(ModuleName::ImagingModuleXYZ))
		{
			QCoreApplication::processEvents();
			_sleep(100);
		}

		msg = LimitMove::getInstance()->motion3DMoveRel(cv::Point3f(0, 0, z1), withCamera);
		if (!msg.empty())
			return PrintLog(LogType::Error, msg, !m_isTreeSystemRun);
		while (CheckModuleIsMoving(ModuleName::ImagingModuleXYZ))
		{
			QCoreApplication::processEvents();
			_sleep(100);
		}

		return "";
	}

	cv::Point3f MotionProcess::CalculateRotaryCenter(cv::Point3f fid_old, cv::Point3f fid_new, double cant, double pant)
	{
		cv::Point3f eyeBoxCenter;
		eyeBoxCenter.x = (fid_old.x - fid_new.x * cos(cant) - fid_new.z * sin(cant)) / (cos(cant) - 1);
		eyeBoxCenter.y = (fid_old.y - fid_new.y * cos(pant) - (fid_old.x - eyeBoxCenter.x) * sin(pant)) / cos(pant);
		eyeBoxCenter.z = (fid_old.z - fid_new.z) / cos(pant);

		return eyeBoxCenter;
	}

	std::string MotionProcess::Motion2DSet(int X_degree, int Y_degree)
	{
		Result result;
		string msg;

		//新增限位
		if (X_degree > 150 && X_degree < 210)
		{
			if (Y_degree > 135)
			{
				msg = "plc 2D Y_degree out of limit at X_degree " + result.errorMsg;
				return msg;
			}
		}
		if (X_degree > 330 || X_degree < 30)
		{
			if (Y_degree < 60)
			{
				msg = "plc 2D Y_degree out of limit at X_degree " + result.errorMsg;
				return msg;
			}
		}

		result = Motion2DModel::getInstance(PLC2D)->Motion2DMoveAbs(X_degree, Y_degree);

		while (CheckModuleIsMoving(ModuleName::ImagingModuleDxDy))
		{
			QCoreApplication::processEvents();
			_sleep(100);
		}
		
		if (!result.success)
		{
			msg = "Measure plc 2D  motion move failed, " + result.errorMsg;
			return msg;
		}
		return msg;
	}

	std::string MotionProcess::WaveTypeSet(int wave_type)
	{
		bool result;
		std::string msg;

		if (wave_type < 1 || wave_type >3)
		{
			msg = "put in wavetype error.";
			return msg;
		}

		if (!SpecSet::getInstance()->is_open)
		{
			result = SpecSet::getInstance()->is_connect();
			if (!result)
			{
				msg = "wavecontrol connect failed.";
				return msg;
			}
		}
		
		result = SpecSet::getInstance()->set_grating(wave_type);
		if (!result)
		{
			msg = "get cur wavelength failed.";
			return msg;
		}

		return "";
	}

	std::string MotionProcess::WaveLengthSet(float wave_length)
	{
		bool result;
		std::string msg;

		result = SpecSet::getInstance()->set_wave(wave_length);
		if (!result)
		{
			msg = "move wavelength failed.";
			return msg;
		}

		return "";
	}

	Result MotionProcess::waitPause(bool isPause)
	{
		return m_waitPause.waitPause(isPause);
	}

	std::string MotionProcess::LoadSLB()
	{
		PrintLog(LogType::Normal, "[LoadSLB]");

		std::string holder_type;
		std::string message = "";
		if (message != "")
			return message;
		if (holder_type != "000")
			return "Please remove the dut first !";

		message = CheckModuleConnectStatus(ModuleName::ImagingModuleXYZ, ModuleName::DutModuleXYZ);
		if (message != "")
			return message;

		ML_Point3D currentPos = Motion3DModel::getInstance(withDUT)->getPosition();

		if (currentPos.z / 1000 > m_slbConfigInfo.slb_DutXYZPosition.z)
		{
			return PrintLog(LogType::Error, "Load dut module xyz motor Z value calibration error", !m_isTreeSystemRun);
		}
		message = LimitMove::getInstance()->motion3DMoveAbsAsync(cv::Point3f(currentPos.x / 1000.0,
			currentPos.y / 1000.0, m_slbConfigInfo.slb_DutXYZPosition.z), withDUT);
		if (!message.empty())
			return message;

		while (CheckModuleIsMoving(ModuleName::DutModuleXYZ))
		{
			if (m_isStopTreeSystem)
			{
				StopModuleMove(ModuleName::DutModuleXYZ);
				m_isStopTreeSystem = false;
				return "Operation is force stopped by user.";
			}

			QCoreApplication::processEvents();
			_sleep(100);
		}

		message = LimitMove::getInstance()->motion3DMoveAbsAsync(cv::Point3f(m_slbConfigInfo.slb_DutXYZPosition.x,
			m_slbConfigInfo.slb_DutXYZPosition.y, m_slbConfigInfo.slb_DutXYZPosition.z), withDUT);
		if (!message.empty())
			return message;

		while (CheckModuleIsMoving(ModuleName::DutModuleXYZ))
		{
			if (m_isStopTreeSystem)
			{
				StopModuleMove(ModuleName::DutModuleXYZ);
				m_isStopTreeSystem = false;
				return "Operation is force stopped by user.";
			}

			QCoreApplication::processEvents();
			_sleep(100);
		}
		PrintModulePosition(ModuleName::DutModuleXYZ);

		if (!LimitMove::getInstance()->perJudgeSLBLimit3D(cv::Point3f(m_slbConfigInfo.slb_LoadImageXYZPosition.x, m_slbConfigInfo.slb_LoadImageXYZPosition.y, m_slbConfigInfo.slb_LoadImageXYZPosition.z)))
			return "SLB imageXYZ module x:" + std::to_string(m_slbConfigInfo.slb_LoadImageXYZPosition.x) + ",y:" + std::to_string(m_slbConfigInfo.slb_LoadImageXYZPosition.y)
			+ ",z:" + std::to_string(m_slbConfigInfo.slb_LoadImageXYZPosition.z) + " is over limits";

		message = LimitMove::getInstance()->motion3DMoveAbsAsync(cv::Point3f(m_slbConfigInfo.slb_LoadImageXYZPosition.x,
			m_slbConfigInfo.slb_LoadImageXYZPosition.y, m_slbConfigInfo.slb_LoadImageXYZPosition.z), withCamera);
		if (!message.empty())
			return message;

		while (CheckModuleIsMoving(ModuleName::ImagingModuleXYZ))
		{
			if (m_isStopTreeSystem)
			{
				StopModuleMove(ModuleName::ImagingModuleXYZ);
				m_isStopTreeSystem = false;
				return "Operation is force stopped by user.";
			}

			QCoreApplication::processEvents();
			_sleep(100);
		}
		PrintModulePosition(ModuleName::ImagingModuleXYZ);

		return "";
	}

	std::string MotionProcess::SLBAlignment()
	{
		PrintLog(LogType::Normal, "[SLBAlignment]");

		std::string message = LoadSLB();
		if (message != "")
			return message;

		message = CheckModuleConnectStatus(ModuleName::ProjectionDxDy, ModuleName::ImagingModuleDxDy);
		if (message != "")
			return message;

		//adjust image & projection module tiptilt
		double projectionTiptiltX = m_slbConfigInfo.slb_projectionTiptilt.dx;
		double projectionTiptiltY = m_slbConfigInfo.slb_projectionTiptilt.dy;
		double imagingTiptiltX = m_slbConfigInfo.slb_imagingTiptilt.dx;
		double imagingTiptiltY = m_slbConfigInfo.slb_imagingTiptilt.dy;
		Result ret = Motion2DModel::getInstance(ACS2DPro)->Motion2DMoveAbsAsync(projectionTiptiltX, projectionTiptiltY);
		if (!ret.success)
			return ret.errorMsg;

		//while (CheckModuleIsMoving(ModuleName::ProjectionDxDy))
		//{
		//	QCoreApplication::processEvents();
		//	_sleep(100);
		//}

		ret = Motion2DModel::getInstance(ACS2DCameraTilt)->Motion2DMoveAbsAsync(imagingTiptiltX, imagingTiptiltY);
		if (!ret.success)
			return ret.errorMsg;
		while (CheckModuleIsMoving(ModuleName::ProjectionDxDy, ModuleName::ImagingModuleDxDy))
		{
			if (m_isStopTreeSystem)
			{
				StopModuleMove(ModuleName::ProjectionDxDy, ModuleName::ImagingModuleDxDy);
				m_isStopTreeSystem = false;
				return "Operation is force stopped by user.";
			}

			QCoreApplication::processEvents();
			_sleep(100);
		}
		PrintModulePosition(ModuleName::ProjectionDxDy, ModuleName::ImagingModuleDxDy);

		cv::Point3f imageXYZPosition = cv::Point3f(m_slbConfigInfo.slb_ImagingXYZPosition.x,
			m_slbConfigInfo.slb_ImagingXYZPosition.y, m_slbConfigInfo.slb_ImagingXYZPosition.z);

		if (!LimitMove::getInstance()->perJudgeSLBLimit3D(imageXYZPosition))
			return "SLB imageXYZ module x:" + std::to_string(imageXYZPosition.x) + ",y:" + std::to_string(imageXYZPosition.y)
			+ ",z:" + std::to_string(imageXYZPosition.z) + " is over limits";

		message = LimitMove::getInstance()->motion3DMoveAbsAsync(imageXYZPosition, withCamera);
		if (!message.empty())
			return message;

		while (CheckModuleIsMoving(ModuleName::ImagingModuleXYZ))
		{
			if (m_isStopTreeSystem)
			{
				StopModuleMove(ModuleName::ImagingModuleXYZ);
				m_isStopTreeSystem = false;
				return "Operation is force stopped by user.";
			}

			QCoreApplication::processEvents();
			_sleep(100);
		}
		PrintModulePosition(ModuleName::ImagingModuleXYZ);

		return "";
	}

	cv::Point2f MotionProcess::manualFiducialPos(cv::Mat& mat)
	{
		cv::Point2f pos(0, 0);
		QImage qimgfid = matToQImageCopy(mat);
		if (qimgfid.isNull())
		{
			PrintLog(LogType::Error, "Fiducial1 cv image transfer qimage failed!", !m_isTreeSystemRun);
			return pos;
		}

		QPointF pointf = emit onSignalgetFiducialPos(qimgfid);
		PrintLog(LogType::Normal, QString("get manual fiducial_1 position:X:%1,Y:%2").arg(pointf.x()).arg(pointf.y()).toStdString());
		if (pointf.x() <= 0 || pointf.y() <= 0)
		{
			PrintLog(LogType::Error, "Fiducial recognition failed!", !m_isTreeSystemRun);
			return pos;
		}
		pos = cv::Point2f(pointf.x(), pointf.y());
		return pos;
	}

	QImage MotionProcess::matToQImageCopy(const cv::Mat& mat)
	{
		switch (mat.type()) {
		case CV_8UC3: {
			QImage image(mat.data, mat.cols, mat.rows,
				static_cast<int>(mat.step),
				QImage::Format_RGB888);
			return image.rgbSwapped().copy(); // BGR→RGB并复制数据 
		}
		case CV_8UC4: {
			QImage image(mat.data, mat.cols, mat.rows,
				static_cast<int>(mat.step),
				QImage::Format_ARGB32);
			return image.copy();
		}
		case CV_8UC1: {
			QImage image(mat.data, mat.cols, mat.rows,
				static_cast<int>(mat.step),
				QImage::Format_Grayscale8);
			return image.copy();
		}
		default:
			return QImage();
		}
	}

	void MotionProcess::SetExposureTime(bool is_auto_exposure, double exposure_time)
	{
		if (is_auto_exposure)
		{
			m_is_auto_exposure = true;
		}
		else
		{
			m_is_auto_exposure = false;
			CameraModel::GetInstance()->SetExposureTime(exposure_time * 1000);
		}
	}

	template<typename T>
	void MotionProcess::setSavePosition(QString strTitle, T pos)
	{
		QString strSavePath = QString(m_strFiducialRootDir + "/" + m_dutSeq + "/fiducial/");
		QDir dir(strSavePath);
		if (!dir.exists())
		{
			dir.mkpath(strSavePath);
		}

		QFile file(QString("%1/%2.txt").arg(strSavePath).arg("CoordinateLog"));
		if (file.open(QIODevice::WriteOnly | QIODevice::Append))
		{
			QString strpos = "";
			if constexpr (std::is_same_v<T, pos2D>)
			{
				strpos = QString("%1|%2").arg(pos.x).arg(pos.y);
			}
			else if constexpr (std::is_same_v<T, pos3D>)
			{
				strpos = QString("%1|%2|%3").arg(pos.x).arg(pos.y).arg(pos.z);
			}
			else if constexpr (std::is_same_v<T, cv::Point2f>)
			{
				strpos = QString("%1|%2").arg(pos.x).arg(pos.y);
			}
			else if constexpr (std::is_same_v<T, cv::Point3f>)
			{
				strpos = QString("%1|%2|%3").arg(pos.x).arg(pos.y).arg(pos.z);
			}
			else if constexpr (std::is_same_v<T, CORE::ML_Point3D>)
			{
				strpos = QString("%1|%2|%3").arg(pos.x).arg(pos.y).arg(pos.z);
			}
			else if constexpr (std::is_same_v<T, double>)
			{
				strpos = QString::number(pos, 'f', 3);
			}
			else if constexpr (std::is_same_v<T, float>)
			{
				strpos = QString::number(pos, 'f', 3);
			}

			file.write(QString("%1 >> %2\n").arg(strTitle).arg(strpos).toLocal8Bit().data());
			file.close();
		}
		else
		{
			PrintLog(LogType::Error, "Failed to open file for saving position: " + file.fileName().toStdString(), !m_isTreeSystemRun);
		}
	}

	void MotionProcess::StopTreeSystemCallback(bool isStopTreeSystem)
	{
		m_isStopTreeSystem = isStopTreeSystem;
	}

	//std::string MotionProcess::JudgeHolderState(std::string& holder_type)
	//{
	//	std::string msg = CheckModuleConnectStatus(ModuleName::PLC);
	//	if (msg != "")
	//		return msg;

	//	std::ostringstream oss;
	//	oss << PLCController::instance()->GetSensorAState()
	//		<< PLCController::instance()->GetSensorBState()
	//		<< PLCController::instance()->GetSensorCState();

	//	holder_type = oss.str();

	//	return "";
	//}
}