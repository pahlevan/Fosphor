#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_Fosphor.h"
#include <QTimer>

class WidgetFosphor;

class Fosphor : public QMainWindow
{
	Q_OBJECT

public:
	Fosphor(QWidget *parent = nullptr);

private:
	Ui::FosphorClass ui;

	QTimer timer_;
	WidgetFosphor*  srf_;
	private slots:
		void feed_data();
};
