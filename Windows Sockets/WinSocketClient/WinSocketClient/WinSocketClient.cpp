#define WIN32_LEAN_AND_MEAN // Отключаем лишние части Windows.h
#include <Windows.h>        // Базовые функции Windows API
#include <winsock2.h>       // Сетевые функции Winsock 2
#include <WS2tcpip.h>       // Функции для работы с адресами
#include <iphlpapi.h>       // (Оставлен как в оригинале)
#include <iostream>         // Ввод/вывод в консоль
#include <string>         

#pragma comment(lib, "Ws2_32.lib") // Линкуем библиотеку Winsock

using namespace std;


bool online = true;      // Глобальный флаг: управляет работой потока приёма
SOCKET ClientSocekt = INVALID_SOCKET; // Глобальный сокет: передаётся потоку для чтения



// Поток WinAPI для асинхронного приёма сообщений от сервера
DWORD WINAPI RecvThread(LPVOID) 
{
    setlocale(0, "ru");
    const int buflen = 512; // Размер буфера
    char recbuf[buflen];    // Буфер данных
    int iResult;            // Результат recv

    while (online)
    { // Пока клиент не закрылся
        iResult = recv(ClientSocekt, recbuf, buflen - 1, 0); // Блокируем поток, ждём данные
        if (iResult > 0)
        { // Если данные пришли
            recbuf[iResult] = '\0'; // Закрываем строку
            cout << recbuf; // Выводим с новой строки
            //cout.flush();           // Принудительно выводим в консоль
        }
        else if (iResult == 0) 
        { // Сервер закрыл соединение
            cout << "\n[SERVER] сервер закрыл соединение" << endl;
            break;
        }
        else 
        { // Ошибка сети
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
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData); // Инициализация Winsock
    if (iResult != 0)
    {
        cout << "WSAStartup: " << iResult << endl;
        return 1;
    }

    struct addrinfo* result = NULL, * ptr = NULL, hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;       // IPv4 или IPv6
    hints.ai_socktype = SOCK_STREAM;   // TCP
    hints.ai_protocol = IPPROTO_TCP;

#define DEFAULT_PORT "27015"
    iResult = getaddrinfo(argv[1], DEFAULT_PORT, &hints, &result); // Преобразуем IP
    if (iResult != 0)
    {
        cout << "getaddrinfo error: " << iResult << endl;
        WSACleanup(); 
        return 1;
    }

    SOCKET ConnectSocket = INVALID_SOCKET;
    ptr = result;
    ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol); // Создаём сокет
    if (ConnectSocket == INVALID_SOCKET) 
    { 
        cout << "socket(): " << WSAGetLastError() << endl;
        freeaddrinfo(result);
        WSACleanup();
        return 1; }

    iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen); // Подключаемся
    if (iResult == SOCKET_ERROR) 
    {
        cout << "нет соединения: " << WSAGetLastError() << endl;
        closesocket(ConnectSocket); freeaddrinfo(result);
        WSACleanup();
        return 1;
    }
    freeaddrinfo(result); // Освобождаем память

    ClientSocekt = ConnectSocket;
    online = true; // Передаём сокет потоку

    const int buflen = 512;
    char sendbuf[buflen] = "";
    char namebuf[64] = "";

    cout << "Введите имя ";
    cin.getline(namebuf, sizeof(namebuf)); // Читаем имя
    

    iResult = send(ConnectSocket, namebuf, strlen(namebuf), 0); // Отправляем имя
    if (iResult == SOCKET_ERROR) 
    {
        cout << "Ошибка: " << WSAGetLastError() << endl; closesocket(ConnectSocket);
        WSACleanup();
        return 1; 
    }

    char ack[4] = "";
    iResult = recv(ConnectSocket, ack, sizeof(ack) - 1, 0); // Ждём "OK" от сервера
    if (iResult > 0)
    {
        ack[iResult] = '\0'; 
        if (string(ack) == "OK")
        {
            cout << "Подключег\n"; 
        }

    }

    HANDLE hRecv = CreateThread(NULL, 0, RecvThread, NULL, 0, NULL); // Запускаем поток приёма

    while (cin.getline(sendbuf, buflen))
    { // Читаем ввод с клавиатуры
        int len = strlen(sendbuf);
        if (len == 0)
            continue;

        if (string(sendbuf) == "/exit" || string(sendbuf) == "/Exit")
        {
            cout << "рассоединение..." << endl;
            break; 
        } // Выход

        iResult = send(ConnectSocket, sendbuf, len, 0); // Отправляем на сервер
        if (iResult == SOCKET_ERROR)
        { cout << "\n[ERROR] send: " << WSAGetLastError() << endl;
        break;
        }
    }

    online = false; // Останавливаем поток
    shutdown(ConnectSocket, SD_SEND); // Корректно закрываем отправку
    if (hRecv) 
    {
        WaitForSingleObject(hRecv, 1000); 
        CloseHandle(hRecv);
    } // Ждём поток до 1 сек
    closesocket(ConnectSocket); WSACleanup(); return 0;
}