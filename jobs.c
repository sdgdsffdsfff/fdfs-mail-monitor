#include "jobs.h"

int strtotime(char *datetime)
{
	struct tm tm_time = { 0 };
	int unixtime;

	strptime(datetime, "%Y-%m-%d %H:%M:%S", &tm_time);

	unixtime = mktime(&tm_time);
	return unixtime;
}

void timetostr(time_t *time, char *datatime)
{
	struct tm ptm = { 0 };

	localtime_r(time, &ptm);
	strftime(datatime, 20, "%Y-%m-%d %H:%M:%S", &ptm);
	return ;
}

static time_t get_job_time()
{
	char strtime[20];
	time_t now, job_time;
	int cut_sec;

	memset(strtime, 0, sizeof(char)*20);
	time(&now);
	//current_time
	timetostr(&now,strtime);
	
	//printf("now %s\n",strtime);

	//job_time
	cut_sec = strtol(strtime + SEC_POS, NULL, 10);

	job_time = strtotime(strtime) - cut_sec + 60;
	
	return job_time;
}

static void *job_entrance(void *arg)
{
	struct job *job = arg;

	job->run = 1;
	job->job_time = get_job_time();
	while(job->run)
	{
		time(&job->now_time);
		if(job->now_time < job->job_time){
			job->wait_time = job->job_time - job->now_time;	
			if(job->wait_time > 10){
				//printf("sleeping..\n");
				sleep(10);
			}
			else sleep(job->wait_time);
		}else{
			job->rsp = job->call(job->job_time,job->arg);
			job->job_time += 60;
		}
	}
	return NULL;
}

void job_service(struct job *job)
{
	int err;
	err = pthread_create(&job->pid, NULL, job_entrance, job);
	if (err != 0){
		//printf("can't create thread: %s\n", strerror(err));
	}
	return ;
}

void job_destory(struct job *job)
{
	job->run = 0;
	pthread_join(job->pid,NULL);
	return;
}
