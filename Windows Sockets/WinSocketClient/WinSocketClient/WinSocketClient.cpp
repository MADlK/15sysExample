#define WIN32_LEAN_AND_MEAN 
#include <Windows.h>        
#include <winsock2.h>       
#include <WS2tcpip.h>       
#include <iphlpapi.h>       
#include <iostream>         
#include <string>         

#pragma comment(lib, "Ws2_32.lib")

using namespace std;


bool online = true;      
SOCKET ClientSocekt = INVALID_SOCKET; 




DWORD WINAPI RecvThread(LPVOID) 
{
    setlocale(0, "ru");
    const int buflen = 512; 
    char recbuf[buflen];    
    int iResult;            

    while (online)
    {
        iResult = recv(ClientSocekt, recbuf, buflen - 1, 0); 
        if (iResult > 0)
        {
            recbuf[iResult] = '\0';
            cout << recbuf; 
            
        }
        else if (iResult == 0) 
        { 
            cout << "\n[SERVER] сервер закрыл соединение" << endl;
            break;
        }
        else 
        { 
            if (online)
            {
                cout << "\n[ERROR] recv: " << WSAGetLastError() << endl; 
            }
            break;
        }
    }
    return 0;
}

int main(int argc, char* argv[]) {
    setlocale(0, "ru"); 

 

    WSAData wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData); 
    if (iResult != 0)
    {
        cout << "WSAStartup: " << iResult << endl;
        return 1;
    }

    struct addrinfo* result = NULL, * ptr = NULL, hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;       
    hints.ai_socktype = SOCK_STREAM;   
    hints.ai_protocol = IPPROTO_TCP;

#define DEFAULT_PORT "27015"
    iResult = getaddrinfo(argv[1], DEFAULT_PORT, &hints, &result);
    if (iResult != 0)
    {
        cout << "getaddrinfo error: " << iResult << endl;
        WSACleanup(); 
        return 1;
    }

    SOCKET ConnectSocket = INVALID_SOCKET;
    ptr = result;
    ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol); 
    if (ConnectSocket == INVALID_SOCKET) 
    { 
        cout << "socket(): " << WSAGetLastError() << endl;
        freeaddrinfo(result);
        WSACleanup();
        return 1; }

    iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen); 
    if (iResult == SOCKET_ERROR) 
    {
        cout << "нет соединения: " << WSAGetLastError() << endl;
        closesocket(ConnectSocket); 
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }
    freeaddrinfo(result); 

    ClientSocekt = ConnectSocket;
    online = true; 

    const int buflen = 512;
    char sendbuf[buflen] = "";
    char namebuf[64] = "";

    cout << "Введите имя ";
    cin.getline(namebuf, sizeof(namebuf)); 
    

    iResult = send(ConnectSocket, namebuf, strlen(namebuf), 0); 
    if (iResult == SOCKET_ERROR) 
    {
        cout << "Ошибка: " << WSAGetLastError() << endl;
        closesocket(ConnectSocket);
        WSACleanup();
        return 1; 
    }

    char ack[4] = "";
    iResult = recv(ConnectSocket, ack, sizeof(ack) - 1, 0); 
    if (iResult > 0)
    {
        ack[iResult] = '\0'; 
        if (string(ack) == "OK")
        {
            cout << "Подключег\n"; 
        }

    }

    HANDLE hRecv = CreateThread(NULL, 0, RecvThread, NULL, 0, NULL);

    while (cin.getline(sendbuf, buflen))
    {
        int len = strlen(sendbuf);
        if (len == 0)
            continue;

        if (string(sendbuf) == "/exit" || string(sendbuf) == "/Exit")
        {
            cout << "рассоединение..." << endl;
            break; 
        } 
        iResult = send(ConnectSocket, sendbuf, len, 0); 
        if (iResult == SOCKET_ERROR)
        { 
            cout << "\n[ERROR] send: " << WSAGetLastError() << endl;
            break;
        }
    }

    online = false; 
    shutdown(ConnectSocket, SD_SEND); 
    if (hRecv) 
    {
        WaitForSingleObject(hRecv, 1000); 
        CloseHandle(hRecv);
    }

    closesocket(ConnectSocket);
    WSACleanup();
    return 0;
}