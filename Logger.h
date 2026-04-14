#ifndef LOGGER_H
#define LOGGER_H
 
#include <fstream>
#include <iostream>
#include <cstdio>
#include <string>
#include <mutex>
#include <ctime>
#include <sstream>

// Headers de red (multiplataforma)
#ifdef _WIN32
    #include <winsock2.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <arpa/inet.h>
    #include <netinet/in.h>
#endif
 
class Logger {
public:
    // Inicializa el logger con el archivo de salida
    static bool init(const std::string& filename) {
        fprintf(stderr, "[DEBUG-LOGGER] Antes de lock\n");
        fflush(stderr);
        
        std::lock_guard<std::mutex> lock(mtx_);
        
        fprintf(stderr, "[DEBUG-LOGGER] Despues de lock\n");
        fflush(stderr);
        
        // Si el filename es /dev/stdout, no abrir archivo (usar stdout)
        if (filename == "/dev/stdout" || filename == "stdout") {
            fprintf(stderr, "[DEBUG-LOGGER] Detectando stdout mode\n");
            fflush(stderr);
            initialized_ = true;
            fprintf(stderr, "[DEBUG-LOGGER] Init completado, retornando true\n");
            fflush(stderr);
            return true;
        }
        
        fprintf(stderr, "[DEBUG-LOGGER] Abriendo archivo: %s\n", filename.c_str());
        fflush(stderr);
        
        file_.open(filename, std::ios::app);
        if (!file_.is_open()) {
            fprintf(stderr, "[LOGGER] No se pudo abrir: %s\n", filename.c_str());
            fflush(stderr);
            return false;
        }
        initialized_ = true;
        return true;
    }
 
    // Registra un evento con IP y puerto del cliente
    static void log(const std::string& level,
                    const std::string& client_ip,
                    int client_port,
                    const std::string& message) {
        if (!initialized_) return;  // No loguear si no está inicializado
        
        std::lock_guard<std::mutex> lock(mtx_);
        std::string entry = "[" + now() + "] [" + level + "] " +
                            client_ip + ":" + std::to_string(client_port) +
                            " | " + message;
        std::cout << entry << std::endl;
        std::cout.flush();
        if (file_.is_open()) {
            file_ << entry << "\n";
            file_.flush();
        }
    }
 
    // Registra mensaje recibido de un cliente
    static void log_recv(const std::string& client_ip, int port,
                         const std::string& msg) {
        log("RECV", client_ip, port, "<<< " + msg);
    }
 
    // Registra respuesta enviada a un cliente
    static void log_send(const std::string& client_ip, int port,
                         const std::string& msg) {
        log("SEND", client_ip, port, ">>> " + msg);
    }
 
    // Registra una alerta generada
    static void log_alert(const std::string& client_ip, int port,
                          const std::string& msg) {
        log("ALERT", client_ip, port, "!!! " + msg);
    }
 
    // Registra un error
    static void log_error(const std::string& client_ip, int port,
                          const std::string& msg) {
        log("ERROR", client_ip, port, msg);
    }
 
    static void close() {
        std::lock_guard<std::mutex> lock(mtx_);
        // Solo cerrar si es un archivo real (no stdout)
        if (file_.is_open()) file_.close();
    }
 
    // Extrae IP y puerto de un sockaddr_in
    static std::string get_ip(const struct sockaddr_in& addr) {
        char buf[INET_ADDRSTRLEN];
        #ifdef _WIN32
            strcpy_s(buf, sizeof(buf), inet_ntoa(addr.sin_addr));
        #else
            inet_ntop(AF_INET, &addr.sin_addr, buf, sizeof(buf));
        #endif
        return std::string(buf);
    }
 
    static int get_port(const struct sockaddr_in& addr) {
        return ntohs(addr.sin_port);
    }
 
private:
    static std::string now() {
        time_t t = time(nullptr);
        char buf[20];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&t));
        return std::string(buf);
    }
 
    static std::ofstream file_;
    static std::mutex    mtx_;
    static bool          initialized_;
};
 
// Definiciones de los miembros estáticos (van aquí para evitar multiple definition)
std::ofstream Logger::file_;
std::mutex    Logger::mtx_;
bool          Logger::initialized_ = false;
 
#endif // LOGGER_H