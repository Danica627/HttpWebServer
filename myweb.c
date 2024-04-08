#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>
int CreatSocket()
{
	int sockfd=socket(AF_INET,SOCK_STREAM,0);
	struct sockaddr_in saddr;
	memset(&saddr,0,sizeof(saddr));
	saddr.sin_family=AF_INET;
	saddr.sin_port=htons(80);
	saddr.sin_addr.s_addr=inet_addr("127.0.0.1");
	//saddr.sin_addr.s_addr=inet_addr("192.168.5.128");
	
	int res=bind(sockfd,(struct sockaddr*)&saddr,sizeof(saddr));
	if(res==-1)
	{
		return -1;
	}
	res=listen(sockfd,5);
	if(res==-1)
	{
		return -1;
	}
	return sockfd;
}

int OpenFile(char* filename,int*filesize)
{
	if(filename==NULL||filesize==NULL)
	{
		return -1;
	}
	char path[128]={"/home/ljx/liujingxuan/test35"};
	if(strcmp(filename,"/")==0)
	{
		strcat(path,"/index.html");
	}
	else
	{
		strcat(path,filename);
	}
	int fd=open(path,O_RDONLY);
	if(fd==-1)
	{
		*filesize=0;
	//	printf("open file erro");
	}
	else
	{
		*filesize=lseek(fd,0,SEEK_END);
		lseek(fd,0,SEEK_SET);
	}
	return fd;
}

char* GetMessage(char* buff)
{
	if(buff==NULL)
	{
		return NULL;
	}
	char* ptr=NULL;
	char* s=strtok_r(buff," ",&ptr);
	if(s==NULL)
	{
		return NULL;
	}
	printf("请求方法：%s\n",s);
	s=strtok_r(NULL," ",&ptr);
	if(s==NULL)
	{
		return NULL;
	}
	return s;
}


void* work_pthread(void*arg)
{
    int c=*(int*)arg;
    while(1)
    {
        char buff[1024]={0};
        int n=recv(c,buff,1023,0);
        if(n<=0)
        {
            break;
        }
        printf("n=%d,read:\n",n);
        printf("%s\n",buff);

		char* filename=GetMessage(buff);
		if(filename==NULL)
		{
			send(c,"404",3,0);
			break;
		}
		printf("filename:%s\n",filename);

		int filesize=0;
		int fd=OpenFile(filename,&filesize);
		if(fd==-1)
		{
			int fd404=open("/home/ljx/liujingxuan/test35/my404.html",O_RDONLY);
			if(fd404==-1)
			{
				printf("my404打开失败！\n");
				close(c);
				continue;
			}
			int len_404=lseek(fd404,0,SEEK_END);
			lseek(fd404,0,SEEK_SET);
			
			//发送404报头
			char buff_404[512]={0};
			strcpy(buff_404,"HTTP/1.1 404\r\n");
			strcat(buff_404,"Server:myweb\r\n");
			sprintf(buff_404+strlen(buff_404),"Content-Length:%d\r\n",len_404);
			strcat(buff_404,"\r\n");
		
			printf("send:\n%s\n",buff_404);
			send(c,buff_404,strlen(buff_404),0);
			char data[512]={0};
			int num=0;
			while((num=read(fd404,data,512))>0)
			{
				send(c,data,num,0);
			}
			close(fd404);
			break;
		}

		
		char sendbuff[512]={0};
		strcpy(sendbuff,"HTTP/1.1 200 OK\r\n");
		strcat(sendbuff,"Server:myweb\r\n");
		sprintf(sendbuff+strlen(sendbuff),"Content-Length:%d\r\n",filesize);
		strcat(sendbuff,"\r\n");
		
		printf("send:\n%s\n",sendbuff);
		send(c,sendbuff,strlen(sendbuff),0);
		char data[512]={0};
		int num=0;
		while((num=read(fd,data,512))>0)
		{
			send(c,data,num,0);
		}
		close(fd);
	}
	close(c);
}


int main()
{
	int sockfd=CreatSocket();
	assert(sockfd!=-1);
	struct sockaddr_in caddr;
	int len =-1;
	int c=-1;
	while(1)
	{
		len=sizeof(caddr);
		c=accept(sockfd,(struct sockaddr*)&caddr,&len);
		if(c<0)
		{
			continue;		
		}
		
		pthread_t id;
		pthread_create(&id,NULL,work_pthread,(void*)&c);
	}
	close(sockfd);
	exit(0);
}
