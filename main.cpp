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
#include "log.h"
#include <algorithm>
#include <ctime>
#include <thread>

const char ip[] = "192.168.1.116";
const unsigned short port = 2000;

#define MAX_EVENT_NUMBER 	100
#define BUF_SIZE				1024 * 2

#//define SOCK_BUF_SIZE			1024 * 2
std::map<int, char *> sock_map;



bool GetHeaderInfo(std::string &strAll,const char *strHeader,std::string & strInfo)
{
	std::size_t indexL = strAll.find(strHeader);
	if(indexL == std::string::npos)
		return false;
	indexL = strAll.find(':',indexL);
	if(indexL == std::string::npos)
		return false;
	std::size_t indexR = strAll.find("\r\n",indexL);
	if(indexR == std::string::npos)
		return false;
	strInfo = strAll.substr(indexL+1,indexR-indexL-1);
	strInfo.erase(0,strInfo.find_first_not_of(' '));//Trim Left
	strInfo.erase(strInfo.find_last_not_of(' ')+1);//Trim Right
	return true;
}

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


int process_msg(int sockfd, char *buf, int & total_num)
{
	if(total_num < 3)
		return 0;
	if(strncmp(buf, "GET", 3) == 0)
	{
		std::cout << "Http GET mothod" << std::endl;
		total_num = 0;
		memset(buf, 0, BUF_SIZE);
		return 0;
	}
	if(total_num < 4)
		return 0;
	if(strncmp(buf, "POST", 4) != 0)
	{
		std::cout << "This is not a GET or POST mothed" << std::endl;
		//total_num = 0;
		//memset(buf, 0, BUF_SIZE);
		close(sockfd);
		return 0;
	}
	char *pHeaderEnd = strstr(buf,"\r\n\r\n");
	if(pHeaderEnd == nullptr)
	{
		std::cout << "Http header dose not end" << std::endl;
		return 0;
	}

	std::string header;
	header.append(buf,(int)(pHeaderEnd-buf+4));	
	
	std::string strContentLength;
	bool bRet = GetHeaderInfo(header,"Content-Length",strContentLength);
	if(!bRet)
	{
		LogI("there is no Content-Length in header,exit thread\n");
		return 0;
	}
	int ContentLength = atoi(strContentLength.c_str());
	//printf("Content-Length:%s:%ld\n",strContentLength.c_str(),ContentLength);
	if(total_num - (int)(pHeaderEnd-buf+4) < ContentLength)
	{
		LogI("recv data is not enough,continue to recv\n");
		return 0;
	}
	//printf("http recv completed\n");


	std::transform(header.begin(), header.end(), header.begin(),::tolower);// to lower case
	std::string strConnection;
	bRet = GetHeaderInfo(header,"connection",strConnection);
	if(!bRet)
	{
		LogI("there is no connection in header,exit thread\n");
		return 0;
	}
	//printf("connection:%s\n",strConnection.c_str());
	time_t tt;
	struct tm stm;

	time(&tt);		
	//localtime_s(&tt, &stm);
	if(ContentLength>0)
	{
		const char *pBody = static_cast<const char*>( pHeaderEnd + 4 );
		
		LogI("ThreadID=0x%X,HttpRecv:%s\r\n",std::this_thread::get_id(), pBody);
				
		
		memset(buf, 0, BUF_SIZE);
		total_num = 0;
		if(strConnection=="keep-alive" || strConnection=="keepalive")
			return 0;
		else if(strConnection=="close")
		{
			close(sockfd);
			return 0;
		}
		return 0;

	}	
	return 0;
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
				memset(buf, 0, BUF_SIZE);
				sock_map[connfd] = buf;
			}else
			{
				std::cout << "sockfd is already int the map" << std::endl;
				buf = sock_map[connfd];
				memset(buf, 0, BUF_SIZE);
			}			
			
			addfd(epollfd, connfd, true);
		}else if(events[i].events & EPOLLIN)
		{
			buf = sock_map[sockfd];			
			std::cout << "event trigger once" << std::endl;
			int total_num = strlen(buf);
			while(true)
			{
				
				int num = recv(sockfd, buf + total_num, BUF_SIZE - 1 - total_num, 0);
				if(num < 0)
				{
					if((errno == EAGAIN) || (errno == EWOULDBLOCK))
					{
						std::cout << "read data over" << std::endl;
						printf("read %d bytes: %s\n", total_num, buf);
						
						process_msg(sockfd, buf, total_num);					
						
					}
					//close(sockfd);
					break;
				}else if(num == 0)
				{
					std::cout << "socket has been closeed"  << std::endl;
					close(sockfd);
				}else
				{
					total_num += num;
					if(total_num >= BUF_SIZE - 1)
					{
						std::cout << "http recv too much data, it is not normal" << std::endl;
						total_num = 0;
						memset(buf, 0, BUF_SIZE);
						break;
					}					
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
	int on = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	
	int ret = bind(listenfd, (struct sockaddr *)&address, sizeof(address));
//	printf("11111\n");
	assert(ret != -1);
	
	ret = listen(listenfd, 5);
//	printf("11111\n");
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


