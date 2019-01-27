#pragma once

#include <QGLFunctions>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>

#include "FFTWindow.h"
#include "gr_complex.h"

#include <mutex>
#include <vector>


struct fosphor;
struct fosphor;
class fifo;

class WidgetFosphor : public QOpenGLWidget, protected QOpenGLFunctions
{
	Q_OBJECT
public:
	enum  ui_action_t {
		DB_PER_DIV_UP,
		DB_PER_DIV_DOWN,
		REF_UP,
		REF_DOWN,
		ZOOM_TOGGLE,
		ZOOM_WIDTH_UP,
		ZOOM_WIDTH_DOWN,
		ZOOM_CENTER_UP,
		ZOOM_CENTER_DOWN,
		RATIO_UP,
		RATIO_DOWN,
		FREEZE_TOGGLE,
	};

public:
	WidgetFosphor(QWidget *parent);
	~WidgetFosphor();

	/* gr::fosphor::base_sink_c implementation */
	void execute_ui_action(enum ui_action_t action);

	void set_frequency_range(const double center, const double span);
	void set_frequency_center(const double center);
	void set_frequency_span(const double span);
	void set_fft_window(const fft::window::win_type win);

	int work(const std::vector<gr_complex> &input_items);

protected:

	void initializeGL() override;
	void resizeGL(int w, int h) override;
	void paintGL() override;
	void keyPressEvent(QKeyEvent *ke) override;	
	void mouseMoveEvent(QMouseEvent *event) override;

private:
	bool init_ = false;


	bool d_active;
	bool d_frozen;

	/* fosphor core */
	fifo *d_fifo;

	struct fosphor *d_fosphor;
	struct fosphor_render *d_render_main;
	struct fosphor_render *d_render_zoom;


	/* settings refresh logic */
	enum {
		SETTING_DIMENSIONS = (1 << 0),
		SETTING_POWER_RANGE = (1 << 1),
		SETTING_FREQUENCY_RANGE = (1 << 2),
		SETTING_FFT_WINDOW = (1 << 3),
		SETTING_RENDER_OPTIONS = (1 << 4),
	};

	uint32_t d_settings_changed;
	std::mutex d_settings_mutex;
	void settings_mark_changed(uint32_t setting);
	uint32_t settings_get_and_reset_changed(void);
	void settings_apply(uint32_t settings);


	/* settings values */
	int d_width;
	int d_height;

	static const int k_db_per_div[];
	int d_db_ref;
	int d_db_per_div_idx;

	bool  d_zoom_enabled;
	double d_zoom_center;
	double d_zoom_width;

	float d_ratio;

	struct {
		double center;
		double span;
	} d_frequency;

	fft::window::win_type d_fft_window;

};
