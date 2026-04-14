#ifndef PROTOCOL_H
#define PROTOCOL_H
 
#include <string>
#include <sstream>
#include <vector>
#include <map>
 
// ── Comandos que puede recibir el servidor ──────────────────────────────────
#define CMD_REGISTER   "REGISTER"
#define CMD_MEASURE    "MEASURE"
#define CMD_DISCONNECT "DISCONNECT"
#define CMD_LOGIN      "LOGIN"
#define CMD_LIST       "LIST"
#define CMD_GET        "GET"
#define CMD_LOGOUT     "LOGOUT"
 
// ── Respuestas que envía el servidor ───────────────────────────────────────
#define RESP_OK      "OK"
#define RESP_ERROR   "ERROR"
#define RESP_ALERT   "ALERT"
#define RESP_DATA    "DATA"
#define RESP_SENSORS "SENSORS"
 
// ── Códigos de error ───────────────────────────────────────────────────────
#define ERR_BAD_REQUEST   "400 Bad request"
#define ERR_UNAUTHORIZED  "401 Unauthorized"
#define ERR_FORBIDDEN     "403 Forbidden"
#define ERR_NOT_FOUND     "404 Not found"
#define ERR_INTERNAL      "500 Internal error"
 
// ── Umbrales de alerta por tipo de sensor ─────────────────────────────────
struct SensorThreshold {
    double min;
    double max;
};
 
static std::map<std::string, SensorThreshold> THRESHOLDS = {
    {"temperature", {-10.0, 80.0}},
    {"humidity",    {0.0,   95.0}},
    {"pressure",    {900.0, 1100.0}},
    {"vibration",   {0.0,   10.0}},
    {"energy",      {0.0,   5000.0}}
};
 
// ── Tipo de cliente ────────────────────────────────────────────────────────
enum ClientRole {
    ROLE_UNKNOWN,
    ROLE_SENSOR,
    ROLE_OPERATOR
};
 
// ── Medición de un sensor ──────────────────────────────────────────────────
struct Measurement {
    std::string sensor_id;
    double      value;
    std::string timestamp;
};
 
// ── Estado de un sensor registrado ────────────────────────────────────────
struct SensorInfo {
    std::string sensor_id;
    std::string type;       // temperature, humidity, etc.
    std::string unit;       // celsius, %, hPa, etc.
    double      last_value;
    std::string last_ts;
    bool        active;
};
 
// ── Utilidades de parsing ──────────────────────────────────────────────────
inline std::vector<std::string> split(const std::string& s) {
    std::vector<std::string> tokens;
    std::istringstream iss(s);
    std::string tok;
    while (iss >> tok) tokens.push_back(tok);
    return tokens;
}
 
// ── Construcción de respuestas ─────────────────────────────────────────────
inline std::string make_ok(const std::string& msg = "") {
    return msg.empty() ? "OK\n" : "OK " + msg + "\n";
}
 
inline std::string make_error(const std::string& code) {
    return "ERROR " + code + "\n";
}
 
inline std::string make_alert(const std::string& sensor_id,
                               const std::string& alert_type,
                               double value,
                               const std::string& ts) {
    return "ALERT " + sensor_id + " " + alert_type + " " +
           std::to_string(value) + " " + ts + "\n";
}
 
inline std::string make_data(const std::string& sensor_id,
                              double value,
                              const std::string& ts) {
    return "DATA " + sensor_id + " " + std::to_string(value) + " " + ts + "\n";
}
 
#endif // PROTOCOL_H