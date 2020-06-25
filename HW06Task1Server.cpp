// HW06Task1Server.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <stdio.h>
#include <conio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>
#define SERVER_ADDR "127.0.0.1"
#define ACTIVE '0' //Account is active
#define BLOCKED '1' //Account is blocked
#define ONLINE '2'
#define USERNAME_SIZE 32
#define PASSWORD_SIZE 32

#define BUFF_SIZE 128

#define USERNAME_MSG 15
#define PASSWORD_MSG 25
#define LOGOUT_MSG 35
#define ACCESS_DENIED -60
#define USERNAME_NOTFOUND -18
#define USERNAME_CORRECT 18

#define PASSWORD_INCORRECT -28
#define PASSWORD_CORRECT 28

#define LOGOUT_ERROR -38
#define LOGOUT_SUCCESS 38

#define ACC_BLOCKED -45
#define ACC_LOGINED 4

//Deffine state 
#define UNDEFINED -1
#define DEFINED 0
#define LOGINED 1

#define ACC_FILE "account.txt"
#define MAX_ACC_COUNT 100
#define MAX_PORT 65535

#pragma comment (lib, "Ws2_32.lib")

//Define section struct
typedef struct Section {
	char countIncoPass;
	int status;
	int indexInListAcc;
} Section, *pSection;
// Account struct
typedef struct Sccount {
	char username[USERNAME_SIZE];
	char password[PASSWORD_SIZE];
	char status;
} Acc;
SOCKET client[FD_SETSIZE];
Section listSect[FD_SETSIZE];
fd_set readfds, initfds;
SOCKET connSock;
Acc * listAcc;
int listLen;


//receive 
int recvExt(SOCKET connSock, char * buff, int buffSize) {
	int dataLen;
	int ret;
	ret = recv(connSock, (char*)&dataLen, sizeof(int), MSG_WAITALL);
	if (ret == SOCKET_ERROR) return SOCKET_ERROR;
	if (dataLen <= buffSize) {
		ret = recv(connSock, buff, dataLen, MSG_WAITALL);
	}
	return ret;
}
//Function sent
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

// split struct account
int splitAcc(char *str, Acc* pAcc) {
	int i = 0;
	int j = 0;
	//Split user name
	while (str[i] != ' ') {
		pAcc->username[j] = str[i];
		i++;
		j++;
	}
	pAcc->username[j] = '\0';
	j = 0;
	i++;
	//Split password
	while (str[i] != ' ') {
		pAcc->password[j] = str[i];
		i++;
		j++;
	}
	pAcc->password[j] = '\0';
	i++;
	pAcc->status = str[i];
	return 0;
}

// Read file, save list account
int readFileToList(Acc **list, const char * fileName) {
	FILE *file;
	fopen_s(&file, fileName, "r");
	if (file == NULL) return -1;
	*list = (Acc*)malloc(MAX_ACC_COUNT * sizeof(Acc));
	int i = 0;
	char buff[66];
	while (!feof(file)) {
		fgets(buff, 66, file);
		splitAcc(buff, *list + i);
		i++;
	}
	fclose(file);
	return i;
}
// update account to file

int updateFile(Acc *list, int listLen, char *fileName) {
	FILE *file;
	fopen_s(&file, ACC_FILE, "w");
	// if file == NULL
	char buff[100];
	buff[0] = '\0';
	int i, j;
	for (i = 0; i < listLen - 1; i++) {
		strcat_s(buff, list[i].username);
		strcat_s(buff, " ");
		strcat_s(buff, list[i].password);
		j = strlen(buff);
		buff[j] = ' ';
		buff[j + 1] = list[i].status;
		buff[j + 2] = '\n';
		buff[j + 3] = '\0';
		fputs(buff, file);
		buff[0] = '\0';
	}
	buff[0] = '\0';
	strcat_s(buff, list[i].username);
	strcat_s(buff, " ");
	strcat_s(buff, list[i].password);
	j = strlen(buff);
	buff[j] = ' ';
	buff[j + 1] = list[i].status;
	buff[j + 2] = '\0';

	fputs(buff, file);
	fclose(file);
	return 0;
}
//Find index of account with username

int getIndexByUse(Acc *list, int listLen, char *username) {
	int i;
	int index = -1;
	for (i = 0; i < listLen; i++) {
		if (strcmp(username, list[i].username) == 0) {
			index = i;
			break;
		}
	}
	if (index == -1) return USERNAME_NOTFOUND;
	if (list[index].status == BLOCKED) return ACC_BLOCKED;
	if (list[index].status == ONLINE) return ACC_LOGINED;
	return index;
}

int processClient(int index, char *buff, int buffSize) {
	listLen = readFileToList(&listAcc, ACC_FILE);
	//Get message from client
	int ret;
	ret = recvExt(client[index], buff, buffSize);
	if (ret <= 0) return SOCKET_ERROR;
	buff[ret] = '\0';
	//update file infor
	//
	char code;
	code = buff[0];
	int state = listSect[index].status;
	int count=0;
	buff[ret] = '\0';

	if (code == USERNAME_MSG && state == UNDEFINED) {
		int indexOfAcc = getIndexByUse(listAcc,listLen,&buff[1]);
		if (indexOfAcc == ACC_LOGINED) {
			code = ACC_LOGINED;
			ret = sendExt(client[index], &code, 1);
		}
		if (indexOfAcc == ACC_BLOCKED) {
			code = ACC_BLOCKED;
			ret = sendExt(client[index], &code, 1);
		}
		else if (indexOfAcc == USERNAME_NOTFOUND) {
			code = USERNAME_NOTFOUND;
			ret = sendExt(client[index], &code, 1);
		}
		else {
			buff[0] = USERNAME_CORRECT;
			listSect[index].status = DEFINED;
			listSect[index].indexInListAcc = indexOfAcc;
			listSect[index].countIncoPass = 0;
			ret = sendExt(client[index], buff, 1);
		}
		
		
	}
	else if (code == PASSWORD_MSG && state == DEFINED) {
		int indexInListAcc = listSect[index].indexInListAcc;
		char *pass = &buff[1];
		char *correctPass = listAcc[listSect[index].indexInListAcc].password;
		if (strcmp(pass, correctPass) == 0) {
			buff[0] = PASSWORD_CORRECT;
			listSect[index].countIncoPass = 0;
			int indexInListAcc = listSect[index].indexInListAcc;
			listAcc[indexInListAcc].status = ONLINE;
			updateFile(listAcc, listLen, ACC_FILE);
		}
		else {
			//Password incorect
			if (listSect[index].countIncoPass == 3) {
				//Update status of account
				int indexInListAcc = listSect[index].indexInListAcc;
				listAcc[indexInListAcc].status = BLOCKED;
				updateFile(listAcc, listLen, ACC_FILE);

				//Update status of section
				listSect[index].status = UNDEFINED;
				buff[0] = ACC_BLOCKED;
			}
			else if (listAcc[indexInListAcc].status == BLOCKED) {
				buff[0] = ACC_BLOCKED;
			}
			else {
				listSect[index].countIncoPass++;
				buff[0] = PASSWORD_INCORRECT;
			}
		}
		ret = sendExt(client[index], buff, 1);
	}
	else if (code == LOGOUT_MSG ) {
		listSect[index].status = UNDEFINED;
		buff[0] = LOGOUT_SUCCESS;
		
		ret = sendExt(client[index], buff, 1);
	}

	return ret;
	//
}
	
int main()
{
	//Handle SERVER_PORT
	int SERVER_PORT;
	printf_s("Server.exe PortNumber\n");
	char str[1000];
	gets_s(str);
	char port[100];
	for (int i = 11; i < strlen(str); i++) {
		port[i - 11] = str[i];
		port[i - 10] = 0;
	}
	SERVER_PORT = atoi(port);//string to int
	
	//Read data

	listLen = readFileToList(&listAcc, ACC_FILE);
	if (listLen <1) {
		if (listLen == -1) printf("Error read data\n");
		else printf("No data in file\n");
		system("pause");
		return 1;
	}
	//step 1: init winsocl
	WSADATA wsaDATA;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaDATA))
		printf_s("version not sp");
	//step 2: contruct SOCKET
	SOCKET listenSock;
	listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	//step 3: blind() address socket
	sockaddr_in severAddr;
	severAddr.sin_family = AF_INET;
	severAddr.sin_port = htons(SERVER_PORT);
	severAddr.sin_addr.s_addr = inet_addr(SERVER_ADDR);

	if (bind(listenSock, (sockaddr *)&severAddr, sizeof(severAddr))) {
		printf_s("errors! can not blind addr");
		_getch();
		return 0;
	}
	//step 4: listen request from client
	if (listen(listenSock, 10)) {//kich thuoc hang doi la 10
		printf_s("Error! can not bind() %d", WSAGetLastError());
		_getch();
		return 0;
	}
	//Sever started
	printf("Sever started at: [%s:%d]\n",
		inet_ntoa(severAddr.sin_addr),
		ntohs(severAddr.sin_port));


	//step 5: Communicate with client

	sockaddr_in clientAddr;
	char buff[BUFF_SIZE];
	int nEvents,ret, clientAddrLen = sizeof(clientAddr);

	for (int i = 0; i < FD_SETSIZE; i++) {
		client[i] = 0;	// 0 indicates available entry
		listSect[i].status = UNDEFINED;
		listSect[i].countIncoPass = 0;
	}

	FD_ZERO(&initfds);
	FD_SET(listenSock, &initfds);

	while (1)
	{
		readfds = initfds;
		nEvents = select(0, &readfds, 0, 0, 0);
		if (nEvents < 0) {
			printf("\nError! Cannot poll sockets: %d", WSAGetLastError());
			break;
		}
		//new client connection
		if (FD_ISSET(listenSock, &readfds)) {
			clientAddrLen = sizeof(clientAddr);
			if ((connSock = accept(listenSock, (sockaddr *)&clientAddr, &clientAddrLen)) < 0) {
				printf("\nError! Cannot accept new connection: %d", WSAGetLastError());
				break;
			}
			else {
				printf("Connection from [%s:%u]\n",
					inet_ntoa(clientAddr.sin_addr),
					ntohs(clientAddr.sin_port));

				int i;
				for (i = 0; i < FD_SETSIZE; i++)
					if (client[i] == 0) {
						client[i] = connSock;
						FD_SET(client[i], &initfds);
						break;
					}

				if (i == FD_SETSIZE) {
					printf("\nToo many clients.");
					closesocket(connSock);
				}

				if (--nEvents == 0)
					continue; //no more event
			}
		}
		//Receive message from clients
		for (int i = 0; i < FD_SETSIZE; i++) {
			if (client[i] <= 0)
				continue;

			if (FD_ISSET(client[i], &readfds)) {
				//
				ret = processClient(i, buff, BUFF_SIZE);
				//
				if (ret <= 0) {
					printf("Error: %d\n", WSAGetLastError());
					FD_CLR(client[i], &initfds);
					listSect[i].status = UNDEFINED;
					listSect[i].countIncoPass = 0;
					int indexInListAcc = listSect[i].indexInListAcc;
					if (listAcc[indexInListAcc].status != BLOCKED) {
						listAcc[indexInListAcc].status = ACTIVE;
						updateFile(listAcc, listLen, ACC_FILE);
					}
					closesocket(client[i]);
					client[i] = 0;
				}
			}
			if (--nEvents <= 0)
				continue; //no more event
		}
	}//
	 //Step 6: Close socket
	closesocket(listenSock);
	//Step 7: Terminate Winsock
	WSACleanup();
	return 0;
}
