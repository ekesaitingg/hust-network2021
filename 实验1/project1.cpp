#pragma once
#include "winsock2.h"
#include <stdio.h>
#include <iostream>
#include <string>

#pragma comment(lib,"ws2_32.lib")
void send_file(char recvBuf[4096], SOCKET sessionSocket,char path[30]);

using namespace std;
int main(void)
{
	WSADATA wsaData;
	short port;
	char ip[16] = "0", path[30] = "0";
	printf("监听地址：");
	scanf("%s", ip);
	printf("监听端口号：");
	scanf("%hd", &port);
	printf("监听目录：");
	scanf("%s", path);

	int nRc = WSAStartup(0x0202, &wsaData);
	if (nRc) printf("Winsock启动发生错误!\n");
	if (wsaData.wVersion != 0x0202) printf("Winsock版本不正确!\n");

	printf("Winsock启动成功!\n");

	SOCKET srvSocket;  //socket函数的返回值
	sockaddr_in addr,clientAddr;
	SOCKET sessionSocket;
	int addrLen;
	srvSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (srvSocket != INVALID_SOCKET) printf("Socket创建成功\n");      //创建socket

	addr.sin_family = AF_INET;
	
	addr.sin_port = htons(port);
	addr.sin_addr.S_un.S_addr = inet_addr(ip);
	int rtn = bind(srvSocket, (LPSOCKADDR)&addr, sizeof(addr));       //绑定socket
	if (rtn != SOCKET_ERROR) printf("socket绑定成功\n");

	rtn=listen(srvSocket, SOMAXCONN);
	if (rtn != SOCKET_ERROR) printf("socket监听成功\n");
	clientAddr.sin_family = AF_INET;
	addrLen = sizeof(clientAddr);
	char recvBuf[4096];
	while (1)
	{
		memset(recvBuf, '\0', 4096);
		sessionSocket = accept(srvSocket, (LPSOCKADDR)&clientAddr, &addrLen);
		if (sessionSocket == INVALID_SOCKET) printf("连接失败\n");
		printf("收到连接请求，客户端为：%s:%u\n", inet_ntoa(clientAddr.sin_addr), htons(clientAddr.sin_port));
		rtn = recv(sessionSocket, recvBuf, 4096, 0);
		if (!rtn) printf("接收数据失败\n");
		printf("从客户端接收到%d字节数据：\n%s", strlen(recvBuf), recvBuf);
		send_file(recvBuf, sessionSocket, path);
		closesocket(sessionSocket);
	}
	closesocket(srvSocket);
	WSACleanup();
	return 0;
}

void send_file(char recvBuf[4096], SOCKET sessionSocket,char path[30])
{
	char filetype[5], filename[200], filepath[500], content_type[300]="Content-type:";
	memset(filetype, 0, sizeof(filetype));
	memset(filename, 0, sizeof(filename));
	memset(filepath, 0, sizeof(filepath));
	int i = 0, j = 0;
	while (recvBuf[i++] != '/');
	while (recvBuf[i] != '.') filename[j++] = recvBuf[i++];
	filename[j] = '.';
	j = 0;
	while (recvBuf[++i] != ' ') filetype[j++] = recvBuf[i];
	strcat(filename, filetype); 
	strcpy(filepath, path);
	strcat(filepath, filename);				//获取请求的文件名及其文件类型
	if (!strcmp(filetype, "html")) strcat(content_type, "text/html\r\n");
	if (!strcmp(filetype, "jpg")) strcat(content_type, "image/jpg\r\n");
	if (!strcmp(filetype, "png")) strcat(content_type, "image/png\r\n");

	char head[] = "HTTP/1.1 200 OK\r\n";
	char not_found[] = "HTTP/1.1 404 NOT FOUND\r\n";
	char forbidden[] = "HTTP.1.1 403 FORBIDDEN\r\n";
	
	if (!strcmp(filename, "3.jpg"))
	{
		send(sessionSocket, forbidden, strlen(forbidden), 0);
		send(sessionSocket, "Content-type:text/plain\r\n", strlen("Content-type:text/plain\r\n"), 0);
		send(sessionSocket, "\r\n", 2, 0);
		printf("已拒绝访问，返回403\n\n");
	}
	else {
		FILE* fp = fopen(filepath, "rb");
		if (!fp) {
			send(sessionSocket, not_found, strlen(not_found), 0);
			send(sessionSocket, "\r\n", 2, 0);
			printf("未找到请求的文件，返回404\n\n");
		}
		else
		{
			send(sessionSocket, head, strlen(head), 0);
			send(sessionSocket, "\r\n", 2, 0);
			fseek(fp, 0L, SEEK_END);
			int flen = ftell(fp);
			char* p = (char*)malloc(flen + 1);
			fseek(fp, 0L, SEEK_SET);
			fread(p, flen, 1, fp);
			send(sessionSocket, p, flen, 0);
			printf("已发送 %s 至客户机\n\n", filename);
			fp = NULL;
		}
	}
	return;
}