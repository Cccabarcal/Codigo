# 🚀 Guía de Despliegue en AWS

## Overview

Esta guía cubre el despliegue completo del Sistema IoT en Amazon Web Services (AWS) usando:
- **EC2**: Instancia para ejecutar servidor y servicios
- **Route 53**: DNS para acceso por nombre de dominio
- **Security Groups**: Firewall para control de puertos

---

## Paso 1: Crear Instancia EC2

### 1.1 Inicializar Launch Instance

1. Acceder a [AWS Console](https://console.aws.amazon.com)
2. Navegar a **EC2 → Instances → Launch Instance**
3. Nombre: `iot-server` (o el que prefieras)

### 1.2 Seleccionar AMI

**Opción 1: Amazon Linux 2** (recomendado)
- Free tier elegible
- Optimizado para AWS
- Seleccionar la más reciente

**Opción 2: Ubuntu 20.04 LTS**
- Más familiar para desarrolladores
- Full support
- Requiere verificación de free tier

### 1.3 Configurar Instancia

- **Instance Type**: `t2.micro` (Free tier)
- **Network**: Default VPC
- **Subnet**: Default
- **Auto-assign Public IP**: Enable
- **IAM Role**: None (para este proyecto)

### 1.4 Storage

- **Size**: 20 GB (default, suficiente)
- **Volume Type**: gp2 (default)
- **Delete on Termination**: Yes

### 1.5 Security Group (⚠️ CRÍTICO)

Crear nuevo security group: `iot-services-sg`

**Inbound Rules**:
```
Type: SSH
Port: 22
Source: <TU_IP> (RESTRICTO A TU IP)

Type: HTTP
Port: 80
Source: 0.0.0.0/0

Type: HTTPS
Port: 443
Source: 0.0.0.0/0

Type: Custom TCP
Port: 8080 (IoT Server)
Source: 0.0.0.0/0

Type: Custom TCP
Port: 9000 (Auth Service)
Source: 0.0.0.0/0

Type: Custom TCP
Port: 5000 (Web Interface)
Source: 0.0.0.0/0
```

**Outbound Rules**:
- All traffic allowed (default)

### 1.6 Key Pair

1. Create new key pair
2. Name: `iot-key` (o el que prefieras)
3. Type: RSA
4. Format: .pem (Linux/macOS) o .ppk (PuTTY Windows)
5. **Guardar en lugar seguro**: `~/.ssh/iot-key.pem`

### 1.7 Launch Instance

Click "Launch Instance"

Esperar a que el estado cambie a **"Running"** (puede tomar 1-2 minutos)

---

## Paso 2: Conectarse a la Instancia

### En Linux/macOS

```bash
# Configurar permisos (IMPORTANTE)
chmod 400 ~/.ssh/iot-key.pem

# Conectar (obtener IP pública de AWS Console)
ssh -i ~/.ssh/iot-key.pem ec2-user@<PUBLIC_IP>
# Para Ubuntu:
# ssh -i ~/.ssh/iot-key.pem ubuntu@<PUBLIC_IP>
```

### En Windows (PowerShell)

```powershell
# Con AWS CLI
aws ec2-instance-connect open-remote-desktop-window --instance-id i-xxxxx

# O con PuTTY
# 1. Convertir key: PuTTYgen → Load .pem → Save private key .ppk
# 2. PuTTY → Host: ec2-user@<PUBLIC_IP> → Auth → Private key .ppk → Open
```

### Verificar Conexión

```bash
# Deberías ver algo como:
ec2-user@ip-172-31-0-xxx:~$

# O para Ubuntu:
ubuntu@ip-172-31-0-xxx:~$
```

---

## Paso 3: Preparar Servidor

### 3.1 Actualizar Sistema

```bash
# Amazon Linux 2
sudo yum update -y
sudo yum install -y gcc-c++ make kernel-devel

# Ubuntu
sudo apt update
sudo apt upgrade -y
sudo apt install -y build-essential
```

### 3.2 Instalar Python y Dependencias

```bash
# Amazon Linux 2
amazon-linux-extras install python3.8 -y
sudo yum install python3-pip -y

# Ubuntu (Python 3 ya está instalado por defecto)
sudo apt install python3-pip -y

# Verificar versión de Python
python3 --version
```

### 3.3 Instalar Flask

#### Opción A: Instalación Global (Rápido para desarrollo)

```bash
# Ubuntu 22.04+ requiere esta bandera (SOLO desarrollo)
pip3 install --break-system-packages flask

# Verificar
python3 -c "import flask; print(flask.__version__)"
```

#### Opción B: Virtual Environment (Recomendado para producción)

```bash
# Crear virtual environment
python3 -m venv venv

# Activar
source venv/bin/activate

# Instalar Flask
pip install flask

# Verificar
python3 -c "import flask; print(flask.__version__)"

# Deactivar (cuando termines)
# deactivate
```

**Nota**: Si usas virtual environment, actívalo cada vez que abras una nueva sesión:
```bash
source venv/bin/activate
```

### 3.4 Crear Directorio de Proyecto

```bash
mkdir -p /home/ec2-user/iot-project
cd /home/ec2-user/iot-project
```

---

## Paso 4: Subir Archivos

### Opción A: Usar SCP (recomendado)

```bash
# Desde tu máquina local (Windows PowerShell)
cd D:\Users\Cristian\Desktop\telematica\Codigo

# Subir TODOS los archivos (más fácil)
scp -i $env:USERPROFILE\.ssh\iot-key.pem -r * ubuntu@<PUBLIC_IP>:/home/ec2-user/iot-project/

# O subir archivos específicos (Linux/macOS)
scp -i ~/.ssh/iot-key.pem server.cpp logger.h protocol.h ubuntu@<PUBLIC_IP>:/home/ec2-user/iot-project/
scp -i ~/.ssh/iot-key.pem *.py ubuntu@<PUBLIC_IP>:/home/ec2-user/iot-project/
scp -i ~/.ssh/iot-key.pem users.json ubuntu@<PUBLIC_IP>:/home/ec2-user/iot-project/
scp -i ~/.ssh/iot-key.pem -r templates/ ubuntu@<PUBLIC_IP>:/home/ec2-user/iot-project/
```

**Verificar que se subieron**:
```bash
# En la instancia AWS
ubuntu@ip-172-31-44-89:~$ cd /home/ec2-user/iot-project/
ubuntu@ip-172-31-44-89:~/iot-project$ ls -la
# Debería mostrar: server.cpp, logger.h, protocol.h, *.py, users.json, templates/, etc.
```

### Opción B: Usar Git

```bash
# En la instancia
cd /home/ec2-user/iot-project
git clone <URL_REPOSITORIO>
cd Codigo  # Si el repo tiene estructura de carpetas
```

### Opción C: Crear manualmente con nano

```bash
# En la instancia, para cada archivo
nano server.cpp
# Copiar y pegar el contenido
# Ctrl+O, Enter, Ctrl+X para guardar
```

---

## Paso 5: Compilar y Probar

### En la Instancia EC2

```bash
# Compilar servidor
g++ -std=c++17 -pthread -o server server.cpp

# Verificar que compiló
ls -la server
```

---

## Paso 6: Ejecutar en Foreground (Testing)

### Terminal 1: Servidor IoT

```bash
./server 8080 server.log
# Debería mostrar:
# [2026-04-14 10:30:45] [SYSTEM] Servidor escuchando en puerto 8080
```

### Terminal 2: Auth Service

```bash
python3 auth_service.py
# Debería mostrar:
# [2026-04-14 10:30:46] [INFO] Escuchando en 0.0.0.0:9000
```

### Terminal 3: Sensores

```bash
python3 sensor_client.py localhost 8080
# Debería conectarse y empezar a enviar mediciones
```

### Terminal 4: Web Interface

```bash
python3 web_interface.py
# Debería mostrar:
# INFO Interfaz Web iniciando en 0.0.0.0:5000
```

### Verificar Conectividad

Desde otra máquina:
```bash
# Probar ports
curl http://<PUBLIC_IP>:5000
telnet <PUBLIC_IP> 8080  # Ctrl+] then quit para salir
```

---

## Paso 7: Ejecutar en Background (Producción)

### Opción A: Usar `nohup` + `screen`

```bash
# Terminal 1: Servidor
screen -S iot-server
./server 8080 server.log
# Ctrl+A D para desconectar

# Terminal 2: Auth
screen -S auth-service
python3 auth_service.py
# Ctrl+A D

# Terminal 3: Sensores
screen -S sensors
python3 sensor_client.py <PUBLIC_IP> 8080
# Ctrl+A D

# Terminal 4: Web
screen -S web
python3 web_interface.py
# Ctrl+A D

# Listar screens
screen -ls

# Reconectar a un screen
screen -r iot-server
```

### Opción B: Usar Systemd (Profesional)

```bash
# Crear service para servidor IoT
sudo bash -c 'cat > /etc/systemd/system/iot-server.service << EOF
[Unit]
Description=IoT Central Server
After=network.target

[Service]
Type=simple
User=ec2-user
WorkingDirectory=/home/ec2-user/iot-project
ExecStart=/home/ec2-user/iot-project/server 8080 server.log
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
EOF
'

# Habilitar y iniciar
sudo systemctl daemon-reload
sudo systemctl enable iot-server
sudo systemctl start iot-server
sudo systemctl status iot-server

# Ver logs
journalctl -u iot-server -f
```

Repetir para `auth_service.service`, `sensors.service`, `web.service`

---

## Paso 8: Configurar DNS (Route 53)

### 8.1 Registrar Dominio (si no lo tienes)

1. Ir a Route 53 → Registered domains
2. Buscar dominio disponible
3. Comprar (ej: `iot-monitor.com`)

### 8.2 Crear Hosted Zone

1. Route 53 → Hosted zones
2. Create hosted zone: `iot-monitor.com`
3. Copiar nameservers a tu registrador si es diferente

### 8.3 Crear Registros

#### Record A (Principal)

- Name: `iot-monitor.com`
- Type: A
- Value: `<PUBLIC_IP_EC2>`
- TTL: 300
- Click Create records

#### Subdomino para Web (Opcional)

- Name: `web.iot-monitor.com`
- Type: A
- Value: `<PUBLIC_IP_EC2>`
- TTL: 300
- Create records

---

## Paso 9: Certificado SSL (HTTPS) - Opcional pero Recomendado

### Usar Let's Encrypt con Certbot

```bash
# En la instancia, instalar certbot
sudo yum install python3-certbot -y  # Amazon Linux
sudo apt install certbot -y          # Ubuntu

# Obtener certificado
sudo certbot certonly --standalone -d iot-monitor.com

# Configurar Flask para usar certificado (en web_interface.py)
# Descomentar la última línea y cambiar:
# app.run(host='0.0.0.0', port=5000, ssl_context=('cert.pem', 'key.pem'))

# Por defecto certbot guarda en:
# /etc/letsencrypt/live/iot-monitor.com/fullchain.pem (cert.pem)
# /etc/letsencrypt/live/iot-monitor.com/privkey.pem (key.pem)
```

---

## Paso 10: Monitoreo y Logs

### Ver Logs en Tiempo Real

```bash
# Servidor
tail -f /home/ec2-user/iot-project/server.log

# Auth
tail -f /home/ec2-user/iot-project/auth_service.log

# Web
tail -f /home/ec2-user/iot-project/web_interface.log

# Sensores
tail -f /home/ec2-user/iot-project/sensor_client.log
```

### CloudWatch (Monitoreo Avanzado)

1. Ir a CloudWatch → Dashboards
2. Create dashboard → Add widgets
3. Seleccionar métricas de EC2
4. Monitorear CPU, red, disco

---

## Paso 11: Mantener Backup

### Crear Snapshot de EBS

1. EC2 → Volumes
2. Select volume de tu instancia
3. Create snapshot
4. Usar para recuperación ante fallas

### Backup Regular

```bash
# Script de backup
#!/bin/bash
BACKUP_DIR="/home/ec2-user/backups"
mkdir -p $BACKUP_DIR
tar -czf $BACKUP_DIR/iot-backup-$(date +%Y%m%d).tar.gz \
  /home/ec2-user/iot-project/*

# Ejecutar diariamente con cron
crontab -e
# Agregar: 0 2 * * * /home/ec2-user/backup.sh
```

---

## Costos Estimados

| Servicio | Cantidad | Precio Mensual |
|----------|----------|---------------|
| EC2 t2.micro | 1 | $0-7 (Free tier 1 año) |
| NAT Gateway | - | - |
| Data transfer | ~5GB | ~$0.50 |
| Route 53 | 1 zone | $0.50 |
| **Total** | | **~$1/mes** |

> Nota: Free tier AWS cubre 1 año de EC2 t2.micro + 5GB descarga/mes

---

## Troubleshooting

### Instancia no responde

```bash
# Verificar desde AWS Console
# EC2 → Instances → Status checks

# Reiniciar
# Instance State → Reboot instance
```

### Port 22 bloqueado

1. Verificar Security Group inbound rules
2. Asegurar que tu IP está en la whitelist
3. Probar desde IP diferente (si es dinámica)

### Aplicación se cuelga

```bash
# Matar procesos
ps aux | grep server
ps aux | grep python3

kill -9 <PID>

# Reiniciar
screen -r iot-server
./server 8080 server.log
```

### Espacio en disco lleno

```bash
# Verificar
df -h

# Limpiar logs antiguos
find . -name "*.log" -mtime +7 -delete

# Si es crítico, aumentar EBS volume (requiere downtime)
```

---

## Sustentación

Para demostrar en sustentación:

1. **Mostrar instancia corriendo** en AWS Console
2. **Conectar con SSH** y mostrar procesos:
   ```bash
   ps aux | grep "server\|python3"
   tail -f server.log
   ```
3. **Probar desde cliente**:
   ```bash
   curl http://<PUBLIC_IP>:5000
   python3 operator_client.py <PUBLIC_IP> 8080
   ```
4. **Mostrar dashboard web** en navegador
5. **Explicar la arquitectura** y flujo de datos

---

## Siguientes Pasos

- Implementar autoscaling (Auto Scaling Groups)
- Agregar RDS para base de datos persistente
- Usar CloudFront para CDN
- Implementar CI/CD con CodePipeline
- Agregar alertas con SNS/Email

---

Última actualización: 14 de Abril de 2026




cd /home/ec2-user/Codigo