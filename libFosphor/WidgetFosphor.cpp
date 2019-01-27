#include "../libFosphor/fosphor/gl_platform.h"

#include "WidgetFosphor.h"

#include "fifo.h"
#include "fosphor/fosphor.h"

#include <QDebug>
#include <QDateTime>
#include <QKeyEvent>

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


	delete this->d_render_zoom;
	delete this->d_render_main;
	delete this->d_fifo;
}

void WidgetFosphor::initializeGL()
{
	initializeOpenGLFunctions();

	GLuint fff9 = 0;
	QOpenGLFunctions::glGenBuffers(1, &fff9);

	GLenum glew_err = glewInit();

	d_fosphor = fosphor_init();

	/* Init FIFO */
	this->d_fifo = new fifo(2 * 1024 * 1024);

	/* Init render options */
	this->d_render_main = new fosphor_render();
	fosphor_render_defaults(this->d_render_main);

	this->d_render_zoom = new fosphor_render();
	fosphor_render_defaults(this->d_render_zoom);
	this->d_render_zoom->options &= ~(FRO_LABEL_PWR | FRO_LABEL_TIME);

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

	const int fft_len = 1024;
	const int batch_mult = 16;
	const int batch_max = 1024;
	const int max_iter = 8;

	int i, tot_len;

	/* Handle pending settings */
	this->settings_apply(this->settings_get_and_reset_changed());

	/* Clear everything */
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	tot_len = this->d_fifo->used();

	/* Process as much we can */
	for (i = 0; i < max_iter && tot_len; i++)
	{
		gr_complex *data;
		int len;

		/* How much can we get from FIFO in one block */
		len = tot_len;
		if (len > this->d_fifo->read_max_size())
			len = this->d_fifo->read_max_size();

		/* Adapt to valid size for fosphor */
		len &= ~((batch_mult * fft_len) - 1);
		if (len > (batch_max * fft_len))
			len = batch_max * fft_len;

		/* Is it worth it ? */
		tot_len -= len;

		if (!len)
			break;

		/* Send to process (if not frozen) */
		if (!this->d_frozen) {
			data = this->d_fifo->read_peek(len, false);
			fosphor_process(this->d_fosphor, data, len);
		}

		/* Discard */
		this->d_fifo->read_discard(len);
	}

	/* Draw */
	fosphor_draw(this->d_fosphor, this->d_render_main);

	if (this->d_zoom_enabled)
		fosphor_draw(this->d_fosphor, this->d_render_zoom);

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
	std::scoped_lock lock(this->d_settings_mutex);
	this->d_settings_changed |= setting;
}

uint32_t WidgetFosphor::settings_get_and_reset_changed(void)
{
	std::scoped_lock lock(this->d_settings_mutex);
	uint32_t v = this->d_settings_changed;
	this->d_settings_changed = 0;
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
		glOrtho(0.0, (double)this->d_width, 0.0, (double)this->d_height, -1.0, 1.0);

		glViewport(0, 0, this->d_width, this->d_height);
	}

	if (settings & SETTING_POWER_RANGE)
	{
		fosphor_set_power_range(this->d_fosphor,
			this->d_db_ref,
			this->k_db_per_div[this->d_db_per_div_idx]
		);
	}

	if (settings & SETTING_FREQUENCY_RANGE)
	{
		fosphor_set_frequency_range(this->d_fosphor,
			this->d_frequency.center,
			this->d_frequency.span
		);
	}

	if (settings & SETTING_FFT_WINDOW)
	{
		std::vector<float> window =
			fft::window::build(this->d_fft_window, 1024, 6.76);
		fosphor_set_fft_window(this->d_fosphor, window.data());
	}

	if (settings & (SETTING_DIMENSIONS | SETTING_RENDER_OPTIONS))
	{
		float f;

		if (this->d_zoom_enabled) {
			int a = (int)(this->d_width * 0.65f);
			this->d_render_main->width = a;
			this->d_render_main->options |= FRO_CHANNELS;
			this->d_render_main->options &= ~FRO_COLOR_SCALE;
			this->d_render_zoom->pos_x = a - 10;
			this->d_render_zoom->width = this->d_width - a + 10;
		}
		else
		{
			this->d_render_main->width = this->d_width;
			this->d_render_main->options &= ~FRO_CHANNELS;
			this->d_render_main->options |= FRO_COLOR_SCALE;
		}

		this->d_render_main->height = this->d_height;
		this->d_render_zoom->height = this->d_height;

		this->d_render_main->histo_wf_ratio = this->d_ratio;
		this->d_render_zoom->histo_wf_ratio = this->d_ratio;

		this->d_render_main->channels[0].enabled = this->d_zoom_enabled;
		this->d_render_main->channels[0].center = (float)this->d_zoom_center;
		this->d_render_main->channels[0].width = (float)this->d_zoom_width;

		f = (float)(this->d_zoom_center - this->d_zoom_width / 2.0);
		this->d_render_zoom->freq_start =
			f > 0.0f ? (f < 1.0f ? f : 1.0f) : 0.0f;

		f = (float)(this->d_zoom_center + this->d_zoom_width / 2.0);
		this->d_render_zoom->freq_stop =
			f > 0.0f ? (f < 1.0f ? f : 1.0f) : 0.0f;

		fosphor_render_refresh(this->d_render_main);
		fosphor_render_refresh(this->d_render_zoom);
	}
}

void
WidgetFosphor::execute_ui_action(enum ui_action_t action)
{
	switch (action)
	{
	case DB_PER_DIV_UP:
		if (this->d_db_per_div_idx < 4)
			this->d_db_per_div_idx++;
		break;

	case DB_PER_DIV_DOWN:
		if (this->d_db_per_div_idx > 0)
			this->d_db_per_div_idx--;
		break;

	case REF_UP:
		this->d_db_ref += k_db_per_div[this->d_db_per_div_idx];
		break;

	case REF_DOWN:
		this->d_db_ref -= k_db_per_div[this->d_db_per_div_idx];
		break;

	case ZOOM_TOGGLE:
		this->d_zoom_enabled ^= 1;
		break;

	case ZOOM_WIDTH_UP:
		if (this->d_zoom_enabled)
			this->d_zoom_width *= 2.0;
		break;

	case ZOOM_WIDTH_DOWN:
		if (this->d_zoom_enabled)
			this->d_zoom_width /= 2.0;
		break;

	case ZOOM_CENTER_UP:
		if (this->d_zoom_enabled)
			this->d_zoom_center += this->d_zoom_width / 8.0;
		break;

	case ZOOM_CENTER_DOWN:
		if (this->d_zoom_enabled)
			this->d_zoom_center -= this->d_zoom_width / 8.0;
		break;

	case RATIO_UP:
		if (this->d_ratio < 0.8f)
			this->d_ratio += 0.05f;
		break;

	case RATIO_DOWN:
		if (this->d_ratio > 0.2f)
			this->d_ratio -= 0.05f;
		break;

	case FREEZE_TOGGLE:
		this->d_frozen ^= 1;
		break;
	}

	this->settings_mark_changed(SETTING_POWER_RANGE | SETTING_RENDER_OPTIONS
	);
}

void
WidgetFosphor::set_frequency_range(const double center, const double span)
{
	this->d_frequency.center = center;
	this->d_frequency.span = span;
	this->settings_mark_changed(SETTING_FREQUENCY_RANGE);
}

void
WidgetFosphor::set_frequency_center(const double center)
{
	this->d_frequency.center = center;
	this->settings_mark_changed(SETTING_FREQUENCY_RANGE);
}

void
WidgetFosphor::set_frequency_span(const double span)
{
	this->d_frequency.span = span;
	this->settings_mark_changed(SETTING_FREQUENCY_RANGE);
}

void
WidgetFosphor::set_fft_window(const fft::window::win_type win)
{
	if (win == this->d_fft_window)	/* Reloading FFT window takes time */
		return;			/* avoid doing it if possible */

	this->d_fft_window = win;
	this->settings_mark_changed(SETTING_FFT_WINDOW);
}




int WidgetFosphor::work(const std::vector<gr_complex> &input_items)
{
	const gr_complex *in = input_items.data();
	gr_complex *dst;
	int l, mw;

	/* How much can we hope to write */
	l = input_items.size();
	mw = d_fifo->write_max_size();

	if (l > mw)
		l = mw;
	if (!l)
		return 0;

	/* Get a pointer */
	dst = this->d_fifo->write_prepare(l, true);
	if (!dst)
		return 0;

	/* Do the copy */
	memcpy(dst, in, sizeof(gr_complex) * l);
	this->d_fifo->write_commit(l);

	/* Report what we took */
	return l;
}
