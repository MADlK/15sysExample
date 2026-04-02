#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <iphlpapi.h>
#include <iostream>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

int main(int argc, char* argv[]) {

	WSAData wsaData;

	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (iResult != 0) {
		cout << "WSAStartup filed: " << iResult << endl;
		return 1;
	}


	struct addrinfo* result = NULL,
		* ptr = NULL,
		hints;
	ZeroMemory(&hints, sizeof(hints));

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

#define DEFAULT_PORT "27015"

	iResult = getaddrinfo(argv[1], DEFAULT_PORT, &hints, &result);

	if (iResult != 0) {
		cout << "getaddrinfo error: " << iResult << endl;
		WSACleanup();
		return 1;
	}

	SOCKET ConnectSocket = INVALID_SOCKET;

	ptr = result;
	
	ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

	if (ConnectSocket == INVALID_SOCKET) {
		cout << "Error at socket() : " << WSAGetLastError() << endl;
		WSACleanup();
		return 1;
	}

	iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);

	if (iResult == SOCKET_ERROR) {
		cout << "not connect" << WSAGetLastError() << endl;
		closesocket(ConnectSocket);
		ConnectSocket = INVALID_SOCKET;
		WSACleanup();
		return 1;
	}

	const int buflen = 512;

	const char* sendbuf = "this is test";
	
	char recbuf[buflen];

	iResult = 0;
	
	iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);

	if (iResult == SOCKET_ERROR) {
		cout << "error send: " << WSAGetLastError() << endl;
		closesocket(ConnectSocket);
		WSACleanup();
		return 1;
	}

	cout << "Bytes send: " << iResult << endl;

	
	// Новое: чтение с консоли через char[]
	const int buflen = 512;
	char sendbuf[buflen];
	char recbuf[buflen];

	cout << "Connected. Type message (empty line to exit):\n";
	while (cin.getline(sendbuf, buflen))
	{
		if (strlen(sendbuf) == 0) break; // Пустая строка → выход

		iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
		if (iResult == SOCKET_ERROR) {
			cout << "error send: " << WSAGetLastError() << endl;
			break;
		}
		cout << "Bytes send: " << iResult << endl;

		// Ждём ответ от сервера сразу после отправки
		iResult = recv(ConnectSocket, recbuf, buflen - 1, 0);
		if (iResult > 0) {
			recbuf[iResult] = '\0'; // Закрываем строку нулём
			cout << "Server: " << recbuf << endl;
		}
		else if (iResult == 0) {
			cout << "Connection closed by server" << endl;
			break;
		}
	}

	/*do {
		iResult = recv(ConnectSocket, recbuf, buflen, 0);
		if (iResult > 0) {
			cout << "Bytes recevied: " << iResult << endl;
		}
		else if (iResult == 0) {
			cout << "Connection close";
		}
		else {
			cout << "recv error: " << WSAGetLastError() << endl;
		}

	} while (iResult > 0);*/

	iResult = shutdown(ConnectSocket, SD_SEND);

	if (iResult == SOCKET_ERROR) {
		cout << "error shutdown: " << WSAGetLastError() << endl;
		closesocket(ConnectSocket);
		WSACleanup();
		return 1;
	}

	closesocket(ConnectSocket);
	WSACleanup();
	
	return 0;
}