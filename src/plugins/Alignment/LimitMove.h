#pragma once

#include <QObject>
//#include "MotionProcess.h"
#include "piMotionActor/configItem.h"

namespace AAProcess
{
	class LimitMove : public QObject
	{
		Q_OBJECT
	public:
		static LimitMove* getInstance();
		~LimitMove();

		std::string motion3DMoveAbsSync(cv::Point3f targetPos, motion3DType type);
		std::string motion3DMoveAbsAsync(cv::Point3f targetPos, motion3DType type);
		std::string motion3DMoveRel(cv::Point3f offsetPos, motion3DType type);
		std::string orientalMoveAbs(cv::Point3f targetPos);
		std::string orientalMoveAbs(int type, double targetPos);
		std::string orientalMoveRel(cv::Point3f offsetPos);
		std::string orientalMoveRel(int type, double offsetPos);
		std::string moveAbsToSafePos(cv::Point3f safePos, motion3DType type);

		bool perJudgeSLBLimit3D(cv::Point3f targetPos);

	private:
		explicit LimitMove(QObject* parent = nullptr);

		/*超出返回true*/
		bool perJudgeLimit3D(cv::Point3f targetPos, motion3DType type);
		bool perJudgeLimitOriental(cv::Point3f targetPos);

	private:
		static LimitMove* self;
	};

	class SpecSet : public QObject
	{
		Q_OBJECT
	public:
		static SpecSet* getInstance();
		~SpecSet();

		bool is_connect();
		bool dis_connect();
		bool set_grating(int type_value);
		bool set_wave(float wave_value);

	private:
		explicit SpecSet(QObject* parent = nullptr);

	public:
		QString g_spec_sn;
		int g_spec_id;
		bool is_open;
	private:
		static SpecSet* self;		
	};
}
