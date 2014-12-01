#ifndef JOBS_H
#define JOBS_H

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define SEC_POS				17
#define MIN_POS				14
#define HOUR_POS			11

struct job{
	time_t now_time;
	time_t job_time;
	time_t wait_time;
	pthread_t pid;
	void* (*call)(time_t job_time, void *);
	void *arg;
	void *rsp;
	char run;
};

int strtotime(char *datetime);
void timetostr(time_t *time, char *datatime);

void job_service(struct job *job);
void job_destory(struct job *job);

#endif
