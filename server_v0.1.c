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

/*子线程中先接收从客户端发来的消息，再发送一个消息给客户端*/
void *receive(void *pth_arg) {
	int ret = 0;
	long cfd = (long)pth_arg;
	char buf[100] = {0};
	sign_in((void*)cfd);
	while(1) {
		bzero(&buf, sizeof(buf));
		ret = recv(cfd, &buf, sizeof(buf),0);	
		if (-1 == ret) {
			print_err("recv failed",__LINE__,errno);
		}
		else if (ret > 0)
			printf("recv from client %s \n",buf);
		ret = send(cfd,"SIMS>", sizeof("SIMS>"), 0);
		if (-1 == ret) print_err("send failed", __LINE__, errno);
	}
}

void *sign_in(void *pth_arg)
{
    int ret = 0;
	long cfd = (long)pth_arg;
	char buf[100] = {0};
    ret = send(cfd, "Please input your account!\nSIMS>", sizeof("Please input your account!\nSIMS>"),0);
    if (-1 == ret) print_err("send failed", __LINE__, errno);
    bzero(&buf, sizeof(buf));
    ret = recv(cfd, &buf, sizeof(buf),0);
    if (-1 == ret) {
			print_err("recv failed",__LINE__,errno);
		}
    else if (ret > 0)
        printf("recv from client %s \n",buf);
	//检查账户，判断权限
	ret = send(cfd,"Please input your password!\nSIMS>", sizeof("Please input your password!\nSIMS>"), 0);
	if (-1 == ret) print_err("send failed", __LINE__, errno);
    char account[100] = {0};
    memcpy(account, buf, strlen(buf));
    bzero(&buf, sizeof(buf));
    ret = recv(cfd, &buf, sizeof(buf),0);
    if (-1 == ret) {
			print_err("recv failed",__LINE__,errno);
		}
    else if (ret > 0)
        printf("recv from client %s \n",buf);
	//检查密码
	ret = send(cfd,"SIMS>", sizeof("SIMS>"), 0);
	if (-1 == ret) print_err("send failed", __LINE__, errno);
    char password[100] = {0};
    memcpy(password, buf, strlen(buf));  
    //接受到了account和password，需要处理
    printf("account: %s\n", account);
    printf("password: %s\n", password);
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

	ret = bind(skfd, (struct sockaddr*)&addr, sizeof(addr));
	if ( -1 == ret) {
		print_err("bind failed",__LINE__,errno);
	}
 
	ret = listen(skfd, 3);
	if ( -1 == ret ) {
		print_err("listen failed", __LINE__, errno);
	}
	char buf[100] = {0};
	
	//使用accept阻塞形式得监听客户端的发来的连接，并返回通信描述符
	long cfd = -1;
	pthread_t id;
	while (1) {
		struct sockaddr_in caddr = {0};
		int csize = sizeof(caddr);
		cfd = accept(skfd, (struct sockaddr*)&caddr, &csize);
		if (-1 == cfd) {
			print_err("accept failed", __LINE__, errno);
		}
		//建立连接后打印一下客户端的ip和端口号
		printf("cport = %d, caddr = %s\n", ntohs(caddr.sin_port),inet_ntoa(caddr.sin_addr));
		//使用accept返回到描述符，创建子线程进行数据传输
		int ret = pthread_create(&id,NULL,receive,(void*)cfd);
		if(-1 == ret) print_err("accept failed", __LINE__, errno); 
	}
	return 0;
}

