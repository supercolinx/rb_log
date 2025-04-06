#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include "rb_log.h"

static int s_times = 1;
static sem_t s_read, s_write;

static void *thread_write(void *arg)
{
	ssize_t size;
	char buf[256];
	int i = 0;

	while (1)
	{
		sem_wait(&s_write);
		if (i % 2 == 0) {
			sprintf(buf, "ABCDEFG:%.04d\n", i++);
		} else {
			sprintf(buf, "010u4o3iu03qu50w:%.04d\n", i++);
		}
		size = rbg_write(arg, buf, strlen(buf));
		sem_post(&s_read);
		if (s_times == i) {
			printf("write done\n");
			break;
		}
	}

	return NULL;
}

static void *thread_read(void *arg)
{
	ssize_t size, _size;
	char buf[256];
	int i = 0;

	while (1)
	{
		memset(buf, 0, sizeof(buf));

		sem_wait(&s_read);
		size = rbg_read(arg, buf, sizeof(buf));
		sem_post(&s_write);

		if (s_times == ++i) {
			printf("read done\n");
			break;
		}
	}

	return NULL;
}

int main(int argc, char *argv[])
{
	pthread_t pt1, pt2;
	sem_init(&s_read, 0, 0);
	sem_init(&s_write, 0, 1);
	void *rbg = rbg_create("tmp.log", 1);

	if (2 <= argc) {
		s_times = atoi(argv[1]);
	}

	pthread_create(&pt1, NULL, thread_write, rbg);
	pthread_create(&pt2, NULL, thread_read, rbg);

	pthread_join(pt1, NULL);
	pthread_join(pt2, NULL);
	rbg_destory(rbg);
	return 0;
}
