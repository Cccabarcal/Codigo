/*
 * server.cpp — Servidor central del Sistema IoT
 *
 * Uso: ./server <puerto> <archivo_logs>
 * Ejemplo: ./server 8080 server.log
 *
 * Compilar: g++ -std=c++17 -pthread -o server server.cpp
 */
 
#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <map>
#include <vector>
#include <deque>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <csignal>
 
// Sockets (multiplataforma)
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    #pragma comment(lib, "advapi32.lib")
    typedef int socklen_t;
    #define MSG_NOSIGNAL 0
    #define close closesocket
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <netdb.h>
#endif
 
#include "protocol.h"
#include "Logger.h"
 
// ── Configuración ──────────────────────────────────────────────────────────
#define MAX_CLIENTS     100
#define BUFFER_SIZE     1024
#define HISTORY_SIZE    50      // mediciones guardadas por sensor
#define AUTH_HOST       "localhost"  // Servicio de autenticación
#define AUTH_PORT       9000
 
// ── Estado global del servidor ─────────────────────────────────────────────
std::mutex sensors_mutex;
std::mutex operators_mutex;
std::mutex history_mutex;
 
// sensor_id → info del sensor
std::map<std::string, SensorInfo> sensors;
 
// fd del socket → info del operador (fd, ip, port)
struct OperatorConn {
    int         fd;
    std::string ip;
    int         port;
    std::string username;
};
std::vector<OperatorConn> operators;
 
// sensor_id → historial de mediciones (últimas HISTORY_SIZE)
std::map<std::string, std::deque<Measurement>> history;
 
int server_fd = -1;
 
// ── Resolución de nombres (sin IPs hardcodeadas) ───────────────────────────
std::string resolve_host(const std::string& hostname) {
    struct addrinfo hints{}, *res;
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
 
    int rc = getaddrinfo(hostname.c_str(), nullptr, &hints, &res);
    if (rc != 0) {
        Logger::log("DNS", "0.0.0.0", 0,
                    "Error resolviendo " + hostname + ": " + gai_strerror(rc));
        return "";
    }
    char buf[INET_ADDRSTRLEN];
    auto* addr = (struct sockaddr_in*)res->ai_addr;
    inet_ntop(AF_INET, &addr->sin_addr, buf, sizeof(buf));
    freeaddrinfo(res);
    return std::string(buf);
}
 
// ── Autenticación externa ──────────────────────────────────────────────────
// Protocolo mínimo: envía "AUTH <user> <pass>\n", recibe "OK <role>\n" o "ERROR ...\n"
bool authenticate_user(const std::string& user,
                       const std::string& pass,
                       std::string& role_out) {
    std::string auth_ip = resolve_host(AUTH_HOST);
    if (auth_ip.empty()) {
        // Si el DNS falla, el sistema no termina — reporta error y continúa
        Logger::log("AUTH", "0.0.0.0", 0, "DNS falló para " + std::string(AUTH_HOST));
        return false;
    }
 
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return false;
 
    struct timeval tv{3, 0};  // timeout 3s
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
 
    struct sockaddr_in auth_addr{};
    auth_addr.sin_family = AF_INET;
    auth_addr.sin_port   = htons(AUTH_PORT);
    inet_pton(AF_INET, auth_ip.c_str(), &auth_addr.sin_addr);
 
    if (connect(sock, (struct sockaddr*)&auth_addr, sizeof(auth_addr)) < 0) {
        Logger::log("AUTH", "0.0.0.0", 0, "No se pudo conectar al servicio auth");
        close(sock);
        return false;
    }
 
    std::string req = "AUTH " + user + " " + pass + "\n";
    send(sock, req.c_str(), req.size(), 0);
 
    char buf[256] = {};
    recv(sock, buf, sizeof(buf) - 1, 0);
    close(sock);
 
    std::string resp(buf);
    if (resp.rfind("OK ", 0) == 0) {
        role_out = resp.substr(3);
        // quitar \n al final
        if (!role_out.empty() && role_out.back() == '\n')
            role_out.pop_back();
        return true;
    }
    return false;
}
 
// ── Enviar mensaje a todos los operadores conectados ──────────────────────
void broadcast_operators(const std::string& msg) {
    std::lock_guard<std::mutex> lock(operators_mutex);
    for (auto& op : operators) {
        send(op.fd, msg.c_str(), msg.size(), MSG_NOSIGNAL);
        Logger::log_send(op.ip, op.port, msg);
    }
}
 
// ── Detectar anomalía y emitir alerta ─────────────────────────────────────
void check_and_alert(const SensorInfo& info, double value, const std::string& ts) {
    auto it = THRESHOLDS.find(info.type);
    if (it == THRESHOLDS.end()) return;
 
    std::string alert_type;
    if (value > it->second.max) alert_type = "HIGH_" + info.type;
    else if (value < it->second.min) alert_type = "LOW_"  + info.type;
 
    if (!alert_type.empty()) {
        std::string alert = make_alert(info.sensor_id, alert_type, value, ts);
        Logger::log_alert("SERVER", 0, alert);
        broadcast_operators(alert);
    }
}
 
// ── Guardar medición en historial ─────────────────────────────────────────
void store_measurement(const std::string& sensor_id, double value,
                       const std::string& ts) {
    std::lock_guard<std::mutex> lock(history_mutex);
    auto& dq = history[sensor_id];
    dq.push_back({sensor_id, value, ts});
    if ((int)dq.size() > HISTORY_SIZE) dq.pop_front();
}
 
// ── Construir lista de sensores activos ───────────────────────────────────
std::string build_sensor_list() {
    std::lock_guard<std::mutex> lock(sensors_mutex);
    std::string list;
    for (auto& [id, info] : sensors) {
        if (info.active) {
            if (!list.empty()) list += ",";
            list += id + ":" + info.type;
        }
    }
    return "SENSORS " + list + "\n";
}
 
// ── Manejo de cliente (hilo por conexión) ─────────────────────────────────
void handle_client(int client_fd, struct sockaddr_in client_addr) {
    std::string ip   = Logger::get_ip(client_addr);
    int         port = Logger::get_port(client_addr);
 
    Logger::log("CONN", ip, port, "Nueva conexión aceptada");
 
    ClientRole  role      = ROLE_UNKNOWN;
    std::string sensor_id;
    std::string username;
    char        buf[BUFFER_SIZE];
    std::string partial;   // buffer para mensajes incompletos
 
    while (true) {
        memset(buf, 0, sizeof(buf));
        ssize_t n = recv(client_fd, buf, sizeof(buf) - 1, 0);
        if (n <= 0) {
            // Conexión cerrada o error
            Logger::log("CONN", ip, port, "Conexión cerrada (recv=" + std::to_string(n) + ")");
            break;
        }
 
        partial += std::string(buf, n);
 
        // Procesar todos los mensajes completos (terminan en \n)
        size_t pos;
        while ((pos = partial.find('\n')) != std::string::npos) {
            std::string line = partial.substr(0, pos);
            partial.erase(0, pos + 1);
 
            // quitar \r si viene de Windows
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.empty()) continue;
 
            Logger::log_recv(ip, port, line);
            auto tokens = split(line);
            if (tokens.empty()) continue;
 
            std::string cmd = tokens[0];
            std::string response;
 
            // ── REGISTER SENSOR <id> <tipo> <unidad> ──────────────────────
            if (cmd == CMD_REGISTER && tokens.size() == 5 && tokens[1] == "SENSOR") {
                sensor_id = tokens[2];
                SensorInfo info;
                info.sensor_id  = sensor_id;
                info.type       = tokens[3];
                info.unit       = tokens[4];
                info.last_value = 0.0;
                info.last_ts    = "";
                info.active     = true;
 
                {
                    std::lock_guard<std::mutex> lock(sensors_mutex);
                    sensors[sensor_id] = info;
                }
                role = ROLE_SENSOR;
                response = make_ok("REGISTERED " + sensor_id);
 
                // Notificar operadores del nuevo sensor
                broadcast_operators("OK NEW_SENSOR " + sensor_id + " " +
                                    tokens[3] + "\n");
            }
 
            // ── MEASURE <sensor_id> <valor> <timestamp> ───────────────────
            else if (cmd == CMD_MEASURE && tokens.size() == 4) {
                if (role != ROLE_SENSOR) {
                    response = make_error(ERR_FORBIDDEN);
                } else {
                    double value = 0.0;
                    try { value = std::stod(tokens[2]); }
                    catch (...) { response = make_error(ERR_BAD_REQUEST); goto send_resp; }
 
                    std::string ts = tokens[3];
 
                    // Actualizar estado del sensor
                    {
                        std::lock_guard<std::mutex> lock(sensors_mutex);
                        if (sensors.count(tokens[1])) {
                            sensors[tokens[1]].last_value = value;
                            sensors[tokens[1]].last_ts    = ts;
                        }
                    }
 
                    store_measurement(tokens[1], value, ts);
 
                    // Buscar info para verificar umbral
                    SensorInfo info_copy;
                    {
                        std::lock_guard<std::mutex> lock(sensors_mutex);
                        info_copy = sensors[tokens[1]];
                    }
                    check_and_alert(info_copy, value, ts);
 
                    // Reenviar medición a operadores
                    broadcast_operators(make_data(tokens[1], value, ts));
                    response = make_ok();
                }
            }
 
            // ── LOGIN <usuario> <password> ────────────────────────────────
            else if (cmd == CMD_LOGIN && tokens.size() == 3) {
                std::string role_str;
                bool ok = authenticate_user(tokens[1], tokens[2], role_str);
                if (ok) {
                    role     = ROLE_OPERATOR;
                    username = tokens[1];
                    {
                        std::lock_guard<std::mutex> lock(operators_mutex);
                        operators.push_back({client_fd, ip, port, username});
                    }
                    response = make_ok("Welcome " + username + " role=" + role_str);
                    Logger::log("AUTH", ip, port, "Login OK: " + username);
                } else {
                    response = make_error(ERR_UNAUTHORIZED);
                    Logger::log("AUTH", ip, port, "Login FAILED: " + tokens[1]);
                }
            }
 
            // ── LIST SENSORS ──────────────────────────────────────────────
            else if (cmd == CMD_LIST && tokens.size() == 2 && tokens[1] == "SENSORS") {
                if (role != ROLE_OPERATOR) {
                    response = make_error(ERR_UNAUTHORIZED);
                } else {
                    response = build_sensor_list();
                }
            }
 
            // ── GET MEASURE <sensor_id> <cantidad> ────────────────────────
            else if (cmd == CMD_GET && tokens.size() == 4 && tokens[1] == "MEASURE") {
                if (role != ROLE_OPERATOR) {
                    response = make_error(ERR_UNAUTHORIZED);
                } else {
                    std::string sid = tokens[2];
                    int count = 1;
                    try { count = std::stoi(tokens[3]); } catch (...) {}
 
                    std::lock_guard<std::mutex> lock(history_mutex);
                    if (history.find(sid) == history.end()) {
                        response = make_error(ERR_NOT_FOUND);
                    } else {
                        auto& dq = history[sid];
                        int start = std::max(0, (int)dq.size() - count);
                        response = "";
                        for (int i = start; i < (int)dq.size(); i++) {
                            response += make_data(dq[i].sensor_id,
                                                  dq[i].value,
                                                  dq[i].timestamp);
                        }
                        if (response.empty()) response = make_error(ERR_NOT_FOUND);
                    }
                }
            }
 
            // ── DISCONNECT <sensor_id> ────────────────────────────────────
            else if (cmd == CMD_DISCONNECT && tokens.size() == 2) {
                {
                    std::lock_guard<std::mutex> lock(sensors_mutex);
                    if (sensors.count(tokens[1]))
                        sensors[tokens[1]].active = false;
                }
                broadcast_operators("OK SENSOR_OFF " + tokens[1] + "\n");
                response = make_ok("Bye");
                Logger::log_send(ip, port, response);
                send(client_fd, response.c_str(), response.size(), MSG_NOSIGNAL);
                goto cleanup;
            }
 
            // ── LOGOUT ────────────────────────────────────────────────────
            else if (cmd == CMD_LOGOUT) {
                response = make_ok("Bye");
                Logger::log_send(ip, port, response);
                send(client_fd, response.c_str(), response.size(), MSG_NOSIGNAL);
                goto cleanup;
            }
 
            // ── Comando desconocido ───────────────────────────────────────
            else {
                response = make_error(ERR_BAD_REQUEST);
            }
 
            send_resp:
            if (!response.empty()) {
                Logger::log_send(ip, port, response);
                send(client_fd, response.c_str(), response.size(), MSG_NOSIGNAL);
            }
        } // while mensajes completos
    } // while recv
 
    cleanup:
    // Si era operador, eliminarlo de la lista
    if (role == ROLE_OPERATOR) {
        std::lock_guard<std::mutex> lock(operators_mutex);
        operators.erase(
            std::remove_if(operators.begin(), operators.end(),
                [client_fd](const OperatorConn& o){ return o.fd == client_fd; }),
            operators.end());
    }
    // Si era sensor, marcarlo inactivo
    if (role == ROLE_SENSOR && !sensor_id.empty()) {
        std::lock_guard<std::mutex> lock(sensors_mutex);
        if (sensors.count(sensor_id))
            sensors[sensor_id].active = false;
        broadcast_operators("OK SENSOR_OFF " + sensor_id + "\n");
    }
    close(client_fd);
    Logger::log("CONN", ip, port, "Hilo terminado");
}
 
// ── Señal para cierre limpio ──────────────────────────────────────────────
void handle_signal(int sig) {
    Logger::log("SYSTEM", "0.0.0.0", 0, "Señal " + std::to_string(sig) + " recibida. Cerrando...");
    if (server_fd >= 0) close(server_fd);
    Logger::close();
    exit(0);
}
 
// ── Main ──────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    // DEBUG: Imprimir inmediatamente para verificar que el programa está corriendo
    fprintf(stderr, "[DEBUG] Servidor iniciando...\n");
    fflush(stderr);
    
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <puerto> <archivo_logs>\n", argv[0]);
        return 1;
    }

    #ifdef _WIN32
        WSADATA wsa_data;
        if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
            fprintf(stderr, "Error: No se pudo inicializar Winsock\n");
            return 1;
        }
    #endif

    int         port     = std::stoi(argv[1]);
    std::string log_file = argv[2];
    
    fprintf(stderr, "[DEBUG] Puerto: %d, Log file: %s\n", port, log_file.c_str());
    fflush(stderr);
 
    if (!Logger::init(log_file)) {
        fprintf(stderr, "[DEBUG] ERROR: Logger::init() falló\n");
        fflush(stderr);
        return 1;
    }
    
    fprintf(stderr, "[DEBUG] Logger inicializado correctamente\n");
    fflush(stderr);
 
    signal(SIGINT,  handle_signal);
    signal(SIGTERM, handle_signal);
    signal(SIGPIPE, SIG_IGN);  // evita crash si el cliente cierra la conexión
 
    // ── Crear socket TCP ──────────────────────────────────────────────────
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        Logger::log("SYSTEM", "0.0.0.0", 0, "Error creando socket");
        return 1;
    }
 
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
 
    struct sockaddr_in srv_addr{};
    srv_addr.sin_family      = AF_INET;
    srv_addr.sin_addr.s_addr = INADDR_ANY;
    srv_addr.sin_port        = htons(port);
 
    if (bind(server_fd, (struct sockaddr*)&srv_addr, sizeof(srv_addr)) < 0) {
        Logger::log("SYSTEM", "0.0.0.0", 0, "Error en bind puerto " + std::to_string(port));
        return 1;
    }
 
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        Logger::log("SYSTEM", "0.0.0.0", 0, "Error en listen");
        return 1;
    }
 
    Logger::log("SYSTEM", "0.0.0.0", port,
                "Servidor escuchando en puerto " + std::to_string(port));
 
    // ── Loop principal: aceptar conexiones ────────────────────────────────
    while (true) {
        struct sockaddr_in client_addr{};
        socklen_t          addr_len = sizeof(client_addr);
 
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
        if (client_fd < 0) {
            Logger::log_error("0.0.0.0", 0, "Error en accept");
            continue;  // no termina, sigue aceptando
        }
 
        // Hilo separado por cliente
        std::thread(handle_client, client_fd, client_addr).detach();
    }
 
    #ifdef _WIN32
        WSACleanup();
    #endif

    return 0;
}
 