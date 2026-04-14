/*
 * server.cpp — Servidor central del Sistema IoT Distribuido
 * 
 * Descripción:
 *   Servidor de monitoreo que actúa como hub central para sensores y operadores.
 *   Procesa mediciones, detecta anomalías, almacena historial y emite alertas.
 *   Maneja múltiples clientes simultáneamente mediante threading.
 *
 * Compilación:
 *   g++ -std=c++17 -pthread -Wall -Wextra -o server server.cpp
 *
 * Ejecución:
 *   ./server <puerto> <archivo_logs>
 *   ./server 8080 server.log
 *
 * Protocolo: Ver PROTOCOLO.md para especificación completa
 */

#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <mutex>
#include <map>
#include <vector>
#include <deque>
#include <sstream>
#include <cstring>
#include <cstdio>
#include <csignal>
#include <ctime>
#include <algorithm>

// Sockets - Multiplataforma
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
    #define MSG_NOSIGNAL 0
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <netdb.h>
    #define close closesocket
    #define SOCKET int
    #define INVALID_SOCKET -1
#endif

#include "protocol.h"

// ════════════════════════════════════════════════════════════════════════════
// CONFIGURACIÓN
// ════════════════════════════════════════════════════════════════════════════

#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024
#define HISTORY_SIZE 50
#define AUTH_HOST "localhost"
#define AUTH_PORT 9000

// ════════════════════════════════════════════════════════════════════════════
// ESTADO GLOBAL
// ════════════════════════════════════════════════════════════════════════════

std::mutex sensors_mutex;
std::mutex operators_mutex;
std::mutex history_mutex;
std::ofstream logfile;
std::mutex log_mutex;

// Mapa de sensores registrados
std::map<std::string, SensorInfo> sensors;

// Información de operadores conectados
struct OperatorConnection {
    SOCKET fd;
    std::string ip;
    int port;
    std::string username;
};
std::vector<OperatorConnection> operators;

// Historial de mediciones
std::map<std::string, std::deque<Measurement>> history;

SOCKET server_socket = INVALID_SOCKET;

// ════════════════════════════════════════════════════════════════════════════
// LOGGING
// ════════════════════════════════════════════════════════════════════════════

void log_event(const std::string& level, const std::string& ip, int port,
               const std::string& message) {
    std::lock_guard<std::mutex> lock(log_mutex);
    
    // Obtener timestamp
    time_t now = time(nullptr);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    // Formato: [TIMESTAMP] [LEVEL] ip:port | mensaje
    std::string entry = std::string("[") + timestamp + "] [" + level + "] " +
                       ip + ":" + std::to_string(port) + " | " + message;
    
    // Consola
    std::cout << entry << std::endl;
    std::cout.flush();
    
    // Archivo
    if (logfile.is_open()) {
        logfile << entry << std::endl;
        logfile.flush();
    }
}

// ════════════════════════════════════════════════════════════════════════════
// AUTENTICACIÓN
// ════════════════════════════════════════════════════════════════════════════

bool authenticate_user(const std::string& user, const std::string& pass,
                       std::string& role_out) {
    // Conectar a servicio de autenticación
    SOCKET auth_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (auth_socket == INVALID_SOCKET) {
        log_event("AUTH", "0.0.0.0", 0, "No se pudo crear socket for auth");
        return false;
    }
    
    struct sockaddr_in auth_addr{};
    auth_addr.sin_family = AF_INET;
    auth_addr.sin_port = htons(AUTH_PORT);
    inet_pton(AF_INET, AUTH_HOST, &auth_addr.sin_addr);
    
    if (connect(auth_socket, (struct sockaddr*)&auth_addr, sizeof(auth_addr)) < 0) {
        log_event("AUTH", "0.0.0.0", 0, "No se pudo conectar a auth service");
        close(auth_socket);
        return false;
    }
    
    // Enviar petición: AUTH <user> <pass>
    std::string request = "AUTH " + user + " " + pass + "\n";
    send(auth_socket, request.c_str(), request.length(), MSG_NOSIGNAL);
    
    // Recibir respuesta
    char buf[256] = {0};
    ssize_t n = recv(auth_socket, buf, sizeof(buf) - 1, 0);
    close(auth_socket);
    
    if (n <= 0) return false;
    
    std::string response(buf, n);
    if (response.find("OK") == 0) {
        // Extraer role: "OK <role>"
        std::istringstream iss(response);
        std::string ok_word, role;
        iss >> ok_word >> role;
        role_out = role;
        return true;
    }
    
    return false;
}

// ════════════════════════════════════════════════════════════════════════════
// BROADCAST A OPERADORES
// ════════════════════════════════════════════════════════════════════════════

void broadcast_to_operators(const std::string& message) {
    std::lock_guard<std::mutex> lock(operators_mutex);
    for (auto& op : operators) {
        send(op.fd, message.c_str(), message.length(), MSG_NOSIGNAL);
    }
}

// ════════════════════════════════════════════════════════════════════════════
// VERIFICACIÓN DE UMBRALES Y ALERTAS
// ════════════════════════════════════════════════════════════════════════════

void check_thresholds(const std::string& sensor_id, double value) {
    std::lock_guard<std::mutex> lock(sensors_mutex);
    
    if (sensors.count(sensor_id) == 0) return;
    
    const SensorInfo& info = sensors[sensor_id];
    
    // Buscar umbrales
    if (THRESHOLDS.count(info.type) > 0) {
        const SensorThreshold& threshold = THRESHOLDS[info.type];
        
        if (value < threshold.min || value > threshold.max) {
            std::string alert = "ALERT " + sensor_id + " exceeds threshold: " +
                              std::to_string(value) + " " + info.unit + "\n";
            log_event("ALERT", "0.0.0.0", 0, alert);
            broadcast_to_operators(alert);
        }
    }
}

// ════════════════════════════════════════════════════════════════════════════
// MANEJO DE CLIENTE
// ════════════════════════════════════════════════════════════════════════════

void handle_client(SOCKET client_fd, struct sockaddr_in client_addr) {
    char ip_buf[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, ip_buf, INET_ADDRSTRLEN);
    std::string client_ip(ip_buf);
    int client_port = ntohs(client_addr.sin_port);
    
    log_event("CONN", client_ip, client_port, "Nueva conexión");
    
    ClientRole role = ROLE_UNKNOWN;
    std::string sensor_id;
    std::string username;
    char buffer[BUFFER_SIZE];
    std::string partial_msg;
    
    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t n = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
        
        if (n <= 0) {
            log_event("CONN", client_ip, client_port, "Desconectado");
            break;
        }
        
        partial_msg += std::string(buffer, n);
        
        // Procesar líneas completas (terminadas en \n)
        size_t pos;
        while ((pos = partial_msg.find('\n')) != std::string::npos) {
            std::string line = partial_msg.substr(0, pos);
            partial_msg.erase(0, pos + 1);
            
            // Remover \r si existe
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            if (line.empty()) continue;
            
            log_event("RECV", client_ip, client_port, line);
            
            // Parsear comando
            std::vector<std::string> tokens;
            std::istringstream iss(line);
            std::string token;
            while (iss >> token) {
                tokens.push_back(token);
            }
            
            if (tokens.empty()) continue;
            
            std::string response = "ERROR Bad request\n";
            
            // ─────────────────────────────────────────────────────────────────
            // REGISTER SENSOR <id> <tipo> <unidad>
            // ─────────────────────────────────────────────────────────────────
            if (tokens[0] == CMD_REGISTER && tokens.size() >= 5 && 
                tokens[1] == "SENSOR") {
                
                sensor_id = tokens[2];
                std::string type = tokens[3];
                std::string unit = tokens[4];
                
                SensorInfo info;
                info.sensor_id = sensor_id;
                info.type = type;
                info.unit = unit;
                info.last_value = 0.0;
                info.last_ts = "";
                info.active = true;
                
                {
                    std::lock_guard<std::mutex> lock(sensors_mutex);
                    sensors[sensor_id] = info;
                }
                
                role = ROLE_SENSOR;
                response = "OK REGISTERED " + sensor_id + "\n";
                
                // Notificar operadores
                std::string notification = "NOTIFY NEW_SENSOR " + sensor_id + 
                                         " " + type + "\n";
                broadcast_to_operators(notification);
                
                log_event("SENSOR", client_ip, client_port, 
                         "Registro exitoso: " + sensor_id);
            }
            
            // ─────────────────────────────────────────────────────────────────
            // MEASURE <sensor_id> <valor> <timestamp>
            // ─────────────────────────────────────────────────────────────────
            else if (tokens[0] == CMD_MEASURE && tokens.size() >= 4) {
                std::string sid = tokens[1];
                double value = std::stod(tokens[2]);
                std::string ts = tokens[3];
                
                // Actualizar sensor
                {
                    std::lock_guard<std::mutex> lock(sensors_mutex);
                    if (sensors.count(sid)) {
                        sensors[sid].last_value = value;
                        sensors[sid].last_ts = ts;
                    }
                }
                
                // Agregar al historial
                {
                    std::lock_guard<std::mutex> lock(history_mutex);
                    history[sid].push_back({sid, value, ts});
                    if (history[sid].size() > HISTORY_SIZE) {
                        history[sid].pop_front();
                    }
                }
                
                // Verificar umbrales
                check_thresholds(sid, value);
                
                // Notificar operadores
                std::string data_msg = "DATA " + sid + " " + 
                                      std::to_string(value) + " " + ts + "\n";
                broadcast_to_operators(data_msg);
                
                response = "OK\n";
            }
            
            // ─────────────────────────────────────────────────────────────────
            // LOGIN <usuario> <password>
            // ─────────────────────────────────────────────────────────────────
            else if (tokens[0] == CMD_LOGIN && tokens.size() >= 3) {
                std::string user = tokens[1];
                std::string pass = tokens[2];
                std::string role_str;
                
                if (authenticate_user(user, pass, role_str)) {
                    role = ROLE_OPERATOR;
                    username = user;
                    
                    {
                        std::lock_guard<std::mutex> lock(operators_mutex);
                        operators.push_back({client_fd, client_ip, client_port, user});
                    }
                    
                    response = "OK Welcome " + user + " role=" + role_str + "\n";
                    log_event("AUTH", client_ip, client_port, "Login exitoso: " + user);
                } else {
                    response = "ERROR Unauthorized\n";
                    log_event("AUTH", client_ip, client_port, "Login fallido: " + user);
                }
            }
            
            // ─────────────────────────────────────────────────────────────────
            // LIST SENSORS
            // ─────────────────────────────────────────────────────────────────
            else if (tokens[0] == CMD_LIST && tokens.size() >= 2 && 
                    tokens[1] == "SENSORS") {
                
                if (role != ROLE_OPERATOR) {
                    response = "ERROR Unauthorized\n";
                } else {
                    std::lock_guard<std::mutex> lock(sensors_mutex);
                    response = "SENSORS ";
                    for (const auto& [id, info] : sensors) {
                        response += id + ":" + info.type + " ";
                    }
                    response += "\n";
                }
            }
            
            // ─────────────────────────────────────────────────────────────────
            // GET MEASURE <sensor_id> <cantidad>
            // ─────────────────────────────────────────────────────────────────
            else if (tokens[0] == CMD_GET && tokens.size() >= 4 &&
                    tokens[1] == "MEASURE") {
                
                if (role != ROLE_OPERATOR) {
                    response = "ERROR Unauthorized\n";
                } else {
                    std::string sid = tokens[2];
                    int count = std::stoi(tokens[3]);
                    
                    std::lock_guard<std::mutex> lock(history_mutex);
                    if (history.count(sid) > 0) {
                        response = "DATA ";
                        int shown = 0;
                        for (auto it = history[sid].rbegin(); 
                             it != history[sid].rend() && shown < count; 
                             ++it, ++shown) {
                            response += std::to_string(it->value) + " ";
                        }
                        response += "\n";
                    } else {
                        response = "ERROR Sensor not found\n";
                    }
                }
            }
            
            // ─────────────────────────────────────────────────────────────────
            // DISCONNECT / LOGOUT
            // ─────────────────────────────────────────────────────────────────
            else if (tokens[0] == CMD_DISCONNECT || tokens[0] == CMD_LOGOUT) {
                response = "OK\n";
                send(client_fd, response.c_str(), response.length(), MSG_NOSIGNAL);
                log_event("CONN", client_ip, client_port, "Desconexión solicitada");
                close(client_fd);
                return;
            }
            
            // Enviar respuesta
            log_event("SEND", client_ip, client_port, response);
            send(client_fd, response.c_str(), response.length(), MSG_NOSIGNAL);
        }
    }
    
    // Remover operador si estaba conectado
    {
        std::lock_guard<std::mutex> lock(operators_mutex);
        operators.erase(
            std::remove_if(operators.begin(), operators.end(),
                          [client_fd](const OperatorConnection& op) {
                              return op.fd == client_fd;
                          }),
            operators.end()
        );
    }
    
    close(client_fd);
}

// ════════════════════════════════════════════════════════════════════════════
// MANEJADOR DE SEÑALES
// ════════════════════════════════════════════════════════════════════════════

void signal_handler(int sig) {
    std::cout << "\n[SEÑAL " << sig << "] Cerrando servidor..." << std::endl;
    if (server_socket != INVALID_SOCKET) {
        close(server_socket);
    }
    exit(0);
}

// ════════════════════════════════════════════════════════════════════════════
// MAIN
// ════════════════════════════════════════════════════════════════════════════

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Uso: " << argv[0] << " <puerto> <archivo_logs>" << std::endl;
        std::cerr << "Ej:  " << argv[0] << " 8080 server.log" << std::endl;
        return 1;
    }
    
    int port = std::stoi(argv[1]);
    std::string log_filename = argv[2];
    
    // Abrir archivo de logs
    logfile.open(log_filename, std::ios::app);
    if (!logfile.is_open()) {
        std::cerr << "Error: No se pudo abrir archivo de logs: " << log_filename << std::endl;
        return 1;
    }
    
    // Inicializar Winsock en Windows
    #ifdef _WIN32
        WSADATA wsa_data;
        if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
            log_event("SYSTEM", "0.0.0.0", 0, "Error inicializando Winsock");
            return 1;
        }
    #endif
    
    // Configurar manejadores de señales
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGPIPE, SIG_IGN);
    
    // Crear socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        log_event("SYSTEM", "0.0.0.0", 0, "Error creando socket");
        return 1;
    }
    
    // SO_REUSEADDR para reutilizar puerto
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // Bind
    struct sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        log_event("SYSTEM", "0.0.0.0", 0, "Error en bind puerto " + std::to_string(port));
        return 1;
    }
    
    // Listen
    if (listen(server_socket, MAX_CLIENTS) < 0) {
        log_event("SYSTEM", "0.0.0.0", 0, "Error en listen");
        return 1;
    }
    
    log_event("SYSTEM", "0.0.0.0", port, 
             "Servidor escuchando en puerto " + std::to_string(port));
    
    // Accept loop
    while (true) {
        struct sockaddr_in client_addr{};
        socklen_t addr_len = sizeof(client_addr);
        
        SOCKET client_socket = accept(server_socket, (struct sockaddr*)&client_addr,
                                      &addr_len);
        
        if (client_socket == INVALID_SOCKET) {
            log_event("ERROR", "0.0.0.0", 0, "Error en accept");
            continue;
        }
        
        // Crear thread para manejar cliente
        std::thread(&handle_client, client_socket, client_addr).detach();
    }
    
    #ifdef _WIN32
        WSACleanup();
    #endif
    
    return 0;
}
