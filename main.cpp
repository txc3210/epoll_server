#include <stdio.h>
#include <string.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <map>

const char ip[] = "192.168.1.116";
const unsigned short port = 2000;

#define MAX_EVENT_NUMBER 	100
#define BUF_SIZE				1024 * 2

#//define SOCK_BUF_SIZE			1024 * 2
std::map<int, char *> sock_map;

int set_nonblocking(int fd)
{
	int old_option = fcntl(fd, F_GETFL);
	int new_option = old_option | O_NONBLOCK;
	fcntl(fd, F_SETFL, new_option);
	return old_option;
}

void addfd(int epollfd, int fd, bool enable_et)
{
	epoll_event event;
	event.data.fd = fd;
	event.events = EPOLLIN;
	if(enable_et)
		event.events |= EPOLLET;
	epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
	set_nonblocking(fd);
}

void et(epoll_event* events, int number, int epollfd, int listenfd)
{
	//char buf[BUF_SIZE];
	char * buf = nullptr;
	for(int i = 0; i < number; ++i)
	{
		int sockfd = events[i].data.fd;
		if(sockfd == listenfd)
		{
			std::cout << "new client connect...." << std::endl;
			struct sockaddr_in client_address;
			socklen_t client_addr_len = sizeof(client_address);
			int connfd = accept(listenfd, (struct sockaddr*)&client_address, &client_addr_len);
			if(sock_map.count(connfd) == 0)
			{
				std::cout << "sockfd is not int the map" << std::endl;
				buf = new char[BUF_SIZE];
				sock_map[connfd] = buf;
			}else
			{
				std::cout << "sockfd is already int the map" << std::endl;
			}			
			
			addfd(epollfd, connfd, true);
		}else if(events[i].events & EPOLLIN)
		{
			buf = sock_map[sockfd];
			std::cout << "event trigger once" << std::endl;
			int total_num = strlen(buf);
			while(true)
			{
				memset(buf, 0, BUF_SIZE);
				int num = recv(sockfd, buf, BUF_SIZE - 1, 0);
				if(num < 0)
				{
					if((errno == EAGAIN) || (errno == EWOULDBLOCK))
					{
						std::cout << "read later" << std::endl;
						printf("111 read %d bytes: %s\n", num, buf);
						break;
					}
					close(sockfd);
					break;
				}else if(num == 0)
				{
					close(sockfd);
				}else
				{
					printf("read %d bytes: %s\n", num, buf);
				}
			}
		}else
		{
			std::cout << "someting else happend" << std::endl;
		}

	}
}

int main(int argc, char * argv[])
{
	std::cout << "epoll server start....." <<  std::endl;
	struct sockaddr_in address;
	bzero(&address, sizeof(address));
	address.sin_family = AF_INET;
	//inet_pton(AF_INET, ip, &address.sin_addr);
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);
	
	int listenfd = socket(PF_INET, SOCK_STREAM, 0);
	assert(listenfd >= 0);
	
	int ret = bind(listenfd, (struct sockaddr *)&address, sizeof(address));
	printf("11111\n");
	assert(ret != -1);
	
	ret = listen(listenfd, 5);
	printf("11111\n");
	assert(ret != -1);
	
	epoll_event events[MAX_EVENT_NUMBER];
	int epollfd = epoll_create(5);
	assert(epollfd != -1);
	addfd(epollfd, listenfd, true);
	
	while(true)
	{
		int ret = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
		if(ret < 0)
		{
			std::cout << "epoll wait failed" << std::endl;
			break;
		}
		et(events, ret, epollfd, listenfd);
	}
	return 0;
}


