#include "Fosphor.h"
#include "../libFosphor/WidgetFosphor.h"

//#include "qt_sink_c_impl.h"

Fosphor::Fosphor(QWidget *parent)
	: QMainWindow(parent)
	, timer_(this)
{
	ui.setupUi(this);

	srf_ = new WidgetFosphor(this);
	setCentralWidget(srf_);

	connect(&timer_, SIGNAL(timeout()), this, SLOT(feed_data()));
	timer_.start(50);

	// 	qt_sink_c_impl* srf = new qt_sink_c_impl(this);
	//
	//
	// 	setCentralWidget(srf->qwidget());
	//
	// 	srf->set_fft_window(fft::window::WIN_KAISER);
	// 	srf->start();
	//
	// 	std::vector<gr_complex> input_items(100000);
	// 	for (auto &it : input_items)
	// 	{
	// 		it = rand();
	// 	}
	//
	//
	// 	srf->work(input_items);
}

void Fosphor::feed_data()
{
	std::vector<gr_complex> ssss_vec(10'000);

	for (auto &it : ssss_vec)
	{
		it.imag((rand() % 10000) / 10000.0);
		it.real((rand() % 10000) / 100000.0);
	}

	srf_->work(ssss_vec);
}