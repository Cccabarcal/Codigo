# Protocolo de Aplicación - Sistema IoT de Monitoreo

## 1. Visión General
Protocolo basado en texto sobre TCP para comunicación entre:
- **SENSORES**: Dispositivos IoT que envían mediciones
- **OPERADORES**: Usuarios que monitorean el sistema
- **SERVIDOR**: Central de procesamiento y orquestación

## 2. Características Técnicas
- **Tipo de Socket**: SOCK_STREAM (TCP)
- **Justificación**: Entrega confiable, ordenada. Crítico en monitoreo industrial.
- **Formato**: Líneas de texto terminadas con `\n`
- **Codificación**: UTF-8
- **Puerto**: 8080 (configurable)

## 3. Sintaxis General
```
COMANDO [PARAM1] [PARAM2] ... [PARAMN]
RESPUESTA [CÓDIGO|DATOS]
```

---

## 4. COMANDOS DEL PROTOCOLO

### 4.1 REGISTRO DE SENSORES

#### `REGISTER SENSOR <sensor_id> <tipo> <unidad>`
**Enviado por**: Sensor (al conectarse)
**Respuesta esperada**: `OK REGISTERED <sensor_id>`
**Errores posibles**: 400, 500

**Ejemplo**:
```
→ REGISTER SENSOR temp-01 temperature celsius
← OK REGISTERED temp-01
```

**Tipos de sensor permitidos**:
- `temperature` (rango: -10 a 80 °C)
- `humidity` (rango: 0 a 95 %)
- `pressure` (rango: 900 a 1100 hPa)
- `vibration` (rango: 0 a 10 m/s²)
- `energy` (rango: 0 a 5000 W)

---

### 4.2 ENVÍO DE MEDICIONES

#### `MEASURE <sensor_id> <valor> <timestamp>`
**Enviado por**: Sensor (periódicamente, cada 5-10s)
**Respuesta esperada**: `OK`
**Errores posibles**: 400, 403, 500

**Ejemplo**:
```
→ MEASURE temp-01 25.3 2026-04-14T10:30:45Z
← OK
```

**Flujo de procesamiento en servidor**:
1. Valida credenciales del sensor
2. Actualiza estado en mapa de sensores
3. Verifica umbrales (si valor fuera de rango → genera ALERT)
4. Almacena en historial (últimas 50 mediciones)
5. Reenvía a todos los OPERADORES conectados

---

### 4.3 AUTENTICACIÓN DE OPERADORES

#### `LOGIN <usuario> <password>`
**Enviado por**: Operador
**Respuesta esperada**: `OK Welcome <user> role=<role>`
**Errores posibles**: 401 (credenciales inválidas), 500

**Flujo**:
1. Servidor consulta servicio auth externo
2. Servicio auth valida credenciales y retorna role
3. Operador se agrega a lista de conectados
4. Recibe notificaciones en tiempo real

**Ejemplo**:
```
→ LOGIN carlos password123
← OK Welcome carlos role=admin
```

---

### 4.4 LISTAR SENSORES ACTIVOS

#### `LIST SENSORS`
**Enviado por**: Operador (solo después de LOGIN)
**Respuesta esperada**: `SENSORS <sensor1>:<tipo>,<sensor2>:<tipo>,...`
**Errores posibles**: 401 (no autenticado), 404 (sin sensores)

**Ejemplo**:
```
→ LIST SENSORS
← SENSORS temp-01:temperature,humid-01:humidity,press-01:pressure
```

---

### 4.5 OBTENER HISTORIAL DE MEDICIONES

#### `GET MEASURE <sensor_id> <cantidad>`
**Enviado por**: Operador (solo después de LOGIN)
**Respuesta esperada**: Múltiples líneas `DATA <sensor_id> <valor> <timestamp>`
**Errores posibles**: 401, 404 (sensor no existe), 500

**Ejemplo**:
```
→ GET MEASURE temp-01 5
← DATA temp-01 25.3 2026-04-14T10:30:45Z
← DATA temp-01 25.5 2026-04-14T10:31:00Z
← DATA temp-01 25.2 2026-04-14T10:31:15Z
← DATA temp-01 25.4 2026-04-14T10:31:30Z
← DATA temp-01 25.6 2026-04-14T10:31:45Z
```

---

### 4.6 ALERTAS (Enviadas por SERVIDOR a OPERADORES)

#### `ALERT <sensor_id> <tipo_alerta> <valor> <timestamp>`
**Enviado por**: Servidor (cuando se detecta anomalía)
**Dirección**: Servidor → Todos los OPERADORES conectados
**No requiere respuesta**

**Tipos de alerta**:
- `HIGH_temperature`: Temp > 80°C
- `LOW_temperature`: Temp < -10°C
- `HIGH_humidity`: Humedad > 95%
- `LOW_humidity`: Humedad < 0%
- `HIGH_pressure`: Presión > 1100 hPa
- `LOW_pressure`: Presión < 900 hPa
- `HIGH_vibration`: Vibración > 10 m/s²
- `LOW_vibration`: Vibración < 0 m/s²
- `HIGH_energy`: Energía > 5000 W
- `LOW_energy`: Energía < 0 W

**Ejemplo**:
```
← ALERT temp-01 HIGH_temperature 85.2 2026-04-14T10:35:20Z
← ALERT humid-01 LOW_humidity -2.1 2026-04-14T10:35:25Z
```

---

### 4.7 DESCONEXIÓN DE SENSOR

#### `DISCONNECT <sensor_id>`
**Enviado por**: Sensor
**Respuesta esperada**: `OK Bye`
**Comportamiento servidor**:
1. Marca sensor como inactivo
2. Notifica a operadores: `OK SENSOR_OFF <sensor_id>`
3. Cierra conexión

**Ejemplo**:
```
→ DISCONNECT temp-01
← OK Bye
```

---

### 4.8 LOGOUT DE OPERADOR

#### `LOGOUT`
**Enviado por**: Operador
**Respuesta esperada**: `OK Bye`
**Comportamiento servidor**:
1. Remueve operador de lista de conectados
2. Cierra conexión

**Ejemplo**:
```
→ LOGOUT
← OK Bye
```

---

## 5. CÓDIGOS DE ERROR

| Código | Significado | Solución |
|--------|-------------|----------|
| 400 | Bad request | Formato inválido, parámetros faltantes |
| 401 | Unauthorized | Credenciales inválidas o no autenticado |
| 403 | Forbidden | Rol insuficiente para operación |
| 404 | Not found | Sensor o recurso no existe |
| 500 | Internal error | Error en servidor (timeout, DB, etc.) |

**Formato de error**:
```
ERROR <codigo> <mensaje_opcional>
```

---

## 6. MÁQUINA DE ESTADOS

### Sensor
```
DESCONECTADO
    ↓ (conecta)
EN CONEXIÓN
    ↓ (envía REGISTER)
REGISTRADO
    ↓ (envía MEASURE periódicamente)
ACTIVO
    ↓ (anomalía detectada)
ACTIVO → ALERTA ENVIADA (broadcast)
    ↓ (envía DISCONNECT)
DESCONECTADO
```

### Operador
```
DESCONECTADO
    ↓ (conecta)
EN CONEXIÓN
    ↓ (envía LOGIN)
AUTENTICADO
    ↓ (puede enviar LIST, GET)
MONITOREO ACTIVO
    ↓ (recibe ALERT en tiempo real)
RECIBIENDO ALERTAS
    ↓ (envía LOGOUT)
DESCONECTADO
```

---

## 7. RESTRICCIONES Y REGLAS

| Regla | Descripción |
|-------|-------------|
| **Múltiples conexiones** | Cada sensor = 1 conexión, cada operador = 1 conexión |
| **Autenticación requerida** | Operadores DEBEN hacer LOGIN antes de consultar datos |
| **Broadcast de alertas** | Alertas se envían a todos los operadores conectados INMEDIATAMENTE |
| **Historial limitado** | Se guardan últimas 50 mediciones por sensor |
| **Timeout de inactividad** | Conexión se cierra si no hay actividad en 5 minutos |
| **Concurrencia** | Servidor maneja múltiples clientes simultáneamente con threads |

---

## 8. EJEMPLOS COMPLETOS DE SESIONES

### Sesión Sensor
```
CONEXIÓN ESTABLECIDA
→ REGISTER SENSOR temp-01 temperature celsius
← OK REGISTERED temp-01
→ MEASURE temp-01 25.3 2026-04-14T10:30:45Z
← OK
→ MEASURE temp-01 25.5 2026-04-14T10:31:00Z
← OK
→ MEASURE temp-01 85.2 2026-04-14T10:31:15Z
← OK
← ALERT temp-01 HIGH_temperature 85.2 2026-04-14T10:31:15Z  [enviado por servidor a operadores]
→ DISCONNECT temp-01
← OK Bye
CONEXIÓN CERRADA
```

### Sesión Operador
```
CONEXIÓN ESTABLECIDA
→ LOGIN carlos password123
← OK Welcome carlos role=admin
→ LIST SENSORS
← SENSORS temp-01:temperature,humid-01:humidity
→ GET MEASURE temp-01 3
← DATA temp-01 25.3 2026-04-14T10:30:45Z
← DATA temp-01 25.5 2026-04-14T10:31:00Z
← DATA temp-01 85.2 2026-04-14T10:31:15Z
← ALERT temp-01 HIGH_temperature 85.2 2026-04-14T10:31:15Z  [recibido en tiempo real]
→ LOGOUT
← OK Bye
CONEXIÓN CERRADA
```

---

## 9. SERVICIO DE AUTENTICACIÓN (EXTERNO)

El servidor principal NO almacena usuarios. Delega en servicio auth.

### Protocolo Interno (Servidor ↔ Auth Service)
```
→ AUTH <usuario> <password>
← OK <role>
  o
← ERROR <codigo>
```

**Roles válidos**:
- `admin`: Puede consultar datos, recibir alertas
- `operator`: Puede consultar datos, recibir alertas (más limitado)
- `sensor`: Solo para sensores (interno)

---

## 10. LOGGING REQUERIDO

El servidor DEBE registrar (en archivo + consola):
```
[TIMESTAMP] [NIVEL] IP:PUERTO | MENSAJE

Niveles:
- SYSTEM: Eventos del servidor (inicio, stop)
- CONN: Conexiones (aceptadas, cerradas)
- RECV: Mensajes recibidos
- SEND: Mensajes enviados
- ALERT: Alertas generadas
- AUTH: Eventos de autenticación
- ERROR: Errores
```

**Ejemplo**:
```
[2026-04-14 10:30:45] [CONN] 192.168.1.100:54321 | Nueva conexión aceptada
[2026-04-14 10:30:46] [RECV] 192.168.1.100:54321 | REGISTER SENSOR temp-01 temperature celsius
[2026-04-14 10:30:46] [SEND] 192.168.1.100:54321 | OK REGISTERED temp-01
[2026-04-14 10:31:15] [ALERT] SERVER:0 | temp-01 HIGH_temperature 85.2 2026-04-14T10:31:15Z
```

---

## 11. MATRIZ DE COMPATIBILIDAD

| Entidad | Puede | No puede |
|---------|-------|----------|
| **Sensor** | REGISTER, MEASURE, DISCONNECT | LOGIN, LIST, GET, LOGOUT |
| **Operador** | LOGIN, LIST, GET, LOGOUT | REGISTER, MEASURE, DISCONNECT |
| **Servidor** | Validar, procesar, broadcast | ... |

---

## Versionado
- **v1.0**: 2026-04-14 - Especificación inicial completa

