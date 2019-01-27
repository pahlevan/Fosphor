#pragma once

#include <mutex>
#include <condition_variable>

#include "gr_complex.h"

class fifo
{
private:
	gr_complex *d_buf;
	int d_len;
	int d_rp;
	int d_wp;

	std::mutex d_mutex;
	std::condition_variable d_cond_empty;
	std::condition_variable d_cond_full;

public:
	fifo(int length);
	~fifo();

	int free();
	int used();

	int write_max_size();
	gr_complex *write_prepare(int size, bool wait = true);
	void write_commit(int size);

	int read_max_size();
	gr_complex *read_peek(int size, bool wait = true);
	void read_discard(int size);
};
