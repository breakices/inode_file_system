#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>

#define IP "10.0.8.17"
#define PORT 7000

void print_err(char *str, int line, int err_no) {
	printf("%d, %s :%s\n",line,str,strerror(err_no));
	_exit(-1);
}

int main()
{
	int skfd = -1, ret = -1;
	skfd = socket(AF_INET, SOCK_STREAM, 0);
	if ( -1 == skfd) {
		print_err("socket failed",__LINE__,errno);
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET; //设置tcp协议族
	addr.sin_port = htons(PORT); //设置端口号
	addr.sin_addr.s_addr = inet_addr(IP); //设置ip地址
	
	//主动发送连接请求
	ret = connect(skfd,(struct sockaddr*)&addr, sizeof(addr));
	if(-1 == ret) print_err("connect failed", __LINE__, errno);

	char buf[100] = {0};
	char rec[100] = {0};
	//客户端发送消息，并接受从服务端返回的消息
	while (1) {
		bzero(&rec, sizeof(recv));
                ret = recv(skfd, &rec, sizeof(rec), 0);
                if(-1 == ret) print_err("recv failed", __LINE__, errno);
                else if(ret > 0) printf("%s",rec);

		bzero(&buf, sizeof(buf));
		scanf("%s",buf);

		ret = send(skfd,&buf,sizeof(buf), 0);
		if (-1 == ret) {
			print_err("send failed", __LINE__, errno);
		}
	}
	
	return 0;
}

