#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <iphlpapi.h>
#include <iostream>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

//переделать чтобы поток только получал сообщение
DWORD WINAPI Client(LPVOID CS)
{
	SOCKET* ClientSock = (SOCKET*) CS;
	SOCKET ClientSocket = *ClientSock;
	const int buflen = 512;
	int iResult = 0;
	int sendResult;
	char recvbuf[buflen] ="";
	char Name[64] ="new";
	const char* sendbuf = "send from server";

	iResult = recv(ClientSocket, recvbuf, buflen - 1, 0);
	
	if (iResult > 0) {
		recvbuf[iResult] = '\0'; 
		memcpy(Name, recvbuf, iResult);
		//Name[iResult] = '\0';
		
		cout << "[SERVER LOG] Client [" << Name  << "] connected" << endl;
	}
	else
	{
		closesocket(ClientSocket);
		return 0;

	}
	const char* repl = "МОЛОДЕЦ";
	sendResult = send(ClientSocket, repl, (int)strlen(repl), 0);
	
	//const int buflen = 512;
	//char recvbuf[buflen];
	//iResult = 0;
	do {
		iResult = recv(ClientSocket, recvbuf, buflen - 1, 0);
		if (iResult > 0) {
			recvbuf[iResult] = '\0'; 
			cout << "[SERVER LOG] Client wrote: " << recvbuf << endl;

			
			const char* reply = "\n";
			sendResult = send(ClientSocket, reply, (int)strlen(reply), 0);
			if (sendResult == SOCKET_ERROR) {
				cout << "send failed: " << WSAGetLastError() << endl;
				break;
			}
		}
		else if (iResult == 0)
			cout << "Отключился" << endl;
		else 
		{
			cout << "recv failed: " << WSAGetLastError() << endl;
			break;
		}
	} while (iResult > 0);

	closesocket(ClientSocket);

	return 0;
}







int main(int argc, char* argv[]) {

	setlocale(0, "ru");

	WSAData wsaData;

	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (iResult != 0) {
		cout << "WSAStartup filed: " << iResult << endl;
		return 1;
	}

#define DEFAULT_PORT "27015"

	struct addrinfo* result = NULL, * ptr = NULL, hints;
	ZeroMemory(&hints, sizeof(hints));

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);

	if (iResult != 0) {
		cout << "getaddrinfo error: " << iResult << endl;
		WSACleanup();
		return 1;
	}

	SOCKET ListenSocket = INVALID_SOCKET;

	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

	if (ListenSocket == INVALID_SOCKET) {
		cout << "error at socket(): " << WSAGetLastError() << endl;
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);

	if (iResult == SOCKET_ERROR) {
		cout << "bind filed: " << WSAGetLastError() << endl;
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}


	freeaddrinfo(result);

	if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
		cout << "Listen failed: " << WSAGetLastError() << endl;
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	cout << "[SERVER] Listen"<<endl;

	
	SOCKET ClientSocket;
	while (true)
	{
		ClientSocket = accept(ListenSocket, NULL, NULL);
		if (ClientSocket == INVALID_SOCKET) {
			cout << "accept filed: " << WSAGetLastError() << endl;

			closesocket(ListenSocket);
			WSACleanup();
			break;
		}
		cout<<"[SERVER] новый клиент" << endl;
		HANDLE hThread = CreateThread(NULL,0,Client,&ClientSocket,0,NULL);
	}

	

	
	cout << "1" << endl;









	/*do {
		iResult = recv(ClientSocket, recvbuf, buflen, 0);
		if (iResult > 0) {
			cout << "Receved bytes: " << iResult << endl;
			sendResult = send(ClientSocket, sendbuf, sizeof(sendbuf), 0);
			if (sendResult == SOCKET_ERROR) {
				cout << "send filed" << WSAGetLastError();
				closesocket(ClientSocket);
				WSACleanup();
				return 1;
			}
			cout << "Bytes send: " << sendResult << endl;
		}
		else if (iResult == 0)
			cout << "connection closed" << endl;
		else {
			cout << "recv filed: " << WSAGetLastError();
			closesocket(ClientSocket);
			WSACleanup();
			return 1;
		}
	} while (iResult > 0);*/

	iResult = shutdown(ClientSocket, SD_SEND);

	if (iResult == SOCKET_ERROR) {
		cout << "error shutdown: " << WSAGetLastError() << endl;
		closesocket(ClientSocket); 
		WSACleanup();
		return 1;
	}

	closesocket(ClientSocket);
	WSACleanup();

	return 0;
}