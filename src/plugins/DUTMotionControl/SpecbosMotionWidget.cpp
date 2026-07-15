#include "SpecbosMotionWidget.h"
#include "BD3EMotorControl.h"
#include "loggingwrapper.h"
#include <QtConcurrent>
#include <QMessageBox>

SpecbosMotionWidget::SpecbosMotionWidget(QString toolBoxName, QWidget* parent) : IToolBox(toolBoxName, parent)
{
	ui.setupUi(this);
	init();
}

SpecbosMotionWidget::~SpecbosMotionWidget()
{

}

void SpecbosMotionWidget::init()
{
	ui.label_status->setText("Not connected.");
	connect(&watcher, &QFutureWatcher<Result>::finished, this, &SpecbosMotionWidget::handleFinished);
	connect(ui.rotationline, &QLineEdit::returnPressed, this, &SpecbosMotionWidget::setRotationDegree);
}

void SpecbosMotionWidget::on_btn_connect_clicked()
{
	setEnabled(false);
	QFuture<Result> future = QtConcurrent::run([=]() { return BD3EMotorControl::getInstance()->Connect(); });
	watcher.setFuture(future);
}

void SpecbosMotionWidget::on_btn_disconnect_clicked()
{
	if (!BD3EMotorControl::getInstance()->IsConnected())
	{
		return;
	}
	Result ret = BD3EMotorControl::getInstance()->Disconnect();
	if (ret.success)
	{
		ui.label_status->setText("Disconnect Success.");
	}
}

void SpecbosMotionWidget::on_refresh_clicked()
{
	if (!BD3EMotorControl::getInstance()->IsConnected())
	{
		LoggingWrapper::instance()->warn("module is not connected");
		return;
	}
	double degree = BD3EMotorControl::getInstance()->GetCurrDegree(0);
	ui.degState_2->setText(QString::number(degree, 'f', 3));
}

void SpecbosMotionWidget::on_stop_clicked()
{
	if (!BD3EMotorControl::getInstance()->IsConnected())
	{
		LoggingWrapper::instance()->warn("module is not connected");
		return;
	}
	Result ret = BD3EMotorControl::getInstance()->StopMove(0);
	on_refresh_clicked();
	if (!ret.success)
	{
		QMessageBox::warning(this, tr("Stop Moving Failed"), QString::fromStdString(ret.errorMsg));
	}
}

void SpecbosMotionWidget::setRotationDegree()
{
	if (!BD3EMotorControl::getInstance()->IsConnected())
	{
		LoggingWrapper::instance()->warn("module is not connected");
		return;
	}
	float degree = ui.rotationline->text().toFloat();
	//TODO:limit
	QFuture<Result> future = QtConcurrent::run([=]() {
		return BD3EMotorControl::getInstance()->MoveByDegreeSync(0, degree);
		});
	watcher.setFuture(future);
}

void SpecbosMotionWidget::handleFinished()
{
	setEnabled(true);
	if (watcher.result().success)
	{
		ui.label_status->setText("Connected.");
		on_refresh_clicked();
	}
	else
	{
		ui.label_status->setText("Connected Failed.");
	}
}

void SpecbosMotionWidget::setEnabled(bool enabled)
{
	ui.btn_connect->setEnabled(enabled);
	ui.btn_disconnect->setEnabled(enabled);
}

