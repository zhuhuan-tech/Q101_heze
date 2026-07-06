#pragma once

#include <QObject>
#include <QThread>

class plcReadPosClass : public QObject
{
	Q_OBJECT

public:
	plcReadPosClass(QObject* parent = nullptr);
	~plcReadPosClass();

	bool open();

	bool close();

	void GetDouble(int dbNum, int start, double& value);

private:
	class PLCControllerImpl;
	std::unique_ptr<PLCControllerImpl> m_pImpl;

};
