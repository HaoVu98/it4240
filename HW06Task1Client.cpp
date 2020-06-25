// HW06Task1Client.cpp : Defines the entry point for the console application.
//


#include "stdafx.h"

#include <stdio.h>
#include <conio.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#define BUFF_SIZE 128

#define USERNAME_MSG 15
#define PASSWORD_MSG 25
#define LOGOUT_MSG 35

#define USERNAME_NOTFOUND -18
#define USERNAME_CORRECT 18

#define PASSWORD_INCORRECT -28
#define PASSWORD_CORRECT 28

#define LOGOUT_ERROR -38
#define LOGOUT_SUCCESS 38

#define ACC_BLOCKED -45
#define ACC_LOGINED 4

#define ACC_ACTIVE 40

#define ACTIVE '0' //Account is active
#define BLOCKED '1' //Account is blocked
#define ONLINE '2'//Account is online
#define USERNAME_SIZE 32
#define PASSWORD_SIZE 32
//Deffine state 
#define UNDEFINED -1
#define DEFINED 0
#define LOGINED 1

#define ACC_FILE "account.txt"
#define MAX_ACC_COUNT 100
#pragma comment (lib, "Ws2_32.lib")



//receive 
int recvExt(SOCKET connSock, char * buff, int buffSize) {
	int dataLen;
	int ret;
	//Receive 4 byte of data length
	ret = recv(connSock, (char*)&dataLen, sizeof(int), MSG_WAITALL);
	if (ret == SOCKET_ERROR) return SOCKET_ERROR;
	//Receive data
	if (dataLen <= buffSize) {
		ret = recv(connSock, buff, dataLen, MSG_WAITALL);
	}
	return ret;
}

int sendExt(SOCKET connSock, char *buff, int dataLen) {
	int pDataLen = dataLen;
	int ret;
	//Send data length
	ret = send(connSock, (char*)&pDataLen, sizeof(int), 0);
	if (ret != sizeof(int)) return SOCKET_ERROR;
	//Send data
	ret = send(connSock, buff, pDataLen, 0);
	return ret;
}

/*Scan username and check account
return opCode
*/
int enterUsername(SOCKET sock, char * buff, int buffSize) {
	int ret;
	char optCode = -1;
	while (optCode != USERNAME_CORRECT) {
		buff[0] = USERNAME_MSG;
		printf("Import user name: ");
		gets_s(&buff[1], buffSize - 1);
		ret = sendExt(sock, buff, strlen(buff));
		if (ret == SOCKET_ERROR) {
			printf("Error code: %d\n", WSAGetLastError());
			return SOCKET_ERROR;
		}
		ret = recvExt(sock, buff, buffSize);
		if (ret == SOCKET_ERROR) {
			printf("Error code: %d\n", WSAGetLastError());
			return SOCKET_ERROR;
		}
		optCode = buff[0];
		if (optCode == USERNAME_NOTFOUND) printf("Username not exist\n");
		if (optCode == ACC_BLOCKED) printf("Account is blocked. Import another username\n");
		if (optCode == ACC_LOGINED) printf("Account is online. Import another username\n");
	}
	return optCode;
}
/*Scan pass
return opCode
*/
int enterPassword(SOCKET sock, char * buff, int buffSize) {
	int ret;
	char optCode = -1;
	while (optCode != PASSWORD_CORRECT && optCode != ACC_BLOCKED && ACC_LOGINED) {
		buff[0] = PASSWORD_MSG;
		printf("Enter password: ");
		gets_s(&buff[1], buffSize - 1);
		ret = sendExt(sock, buff, strlen(buff));
		if (ret == SOCKET_ERROR) {
			printf("Error code: %d\n", WSAGetLastError());
			return SOCKET_ERROR;
		}
		ret = recvExt(sock, buff, buffSize);
		if (ret == SOCKET_ERROR) {
			printf("Error code: %d\n", WSAGetLastError());
			return SOCKET_ERROR;
		}
		optCode = buff[0];
	}
	return optCode;
}

int main()
{
	int SERVER_PORT;
	int loc = 0;	//location ServerPortNumber
	char SERVER_ADDR[100];
	char str[1000];
	printf_s("Client.exe ServerIPAddress ServerPortNumber\n");
	gets_s(str);
	//handle SERVER_ADDR
	for (int i = 11; str[i] != ' '; i++) {
		SERVER_ADDR[i - 11] = str[i];
		SERVER_ADDR[i - 10] = 0;
		loc = i + 1;
	}
	//handle SERVER_PORT
	char port[100];
	for (int i = loc; i < strlen(str); i++) {
		port[i - loc] = str[i];
		port[i - loc + 1] = 0;
	}
	SERVER_PORT = atoi(port);	//string to int
	//Step 1: Inittiate WinSock
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData))
		printf("Version is not supported\n");

	//Step 2: Construct socket	
	SOCKET client;
	client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	int tv = 10000; //Time out 10000ms
	setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (const char*)(&tv), sizeof(int));

	//Step 3: Specify server address
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	serverAddr.sin_addr.s_addr = inet_addr(SERVER_ADDR);

	//Step 4: Request to connect server
	if (connect(client, (sockaddr *)&serverAddr, sizeof(serverAddr))) {
		printf("Error! Cannot connect server. %d", WSAGetLastError());
		_getch();
		return 0;
	}
	printf("Connected server!\n");

	//Step 5: Communicate with server
	char buff[BUFF_SIZE];
	int ret, severAddrLen = sizeof(serverAddr);
	int disconnect = 0;
	int state = UNDEFINED;
	printf("Login\n");
	//Send message
	do {
		
		ret = enterUsername(client, buff, BUFF_SIZE);
		if (ret == SOCKET_ERROR) {
			printf("Error : %d\n", WSAGetLastError());
			disconnect = 1;
		}
		else {
			state = DEFINED;
			ret = enterPassword(client, buff, BUFF_SIZE);
			if (ret == SOCKET_ERROR) {
				printf("Error code: %d\n", WSAGetLastError());
				disconnect = 1;
			}
			else if (ret == ACC_BLOCKED) {
				state = UNDEFINED;
				printf("Account is blocked\n");
			}
			if (ret == ACC_LOGINED) {
				state = UNDEFINED;
			}
			else if (ret == PASSWORD_CORRECT) {
				state = LOGINED;
				printf("Logged in successfully\n");
			}
		}
	} while (state != LOGINED && disconnect == 0);

	if (disconnect == 0) {
		printf("Press 1 to exit\n");
		int esc = -1;
		scanf_s("%d", &esc);
			buff[0] = LOGOUT_MSG;
			ret = sendExt(client, buff, 1);
		
		if (ret == SOCKET_ERROR) {
			printf("Error code: %d\n", WSAGetLastError());
		}
		else {
			ret = recvExt(client, buff, BUFF_SIZE);
			if (ret == SOCKET_ERROR) {
				printf("Error code: %d\n", WSAGetLastError());
			}
			else {
				if (buff[0] == LOGOUT_SUCCESS)
					printf("Logout success\n");
				_getch();
			}
		}
	}
	//Disconnect to sever
	printf("Disconnect to sever\n");
	//Shutdow tcp socket
	shutdown(client, SD_SEND);

	//Step 6: Close socket
	closesocket(client);

	//Step 7: Terminate Winsock
	WSACleanup();

	_getch();
	return 0;
}