#!/usr/bin/env python3
"""
Interfaz Web para Sistema IoT
Servidor Flask que permite visualizar sensores desde navegador
Uso: python3 web_interface.py
"""

from flask import Flask, render_template, request, jsonify, session
import socket
import threading
import json
import logging
from datetime import datetime
import os

# ──────────────────────────────────────────────────────────────────────────────
# CONFIGURACIÓN
# ──────────────────────────────────────────────────────────────────────────────

app = Flask(__name__)
app.secret_key = "iot_system_secret_key_2026"

# Configuración desde variables de entorno (sin IPs hardcodeadas)
WEB_PORT = int(os.getenv("WEB_PORT", "5000"))
WEB_HOST = os.getenv("WEB_HOST", "0.0.0.0")
IOT_SERVER_HOST = os.getenv("IOT_SERVER_HOST", "localhost")
IOT_SERVER_PORT = int(os.getenv("IOT_SERVER_PORT", "8080"))
AUTH_SERVER_HOST = os.getenv("AUTH_SERVER_HOST", "localhost")
AUTH_SERVER_PORT = int(os.getenv("AUTH_SERVER_PORT", "9000"))
BUFFER_SIZE = 1024

# ──────────────────────────────────────────────────────────────────────────────
# SETUP DE LOGGING
# ──────────────────────────────────────────────────────────────────────────────

logging.basicConfig(
    level=logging.INFO,
    format='[%(asctime)s] [%(levelname)s] %(message)s',
    handlers=[
        logging.FileHandler('web_interface.log'),
        logging.StreamHandler()
    ]
)
logger = logging.getLogger()

# ──────────────────────────────────────────────────────────────────────────────
# CLIENTE IOT (reutilizable)
# ──────────────────────────────────────────────────────────────────────────────

class IotClient:
    """Cliente para conectar al servidor IoT"""
    
    def __init__(self, host=IOT_SERVER_HOST, port=IOT_SERVER_PORT):
        self.host = host
        self.port = port
        self.sock = None
    
    def connect(self):
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.connect((self.host, self.port))
            return True
        except Exception as e:
            logger.error(f"Error conectando a IoT: {str(e)}")
            return False
    
    def login(self, username, password):
        try:
            msg = f"LOGIN {username} {password}\n"
            self.sock.send(msg.encode('utf-8'))
            response = self.sock.recv(BUFFER_SIZE).decode('utf-8').strip()
            return response.startswith("OK")
        except Exception as e:
            logger.error(f"Error en login: {str(e)}")
            return False
    
    def list_sensors(self):
        try:
            msg = "LIST SENSORS\n"
            self.sock.send(msg.encode('utf-8'))
            response = self.sock.recv(BUFFER_SIZE).decode('utf-8').strip()
            
            if response.startswith("SENSORS"):
                sensor_data = response.replace("SENSORS ", "")
                sensors = {}
                if sensor_data:
                    for pair in sensor_data.split(","):
                        try:
                            sensor_id, sensor_type = pair.split(":")
                            sensors[sensor_id] = sensor_type
                        except:
                            pass
                return sensors
            return {}
        except Exception as e:
            logger.error(f"Error listando sensores: {str(e)}")
            return {}
    
    def get_measurements(self, sensor_id, count=10):
        try:
            msg = f"GET MEASURE {sensor_id} {count}\n"
            self.sock.send(msg.encode('utf-8'))
            
            measurements = []
            for _ in range(count):
                try:
                    response = self.sock.recv(BUFFER_SIZE).decode('utf-8').strip()
                    if response.startswith("DATA"):
                        parts = response.split()
                        if len(parts) >= 4:
                            measurements.append({
                                'sensor_id': parts[1],
                                'value': float(parts[2]),
                                'timestamp': parts[3]
                            })
                    else:
                        break
                except:
                    break
            
            return measurements
        except Exception as e:
            logger.error(f"Error obteniendo mediciones: {str(e)}")
            return []
    
    def logout(self):
        try:
            msg = "LOGOUT\n"
            self.sock.send(msg.encode('utf-8'))
            self.sock.close()
        except:
            pass

# ──────────────────────────────────────────────────────────────────────────────
# RUTAS
# ──────────────────────────────────────────────────────────────────────────────

@app.route('/')
def index():
    """Página de inicio"""
    return render_template('index.html')

@app.route('/login', methods=['POST'])
def login():
    """Endpoint de login"""
    data = request.json
    username = data.get('username')
    password = data.get('password')
    
    if not username or not password:
        return jsonify({'success': False, 'message': 'Usuario y contraseña requeridos'}), 400
    
    client = IotClient()
    if not client.connect():
        return jsonify({'success': False, 'message': 'No se pudo conectar al servidor IoT'}), 500
    
    if not client.login(username, password):
        client.logout()
        return jsonify({'success': False, 'message': 'Credenciales inválidas'}), 401
    
    # Guardar sesión (incluyendo contraseña para re-autenticaciones)
    session['username'] = username
    session['password'] = password
    session['logged_in'] = True
    logger.info(f"Login exitoso: {username}")
    
    return jsonify({'success': True, 'message': f'Bienvenido {username}'}), 200

@app.route('/sensors', methods=['GET'])
def get_sensors():
    """Obtiene lista de sensores"""
    if not session.get('logged_in'):
        return jsonify({'success': False, 'message': 'No autenticado'}), 401
    
    username = session.get('username')
    password = session.get('password')
    
    client = IotClient()
    if not client.connect():
        return jsonify({'success': False, 'message': 'No se pudo conectar al servidor IoT'}), 500
    
    if not client.login(username, password):
        client.logout()
        return jsonify({'success': False, 'message': 'Error de autenticación'}), 401
    
    sensors = client.list_sensors()
    client.logout()
    
    if sensors:
        sensor_list = [{'id': k, 'type': v} for k, v in sensors.items()]
        return jsonify({'success': True, 'sensors': sensor_list}), 200
    
    return jsonify({'success': False, 'message': 'Sin sensores disponibles'}), 404

@app.route('/measurements/<sensor_id>', methods=['GET'])
def get_measurement(sensor_id):
    """Obtiene mediciones de un sensor"""
    if not session.get('logged_in'):
        return jsonify({'success': False, 'message': 'No autenticado'}), 401
    
    username = session.get('username')
    password = session.get('password')
    count = request.args.get('count', 10, type=int)
    
    client = IotClient()
    if not client.connect():
        return jsonify({'success': False, 'message': 'No se pudo conectar al servidor IoT'}), 500
    
    if not client.login(username, password):
        client.logout()
        return jsonify({'success': False, 'message': 'Error de autenticación'}), 401
    
    measurements = client.get_measurements(sensor_id, count)
    client.logout()
    
    return jsonify({'success': True, 'measurements': measurements}), 200

@app.route('/logout', methods=['POST'])
def logout():
    """Logout"""
    session.clear()
    logger.info("Logout exitoso")
    return jsonify({'success': True}), 200

@app.route('/health', methods=['GET'])
def health():
    """Health check"""
    return jsonify({'status': 'ok', 'timestamp': datetime.now().isoformat()}), 200

if __name__ == '__main__':
    logger.info("=" * 70)
    logger.info(f"Interfaz Web iniciando en {WEB_HOST}:{WEB_PORT}")
    logger.info(f"Servidor IoT: {IOT_SERVER_HOST}:{IOT_SERVER_PORT}")
    logger.info("=" * 70)
    
    app.run(host=WEB_HOST, port=WEB_PORT, debug=False)
