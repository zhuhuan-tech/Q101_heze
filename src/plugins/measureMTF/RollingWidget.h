#pragma once
#include "MLColorimeterMode.h"

#include <QWidget>
#include "ui_RollingWidget.h"
#include "accordion/qaccordion.hpp"
#include "Result.h"
#include <QFutureWatcher>
#include <opencv2\opencv.hpp>

class RollingWidget : public QWidget, public MLCameraCallback
{
	Q_OBJECT

public:
	RollingWidget(QWidget *parent = nullptr);
	~RollingWidget();
	void setQss(QString filename);
	void initLiveImageRadio();
public:
	virtual void NotifyCameraStateChanged(MLCameraState old_state, MLCameraState new_state) override;
	virtual void NotifyCameraFrameReceived(cv::Mat img, MLPixelFormat format) override;
	virtual void NotifyCameraGrayLevel(int gray_level) override;
private:
	void initUI();
signals:
	void enableWidget(QWidget* wgt, bool enable);
	void loadImageSignal(const QString& fileName);
	void messageShow(QString msg);
	//void setImageRotateModeSignal(bool isSLB);
public slots:
	void on_connectCamera_clicked();
	Result connectCamera();
	void on_disconnectCamera_clicked();
	void connectCameraSlot();
private slots:

	void on_btn_unfold_clicked();
	void on_btn_fold_clicked();
private:
	Ui::RollingWidgetClass ui;
	QFutureWatcher<Result> watcher;
	QAccordion* m_accordion;
};

