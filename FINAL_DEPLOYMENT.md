# 🚀 INSTRUCCIONES FINALES - COMMIT Y DESPLIEGUE EN AWS

## 📝 Resumen Ejecutivo

Has completado el desarrollo del **Sistema Distribuido de Monitoreo de Sensores IoT**. 

**IMPORTANTE**: El servidor C++ está optimizado para compilar en **Linux/AWS EC2**, no en Windows.

---

## ✅ CAMBIOS REALIZADOS (19 de Abril de 2026)

### 1. Limpieza de Versiones
- ✅ Renombrado `server_final.cpp` → `server_final.cpp.bak`
- ✅ Renombrado `server_simple.cpp` → `server_simple.cpp.bak`
- ✅ Mantenido `server.cpp` como versión única oficial

### 2. Configuración sin IPs Hardcodeadas
- ✅ **web_interface.py**: Actualizado para usar variables de entorno
  - `IOT_SERVER_HOST` (default: localhost)
  - `IOT_SERVER_PORT` (default: 8080)
  - `AUTH_SERVER_HOST` (default: localhost)
  - `AUTH_SERVER_PORT` (default: 9000)
- ✅ **sensor_client.py**: Ya usa argumentos de línea de comandos
- ✅ **operator_client.py**: Ya usa argumentos de línea de comandos

### 3. Dependencias Python Actualizadas
- ✅ flask: 2.3.2 → **3.0.0** (versión actual)
- ✅ Werkzeug: 2.3.6 → **3.0.0** (versión actual)
- ✅ Agregado: **requests==2.31.0** (para HTTP calls)

### 4. Resolución DNS Implementada
- ✅ `server.cpp` ya incluye `getaddrinfo()` para resolución de nombres
- ✅ Manejo de errores de DNS (no termina el servidor si falla)

---

## ⚡ PASO 1: VERIFICAR CAMBIOS LOCALMENTE

### En Windows (PowerShell)

```powershell
cd D:\Users\Cristian\Desktop\telematica\Codigo

# Ver cambios
git status

# Verificar que los archivos se actualizaron
ls server*.cpp*
# Deberías ver:
# - server.cpp (versión actual)
# - server_final.cpp.bak (backup)
# - server_simple.cpp.bak (backup)

# Ver cambios en archivos
git diff requirements.txt
git diff web_interface.py
```

---

## 💾 PASO 2: COMMIT A GIT (IMPORTANTE)

---

## 🌐 PASO 3: DESPLIEGUE EN AWS

### 3.1 Crear instancia EC2 (si no la tienes)

**En AWS Console:**

1. EC2 → Launch Instance
2. **Imagen:** Ubuntu 22.04 LTS
3. **Tipo:** t2.micro
4. **Security Groups:** Abrir puertos:
   - 5000 (web)
   - 8080 (servidor IoT)
   - 9000 (auth)
   - 22 (SSH)

### 3.2 SSH a la instancia

```bash
# Desde tu máquina local
ssh -i ~/.ssh/iot-key.pem ubuntu@54.242.32.222

# Desde Windows (si tienes PuTTY configurado)
putty -ssh ubuntu@54.242.32.222 -i iot-key.ppk
```

### 3.3 Clonar el repositorio

En la instancia EC2:

```bash
# Ir a home
cd ~

# Clonar repositorio
git clone <TU_REPO_URL>

# Entrar al directorio
cd Codigo
ls -la
```

Debería ver:
```
server.cpp        Logger.h          protocol.h
auth_service.py   sensor_client.py  web_interface.py
operator_client.py users.json        requirements.txt
PROTOCOLO.md      README.md         DESPLIEGUE_AWS.md
build.sh
```

---

## 🔨 PASO 4: COMPILACIÓN E INSTALACIÓN

### 4.1 Actualizar sistema

```bash
# Actualizar paquetes
sudo apt update
sudo apt upgrade -y

# Instalar herramientas de compilación
sudo apt install -y build-essential python3-pip
```

### 4.2 Compilar servidor

```bash
# Desde ~/Codigo
bash build.sh
```

**Salida esperada:**
```
==================================
Compilando Servidor IoT (C++)
==================================
✓ g++ encontrado: g++ (Ubuntu 11.2.0-19ubuntu1) 11.2.0
Compilando server.cpp...
✓ Compilación exitosa

Ejecutar con:
  ./server 8080 server.log
```

### 4.3 Instalar dependencias Python

```bash
# Web framework
pip3 install --break-system-packages flask

# Verificar
python3 -c "import flask; print(flask.__version__)"
```

---

## ▶️ PASO 5: EJECUTAR EL SISTEMA

Abrir **5 terminales SSH** diferentes a la misma instancia:

### Terminal 1: Servidor IoT (Puerto 8080)

```bash
cd ~/Codigo
./server 8080 server.log
```

**Salida esperada:**
```
[2026-04-14 15:30:00] [SYSTEM] 0.0.0.0:8080 | Servidor escuchando en puerto 8080
```

### Terminal 2: Servicio de Autenticación (Puerto 9000)

```bash
cd ~/Codigo
python3 auth_service.py
```

**Salida esperada:**
```
[2026-04-14 15:30:01] [INFO] Servicio iniciando en 0.0.0.0:9000
[2026-04-14 15:30:01] [INFO] Escuchando en 0.0.0.0:9000
```

### Terminal 3: Sensores IoT

```bash
cd ~/Codigo
python3 sensor_client.py 54.242.32.222 8080
```

**Salida esperada:**
```
[2026-04-14 15:30:02] [INFO] Cliente Sensor IoT iniciando
[2026-04-14 15:30:02] [INFO] Servidor: 54.242.32.222:8080
[2026-04-14 15:30:02] [INFO] Creados 6 sensores
[2026-04-14 15:30:05] [INFO] Todos los sensores iniciados
```

### Terminal 4: Interface Web (Puerto 5000)

```bash
cd ~/Codigo
python3 web_interface.py
```

**Salida esperada:**
```
[2026-04-14 15:30:06] [INFO] Interfaz Web iniciando en 0.0.0.0:5000
[2026-04-14 15:30:06] [INFO] WARNING: This is a development server...
 * Running on http://0.0.0.0:5000
```

### Terminal 5: Cliente Operador (Opcional)

```bash
cd ~/Codigo
python3 operator_client.py 54.242.32.222 8080
```

---

## ✅ PASO 6: VERIFICACIÓN DEL SISTEMA

### 6.1 Verificar puertos escuchando

```bash
netstat -tuln | grep -E "5000|8080|9000"
```

**Debería mostrar:**
```
tcp    0    0 0.0.0.0:9000    0.0.0.0:*    LISTEN
tcp    0    0 0.0.0.0:5000    0.0.0.0:*    LISTEN
tcp    0    0 0.0.0.0:8080    0.0.0.0:*    LISTEN
```

### 6.2 Verificar procesos

```bash
ps aux | grep -E "server|python3" | grep -v grep
```

**Debería mostrar:**
```
ubuntu  1234  ./server 8080 server.log
ubuntu  1235  python3 auth_service.py
ubuntu  1236  python3 sensor_client.py localhost 8080
ubuntu  1237  python3 web_interface.py
```

### 6.3 Probar conectividad

**Desde otra máquina (o terminal SSH):**

```bash
# Test TCP al servidor
nc -zv 54.242.32.222 8080
# Salida: Connection to 54.242.32.222 8080 port [tcp/*] succeeded!

# Test HTTP a interfaz web
curl http://54.242.32.222:5000
# Debería devolver HTML de la página de login
```

### 6.4 Acceder a interfaz web

**En navegador:**
```
http://54.242.32.222:5000
```

**Login:**
- Usuario: `carlos`
- Password: `password123`

---

## 📊 PASO 7: VERIFICAR LOGS

### Ver logs en tiempo real

```bash
# Terminal dedicada para monitoreo
cd ~/Codigo

# Servidor
tail -f server.log

# Auth Service  
tail -f auth_service.log

# Sensores
tail -f sensor_client.log

# Web
tail -f web_interface.log
```

### Logs esperados (muestras)

**server.log:**
```
[2026-04-14 15:32:00] [CONN] 54.242.32.100:54321 | Nueva conexión
[2026-04-14 15:32:00] [RECV] 54.242.32.100:54321 | REGISTER SENSOR temp-01 temperature celsius
[2026-04-14 15:32:00] [SEND] 54.242.32.100:54321 | OK REGISTERED temp-01
```

**sensor_client.log:**
```
[2026-04-14 15:32:00] [INFO] Conectando a 54.242.32.222:8080
[2026-04-14 15:32:01] [INFO] temp-01 registrado
[2026-04-14 15:32:06] [INFO] temp-01: 22.5 celsius
```

---

## 🎯 PASO 8: PRUEBAS FUNCIONALES

### Test 1: Registración de sensor

**En terminal local:**
```bash
echo "REGISTER SENSOR test-01 temperature celsius" | nc 54.242.32.222 8080
```

**Respuesta esperada:**
```
OK REGISTERED test-01
```

### Test 2: Envío de medición

```bash
echo "MEASURE test-01 25.5 2026-04-14T15:35:00Z" | nc 54.242.32.222 8080
```

**Respuesta esperada:**
```
OK
```

---

## 📋 PASO 9: BACKUPS Y MANTENIMIENTO

### Hacer backup de logs

```bash
# Crear directorio de backup
mkdir -p ~/backups
tar -czf ~/backups/iot-backup-$(date +%Y%m%d-%H%M%S).tar.gz *.log
```

### Reiniciar servicios

```bash
# Matar todos los procesos
pkill -f "server\|python3" 

# Volver a ejecutar (desde paso 5)
./server 8080 server.log &
python3 auth_service.py &
python3 sensor_client.py localhost 8080 &
python3 web_interface.py &
```

---

## 🚨 TROUBLESHOOTING

### Problema: "Connection timed out" (error 110)

**Causa:** El servidor no está escuchando en el puerto 8080

**Solución:**
```bash
# Verificar que el servidor está corriendo
ps aux | grep " ./server"

# Verificar puerto
netstat -tuln | grep 8080

# Si no está, reiniciarlo:
cd ~/Codigo
./server 8080 server.log &
```

### Problema: "Python: Module not found"

**Causa:** Flask no está instalado

**Solución:**
```bash
pip3 install --break-system-packages flask
```

### Problema: "Permission denied" al compilar

**Causa:** Permisos insuficientes

**Solución:**
```bash
# Cambiar permisos
chmod +x build.sh
chmod +x server.cpp

# O usar sudo
sudo bash build.sh
```

---

## 📞 SOPORTE ADICIONAL

- **PROTOCOLO.md** - Especificación completa del protocolo TCP
- **README.md** - Guía de usuario general
- **DESPLIEGUE_AWS.md** - Instrucciones detalladas de AWS
- **CHECKLIST_FINAL.md** - Verificación de requisitos del proyecto

---

## ✨ ¡LISTO PARA SUSTENTACIÓN!

Una vez que todos los servicios estén ejecutándose y verificados:

✅ Proyecto compilado y ejecutándose en AWS  
✅ Todos los puertos escuchando  
✅ Logs muestran actividad normal  
✅ Interfaz web accesible  
✅ Protocolo funcionando correctamente  

**Tu proyecto está listo para presentación.**

