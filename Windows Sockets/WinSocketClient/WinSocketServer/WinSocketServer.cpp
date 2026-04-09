#define WIN32_LEAN_AND_MEAN
#include <Windows.h>      
#include <winsock2.h>     
#include <WS2tcpip.h>     
#include <iphlpapi.h>     
#include <iostream>       
#include <string>

#pragma comment(lib, "Ws2_32.lib")

using namespace std; 


struct ClientData {
    SOCKET sock;       
    char name[64];     
    bool connected;    
};

ClientData clients[256]; 
int client_count = 0;   



// всем кроме отправителя 
void AllSend(const char* msg, SOCKET sender)
{
    for (int i = 0; i < 256; i++) 
    { 
        
        // в сети?                  не отправитель?              не инвалид   
        if (clients[i].connected && clients[i].sock != sender && clients[i].sock != INVALID_SOCKET) 
        {
            send(clients[i].sock, msg, strlen(msg), 0); 
        }
    }
}





DWORD WINAPI Client(LPVOID param) 
{
    setlocale(0, "ru");
    ClientData* data = (ClientData*)param; 
    SOCKET ClientSocket = data->sock;      

    const int buflen = 512;                
    int iResult = 0;                       
    int sendResult;                        
    char recvbuf[buflen] = "";             

    char Name[64] = "";                    
    int j = 0;                             
    while (j < 63 && data->name[j] != '\0')
    { 
        Name[j] = data->name[j]; j++;
    }
    //Name[j] = '\0';                        

    send(ClientSocket, "OK", 2, 0);        

    do
    { 
        iResult = recv(ClientSocket, recvbuf, buflen - 1, 0); 
        if (iResult > 0)
        {                  
            recvbuf[iResult] = '\0';        

            if (recvbuf[0] == '/')
            {
                cout << "[SERVER CMD] [" << Name << "] подключился: " << recvbuf << endl; 

                if (string(recvbuf) == "/users" || string(recvbuf) == "/Users") 
                { 
                    char user_list[2048] = "\n=== В сети ===\n"; 
                    

                    for (int i = 0; i < 256; i++)
                    { 
                        if (clients[i].connected)
                        { 
                            
                            strcat_s(user_list, "- ");
                           
                            

                            strcat_s(user_list, clients[i].name );
                            strcat_s(user_list, "\n");

                   
                        }
                    }
                   
                    strcat_s(user_list, "\0");
                    send(ClientSocket, user_list, strlen(user_list), 0); 
                    continue; 
                }
                continue; 
            }

            cout << "[SERVER] [" << Name << "]: " << recvbuf << endl;

            
            char full_msg[700] = "[";  
            strcat_s(full_msg, Name);
            strcat_s(full_msg, "]: ");
            strcat_s(full_msg, recvbuf);
            strcat_s(full_msg, "\n");
            AllSend(full_msg, ClientSocket); 
        }
        else if (iResult == 0)
        { 
            cout << "[SERVER] [" << Name << "] отсоединился" << endl;
            break;
        }
        else 
        { 
            cout << "[SERVER] recv: " << WSAGetLastError() << endl;
            break;
        }
    } while (iResult > 0);

    for (int i = 0; i < 256; i++)
    {
        if (clients[i].sock == ClientSocket) 
        {
            clients[i].connected = false; 
            break;
        }
    }

    char leave_msg[128] = "[SERVER] "; 
    strcat_s(leave_msg, Name);
    strcat_s(leave_msg, " вышел\n");
    AllSend(leave_msg, ClientSocket);

    closesocket(ClientSocket); 
    delete data;              
    return 0;                 
}










int main(int argc, char* argv[])
{
    setlocale(0, "ru"); 
    WSAData wsaData;   
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0)
    {
        cout << "WSAStartup failed: " << iResult << endl;
        return 1; 
    }

#define DEFAULT_PORT "27015" 
    struct addrinfo*
        result = NULL,
        * ptr = NULL,
        hints;
    ZeroMemory(&hints, sizeof(hints)); 
    hints.ai_family = AF_INET;         
    hints.ai_socktype = SOCK_STREAM;   
    hints.ai_protocol = IPPROTO_TCP;   
    hints.ai_flags = AI_PASSIVE;       

    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0)
    {
        cout << "getaddrinfo error: " << iResult << endl; 
        WSACleanup();
        return 1;
    }

    SOCKET ListenSocket = INVALID_SOCKET;
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol); 
    if (ListenSocket == INVALID_SOCKET) 
    {
        cout << "error at socket(): " << WSAGetLastError() << endl;
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen); 
    if (iResult == SOCKET_ERROR)
    {
        cout << "bind failed: " << WSAGetLastError() << endl;
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup(); return 1; }

    freeaddrinfo(result);

    if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR)
    { 
        cout << "Listen: " << WSAGetLastError() << endl;
        closesocket(ListenSocket); 
        WSACleanup();
        return 1;
    }

    

    for (int i = 0; i < 256; i++)
    { 
        clients[i].connected = false;
        clients[i].sock = INVALID_SOCKET;
    }

    while (true) //цикл подключения клиента
    { 
        SOCKET ClientSocket = accept(ListenSocket, NULL, NULL); 
        if (ClientSocket == INVALID_SOCKET)
        {
            cout << "accept failed: " << WSAGetLastError() << endl;
            continue; 
        }
        cout << "[SERVER] Подключился новый пользователь" << endl;

        char recvbuf[512] = "";
        iResult = recv(ClientSocket, recvbuf, 63, 0); // первым имя
        char temp_name[64] = " ";
        if (iResult > 0)
        {
            strcat_s(temp_name, recvbuf);
        }

        int slot = -1;
        for (int i = 0; i < 256; i++) 
        { 
            if (!clients[i].connected)
            {
                slot = i;
                break; 
            }
        } // Ищем свободный слот для подключения клиента
        
        if (slot == -1)
        {
            send(ClientSocket, "сервер заполнен\n", 20, 0);
            closesocket(ClientSocket); 
            continue; 
        }

        ClientData* newData = new ClientData; 
        newData->sock = ClientSocket;
        newData->connected = true; 
        strcat_s(newData->name, temp_name);

        clients[slot] = *newData;
        client_count++; 

        char join_msg[128] = "[SERVER] "; // Уведомление о входе
        strcat_s(join_msg,temp_name );
        strcat_s(join_msg, "присоединился");

        AllSend(join_msg, ClientSocket);//отправка всем

        HANDLE hThread = CreateThread(NULL, 0, Client, newData, 0, NULL);
        if (hThread)
            CloseHandle(hThread);
    }

    closesocket(ListenSocket); 
    WSACleanup();
    return 0; 
}