#define WIN32_LEAN_AND_MEAN // Отключаем лишние части Windows.h для ускорения компиляции
#include <Windows.h>        // Подключаем базовые функции Windows API
#include <winsock2.h>       // Подключаем библиотеку Winsock 2 для сетевого взаимодействия
#include <WS2tcpip.h>       // Подключаем функции для работы с IPv4/IPv6 (getaddrinfo)
#include <iphlpapi.h>       // Подключаем (оставлен как в оригинале, хотя не используется напрямую)
#include <iostream>         // Подключаем стандартный ввод/вывод для cout/cin
#include <string>

#pragma comment(lib, "Ws2_32.lib") // Линкуем библиотеку Winsock автоматически при компиляции

using namespace std; // Используем стандартное пространство имен для удобства

// Структура для хранения данных одного подключенного клиента
struct ClientData {
    SOCKET sock;       // Дескриптор сокета клиента (уникальный ID соединения)
    char name[64];     // Имя клиента (максимум 63 символа + нуль-терминатор)
    bool connected;    // Флаг активности: true = в сети, false = отключился
};

ClientData clients[32]; // Глобальный массив для хранения всех клиентов (лимит 32)
int client_count = 0;   // Счётчик подключённых клиентов (для отладки/ограничений)

//// Функция ручного вычисления длины строки (полная замена strlen)
//int str_len(const char* s) {
//    int len = 0;                      // Инициализируем счётчик длины
//    while (s[len] != '\0') len++;     // Считаем символы до конца строки
//    return len;                       // Возвращаем итоговую длину
//}

//// Функция ручного сравнения двух строк (полная замена strcmp)
//bool str_equal(const char* a, const char* b) {
//    int i = 0;                        // Индекс текущего символа
//    while (a[i] != '\0' && b[i] != '\0') { // Пока обе строки не закончились
//        if (a[i] != b[i]) return false;    // Если символы не совпали, строки разные
//        i++;                               // Переходим к следующему символу
//    }
//    return a[i] == '\0' && b[i] == '\0'; // Строки равны, если обе закончились одновременно
//}

// всем кроме отправителя 
void broadcast(const char* msg, SOCKET sender)
{
    for (int i = 0; i < 32; i++) 
    { 
        
        // в сети?                  не отправитель?              не инвалид   
        if (clients[i].connected && clients[i].sock != sender && clients[i].sock != INVALID_SOCKET) 
        {
            send(clients[i].sock, msg, strlen(msg), 0); 
        }
    }
}

//// Функция отправки приватного сообщения одному конкретному клиенту
//void send_private(SOCKET target, const char* msg)
//{
//    send(target, msg, strlen(msg), 0); // Отправляем только целевому сокету
//}

// Функция потока WinAPI, которая выполняется отдельно для каждого клиента
DWORD WINAPI Client(LPVOID param) 
{
    setlocale(0, "ru");
    ClientData* data = (ClientData*)param; // Преобразуем универсальный указатель в структуру клиента
    SOCKET ClientSocket = data->sock;      // Получаем сокет для удобства

    const int buflen = 512;                // Максимальный размер буфера приёма
    int iResult = 0;                       // Переменная для результата recv/send
    int sendResult;                        // Переменная для результата send
    char recvbuf[buflen] = "";             // Буфер для входящих сообщений (обнулён)

    char Name[64] = "";                    // Локальная копия имени клиента
    int j = 0;                             // Вспомогательный индекс
    while (j < 63 && data->name[j] != '\0') { // Копируем имя в локальный буфер
        Name[j] = data->name[j]; j++;
    }
    Name[j] = '\0';                        // Гарантируем нуль-терминатор

    send(ClientSocket, "OK", 2, 0);        // Отправляем подтверждение готовности чата

    do
    { // Цикл приёма сообщений, пока соединение живо
        iResult = recv(ClientSocket, recvbuf, buflen - 1, 0); // Ждём данные от клиента
        if (iResult > 0) {                  // Если принято >0 байт
            recvbuf[iResult] = '\0';        // Ставим конец строки

            if (recvbuf[0] == '/') {        // Если начинается с '/', это команда
                cout << "[SERVER CMD] [" << Name << "] подключился: " << recvbuf << endl; // Логируем

                if (string(recvbuf) == "/users" || string(recvbuf) == "/Users") 
                { // Если /users
                    char user_list[2048] = "\n=== В сети ===\n"; // Буфер списка
                    //int pos = strlen(user_list); // Текущая позиция в буфере

                    for (int i = 0; i < 32; i++)
                    { // Перебираем всех клиентов
                        if (clients[i].connected)
                        { // Если активен
                            //user_list[pos++] = '-';
                            //user_list[pos++] = ' '; // Маркер "- "
                            strcat_s(user_list, "- ");
                           
                            //int k = 0;
                            //while (clients[i].name[k] != '\0' && pos < 2047)
                            //{
                            //    user_list[pos++] = clients[i].name[k++];
                            //} // Копируем имя

                            strcat_s(user_list, clients[i].name );
                            strcat_s(user_list, "\n");

                            //user_list[pos++] = '\n'; // Перенос строки
                        }
                    }
                    //user_list[pos] = '\0'; // Закрываем строку
                    strcat_s(user_list, "\0");
                    send(ClientSocket, user_list, strlen(user_list), 0); // Шлём ТОЛЬКО запросившему
                    //send_private(ClientSocket, user_list);
                    continue; // Не рассылаем в общий чат
                }
                continue; // Неизвестная команда: игнорируем, ждём следующую
            }

            cout << "[SERVER] [" << Name << "]: " << recvbuf << endl; // Логируем обычное сообщение

            
            char full_msg[700] = "[";  //сообщение
            strcat_s(full_msg, Name);
            strcat_s(full_msg, "]: ");
            strcat_s(full_msg, recvbuf);
            strcat_s(full_msg, "\n");
            broadcast(full_msg, ClientSocket); // Рассылаем всем, кроме автора
        }
        else if (iResult == 0)
        { 
            cout << "[SERVER] [" << Name << "] отсоединился" << endl;
            break;
        }
        else 
        { // Ошибка сети или обрыв
            cout << "[SERVER] recv: " << WSAGetLastError() << endl;
            break;
        }
    } while (iResult > 0);

    for (int i = 0; i < 32; i++)
    { // Помечаем клиента как отключенного
        if (clients[i].sock == ClientSocket) 
        {
            clients[i].connected = false; 
            break;
        }
    }

    char leave_msg[128] = "[SERVER] "; // Формируем уведомление об уходе
    int pos = 9; int k = 0;
    while (Name[k] != '\0' && pos < 127) 
    {
        leave_msg[pos++] = Name[k++]; 
    }
    const char* suffix = " left the chat\n";
    k = 0; while (suffix[k] != '\0' && pos < 127)
    { 
        leave_msg[pos++] = suffix[k++];
    }
    leave_msg[pos] = '\0';
    broadcast(leave_msg, ClientSocket);

    closesocket(ClientSocket); // Закрываем сокет клиента
    delete data;               // Освобождаем память кучи
    return 0;                  // Завершаем поток
}










int main(int argc, char* argv[])
{
    setlocale(0, "ru"); // Включаем русскую локаль
    WSAData wsaData;    // Структура для Winsock
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData); // Инициализируем Winsock 2.2
    if (iResult != 0)
    {
        cout << "WSAStartup failed: " << iResult << endl;
        return 1; 
    }

#define DEFAULT_PORT "27015" // Порт для прослушивания
    struct addrinfo*
        result = NULL,
        * ptr = NULL,
        hints;
    ZeroMemory(&hints, sizeof(hints)); // Обнуляем структуру
    hints.ai_family = AF_INET;         // Только IPv4
    hints.ai_socktype = SOCK_STREAM;   // TCP
    hints.ai_protocol = IPPROTO_TCP;   // Протокол TCP
    hints.ai_flags = AI_PASSIVE;       // Для сервера (bind ко всем интерфейсам)

    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result); // Получаем бинарный адрес
    if (iResult != 0)
    {
        cout << "getaddrinfo error: " << iResult << endl; 
        WSACleanup();
        return 1;
    }

    SOCKET ListenSocket = INVALID_SOCKET;
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol); // Создаём сокет
    if (ListenSocket == INVALID_SOCKET) 
    {
        cout << "error at socket(): " << WSAGetLastError() << endl;
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen); // Привязываем к IP:Port
    if (iResult == SOCKET_ERROR)
    {
        cout << "bind failed: " << WSAGetLastError() << endl;
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup(); return 1; }

    freeaddrinfo(result); // Память адреса больше не нужна

    if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR)
    { // Переводим в режим прослушивания
        cout << "Listen: " << WSAGetLastError() << endl;
        closesocket(ListenSocket); 
        WSACleanup();
        return 1;
    }

    

    for (int i = 0; i < 32; i++)
    { // Инициализируем массив клиентов
        clients[i].connected = false;
        clients[i].sock = INVALID_SOCKET;
    }

    while (true) 
    { // Бесконечный цикл ожидания подключений
        SOCKET ClientSocket = accept(ListenSocket, NULL, NULL); // Блокируем, ждём клиента
        if (ClientSocket == INVALID_SOCKET)
        {
            cout << "accept failed: " << WSAGetLastError() << endl;
            continue; 
        }
        cout << "[SERVER] Подключился новый пользователь" << endl;

        char recvbuf[512] = "";
        iResult = recv(ClientSocket, recvbuf, 63, 0); // Читаем имя первым пакетом
        char temp_name[64] = "Unknown";
        if (iResult > 0)
        {
            recvbuf[iResult] = '\0';
            int len = (iResult < 63) ? iResult : 63;
            int j = 0; while (j < len) { temp_name[j] = recvbuf[j]; j++; }
            temp_name[len] = '\0';
        }

        int slot = -1;
        for (int i = 0; i < 32; i++) 
        { 
            if (!clients[i].connected)
            {
                slot = i; break; 
            }
        } // Ищем свободный слот
        
        if (slot == -1)
        {
            send(ClientSocket, "Server full\n", 12, 0);
            closesocket(ClientSocket); 
            continue; 
        }

        ClientData* newData = new ClientData; // Выделяем память в куче
        newData->sock = ClientSocket; newData->connected = true; // Заполняем
        int j = 0;
        while (j < 63 && temp_name[j] != '\0')
        {
            newData->name[j] = temp_name[j];
            j++;
        }
        newData->name[j] = '\0';

        clients[slot] = *newData;
        client_count++; // Добавляем в массив

        char join_msg[128] = "[SERVER] "; // Уведомление о входе
        int pos = 9; j = 0; while (temp_name[j] != '\0' && pos < 127)
        {
            join_msg[pos++] = temp_name[j++];
        }
        const char* suffix = " joined the chat\n";
        j = 0;
        while (suffix[j] != '\0' && pos < 127)
        {
            join_msg[pos++] = suffix[j++]; 
        }
        join_msg[pos] = '\0';
        broadcast(join_msg, ClientSocket);

        HANDLE hThread = CreateThread(NULL, 0, Client, newData, 0, NULL); // Создаём поток
        if (hThread) CloseHandle(hThread); // Закрываем дескриптор, поток работает дальше
    }

    closesocket(ListenSocket); 
    WSACleanup();
    return 0; // (Недостижимо из-за while(true))
}