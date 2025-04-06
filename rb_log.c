/* 循环缓冲日志(线程安全)
 * TODO 支持进程
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/sendfile.h>
#include "rb_log.h"

#define rb_log_max(a,b) (((a) > (b)) ? (a) : (b))
#define rb_log_min(a,b) (((a) < (b)) ? (a) : (b))

struct rb_log {
	int fd;
	size_t pread;
	size_t pwrite;
	size_t size;
	size_t used;
	void *buffer;
	pthread_rwlock_t rwlock;
};

void
rbg_destory(void *rbg)
{
	struct rb_log *rb = (struct rb_log*)rbg;
	msync(rb->buffer, rb->size, MS_SYNC);
	close(rb->fd);
	pthread_rwlock_destroy(&rb->rwlock);
	free(rb);
}

void *
rbg_create(const char *pathname, int pages)
{
	char buf[256] = {0};
	struct rb_log *rb = (struct rb_log*)malloc(sizeof(struct rb_log));
	if (!rb) {
		return NULL;
	}

	if (0 == access(pathname, F_OK)) {
		snprintf(buf, sizeof(buf), "%s.bak", pathname);
	}

	rb->fd = open(pathname, O_RDWR | O_CREAT, 0644);
	if (-1 == rb->fd) {
		goto RB_CREATE_ERROR1;
	}

	rb->pread= rb->pwrite = rb->used = 0;
	rb->size = pages * sysconf(_SC_PAGESIZE);
	if (-1 == ftruncate(rb->fd, (off_t)rb->size)) {
		goto RB_CREATE_ERROR0;
	}

	rb->buffer = mmap(NULL,
			rb->size,
			PROT_READ | PROT_WRITE,
			MAP_SHARED,
			rb->fd, 0);
	if (MAP_FAILED == rb->buffer) {
		goto RB_CREATE_ERROR0;
	}

	if (-1 == rbg_backup(rb, buf)) {/* backup */
		fprintf(stderr, "[E] backup %s\n", pathname);
	}

	memset(rb->buffer, 0, rb->size);
	pthread_rwlock_init(&rb->rwlock, NULL);

	return rb;
RB_CREATE_ERROR0:
	close(rb->fd);
RB_CREATE_ERROR1:
	free(rb);
	return NULL;
}

static ssize_t
rbg_writeline(struct rb_log *rb, const char *buf, size_t size)
{
	size_t i = 0;
	while (i < size)
	{
		((char*)rb->buffer)[rb->pwrite] = buf[i++];
		if (++rb->pwrite >= rb->size) {
			rb->pwrite = rb->pwrite % rb->size;
		}
	}
	if (i > rb->size - rb->used) {
		rb->pread = rb->pwrite;
		rb->used = rb->size;
	} else {
		rb->used = rb->used + i;
	}
	return i;
}

ssize_t
rbg_write(void *rbg, const char *buf, size_t size)
{
	struct rb_log *rb = (struct rb_log*)rbg;
	pthread_rwlock_wrlock(&rb->rwlock);
	size = rbg_writeline(rb, buf, size);
	pthread_rwlock_unlock(&rb->rwlock);

	return size;
}

static ssize_t
rbg_readline(struct rb_log *rb, char *buf, size_t size)
{
	size_t i = 0;
	while (rb->used && i < size)
	{
		char ch = ((char*)rb->buffer)[rb->pread];
		if (++rb->pread >= rb->size) {
			rb->pread = rb->pread % rb->size;
		}
		rb->used--;

		if (ch) {
			buf[i++] = ch;
			if ('\n' == ch) {
				break;
			}
		}
	}

	return i;
}

ssize_t
rbg_read(void *rbg, char *buf, size_t size)
{
	struct rb_log *rb = (struct rb_log*)rbg;

	pthread_rwlock_rdlock(&rb->rwlock);
	size = rbg_readline(rb, buf, size);
	pthread_rwlock_unlock(&rb->rwlock);

	return size;
}

ssize_t
rbg_backup(const void *rbg, const char *pathname)
{
	struct rb_log *rb = (struct rb_log*)rbg;
	int fd = open(pathname, O_WRONLY | O_CREAT, 0644);
	if (-1 == fd) {
		return -1;
	}

	pthread_rwlock_rdlock(&rb->rwlock);
	msync(rb->buffer, rb->size, MS_SYNC);
	ssize_t size = sendfile(fd, rb->fd, 0, rb->size);
	pthread_rwlock_unlock(&rb->rwlock);

	close(fd);
	return size;
}
