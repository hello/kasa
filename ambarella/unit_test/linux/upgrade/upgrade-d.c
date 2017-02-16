#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVPORT 3339
#define BACKLOG 10
#define MAXDATASIZE 1024 *100

#define STARTK    0x12345678
#define STARTR    0x87654321


#define REBOOT    0x43218765
#define NANDFLAG  0x56981368


socklen_t sin_size;
char buf[MAXDATASIZE];

int main()
{
	int sockfd,client_fd;
	struct sockaddr_in my_addr;
	struct sockaddr_in remote_addr;
	int fd=0,recvbytes;
	int start;

	//system("mkdir /ambarella");
	//system("flash_eraseall  /dev/mtd12");
	system("mount -t jffs2 /dev/mtdblock12 /mnt");


	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket error"); exit(1);
	}
	my_addr.sin_family=AF_INET;
	my_addr.sin_port=htons(SERVPORT);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	bzero(&(my_addr.sin_zero),8);
	if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) {
		perror("bind error ");
		exit(1);
	}
	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen error ");
		exit(1);
	}
again:

	sin_size = sizeof(struct sockaddr_in);
	if ((client_fd = accept(sockfd, (struct sockaddr *)&remote_addr, &sin_size)) == -1) {
		perror("accept error");
		//continue;
	}

	printf("received a connection from %s\n", inet_ntoa(remote_addr.sin_addr));

	if ((recvbytes=recv(client_fd, buf, MAXDATASIZE, 0)) ==-1) {
		perror("recv error");
		exit(1);
	}
	printf("recvbytes:  %d \n",recvbytes);
	start=*(int *)buf;
	printf("start:  %x \n",start);
	if(start==STARTK)
	{
		fd=open("/mnt/kernel",O_CREAT|O_RDWR,S_IRWXU);
		if(fd<0){
		perror("open error ");
		exit(1);
		}

		while(1)
		{


			if ((recvbytes=recv(client_fd, buf, MAXDATASIZE, 0)) ==-1) {
				perror("recv error");
				exit(1);
			}
			printf("recvbytes:  %d \n",recvbytes);
			///if(recvbytes<MAXDATASIZE && recvbytes>0)
			//if(recvbytes>0)
			//{
			//	write(fd,buf,recvbytes);
			//	break;
			//}
			if(recvbytes==0)
			{
				break;
			}

			write(fd,buf,recvbytes);
		}

		close(fd);

		printf("close fd\n");

	}
	else if(start==STARTR)
	{
		fd=open("/mnt/rootfs",O_CREAT|O_RDWR,S_IRWXU);
		if(fd<0){
		perror("open error ");
		exit(1);
		}

		while(1)
		{


			if ((recvbytes=recv(client_fd, buf, MAXDATASIZE, 0)) ==-1) {
				perror("recv error");
				exit(1);
			}
			printf("recvbytes:  %d \n",recvbytes);
			//if(recvbytes<MAXDATASIZE && recvbytes>0)
			//if(recvbytes>0)
			//{
			//	write(fd,buf,recvbytes);
			//	break;
			//}
			if(recvbytes==0)
			{
				break;
			}

			write(fd,buf,recvbytes);
		}

		close(fd);

		printf("close fd\n");

	}
	else if(start==REBOOT)//recvbytes==4 &&
	{
		close(fd);
		//printf("close fd\n");
		printf("reboot\n");
		system("reboot");
	}
	else if(start==NANDFLAG)//recvbytes==4 &&
	{
		printf("nandwrite -S \n");
		system("nandwrite -S 1");

	}

	close(client_fd);
goto again;

}

