# 🌐 Sistema Distribuido de Monitoreo de Sensores IoT

> Plataforma de monitoreo en tiempo real de sensores IoT distribuidos, con servidor central, múltiples clientes y servicio de autenticación.

## 📑 Tabla de Contenidos

1. [Descripción del Proyecto](#descripción-del-proyecto)
2. [Documentación Rápida](#documentación-rápida)
3. [Arquitectura](#arquitectura)
4. [Requisitos Previos](#requisitos-previos)
5. [Instalación](#instalación)
6. [Ejecución Local](#ejecución-local)
7. [Despliegue en AWS](#despliegue-en-aws)
8. [Estructura de Archivos](#estructura-de-archivos)
9. [Protocolo de Comunicación](#protocolo-de-comunicación)
10. [Guía de Uso](#guía-de-uso)
11. [Troubleshooting](#troubleshooting)

---

## Descripción del Proyecto

## 📚 Documentación Rápida

**Para despliegue en AWS:**
- 👉 Lee [`FINAL_DEPLOYMENT.md`](FINAL_DEPLOYMENT.md) - Guía paso a paso completa

**Para detalles técnicos:**
- 📖 [`PROTOCOLO.md`](PROTOCOLO.md) - Especificación del protocolo TCP/IP
- 📋 [`CHECKLIST_FINAL.md`](CHECKLIST_FINAL.md) - Verificación de requisitos
- 🌐 [`DESPLIEGUE_AWS.md`](DESPLIEGUE_AWS.md) - Configuración detallada AWS

**Para ejecución local:**
- Ejecuta `./prepare_deployment.sh` (Linux/macOS) o `.\prepare_deployment.ps1` (Windows)
- Luego `bash run_local.sh` para instrucciones de ejecución

---

Este proyecto implementa un **Sistema de Monitoreo IoT Distribuido** que permite:

- 📊 **Simulación de sensores**: Múltiples sensores IoT (temperatura, humedad, presión, vibración, energía)
- 🖥️ **Servidor central**: Procesa mediciones, detecta anomalías y genera alertas
- 👥 **Múltiples clientes**: Sensores y operadores se conectan simultáneamente
- 🔐 **Autenticación**: Servicio auth externo para validar usuarios
- 📱 **Múltiples interfaces**: Aplicación de escritorio (GUI), interfaz web, protocolo socket
- ⚠️ **Alertas en tiempo real**: Notificación inmediata de valores anómalos

---

## Arquitectura

```
                          ┌─────────────────────┐
                          │   AWS EC2 Instance  │
                          │   (Despliegue)      │
                          └──────────┬──────────┘
                                     │
          ┌──────────────────────────┼──────────────────────────┐
          │                          │                          │
      ┌───▼────┐             ┌──────▼───────┐          ┌──────▼────┐
      │ Sensor │             │ IoT Server   │          │ Web Server│
      │ Clients│  ◄───TCP───►│  (C++,       │  JSON   │ (Flask)   │
      │(Python)│ :8080       │   puerto     │◄───────►│ :5000     │
      └────────┘             │   8080)      │         └───────────┘
                             │              │
    ┌──────────────────┐     │ • Procesa    │     ┌──────────────────┐
    │ Operator Client  │◄────► • Alerta     │     │ Login         │
    │ (Python + GUI)   │:8080  • Almacena   │     │ Sensors list  │
    │ Tkinter          │       │ • Broadcast│     │ Measurements  │
    └──────────────────┘       └─────┬──────┘     └──────────────────┘
                                     │
                          ┌──────────▼────────┐
                          │ Auth Service      │
                          │ (Python, :9000)   │
                          │ • Valida credenciales
                          │ • Retorna roles    │
                          └───────────────────┘
```

---

## Requisitos Previos

### Sistema Operativo
- **Windows 10+**, **macOS**, o **Linux**

### Software Requerido

#### Para el Servidor (C++)
- `g++` compiler con soporte C++17
- `pthread` library

#### Para Clientes y Servicios (Python)
- **Python 3.8+**
- `flask` (para interfaz web)
- `socket` (built-in)
- `threading` (built-in)
- `json` (built-in)

### Instalación de Dependencias

#### En Windows (PowerShell como Admin)
```powershell
# Instalar Python (descargar desde python.org)
# Verificar:
python --version

# Instalar Flask:
pip install flask
```

#### En macOS/Linux
```bash
# Compiler C++
sudo apt install build-essential  # Ubuntu/Debian
brew install gcc  # macOS

# Python 3
sudo apt install python3 python3-pip

# Dependencias Python
pip3 install flask
```

---

## Instalación

### 1. Clonar/Descargar el Proyecto

```bash
cd ~/Proyectos
git clone <URL_DEL_REPOSITORIO>
cd telematica/Codigo
```

### 2. Compilar el Servidor C++

```bash
# En el directorio del proyecto
g++ -std=c++17 -pthread -o server server.cpp

# Verificar compilación
./server --help  # No tiene help, pero verifica que compila
```

**En Windows (PowerShell)**:
```powershell
g++ -std=c++17 -pthread -o server.exe server.cpp
```

### 3. Preparar Archivos Python

Los scripts Python ya están listos. Hacer ejecutables (Linux/macOS):

```bash
chmod +x auth_service.py
chmod +x sensor_client.py
chmod +x operator_client.py
chmod +x web_interface.py
```

---

## Ejecución Local

### Terminal 1: Iniciar Servidor IoT Central (C++)

```bash
./server 8080 server.log
```

**Salida esperada**:
```
[2026-04-14 10:30:45] [SYSTEM] Servidor escuchando en puerto 8080
[2026-04-14 10:30:45] [CONN] 0.0.0.0:0 | Servidor iniciado. Log: server.log
```

### Terminal 2: Iniciar Servicio de Autenticación (Python)

```bash
python3 auth_service.py
```

**Salida esperada**:
```
[2026-04-14 10:30:46] [INFO] Servicio de Autenticación iniciando en 0.0.0.0:9000
[2026-04-14 10:30:46] [INFO] Escuchando en 0.0.0.0:9000
```

### Terminal 3: Iniciar Sensores Simulados (Python)

```bash
python3 sensor_client.py localhost 8080
```

**Salida esperada**:
```
[2026-04-14 10:30:47] Cliente Sensor IoT iniciando
[2026-04-14 10:30:47] Servidor: localhost:8080
[2026-04-14 10:30:47] Creados 6 sensores
[2026-04-14 10:30:48] [temp-01] Conectado a localhost:8080
[2026-04-14 10:30:48] [temp-01] → REGISTER SENSOR temp-01 temperature celsius
[2026-04-14 10:30:48] [temp-01] ← OK REGISTERED temp-01
```

### Terminal 4: Iniciar Cliente Operador (Python GUI)

```bash
python3 operator_client.py localhost 8080
```

**Ventana esperada**:
- Formulario de login (usuario: `carlos`, contraseña: `password123`)
- Después del login: panel con sensores, mediciones y alertas

### Terminal 5 (Opcional): Interfaz Web (Python Flask)

```bash
python3 web_interface.py
```

**Salida esperada**:
```
[2026-04-14 10:31:00] INFO Interfaz Web iniciando en 0.0.0.0:5000
[2026-04-14 10:31:00] INFO Servidor IoT: localhost:8080
```

Acceder en navegador: **http://localhost:5000**

---

## Despliegue en AWS - Guía Paso a Paso

### Paso 1: Crear Instancia EC2

1. Ir a **AWS Console** → **EC2** → **Launch Instance**
2. Seleccionar: **Ubuntu 22.04 LTS** (Free Tier elegible)
3. Tipo de Instancia: **t2.micro**
4. Configurar **Security Group** con estos puertos:
   - Puerto **22** (SSH): Origen tu IP
   - Puerto **8080** (IoT Server): Origen 0.0.0.0/0
   - Puerto **9000** (Auth Service): Origen 0.0.0.0/0
   - Puerto **5000** (Web Interface): Origen 0.0.0.0/0
5. Crear/seleccionar **Key Pair** (.pem)
6. Hacer clic en **Launch**

---

### Paso 2: Conectar a la Instancia por SSH

Desde **tu máquina local** (PowerShell/Terminal):

```bash
# Cambiar permisos del archivo de key (IMPORTANTE)
chmod 400 ~/.ssh/iot-key.pem

# Conectar por SSH (obtén PUBLIC_IP de AWS Console)
ssh -i ~/.ssh/iot-key.pem ubuntu@<PUBLIC_IP>
```

**Ejemplo:**
```bash
ssh -i ~/.ssh/iot-key.pem ubuntu@54.242.32.222
```

Deberías ver algo como:
```
ubuntu@ip-172-31-44-89:~$
```

---

### Paso 3: Actualizar Sistema e Instalar Dependencias

En **la instancia EC2**, ejecuta:

```bash
# Actualizar sistema
sudo apt update
sudo apt upgrade -y

# Instalar compilador C++
sudo apt install -y build-essential

# Instalar Python y pip
sudo apt install -y python3 python3-pip

# Instalar librerías Python necesarias
sudo apt install -y python3-flask python3-dev python3-requests
```

**Verificar instalaciones:**
```bash
g++ --version
python3 --version
python3 -c "import flask; print(flask.__version__)"
```

---

### Paso 4: Clonar el Repositorio

En **la instancia EC2**:

```bash
# Crear directorio
mkdir -p /home/ubuntu/Codigo

# Navegar
cd /home/ubuntu/Codigo

# Clonar desde GitHub (si tienes configurado git)
git clone <URL_DE_TU_REPOSITORIO>

# O si está todo en una carpeta
cd Codigo

# Verificar que están todos los archivos
ls -la
```

Deberías ver:
```
server.cpp
Logger.h
protocol.h
auth_service.py
sensor_client.py
operator_client.py
web_interface.py
users.json
```

---

### Paso 5: Compilar el Servidor C++

En **la instancia EC2**:

```bash
cd /home/ubuntu/Codigo

# Compilar
g++ -std=c++17 -pthread -o server server.cpp

# Verificar que compiló correctamente
ls -la server
file server
```

**Salida esperada:**
```
-rwxr-xr-x 1 ubuntu ubuntu 45000 Apr 20 01:30 server
server: ELF 64-bit LSB executable, x86-64, version 1
```

---

### Paso 6: Ejecutar los Servicios (4 Terminales SSH)

Abre **4 conexiones SSH diferentes** a tu instancia EC2.

#### **Terminal 1: Servidor IoT Central (C++)**

```bash
cd /home/ubuntu/Codigo
./server 8080 server.log
```

**Salida esperada:**
```
[2026-04-20 01:33:00] [SYSTEM] 0.0.0.0:8080 | Servidor escuchando en puerto 8080
```

---

#### **Terminal 2: Servicio de Autenticación (Python)**

```bash
cd /home/ubuntu/Codigo
python3 auth_service.py
```

**Salida esperada:**
```
[2026-04-20 01:33:01] [INFO] Escuchando en 0.0.0.0:9000
```

---

#### **Terminal 3: Clientes Sensor (Python)**

```bash
cd /home/ubuntu/Codigo
python3 sensor_client.py localhost 8080
```

**Salida esperada:**
```
[2026-04-20 01:33:02] [DEBUG] [temp-01] → REGISTER SENSOR temp-01 temperature celsius
[2026-04-20 01:33:02] [DEBUG] [temp-01] ← OK REGISTERED temp-01
[2026-04-20 01:33:05] [DEBUG] [temp-01] → MEASURE temp-01 23.5 2026-04-20T01:33:05Z
[2026-04-20 01:33:05] [DEBUG] [temp-01] ← OK
```

---

#### **Terminal 4: Interfaz Web Flask (Python)**

```bash
cd /home/ubuntu/Codigo
python3 web_interface.py
```

**Salida esperada:**
```
[2026-04-20 01:33:06] [INFO] Interfaz Web iniciando en 0.0.0.0:5000
WARNING: This is a development server.
 * Running on http://0.0.0.0:5000
```

---

### Paso 7: Verificar que Todo Funciona (Terminal 5)

Abre una **5ª conexión SSH** para verificación:

```bash
# Ver procesos activos
ps aux | grep -E "server|python3" | grep -v grep
```

Debería mostrar:
```
./server 8080 server.log
python3 auth_service.py
python3 sensor_client.py localhost 8080
python3 web_interface.py
```

---

```bash
# Ver puertos escuchando
netstat -tuln | grep -E "5000|8080|9000"
```

Debería mostrar:
```
tcp    0    0 0.0.0.0:5000    0.0.0.0:*    LISTEN
tcp    0    0 0.0.0.0:8080    0.0.0.0:*    LISTEN
tcp    0    0 0.0.0.0:9000    0.0.0.0:*    LISTEN
```

---

```bash
# Ver si hay sensores registrados
grep "REGISTER SENSOR" /home/ubuntu/Codigo/server.log | head -5
```

Debería mostrar:
```
[2026-04-20 01:33:02] [DEBUG] temp-01 REGISTER SENSOR
[2026-04-20 01:33:02] [DEBUG] temp-02 REGISTER SENSOR
[2026-04-20 01:33:03] [DEBUG] humid-01 REGISTER SENSOR
```

---

### Paso 8: Acceder a la Interfaz Web

Desde **tu navegador** (en tu máquina local):

```
http://<PUBLIC_IP>:5000
```

**Ejemplo:**
```
http://54.242.32.222:5000
```

**Login con:**
- Usuario: `carlos`
- Contraseña: `password123`

**Verás:**
- ✅ Lista de sensores activos
- ✅ Mediciones en tiempo real
- ✅ Alertas si hay valores anómalos

---

### Paso 9: Monitoreo y Logs en Vivo

En **Terminal 5**, ver logs en tiempo real:

```bash
# Ver logs del servidor
tail -f /home/ubuntu/Codigo/server.log

# En otra terminal: Ver logs de sensores
tail -f /home/ubuntu/Codigo/sensor_client.log

# En otra terminal: Ver logs de web
tail -f /home/ubuntu/Codigo/web_interface.log
```

---

### Paso 10: Configurar DNS (Route 53) - Opcional

Para acceder con un dominio en lugar de IP:

1. Ir a **AWS Route 53**
2. **Crear Hosted Zone** con tu dominio
3. **Crear registro A** que apunte a la **IP pública de EC2**
4. Esperar propagación DNS (15-30 min)

Luego acceder:
```
http://tudominio.com:5000
```

---

### Troubleshooting en AWS

#### **"Connection refused"**
```bash
# Verificar que el servidor está corriendo
ps aux | grep server
```

#### **"Port already in use"**
```bash
# Matar procesos anteriores
pkill -f server
pkill -f auth_service
pkill -f sensor_client
pkill -f web_interface

# Esperar 2 segundos
sleep 2

# Reiniciar servicios
cd /home/ubuntu/Codigo
./server 8080 server.log &
sleep 2
python3 auth_service.py &
```

#### **"No module named 'flask'"**
```bash
sudo apt install -y python3-flask python3-requests
```

#### **Compilación falla**
```bash
# Verificar g++
g++ --version

# Asegurarse de tener build-essential
sudo apt install -y build-essential

# Recompilar
g++ -std=c++17 -pthread -o server server.cpp
```

---

## Configuración DNS (Route 53)

---

## Estructura de Archivos

```
telematica/Codigo/
├── server.cpp                 # Servidor central (C++)
├── Logger.h                   # Sistema de logging
├── protocol.h                 # Definición de protocolo
├── auth_service.py            # Servicio de autenticación
├── sensor_client.py           # Cliente sensor (Python)
├── operator_client.py         # Cliente operador GUI (Tkinter)
├── web_interface.py           # Interfaz web (Flask)
├── users.json                 # Base de datos de usuarios
├── templates/
│   └── index.html             # Página web
├── PROTOCOLO.md               # Especificación del protocolo
├── README.md                  # Este archivo
├── server.log                 # Logs del servidor (generado)
├── auth_service.log           # Logs de auth (generado)
├── sensor_client.log          # Logs de sensores (generado)
├── operator_client.log        # Logs de operador (generado)
└── web_interface.log          # Logs web (generado)
```

---

## Protocolo de Comunicación

Ver **PROTOCOLO.md** para especificación completa.

### Comandos Principales

| Comando | Enviado por | Descripción |
|---------|------------|--|
| `REGISTER SENSOR <id> <tipo> <unidad>` | Sensor | Registra nuevo sensor |
| `MEASURE <sensor_id> <valor> <timestamp>` | Sensor | Envía medición |
| `LOGIN <user> <password>` | Operador | Autentica operador |
| `LIST SENSORS` | Operador | Obtiene sensores activos |
| `GET MEASURE <sensor_id> <count>` | Operador | Obtiene historial |
| `ALERT <sensor_id> <tipo> <valor> <timestamp>` | Servidor | Notifica anomalía |
| `LOGOUT` | Operador | Cierra sesión |

---

## Guía de Uso

### 1. Cliente Operador (GUI)

```
1. Ejecutar: python3 operator_client.py
2. Login:
   - Usuario: carlos
   - Contraseña: password123
3. Interfaz:
   - Izquierda: Lista de sensores
   - Centro-Derecha: Mediciones históricas
   - Abajo: Alertas en tiempo real ⚠️
```

### 2. Interfaz Web

```
1. Ejecutar: python3 web_interface.py
2. Abrir navegador: http://localhost:5000
3. Login (mismas credenciales)
4. Ver sensores y mediciones
5. Auto-refresca cada 5 segundos
```

### 3. Usuarios Pre-configurados

| Usuario | Contraseña | Rol |
|---------|-----------|-----|
| carlos | password123 | admin |
| maria | maria456 | operator |
| juan | juan789 | admin |
| laura | laura321 | operator |

Editar en `users.json`

---

## Troubleshooting

### "Connection refused" en cliente

**Causa**: Servidor no está escuchando
```bash
# Verificar que servidor está corriendo
ps aux | grep server

# Iniciar servidor
./server 8080 server.log
```

### "Address already in use"

```bash
# Encontrar proceso en puerto
lsof -i :8080       # Linux/macOS
netstat -ano | findstr :8080  # Windows

# Matar proceso
kill -9 <PID>       # Linux/macOS
taskkill /PID <PID> /F  # Windows
```

### Python módulos no encontrados

```bash
pip install flask
pip install --upgrade pip
```

### Errores de compilación C++

```bash
# Verificar g++
g++ --version

# Compilar con debugging
g++ -std=c++17 -pthread -g -o server server.cpp

# Ejecutar con más info
./server 8080 server.log 2>&1 | tee output.txt
```

### Firewall bloqueando puertos (AWS)

1. Ir a Security Group de la instancia
2. Edit inbound rules
3. Agregar:
   - Type: Custom TCP, Port: 8080, Source: 0.0.0.0/0
   - Type: Custom TCP, Port: 9000, Source: 0.0.0.0/0
   - Type: Custom TCP, Port: 5000, Source: 0.0.0.0/0

---

## Performance y Límites

| Parámetro | Valor | Ubicación |
|-----------|-------|-----------|
| Max clientes simultáneos | 100 | server.cpp:18 |
| Buffer de socket | 1024 bytes | server.cpp:19 |
| Historial por sensor | 50 mediciones | server.cpp:20 |
| Timeout inactividad | 5 minutos | Implícito |
| Intervalo mediciones | 5 segundos | sensor_client.py:18 |

---

## Logs

Todos los sistemas generan archivos de log:

```bash
# Ver logs en tiempo real
tail -f server.log
tail -f auth_service.log
tail -f sensor_client.log

# Buscar alertas
grep ALERT server.log
grep ERROR server.log
```

---

## Documentación Adicional

- **PROTOCOLO.md**: Especificación técnica completa del protocolo
- **server.cpp**: Código fuente documentado del servidor
- **Logger.h**: Implementación del sistema de logging
- **protocol.h**: Definiciones de estructuras y umbales

---

## Contribuyentes

- Sistema desarrollado para curso de Internet: Arquitectura y Protocolos
- Plataforma: AWS EC2
- Lenguajes: C++ (servidor), Python (clientes, web, auth)

---

## Licencia

Proyecto académico - Universidad

---

## Fecha de Entrega

**14 de Abril de 2026**

## Última Actualización

**14 de Abril de 2026**
