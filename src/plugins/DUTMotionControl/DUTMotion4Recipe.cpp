#include "DUTMotion4Recipe.h"
#include "OrientalMotorControl.h"
#include "MLSpecbosLogic.h"
#include "loggingwrapper.h"
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QStringList>

using namespace DUTMotion;
using namespace ML::MLSpecbos;

DUTMotion4Recipe* DUTMotion4Recipe::self = nullptr;
DUTMotion4Recipe* DUTMotion4Recipe::getInstance()
{
	if (!self)
	{
		self = new DUTMotion4Recipe();
	}
	return self;
}

DUTMotion4Recipe::DUTMotion4Recipe(QObject* parent)
	: QObject(parent)
{
	ObjectManager::getInstance()->registerObject("DUTMotion4Recipe", static_cast<void*>(this));
}

DUTMotion4Recipe::~DUTMotion4Recipe()
{
}

QString DUTMotion4Recipe::getNodeValueByName(BT::TreeNode& node, std::string name)
{
	auto f_value = node.getInput<std::string>(name);
	if (!f_value)
	{
		throw BT::RuntimeError("Missing input [", name, "]: ", f_value.error());
	}
	return QString::fromStdString(f_value.value());
}

NodeStatus DUTMotion::DUTMotion4Recipe::OM_Open(BT::TreeNode& node)
{
	QString module = getNodeValueByName(node, "module");

	int nType = OrientalAxle::DX;
	if (module == "X")
		nType = OrientalAxle::DX;
	else if (module == "Y")
		nType = OrientalAxle::DY;
	else if (module == "Z")
		nType = OrientalAxle::DZ;

	OrientalMotorControl* montor = OrientalMotorControl::getInstance();
	Result ret;
	if (nullptr != montor)
	{
		ret = montor->Connect();
	}
	else
	{
		ret = Result(false, "can not get montor");
	}
	if (!ret.success)
	{
		QString strerr = QString("Recipe [OrientalMontor: OM_Open] run error,%1").arg(QString::fromStdString(ret.errorMsg));
		LoggingWrapper::instance()->error(strerr);
		return BT::NodeStatus::FAILURE;
	}
	return BT::NodeStatus::SUCCESS;
}

NodeStatus DUTMotion::DUTMotion4Recipe::OM_Close(BT::TreeNode& node)
{
	QString module = getNodeValueByName(node, "module");

	int nType = OrientalAxle::DX;
	if (module == "X")
		nType = OrientalAxle::DX;
	else if (module == "Y")
		nType = OrientalAxle::DY;
	else if (module == "Z")
		nType = OrientalAxle::DZ;

	OrientalMotorControl* montor = OrientalMotorControl::getInstance();
	Result ret;
	if (nullptr != montor)
	{
		ret = montor->Disconnect();
	}
	else
	{
		ret = Result(false, "can not get montor");
	}
	if (!ret.success)
	{
		QString strerr = QString("Recipe [OrientalMontor: OM_Open] run error,%1").arg(QString::fromStdString(ret.errorMsg));
		LoggingWrapper::instance()->error(strerr);
		return BT::NodeStatus::FAILURE;
	}
	return BT::NodeStatus::SUCCESS;
}

NodeStatus DUTMotion4Recipe::OM_Degree_Async(BT::TreeNode& node)
{
	QString module = getNodeValueByName(node, "module");
	QString degree = getNodeValueByName(node, "degree");

	int nType = OrientalAxle::DX;
	if (module == "X")
		nType = OrientalAxle::DX;
	else if (module == "Y")
		nType = OrientalAxle::DY;
	else if (module == "Z")
		nType = OrientalAxle::DZ;

	OrientalMotorControl* motorControl = OrientalMotorControl::getInstance();
	Result res = motorControl->MoveByDegreeAsync(nType, degree.toDouble());
	if (!res.success)
	{
		QString message = QString("Recipe [ OrientalMotor : OM_Degree_Async ] run error, %1").arg(QString::fromStdString(res.errorMsg));
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}
	return BT::NodeStatus::SUCCESS;
}

NodeStatus DUTMotion4Recipe::OM_Degree_Sync(BT::TreeNode& node)
{
	QString module = getNodeValueByName(node, "module");
	QString degree = getNodeValueByName(node, "degree");

	int nType = OrientalAxle::DX;
	if (module == "X")
		nType = OrientalAxle::DX;
	else if (module == "Y")
		nType = OrientalAxle::DY;
	else if (module == "Z")
		nType = OrientalAxle::DZ;

	OrientalMotorControl* motorControl = OrientalMotorControl::getInstance();
	Result res = motorControl->MoveByDegreeSync(nType, degree.toDouble());
	if (!res.success)
	{
		QString message = QString("Recipe [ OrientalMotor : OM_Degree_Sync ] run error, %1").arg(QString::fromStdString(res.errorMsg));
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}
	return BT::NodeStatus::SUCCESS;
}

NodeStatus DUTMotion4Recipe::OM_Degree_Abs_Async(BT::TreeNode& node)
{
	QString module = getNodeValueByName(node, "module");
	QString target_Degree = getNodeValueByName(node, "degree");

	int nType = OrientalAxle::DX;
	if (module == "X")
		nType = OrientalAxle::DX;
	else if (module == "Y")
		nType = OrientalAxle::DY;
	else if (module == "Z")
		nType = OrientalAxle::DZ;

	OrientalMotorControl* motorControl = OrientalMotorControl::getInstance();

	double cur_Degree = OrientalMotorControl::getInstance()->GetPosition(nType);
	double degree = target_Degree.toDouble() - cur_Degree;

	Result res = motorControl->MoveByDegreeAsync(nType, degree);
	if (!res.success)
	{
		QString message = QString("Recipe [ OrientalMotor : OM_Degree_Async ] run error, %1").arg(QString::fromStdString(res.errorMsg));
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}
	return BT::NodeStatus::SUCCESS;
}

NodeStatus DUTMotion4Recipe::Specbos_Connect(BT::TreeNode& node)
{
	QString com = getNodeValueByName(node, "com");
	MLSpecbosLogic::getInstance()->Init("E:\\project\\MLSpecbos\\jeticonfig.ini");
	if (!MLSpecbosLogic::getInstance()->Open("COM" + com.toStdString()))
	{
		QString message = QString("Recipe [ Specbos : Specbos Connect ] run error, open specbos failed");
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}

	return BT::NodeStatus::SUCCESS;
}

NodeStatus DUTMotion4Recipe::Specbos_Disconnect(BT::TreeNode& node)
{
	if (!MLSpecbosLogic::getInstance()->Close())
	{
		QString message = QString("Recipe [ Specbos : Specbos Disconnect ] run error, close specbos failed");
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}
	return BT::NodeStatus::SUCCESS;
}

NodeStatus DUTMotion4Recipe::Specbos_LoadCalibration(BT::TreeNode& node)
{
	QString deviceType = getNodeValueByName(node, "deviceType");
	QString fileNum = getNodeValueByName(node, "fileNum");
	QString average = getNodeValueByName(node, "average");
	QString freq = getNodeValueByName(node, "freq");
	QString integTime = getNodeValueByName(node, "integTime");

	if (!MLSpecbosLogic::getInstance()->SetDeviceType(DeviceType(deviceType.toInt())))
	{
		QString message = QString("Recipe [ Specbos : Specbos_LoadCalibration ] run error, set deviceType failed");
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}

	MLSpecbosLogic::getInstance()->LoadCalibrationFile();

	if (!MLSpecbosLogic::getInstance()->SetCalibrationFileNumber(fileNum.toInt()))
	{
		QString message = QString("Recipe [ Specbos : Specbos_LoadCalibration ] run error, set calibration file number failed");
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}

	if (!MLSpecbosLogic::getInstance()->SetAverage(average.toInt()))
	{
		QString message = QString("Recipe [ Specbos : Specbos_LoadCalibration ] run error, set average failed");
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}

	if (!MLSpecbosLogic::getInstance()->SetSyncFreq(freq.toFloat()))
	{
		QString message = QString("Recipe [ Specbos : Specbos_LoadCalibration ] run error, set freq failed");
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}

	if (!MLSpecbosLogic::getInstance()->SetFixedTintConf(integTime.toFloat()))
	{
		QString message = QString("Recipe [ Specbos : Specbos_LoadCalibration ] run error, set fixedTint failed");
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}

	return BT::NodeStatus::SUCCESS;
}

NodeStatus DUTMotion4Recipe::Specbos_Measurement(BT::TreeNode& node)
{
	QString postion = getNodeValueByName(node, "postion");
	QString path = getNodeValueByName(node, "path"); 

	QString file_path = path + "\\" + "Wavelength" + ".csv";
	QString file_path2 = path + "\\" + "Chrom" + ".csv";
	MLSpecbosLogic::getInstance()->TakeMeasurement();
	MeasurementResults* res = MLSpecbosLogic::getInstance()->GetMeasurementResults();
	MeasurementConfig config = MLSpecbosLogic::getInstance()->GetMeasurementConfig();

	QFile file(file_path);
	QStringList lines;
	QFile file2(file_path2);
	QStringList lines2;

	// 文件存在，读取原数据
	if (file.exists())
	{
		if (file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			QTextStream in(&file);

			while (!in.atEnd())
			{
				lines.append(in.readLine());
			}

			file.close();
		}
	}
	if (file2.exists())
	{
		if (file2.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			QTextStream in(&file2);

			while (!in.atEnd())
			{
				lines2.append(in.readLine());
			}

			file2.close();
		}
	}

	// 第一次创建CSV
	if (lines.isEmpty())
	{
		QString header = "Wavelength/position";
		header += "," + postion;
		lines.append(header);
		for (int i = config.dwBegin - 1; i < config.dwEnd; i++)
		{
			int wavelength = i + 1;
			float fsprad = res->fSprad[wavelength - 380];
			lines.append(
				QString::number(wavelength) + "," +
				QString::number(fsprad, 'E', 6)
			);
		}
	}
	else
	{
		// 当前已有多少列
		QStringList header = lines[0].split(",");

		int newColumn = header.size();

		// 增加表头
		lines[0] += "," + postion;

		// 增加数据列
		int row = 1;
		for (int i = config.dwBegin - 1; i < config.dwEnd; i++)
		{
			int wavelength = i + 1;
			float fsprad = res->fSprad[wavelength - 380];
			if (row < lines.size())
			{
				lines[row] += "," + QString::number(fsprad, 'E', 6);
			}
			row++;
		}
	}

	if (lines2.isEmpty())
	{
		QString header = "Chrom/position";
		header += "," + postion;
		lines2.append(header);
		lines2.append("chromx," +QString::number(res->fChromx, 'f', 3));
		lines2.append("chromy," + QString::number(res->fChromy, 'f', 3));
		lines2.append("Y," + QString::number(res->fY, 'f', 3));
	}
	else
	{
		// 当前已有多少列
		QStringList header = lines2[0].split(",");

		int newColumn = header.size();

		// 增加表头
		lines2[0] += "," + postion;

		// 增加数据列
		lines2[1] += "," + QString::number(res->fChromx, 'f', 3);
		lines2[2] += "," + QString::number(res->fChromy, 'f', 3);
		lines2[3] += "," + QString::number(res->fY, 'f', 3);
	}


	// 重新写文件
	if (file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		QTextStream out(&file);

		for (const QString& line : lines)
		{
			out << line << "\n";
		}

		file.close();
	}
	else
	{
		QString message = QString("Recipe [ Specbos : Specbos_Measurement ] run error, write file failed");
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}

	if (file2.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		QTextStream out(&file2);

		for (const QString& line : lines2)
		{
			out << line << "\n";
		}

		file2.close();

	}
	else
	{
		QString message = QString("Recipe [ Specbos : Specbos_Measurement ] run error, write file failed");
		LoggingWrapper::instance()->error(message);
		return BT::NodeStatus::FAILURE;
	}

	return BT::NodeStatus::SUCCESS;

}