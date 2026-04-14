#!/usr/bin/env python3
"""
Servicio de Autenticación para Sistema IoT
Escucha en puerto 9000, valida usuarios contra base de datos local
Protocolo: AUTH <user> <pass> -> OK <role> o ERROR 401
"""

import socket
import threading
import sys
import json
from pathlib import Path
import logging
from datetime import datetime

# ──────────────────────────────────────────────────────────────────────────────
# CONFIGURACIÓN
# ──────────────────────────────────────────────────────────────────────────────

AUTH_PORT = 9000
AUTH_HOST = "0.0.0.0"
LOG_FILE = "auth_service.log"
USERS_FILE = "users.json"
BUFFER_SIZE = 256

# ──────────────────────────────────────────────────────────────────────────────
# SETUP DE LOGGING
# ──────────────────────────────────────────────────────────────────────────────

def setup_logging():
    """Configura logging a archivo y consola"""
    logger = logging.getLogger()
    logger.setLevel(logging.DEBUG)
    
    # Formato
    formatter = logging.Formatter(
        '[%(asctime)s] [%(levelname)s] %(message)s',
        datefmt='%Y-%m-%d %H:%M:%S'
    )
    
    # Handler consola
    console_handler = logging.StreamHandler()
    console_handler.setFormatter(formatter)
    logger.addHandler(console_handler)
    
    # Handler archivo
    file_handler = logging.FileHandler(LOG_FILE)
    file_handler.setFormatter(formatter)
    logger.addHandler(file_handler)
    
    return logger

logger = setup_logging()

# ──────────────────────────────────────────────────────────────────────────────
# BASE DE DATOS DE USUARIOS
# ──────────────────────────────────────────────────────────────────────────────

def load_users():
    """Carga usuarios desde archivo JSON"""
    if Path(USERS_FILE).exists():
        with open(USERS_FILE, 'r') as f:
            return json.load(f)
    else:
        # Crear usuarios por defecto
        default_users = {
            "carlos": {"password": "password123", "role": "admin"},
            "maria": {"password": "maria456", "role": "operator"},
            "sensor_user": {"password": "sensor123", "role": "sensor"},
            "juan": {"password": "juan789", "role": "admin"},
        }
        save_users(default_users)
        return default_users

def save_users(users):
    """Guarda usuarios a archivo JSON"""
    with open(USERS_FILE, 'w') as f:
        json.dump(users, f, indent=2)

# ──────────────────────────────────────────────────────────────────────────────
# VALIDACIÓN
# ──────────────────────────────────────────────────────────────────────────────

def validate_credentials(username, password):
    """
    Valida credenciales de usuario.
    Retorna: (True, role) si válidas, (False, None) si inválidas
    """
    users = load_users()
    if username in users:
        user = users[username]
        if user['password'] == password:
            return (True, user['role'])
    return (False, None)

# ──────────────────────────────────────────────────────────────────────────────
# MANEJADOR DE CLIENTE
# ──────────────────────────────────────────────────────────────────────────────

def handle_client(client_socket, client_address):
    """Maneja una conexión de cliente"""
    client_ip, client_port = client_address
    logger.info(f"Nueva conexión: {client_ip}:{client_port}")
    
    try:
        # Recibir solicitud
        data = client_socket.recv(BUFFER_SIZE).decode('utf-8').strip()
        
        if not data:
            logger.warning(f"{client_ip}:{client_port} - Conexión vacía")
            client_socket.close()
            return
        
        logger.info(f"{client_ip}:{client_port} -> {data}")
        
        # Parsear comando AUTH
        tokens = data.split()
        if len(tokens) < 3 or tokens[0] != "AUTH":
            response = "ERROR 400 Bad request\n"
            logger.warning(f"{client_ip}:{client_port} - Formato inválido")
            client_socket.send(response.encode('utf-8'))
            client_socket.close()
            return
        
        username = tokens[1]
        password = tokens[2]
        
        # Validar
        valid, role = validate_credentials(username, password)
        
        if valid:
            response = f"OK {role}\n"
            logger.info(f"{client_ip}:{client_port} - Auth OK: {username} ({role})")
        else:
            response = "ERROR 401 Unauthorized\n"
            logger.warning(f"{client_ip}:{client_port} - Auth FAILED: {username}")
        
        # Enviar respuesta
        client_socket.send(response.encode('utf-8'))
        
    except Exception as e:
        logger.error(f"{client_ip}:{client_port} - Exception: {str(e)}")
    finally:
        client_socket.close()
        logger.info(f"{client_ip}:{client_port} - Conexión cerrada")

# ──────────────────────────────────────────────────────────────────────────────
# SERVIDOR PRINCIPAL
# ──────────────────────────────────────────────────────────────────────────────

def start_auth_server():
    """Inicia servidor de autenticación"""
    
    logger.info("=" * 70)
    logger.info(f"Servicio de Autenticación iniciando en {AUTH_HOST}:{AUTH_PORT}")
    logger.info("=" * 70)
    
    # Crear socket
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    
    try:
        # Bind y listen
        server_socket.bind((AUTH_HOST, AUTH_PORT))
        server_socket.listen(5)
        logger.info(f"Escuchando en {AUTH_HOST}:{AUTH_PORT}")
        
        # Loop principal
        while True:
            try:
                client_socket, client_address = server_socket.accept()
                
                # Manejar cliente en thread separado
                client_thread = threading.Thread(
                    target=handle_client,
                    args=(client_socket, client_address),
                    daemon=True
                )
                client_thread.start()
                
            except KeyboardInterrupt:
                logger.info("Interrupción detectada, cerrando...")
                break
            except Exception as e:
                logger.error(f"Error aceptando conexión: {str(e)}")
                continue
    
    except OSError as e:
        logger.error(f"Error en socket: {str(e)}")
        sys.exit(1)
    finally:
        server_socket.close()
        logger.info("Servicio de Autenticación detenido")

# ──────────────────────────────────────────────────────────────────────────────
# PUNTO DE ENTRADA
# ──────────────────────────────────────────────────────────────────────────────

if __name__ == "__main__":
    try:
        start_auth_server()
    except KeyboardInterrupt:
        logger.info("Cerrando servicio...")
        sys.exit(0)
