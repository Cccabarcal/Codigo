# 🚀 Despliegue en AWS - Guía Paso a Paso

## 🌍 **Acceso Remoto**

**🔗 Dominio DNS (DuckDNS):** [http://proyectoiota.duckdns.org:5000/](http://proyectoiota.duckdns.org:5000/)

**Credenciales de acceso:**
- Usuario: `carlos`
- Contraseña: `password123`

---

## Paso 1: Crear Instancia EC2

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

## Paso 2: Conectar a la Instancia por SSH

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

## Paso 3: Actualizar Sistema e Instalar Dependencias

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

## Paso 4: Clonar el Repositorio

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

## Paso 5: Compilar el Servidor C++

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

## Paso 6: Ejecutar los Servicios (Abre 4 Terminales SSH)

Abre **4 conexiones SSH diferentes** a tu instancia EC2. En cada una, ejecuta:

---

### **Terminal SSH 1: Servidor IoT Central (C++)**

```bash
cd /home/ubuntu/Codigo
./server 8080 server.log
```

**Salida esperada:**
```
[2026-04-20 01:33:00] [SYSTEM] 0.0.0.0:8080 | Servidor escuchando en puerto 8080
[2026-04-20 01:33:00] [CONN] Esperando conexiones...
```

**⏸️ DEJAR ESTA TERMINAL ABIERTA Y SIN CERRAR**

---

### **Terminal SSH 2: Servicio de Autenticación (Python)**

```bash
cd /home/ubuntu/Codigo
python3 auth_service.py
```

**Salida esperada:**
```
[2026-04-20 01:33:01] [INFO] Escuchando en 0.0.0.0:9000
[2026-04-20 01:33:01] [INFO] Servicio de Autenticación listo
```

**⏸️ DEJAR ESTA TERMINAL ABIERTA Y SIN CERRAR**

---

### **Terminal SSH 3: Clientes Sensor (Python)**

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

**⏸️ DEJAR ESTA TERMINAL ABIERTA Y SIN CERRAR**

---

### **Terminal SSH 4: Interfaz Web Flask (Python)**

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

**⏸️ DEJAR ESTA TERMINAL ABIERTA Y SIN CERRAR**

---

## Paso 7: Verificar que Todo Funciona (Terminal SSH 5)

Abre una **5ª conexión SSH** (sin cerrar las anteriores):

```bash
ssh -i ~/.ssh/iot-key.pem ubuntu@<PUBLIC_IP>
```

**Ejecuta estos comandos de verificación:**

---

### Verificar Procesos Activos

```bash
ps aux | grep -E "server|python3" | grep -v grep
```

Debería mostrar:
```
ubuntu  1234  0.1  0.5 123456 45678 ?  Sl  01:33  0:01 ./server 8080 server.log
ubuntu  1235  0.0  1.2 234567 56789 ?  S   01:33  0:00 python3 auth_service.py
ubuntu  1236  0.0  1.1 234567 56789 ?  S   01:33  0:00 python3 sensor_client.py
ubuntu  1237  0.0  1.3 234567 56789 ?  S   01:33  0:00 python3 web_interface.py
```

---

### Verificar Puertos Escuchando

```bash
netstat -tuln | grep -E "5000|8080|9000"
```

Debería mostrar:
```
tcp    0    0 0.0.0.0:5000    0.0.0.0:*    LISTEN
tcp    0    0 0.0.0.0:8080    0.0.0.0:*    LISTEN
tcp    0    0 0.0.0.0:9000    0.0.0.0:*    LISTEN
```

---

### Verificar Sensores Registrados

```bash
grep "REGISTER SENSOR" /home/ubuntu/Codigo/server.log | head -5
```

Debería mostrar (6 sensores):
```
[2026-04-20 01:33:02] [DEBUG] temp-01 REGISTER SENSOR
[2026-04-20 01:33:02] [DEBUG] temp-02 REGISTER SENSOR
[2026-04-20 01:33:03] [DEBUG] humid-01 REGISTER SENSOR
[2026-04-20 01:33:03] [DEBUG] press-01 REGISTER SENSOR
[2026-04-20 01:33:04] [DEBUG] vibra-01 REGISTER SENSOR
[2026-04-20 01:33:04] [DEBUG] energy-01 REGISTER SENSOR
```

---

### Verificar Mediciones Llegando

```bash
grep "MEASURE" /home/ubuntu/Codigo/server.log | head -3
```

Debería mostrar:
```
[2026-04-20 01:33:05] [DEBUG] temp-01 MEASURE 23.5
[2026-04-20 01:33:05] [DEBUG] temp-02 MEASURE 24.1
[2026-04-20 01:33:06] [DEBUG] humid-01 MEASURE 65.3
```

---

## Paso 8: Acceder a la Interfaz Web

Desde **tu navegador en máquina local**:

```
http://<PUBLIC_IP>:5000
```

**Ejemplo:**
```
http://54.242.32.222:5000
```

**Credenciales de login:**
- Usuario: `carlos`
- Contraseña: `password123`

**Verás:**
- ✅ Tabla con 6 sensores activos
- ✅ Mediciones en tiempo real actualizándose
- ✅ Alertas si algún valor está fuera de rango
- ✅ Historial de mediciones por sensor

---

## Paso 9: Monitoreo en Vivo de Logs

En **Terminal 5**, puedes ver logs en tiempo real:

```bash
# Ver logs del servidor (nuevas líneas)
tail -f /home/ubuntu/Codigo/server.log
```

O abre nuevas terminales para cada log:

```bash
# Terminal 6: Logs de sensores
tail -f /home/ubuntu/Codigo/sensor_client.log

# Terminal 7: Logs de auth
tail -f /home/ubuntu/Codigo/auth_service.log

# Terminal 8: Logs de web
tail -f /home/ubuntu/Codigo/web_interface.log
```

---

## Paso 10: Configurar DNS con DuckDNS (RECOMENDADO - GRATUITO)

**DuckDNS** permite acceder a tu servidor usando un dominio en lugar de IP, **completamente gratis**.

### 10.1: Registrarse en DuckDNS (en tu navegador)

```bash
# Ir a https://www.duckdns.org
# 1. Hacer clic en "Sign in" (usar Google/GitHub)
# 2. Crear subdominio: proyectoiota
# 3. Copiar el TOKEN (cadena larga de caracteres)
```

**✅ YA CONFIGURADO:**
- Dominio: `proyectoiota.duckdns.org`
- Acceso: http://proyectoiota.duckdns.org:5000/

---

### 10.2: Crear Script de Actualización DNS (en tu máquina local)

```bash
cat > update_dns.sh << 'EOF'
#!/bin/bash
DUCKDNS_TOKEN="tu_token_aqui"
DUCKDNS_DOMAIN="proyectoiota"
IP_PUBLICA=$(curl -s http://169.254.169.254/latest/meta-data/public-ipv4)
curl -s "https://www.duckdns.org/update?domains=${DUCKDNS_DOMAIN}&token=${DUCKDNS_TOKEN}&ip=${IP_PUBLICA}"
echo "[$(date)] IP actualizada: $IP_PUBLICA"
EOF

chmod +x update_dns.sh
```

---

### 10.3: Subir Script a EC2 (desde tu máquina local)

```bash
# En tu máquina local
scp -i ~/.ssh/iot-key.pem update_dns.sh ubuntu@<PUBLIC_IP>:~/Codigo/
```

---

### 10.4: Ejecutar Script en EC2 (Terminal SSH 5)

```bash
# En Terminal SSH 5 (la que usas para verificar)
ssh -i ~/.ssh/iot-key.pem ubuntu@<PUBLIC_IP>

cd ~/Codigo
./update_dns.sh
```

**Salida esperada:**
```
[2026-04-20 01:45:00] IP actualizada: 54.242.32.222
```

---

### 10.5: Verificar que DNS Está Resuelto (Terminal SSH 5)

```bash
# Esperar 5-10 segundos
sleep 10

# Verificar DNS
nslookup proyectoiota.duckdns.org
```

**Salida esperada:**
```
Server:    127.0.0.53
Address:   127.0.0.53#53

Non-authoritative answer:
Name:   proyectoiota.duckdns.org
Address: 54.242.32.222
```

**Si muestra la IP pública de tu EC2, ¡DNS está funcionando!** ✅

---

### 10.6: Configurar Actualización Automática con Cron (Terminal SSH 5)

```bash
# En EC2, editar crontab
crontab -e

# Añadir esta línea al final:
*/5 * * * * /home/ubuntu/Codigo/update_dns.sh >> /home/ubuntu/Codigo/dns_update.log 2>&1
```

Esto actualiza automáticamente la IP cada 5 minutos.

---

### 10.7: Acceder por Dominio (desde tu navegador)

```
http://proyectoiota.duckdns.org:5000
```

**Debería mostrar:**
- ✅ Página de login
- ✅ Login con carlos/password123
- ✅ 6 sensores activos
- ✅ Mediciones en tiempo real

---

## Paso 11: Verificación Completa del DNS (Terminal SSH 5)

```bash
# Desde tu máquina local, verificar DNS
nslookup proyectoiota.duckdns.org
```

```bash
# Probar conectividad HTTP
curl -I http://proyectoiota.duckdns.org:5000
```

**Salida esperada:**
```
HTTP/1.1 200 OK
Content-Type: text/html; charset=utf-8
```

```bash
# Probar puerto 8080 (IoT Server)
nc -zv proyectoiota.duckdns.org 8080
```

**Salida esperada:**
```
Connection to proyectoiota.duckdns.org 8080 port [tcp/*] succeeded!
```

---

## ✅ Sistema Completamente Funcional

Si llegaste aquí y todo funciona:

| Verificación | Estado |
|---|---|
| ✅ Terminal SSH 1: Servidor corriendo | `ps aux \| grep server` |
| ✅ Terminal SSH 2: Auth funcionando | `ps aux \| grep auth_service` |
| ✅ Terminal SSH 3: Sensores enviando | Ver logs en `server.log` |
| ✅ Terminal SSH 4: Web respondiendo | `curl -I http://localhost:5000` |
| ✅ DNS resolviendo | `nslookup proyectoiota.duckdns.org` |
| ✅ Acceso remoto | http://proyectoiota.duckdns.org:5000 |

**🎉 ¡Proyecto completamente deployado en AWS!** 🚀

---

## Troubleshooting en AWS

### ❌ **Problema: "Connection refused" en Terminal SSH 1**

**Solución:**

```bash
# Verificar que el servidor está corriendo
ps aux | grep server

# Si no está corriendo, iniciar manualmente
cd /home/ubuntu/Codigo
./server 8080 server.log
```

---

### ❌ **Problema: "Address already in use" al compilar/ejecutar**

**Solución:**

```bash
# Encontrar proceso en puerto 8080
lsof -i :8080

# O en Ubuntu moderno:
ss -tulpn | grep :8080

# Matar proceso anterior
pkill -f "server"
pkill -f "python3"

# Esperar 2 segundos
sleep 2

# Reiniciar servicios
cd /home/ubuntu/Codigo
./server 8080 server.log &
```

---

### ❌ **Problema: "No module named 'flask'" o error de módulos Python**

**Solución:**

```bash
# Instalar módulos faltantes
sudo apt install -y python3-flask python3-requests

# O con pip
pip3 install flask requests
```

---

### ❌ **Problema: Compilación falla con errores de C++**

**Solución:**

```bash
# Verificar que g++ está instalado
g++ --version

# Si no está, instalar
sudo apt install -y build-essential

# Recompilar
cd /home/ubuntu/Codigo
g++ -std=c++17 -pthread -o server server.cpp

# Si sigue fallando, compilar con debug:
g++ -std=c++17 -pthread -g -o server server.cpp 2>&1 | tee compile_errors.txt
```

---

### ❌ **Problema: DNS no resuelve ("Name or service not known")**

**Solución:**

```bash
# Actualizar IP en DuckDNS
cd /home/ubuntu/Codigo
./update_dns.sh

# Esperar 5-10 segundos
sleep 10

# Verificar DNS
nslookup proyectoiota.duckdns.org

# Si sigue sin funcionar, verificar token en update_dns.sh
cat update_dns.sh | grep DUCKDNS_TOKEN
```

---

### ❌ **Problema: Los sensores no aparecen en la interfaz web**

**Solución:**

```bash
# Verificar que sensor_client.py está corriendo
ps aux | grep sensor_client

# Ver si hay errores en los logs
tail -f /home/ubuntu/Codigo/sensor_client.log

# Si no hay logs, iniciar sensor_client en Terminal SSH 3
cd /home/ubuntu/Codigo
python3 sensor_client.py localhost 8080

# Recargar web (Ctrl+Shift+R en navegador)
```

---

### ❌ **Problema: Firewall/Security Group bloquea acceso desde navegador**

**Solución:**

1. **En AWS Console:**
   - Ir a EC2 → Instancias → Seleccionar instancia
   - Ir a "Security groups" → Click en el group
   - Click en "Edit inbound rules"
   - Agregar:
     - **Type:** Custom TCP
     - **Port:** 5000
     - **Source:** 0.0.0.0/0 (o tu IP específica)
   - Guardar

2. **O desde CLI:**
   ```bash
   aws ec2 authorize-security-group-ingress \
     --group-id sg-xxxxxxxx \
     --protocol tcp \
     --port 5000 \
     --cidr 0.0.0.0/0
   ```

---

### ❌ **Problema: La IP de EC2 cambió (si detuviste y reiniciaste instancia)**

**Solución:**

```bash
# El script update_dns.sh detecta automáticamente la nueva IP
# Solo ejecuta:
cd /home/ubuntu/Codigo
./update_dns.sh

# Espera 10 segundos
sleep 10

# Verifica que la IP se actualizó
nslookup proyectoiota.duckdns.org
```

---

### ❌ **Problema: Todos los procesos corriendo pero sin respuesta desde navegador**

**Solución:**

```bash
# Verificar todos los puertos
netstat -tuln

# Debe mostrar:
# tcp  0  0 0.0.0.0:5000   0.0.0.0:*  LISTEN
# tcp  0  0 0.0.0.0:8080   0.0.0.0:*  LISTEN
# tcp  0  0 0.0.0.0:9000   0.0.0.0:*  LISTEN

# Probar desde la misma instancia EC2
curl -I http://localhost:5000/

# Si no responde, ver logs:
tail -f /home/ubuntu/Codigo/web_interface.log

# Reiniciar web_interface.py en Terminal SSH 4
pkill -f web_interface
python3 web_interface.py
```

---

### ✅ **Verificación Rápida de Todo**

Si algo no funciona, ejecuta esto en **Terminal SSH 5**:

```bash
#!/bin/bash
echo "=== PROCESOS ACTIVOS ==="
ps aux | grep -E "server|python3" | grep -v grep

echo -e "\n=== PUERTOS ESCUCHANDO ==="
netstat -tuln | grep -E "5000|8080|9000"

echo -e "\n=== SENSORES REGISTRADOS ==="
grep "REGISTER SENSOR" server.log | wc -l

echo -e "\n=== ÚLTIMO ERROR EN SERVIDOR ==="
grep "ERROR" server.log | tail -1

echo -e "\n=== ESTADO DNS ==="
nslookup proyectoiota.duckdns.org 2>&1 | grep -A2 "Address:"
```

Ejecuta como script:
```bash
chmod +x verify_system.sh
./verify_system.sh
```

