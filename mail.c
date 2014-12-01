#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "jobs.h"
//LOG_TAG

int log_fd;

#ifdef DEBUG
#define LOGI(str) write(log_fd,str,strlen(str))
#define LOGE(str) write(log_fd,str,strlen(str))
#else
#define LOGI(str) write(log_fd,str,strlen(str))
#define LOGE(str) write(log_fd,str,strlen(str))
#endif
#define BUFFSIZE 1024*1024

char request[BUFFSIZE], text[5*BUFFSIZE];

static void daemonize()
{
	int fd0, fd1, fd2;
	pid_t pid;
	struct rlimit rlimit;


	if(getrlimit(RLIMIT_NOFILE, &rlimit) < 0){
		fprintf(stderr, "can not get file limit.\n");
		exit(1);
	}

	if ((pid = fork()) < 0){
		fprintf(stderr, "can not fork.\n");
		exit(1);
	} else if (pid != 0){  /* parent process */
		exit(0);
	}
	setsid();
	if((pid = fork()) != 0) 
		exit(0);
	else if(pid < 0) 
		exit(1);

	/*      
	 * if (chdir("/") < 0){
	 *      fprintf(stderr, ": can not change directory to /\n");
	 *      exit(1);
	 * }
	 */

	if (rlimit.rlim_max == RLIM_INFINITY)
		rlimit.rlim_max = 1024;
	int i;
	for (i = 0; i < rlimit.rlim_max; i ++){
		close(i);
	}

	umask(0);

	fd0 = open("/dev/null", O_RDWR);
	fd1 = dup(0);
	fd2 = dup(0);
	if (fd0 != 0 || fd1 != 1 || fd2 != 2){
		fprintf(stderr, "unexpected file descriptors after daemonizing %d %d %d\n", fd0, fd1, fd2);
		exit(1);
	}
	fprintf(stderr, "start.\n");
}

int httpGet(char* hostname,char *url)
{
	LOGI("httpGet");
	LOGI("\n");
	LOGI(hostname);
	LOGI("\n");
	//char myurl[BUFFSIZE] = {0};
	//char host[BUFFSIZE] = {0};
	//char GET[BUFFSIZE] = {0};
	struct sockaddr_in sin;
	int sockfd;
	if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) == -1) {
		LOGE(strerror(errno));
		LOGI("\n");
		return -100;
	}

	struct hostent * host_addr = gethostbyname(hostname);
	if(host_addr==NULL) {
		LOGE(strerror(errno));
		LOGI("\n");
		return -103;
	}
	sin.sin_family = AF_INET;
	sin.sin_port = htons( (unsigned short)80);
	sin.sin_addr.s_addr = *((unsigned long*)host_addr->h_addr_list[0]);
	if( connect (sockfd,(const struct sockaddr *)&sin, sizeof(struct sockaddr_in) ) == -1 ) {
		LOGE(strerror(errno));
		LOGI("\n");
		return -101;
	}
	LOGI("httpGet send");
	LOGI("\n");
	// 向WEB服务器发送URL信息
	memset(request, 0, BUFFSIZE);
	strcat(request, url); //请求内容与http版本
	//strcat(request, "GET /index.html HTTP/1.1\r\n"); //请求内容与http版本
	strcat(request, "HOST:"); //主机名，，格式："HOST:主机"
	strcat(request, hostname);
	strcat(request, "\r\n");
	strcat(request, "Accept:*/*\r\n"); //接受类型，所有类型
	// strcat(request, "User-Agent:Mozilla/4.0 (compatible; MSIE 5.00; Windows 98)");//指定浏览器类型？
	// strcat(request, "Connection: Keep-Alive\r\n");//设置连接，保持连接
	// strcat(request, "Set Cookie:0\r\n");//设置Cookie
	// strcat(request, "Range: bytes=0 - 500\r\n");//设置请求字符串起止位置，断点续传关键"Range: bytes=999 -"
	strcat(request, "\r\n");//空行表示结束
	LOGI(request);
	LOGI("\n");
	if( send (sockfd, request, strlen(request), 0) == -1){
		LOGE(strerror(errno));
		return -99;
	}
	LOGI("httpGet recv\n");

	struct timeval tv_out;
	tv_out.tv_sec = 5;
	tv_out.tv_usec = 0;

	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv_out, sizeof(tv_out));
	memset(text,0,BUFFSIZE);
	int recv_size = 0,all_size = 0;
	while((recv_size = recv (sockfd, text + all_size, 5*BUFFSIZE, 0)) != 0){
		if(recv_size < 0)
			break;
		//printf("%d %d\n",recv_size,all_size);
		all_size += recv_size;
		if(all_size > 5*BUFFSIZE)
			break;
	}
	//printf("%s\n",text);
	//LOGI(text);
	LOGI("httpGet end\n");
	close(sockfd);
	return 0;
}

char mail[5*1024];
int need_send;

void get_text()
{
	char *start = NULL;
	char *end = NULL;
	memset(mail,0,5*1024);

	need_send = 0;
	if(strtol(strstr(text,"code\" : ") + strlen("code\" : "),0,10) != 200){
		start = strstr(text,"\"message\"");
		end = strstr(text,"code") - 1;
		strncpy(mail,start,end-start);
		need_send = 1;
	}
	LOGI(mail);
	LOGI("\n");
	return ;
}

void send_mail()
{
	char send_command[5*1024 + 1024];
	memset(send_command,0,6*1024);
	if(need_send != 0){
		sprintf(send_command,"echo \"%s\" | mailx -A meizu -v -s \"send from job monitor\" chengyue@meizu.com",mail);
		system(send_command);
		sprintf(send_command,"echo \"%s\" | mailx -A meizu -v -s \"send from job monitor\" xueyuan@meizu.com",mail);
		system(send_command);
	}
}

static void* mail_job_monitor(time_t job_time,void *arg) 
{
	char key[20];
	timetostr(&job_time,key);
	LOGI(key);
	LOGI("\n");
	if(httpGet("172.16.3.14","GET /fastdfs-v1.0/alert/alert.do HTTP/1.1\r\n") < 0)
		return NULL;
	get_text();
	send_mail();
	return NULL;
}
int main()
{
	daemonize();

	log_fd = open("mail_log",O_WRONLY|O_CREAT|O_APPEND);

	//mail_job_monitor(123,NULL);

	//return 0;
	struct job job;
	job_service(&job);
	job.call = mail_job_monitor;
	//mail_job_monitor(0,NULL);
	sleep(1000000000);
	return 0;
}
