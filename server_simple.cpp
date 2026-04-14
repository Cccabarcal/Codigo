/*
 * server_simple.cpp — Versión simplificada del servidor IoT
 * Sin deadlocks, sin problemas de accept()
 */

#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <map>
#include <vector>
#include <cstring>
#include <cstdio>
#include <csignal>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #define close closesocket
#endif

#include "protocol.h"

int server_fd = -1;

void handle_client(int client_fd, struct sockaddr_in client_addr) {
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, INET_ADDRSTRLEN);
    int port = ntohs(client_addr.sin_port);
    
    fprintf(stderr, "[CLIENT %d] Conectado desde %s:%d\n", client_fd, ip_str, port);
    fflush(stderr);
    
    char buf[1024];
    while (true) {
        memset(buf, 0, sizeof(buf));
        size_t n = recv(client_fd, buf, sizeof(buf) - 1, 0);
        
        if (n <= 0) {
            fprintf(stderr, "[CLIENT %d] Desconectado\n", client_fd);
            fflush(stderr);
            break;
        }
        
        std::string msg(buf, n);
        fprintf(stderr, "[CLIENT %d] Recibido: %s", client_fd, msg.c_str());
        fflush(stderr);
        
        // Responder OK a cualquier comando
        std::string response = "OK\n";
        send(client_fd, response.c_str(), response.length(), 0);
    }
    
    close(client_fd);
}

void handle_signal(int sig) {
    fprintf(stderr, "\n[SIGNAL %d] Cerrando servidor\n", sig);
    fflush(stderr);
    if (server_fd >= 0) close(server_fd);
    exit(0);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <puerto> <archivo_logs>\n", argv[0]);
        return 1;
    }

    int port = std::stoi(argv[1]);
    std::string log_file = argv[2];
    
    fprintf(stderr, "[MAIN] Iniciando servidor en puerto %d\n", port);
    fflush(stderr);

    #ifdef _WIN32
        WSADATA wsa_data;
        if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
            fprintf(stderr, "[ERROR] No se pudo inicializar Winsock\n");
            return 1;
        }
    #endif

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    signal(SIGPIPE, SIG_IGN);

    // Crear socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        fprintf(stderr, "[ERROR] No se pudo crear socket\n");
        return 1;
    }
    fprintf(stderr, "[MAIN] Socket creado: fd=%d\n", server_fd);
    fflush(stderr);

    // Reutilizar puerto
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Bind
    struct sockaddr_in srv_addr{};
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = INADDR_ANY;
    srv_addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&srv_addr, sizeof(srv_addr)) < 0) {
        fprintf(stderr, "[ERROR] Bind fallido en puerto %d\n", port);
        return 1;
    }
    fprintf(stderr, "[MAIN] Bind exitoso en puerto %d\n", port);
    fflush(stderr);

    // Listen
    if (listen(server_fd, 100) < 0) {
        fprintf(stderr, "[ERROR] Listen fallido\n");
        return 1;
    }
    fprintf(stderr, "[MAIN] Listen exitoso, aceptando conexiones...\n");
    fflush(stderr);

    // Accept loop
    while (true) {
        struct sockaddr_in client_addr{};
        socklen_t addr_len = sizeof(client_addr);

        fprintf(stderr, "[MAIN] Esperando conexión...\n");
        fflush(stderr);

        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
        
        fprintf(stderr, "[MAIN] Accept retornó: %d\n", client_fd);
        fflush(stderr);

        if (client_fd < 0) {
            fprintf(stderr, "[ERROR] Accept fallido\n");
            continue;
        }

        fprintf(stderr, "[MAIN] Cliente aceptado, creando thread\n");
        fflush(stderr);
        
        std::thread(handle_client, client_fd, client_addr).detach();
    }

    #ifdef _WIN32
        WSACleanup();
    #endif

    return 0;
}
