#include "../libFosphor/fosphor/gl_platform.h"

#include "WidgetFosphor.h"

#include "fifo.h"
#include "fosphor/fosphor.h"

#include <QDebug>
#include <QDateTime>
#include <QKeyEvent>
#include <vector>

const int WidgetFosphor::k_db_per_div[] = { 1, 2, 5, 10, 20 };

WidgetFosphor::WidgetFosphor(QWidget *parent)
	: QOpenGLWidget(parent)
	, d_db_ref(0)
	, d_db_per_div_idx(3)
	, d_zoom_enabled(false)
	, d_zoom_center(0.5)
	, d_zoom_width(0.2)
	, d_ratio(0.35f)
	, d_frozen(false)
	, d_active(false)
	, d_frequency()
	, d_fft_window(fft::window::win_type::WIN_BLACKMAN_hARRIS)
{

	setFocus();
}

WidgetFosphor::~WidgetFosphor()
{
	d_active = false;
	fosphor_release(d_fosphor);


	delete d_render_zoom;
	delete d_render_main;
}

void WidgetFosphor::initializeGL()
{
	initializeOpenGLFunctions();


	GLenum glew_err = glewInit();

	d_fosphor = fosphor_init();

	/* Init FIFO */

	/* Init render options */
	d_render_main = new fosphor_render();
	fosphor_render_defaults(d_render_main);

	d_render_zoom = new fosphor_render();
	fosphor_render_defaults(d_render_zoom);
	d_render_zoom->options &= ~(FRO_LABEL_PWR | FRO_LABEL_TIME);

	init_ = true;
	d_active = true;
}

void WidgetFosphor::resizeGL(int w, int h)
{
	d_width = w;
	d_height = h;
	settings_mark_changed(SETTING_DIMENSIONS);
}

void WidgetFosphor::paintGL()
{
	if (init_ == false)
	{
		return;
	}
	const int fft_len = 64*1024;

	int i, tot_len;

	/* Handle pending settings */
	settings_apply(settings_get_and_reset_changed());

	/* Clear everything */
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);


	/* Process as much we can */
	for (i = 0; i < 8 ; i++)
	{


		std::vector<gr_complex> ssss_vec(fft_len);

		for (auto& it : ssss_vec)
		{
			it.imag((rand() % 10000) / 10000.0);
			it.real((rand() % 10000) / 100000.0);
		}

		fosphor_process(d_fosphor, ssss_vec.data(), fft_len);

	}

	/* Draw */
	fosphor_draw(d_fosphor, d_render_main);

	if (d_zoom_enabled)
		fosphor_draw(d_fosphor, d_render_zoom);

	/* Done, swap buffer */

	qDebug() << QDateTime::currentDateTime();

	emit update();
}


void
WidgetFosphor::keyPressEvent(QKeyEvent *ke)
{
	switch (ke->key()) 
	{
	case Qt::Key_Up:
		execute_ui_action(ui_action_t::REF_DOWN);
		break;
	case Qt::Key_Down:
		execute_ui_action(ui_action_t::REF_UP);
		break;
	case Qt::Key_Left:
		execute_ui_action(ui_action_t::DB_PER_DIV_DOWN);
		break;
	case Qt::Key_Right:
		execute_ui_action(ui_action_t::DB_PER_DIV_UP);
		break;
	case Qt::Key_Z:
		execute_ui_action(ui_action_t::ZOOM_TOGGLE);
		break;
	case Qt::Key_W:
		execute_ui_action(ui_action_t::ZOOM_WIDTH_UP);
		break;
	case Qt::Key_S:
		execute_ui_action(ui_action_t::ZOOM_WIDTH_DOWN);
		break;
	case Qt::Key_D:
		execute_ui_action(ui_action_t::ZOOM_CENTER_UP);
		break;
	case Qt::Key_A:
		execute_ui_action(ui_action_t::ZOOM_CENTER_DOWN);
		break;
	case Qt::Key_Q:
		execute_ui_action(ui_action_t::RATIO_UP);
		break;
	case Qt::Key_E:
		execute_ui_action(ui_action_t::RATIO_DOWN);
		break;
	case Qt::Key_Space:
		execute_ui_action(ui_action_t::FREEZE_TOGGLE);
		break;
	}
}

void WidgetFosphor::mouseMoveEvent(QMouseEvent *event)
{
	setFocus();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void WidgetFosphor::settings_mark_changed(uint32_t setting)
{
	std::scoped_lock lock(d_settings_mutex);
	d_settings_changed |= setting;
}

uint32_t WidgetFosphor::settings_get_and_reset_changed(void)
{
	std::scoped_lock lock(d_settings_mutex);
	uint32_t v = d_settings_changed;
	d_settings_changed = 0;
	return v;
}

void WidgetFosphor::settings_apply(uint32_t settings)
{
	if (settings & SETTING_DIMENSIONS)
	{
		makeCurrent();

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0.0, (double)d_width, 0.0, (double)d_height, -1.0, 1.0);

		glViewport(0, 0, d_width, d_height);
	}

	if (settings & SETTING_POWER_RANGE)
	{
		fosphor_set_power_range(d_fosphor,
			d_db_ref,
			k_db_per_div[d_db_per_div_idx]
		);
	}

	if (settings & SETTING_FREQUENCY_RANGE)
	{
		fosphor_set_frequency_range(d_fosphor,
			d_frequency.center,
			d_frequency.span
		);
	}

	if (settings & SETTING_FFT_WINDOW)
	{
		std::vector<float> window =	fft::window::build(d_fft_window, 64*1024, 6.76);
		fosphor_set_fft_window(d_fosphor, window.data());
	}

	if (settings & (SETTING_DIMENSIONS | SETTING_RENDER_OPTIONS))
	{
		float f;

		if (d_zoom_enabled) {
			int a = (int)(d_width * 0.65f);
			d_render_main->width = a;
			d_render_main->options |= FRO_CHANNELS;
			d_render_main->options &= ~FRO_COLOR_SCALE;
			d_render_zoom->pos_x = a - 10;
			d_render_zoom->width = d_width - a + 10;
		}
		else
		{
			d_render_main->width = d_width;
			d_render_main->options &= ~FRO_CHANNELS;
			d_render_main->options |= FRO_COLOR_SCALE;
		}

		d_render_main->height = d_height;
		d_render_zoom->height = d_height;

		d_render_main->histo_wf_ratio = d_ratio;
		d_render_zoom->histo_wf_ratio = d_ratio;

		d_render_main->channels[0].enabled = d_zoom_enabled;
		d_render_main->channels[0].center = (float)d_zoom_center;
		d_render_main->channels[0].width = (float)d_zoom_width;

		f = (float)(d_zoom_center - d_zoom_width / 2.0);
		d_render_zoom->freq_start =
			f > 0.0f ? (f < 1.0f ? f : 1.0f) : 0.0f;

		f = (float)(d_zoom_center + d_zoom_width / 2.0);
		d_render_zoom->freq_stop =
			f > 0.0f ? (f < 1.0f ? f : 1.0f) : 0.0f;

		fosphor_render_refresh(d_render_main);
		fosphor_render_refresh(d_render_zoom);
	}
}

void
WidgetFosphor::execute_ui_action(enum ui_action_t action)
{
	switch (action)
	{
	case DB_PER_DIV_UP:
		if (d_db_per_div_idx < 4)
			d_db_per_div_idx++;
		break;

	case DB_PER_DIV_DOWN:
		if (d_db_per_div_idx > 0)
			d_db_per_div_idx--;
		break;

	case REF_UP:
		d_db_ref += k_db_per_div[d_db_per_div_idx];
		break;

	case REF_DOWN:
		d_db_ref -= k_db_per_div[d_db_per_div_idx];
		break;

	case ZOOM_TOGGLE:
		d_zoom_enabled ^= 1;
		break;

	case ZOOM_WIDTH_UP:
		if (d_zoom_enabled)
			d_zoom_width *= 2.0;
		break;

	case ZOOM_WIDTH_DOWN:
		if (d_zoom_enabled)
			d_zoom_width /= 2.0;
		break;

	case ZOOM_CENTER_UP:
		if (d_zoom_enabled)
			d_zoom_center += d_zoom_width / 8.0;
		break;

	case ZOOM_CENTER_DOWN:
		if (d_zoom_enabled)
			d_zoom_center -= d_zoom_width / 8.0;
		break;

	case RATIO_UP:
		if (d_ratio < 0.8f)
			d_ratio += 0.05f;
		break;

	case RATIO_DOWN:
		if (d_ratio > 0.2f)
			d_ratio -= 0.05f;
		break;

	case FREEZE_TOGGLE:
		d_frozen ^= 1;
		break;
	}

	settings_mark_changed(SETTING_POWER_RANGE | SETTING_RENDER_OPTIONS
	);
}

void
WidgetFosphor::set_frequency_range(const double center, const double span)
{
	d_frequency.center = center;
	d_frequency.span = span;
	settings_mark_changed(SETTING_FREQUENCY_RANGE);
}

void
WidgetFosphor::set_frequency_center(const double center)
{
	d_frequency.center = center;
	settings_mark_changed(SETTING_FREQUENCY_RANGE);
}

void
WidgetFosphor::set_frequency_span(const double span)
{
	d_frequency.span = span;
	settings_mark_changed(SETTING_FREQUENCY_RANGE);
}

void
WidgetFosphor::set_fft_window(const fft::window::win_type win)
{
	if (win == d_fft_window)	/* Reloading FFT window takes time */
		return;			/* avoid doing it if possible */

	d_fft_window = win;
	settings_mark_changed(SETTING_FFT_WINDOW);
}




int WidgetFosphor::work(const std::vector<gr_complex>& input_items)
{
// 	/*const gr_complex *in = input_items.data();
// 	gr_complex *dst;
// 	int l, mw;
// 
// 	/* How much can we hope to write */
// 	l = input_items.size();
// 	mw = d_fifo->write_max_size();
// 
// 	if (l > mw)
// 		l = mw;
// 	if (!l)
// 		return 0;
// 
// 	/* Get a pointer */
// 	dst = d_fifo->write_prepare(l, true);
// 	if (!dst)
// 		return 0;
// 
// 	/* Do the copy */
// 	memcpy(dst, in, sizeof(gr_complex) * l);
// 	d_fifo->write_commit(l);
// 	*/
// 	/* Report what we took */
// 	return l;
return 0;
 
}
