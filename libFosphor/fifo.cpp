#include "fifo.h"

fifo::fifo(int length) :
	d_len(length), d_rp(0), d_wp(0)
{
	this->d_buf = new gr_complex[this->d_len];
}

fifo::~fifo()
{
	delete[] this->d_buf;
}

int fifo::free()
{
	return (this->d_len - 1) - this->used();
}

int fifo::used()
{
	return (this->d_wp - this->d_rp) & (this->d_len - 1);
}

int fifo::write_max_size()
{
	return this->d_len - this->d_wp;
}

gr_complex* fifo::write_prepare(int size, bool wait)
{
	std::unique_lock lock(this->d_mutex);

	if (!wait && (this->free() < size))
		return nullptr;

	while (this->free() < size)
		this->d_cond_full.wait(lock);

	return &this->d_buf[this->d_wp];
}

void fifo::write_commit(int size)
{
	std::unique_lock lock(this->d_mutex);

	this->d_wp = (this->d_wp + size) & (this->d_len - 1);

	this->d_cond_empty.notify_one();
}

int fifo::read_max_size()
{
	return this->d_len - this->d_rp;
}

gr_complex* fifo::read_peek(int size, bool wait)
{
	std::unique_lock lock(this->d_mutex);

	if (!wait && (this->used() < size))
		return NULL;

	while (this->used() < size)
		this->d_cond_empty.wait(lock);

	return &this->d_buf[this->d_rp];
}

void fifo::read_discard(int size)
{
	std::unique_lock lock(this->d_mutex);

	this->d_rp = (this->d_rp + size) & (this->d_len - 1);

	this->d_cond_full.notify_one();
}