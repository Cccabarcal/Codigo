# Configuración DNS - DuckDNS

## 📍 Introducción

Para cumplir con el requisito de **"resolución de nombres"** sin costos adicionales, utilizamos **DuckDNS**, un servicio DNS dinámico completamente gratuito que permite acceder a la instancia EC2 mediante un dominio en lugar de una IP pública.

---

## ⚠️ Nota sobre la Configuración Actual

**Ya está configurado y en uso:**
- El desarrollador ya utiliza **DuckDNS** para su servidor en casa
- El **token de DuckDNS** está cargado en un **Docker container**
- El dominio `proyectoiota.duckdns.org` está activo y funcionando
- La actualización automática de IP se ejecuta en el container Docker

Para esta instancia en **AWS EC2**, se sigue el mismo modelo de configuración (pasos 1-3 para obtener el token, luego scripts de actualización).

---

## 🚀 Pasos de Configuración

### Paso 1: Registrarse en DuckDNS

1. Acceder a https://www.duckdns.org
2. Hacer clic en **"Sign in"**
3. Seleccionar proveedor de login (Google, GitHub, etc.)
4. Autorizar acceso

### Paso 2: Crear Subdominio

1. En la página principal, escribir un nombre único para tu subdominio
   - Ejemplo: `miproyectoiot`
2. Hacer clic en **"add domain"**
3. El sistema genera automáticamente: **`miproyectoiot.duckdns.org`**

### Paso 3: Obtener Token de API

1. En tu panel de DuckDNS, ir a **"Domains"**
2. Encontrar tu dominio y copiar el **Token** (cadena larga de caracteres)
3. Guardar este token, lo necesitarás para actualizar la IP automáticamente

---

## 🔧 Configuración en AWS EC2

### Script de Actualización de IP

Este script actualiza automáticamente la IP pública de tu instancia EC2 en el registro de DuckDNS.

**En tu máquina local, crear el archivo:**

```bash
cat > update_dns.sh << 'EOF'
#!/bin/bash

# Configuración
DUCKDNS_TOKEN="XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX"
DUCKDNS_DOMAIN="miproyectoiot"

# Obtener IP pública de la instancia EC2
IP_PUBLICA=$(curl -s http://169.254.169.254/latest/meta-data/public-ipv4)

# Actualizar en DuckDNS
RESPONSE=$(curl -s "https://www.duckdns.org/update?domains=${DUCKDNS_DOMAIN}&token=${DUCKDNS_TOKEN}&ip=${IP_PUBLICA}")

# Log
TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')
echo "[$TIMESTAMP] IP actualizada: $IP_PUBLICA | Respuesta: $RESPONSE"

EOF

chmod +x update_dns.sh
```

**Subir el script a EC2:**

```bash
scp -i ~/.ssh/iot-key.pem update_dns.sh ubuntu@<PUBLIC_IP>:~/Codigo/
```

### Ejecutar al Iniciar EC2

**En la instancia EC2, añadir al usuario ubuntu una tarea cron:**

```bash
ssh -i ~/.ssh/iot-key.pem ubuntu@<PUBLIC_IP>

# Crear directorio de scripts si no existe
mkdir -p /home/ubuntu/Codigo

# Editar crontab
crontab -e

# Añadir esta línea al final (ejecuta cada 5 minutos)
*/5 * * * * /home/ubuntu/Codigo/update_dns.sh >> /home/ubuntu/Codigo/dns_update.log 2>&1
```

**O ejecutar manualmente antes de iniciar servicios:**

```bash
cd /home/ubuntu/Codigo
./update_dns.sh
```

---

## 🌐 Configuración en el Código

### Variables de Entorno

Actualizar los scripts Python para usar el dominio de DuckDNS:

**En `sensor_client.py`:**

```python
import os
import socket

# Usar variable de entorno o DuckDNS por defecto
IOT_SERVER_HOST = os.getenv('IOT_SERVER_HOST', 'miproyectoiot.duckdns.org')
IOT_SERVER_PORT = int(os.getenv('IOT_SERVER_PORT', '8080'))

# Resolver el nombre
try:
    server_ip = socket.gethostbyname(IOT_SERVER_HOST)
    logger.info(f"Servidor resuelto: {IOT_SERVER_HOST} → {server_ip}")
except socket.gaierror as e:
    logger.error(f"Error resolviendo {IOT_SERVER_HOST}: {e}")
    # Continuar con el nombre original
    server_ip = IOT_SERVER_HOST
```

**En `web_interface.py`:**

```python
import os
import socket

# Configuración del servidor IoT
IOT_SERVER_HOST = os.getenv('IOT_SERVER_HOST', 'miproyectoiot.duckdns.org')
IOT_SERVER_PORT = int(os.getenv('IOT_SERVER_PORT', '8080'))
AUTH_SERVER_HOST = os.getenv('AUTH_SERVER_HOST', 'localhost')
AUTH_SERVER_PORT = int(os.getenv('AUTH_SERVER_PORT', '9000'))

# Resolver nombres
try:
    iot_ip = socket.gethostbyname(IOT_SERVER_HOST)
    logger.info(f"IoT Server resuelto: {IOT_SERVER_HOST} → {iot_ip}")
except socket.gaierror as e:
    logger.error(f"Error resolviendo {IOT_SERVER_HOST}: {e}")
```

---

## ✅ Verificación

### Verificar que DuckDNS está actualizado:

```bash
# Desde tu máquina local
nslookup miproyectoiot.duckdns.org

# Debería mostrar la IP pública de tu EC2
# Server: 8.8.8.8
# Address: 8.8.8.8#53
# 
# Non-authoritative answer:
# Name: miproyectoiot.duckdns.org
# Address: 54.242.32.222
```

### Ping al dominio:

```bash
ping miproyectoiot.duckdns.org
```

### Probar conectividad a los servicios:

```bash
# Web Interface
curl -I http://miproyectoiot.duckdns.org:5000

# IoT Server (debe conectar en puerto 8080)
nc -zv miproyectoiot.duckdns.org 8080

# Auth Service
nc -zv miproyectoiot.duckdns.org 9000
```

---

## 🎯 Acceso Completo del Sistema

Una vez configurado DuckDNS, accede desde cualquier lugar:

**Interfaz Web:**
```
http://miproyectoiot.duckdns.org:5000
```

**Credenciales:**
- Usuario: `carlos`
- Contraseña: `password123`

**Clientes Sensor (conectarse desde otra máquina):**
```bash
python3 sensor_client.py miproyectoiot.duckdns.org 8080
```

**Cliente Operador (Tkinter - máquina local con GUI):**
```bash
python3 operator_client.py miproyectoiot.duckdns.org 8080
```

---

## 📋 Resumen de Valores

| Parámetro | Valor |
|-----------|-------|
| **Servicio DNS** | DuckDNS |
| **Dominio** | `miproyectoiot.duckdns.org` |
| **Token** | Obtenido en https://www.duckdns.org |
| **IP Pública EC2** | Actualizada automáticamente cada 5 minutos |
| **Web Interface** | http://miproyectoiot.duckdns.org:5000 |
| **IoT Server** | miproyectoiot.duckdns.org:8080 |
| **Auth Service** | miproyectoiot.duckdns.org:9000 |

---

## ⚠️ Troubleshooting

### "Name or service not known"

```bash
# Verificar que DuckDNS tiene la IP actualizada
./update_dns.sh

# Esperar 30 segundos
sleep 30

# Verificar DNS
nslookup miproyectoiot.duckdns.org
```

### Respuesta "OK" en update_dns.sh pero no funciona

```bash
# Verificar IP pública actual en EC2
curl http://169.254.169.254/latest/meta-data/public-ipv4

# Verificar en DuckDNS (abrir navegador)
https://www.duckdns.org
# Ver si la IP en el panel coincide con la IP pública de EC2
```

### Cambió la IP pública de EC2

DuckDNS se actualiza automáticamente cada 5 minutos (si está en crontab).

Para actualizar inmediatamente:
```bash
cd /home/ubuntu/Codigo
./update_dns.sh
```

---

## 🔒 Seguridad

**⚠️ IMPORTANTE:** Nunca compartir el token de DuckDNS públicamente.

- El token permite modificar tu registro DNS
- No incluirlo en commits de GitHub
- Usar variables de entorno si es necesario

---

## 📌 Para la Sustentación

Durante la sustentación, demostrarás:

1. ✅ Resolución de nombres funcionando
2. ✅ Acceso mediante dominio (no IP)
3. ✅ Interfaz web accesible en `miproyectoiot.duckdns.org:5000`
4. ✅ Sistema completo operativo en AWS con DNS

Ejemplo de demostración:

```bash
# 1. Mostrar que DNS funciona
nslookup miproyectoiot.duckdns.org

# 2. Abrir navegador
# http://miproyectoiot.duckdns.org:5000

# 3. Iniciar sesión con carlos/password123

# 4. Mostrar sensores activos
```
