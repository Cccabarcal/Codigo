#!/usr/bin/env python3
"""
Cliente Sensor para Sistema IoT
Simula múltiples sensores enviando mediciones al servidor central
Uso: python3 sensor_client.py <host> <puerto>
"""

import socket
import threading
import time
import random
import sys
import logging
from datetime import datetime
from abc import ABC, abstractmethod

# ──────────────────────────────────────────────────────────────────────────────
# CONFIGURACIÓN
# ──────────────────────────────────────────────────────────────────────────────

DEFAULT_HOST = "localhost"
DEFAULT_PORT = 8080
BUFFER_SIZE = 1024
MEASURE_INTERVAL = 5  # Enviar medición cada 5 segundos

# ──────────────────────────────────────────────────────────────────────────────
# SETUP DE LOGGING
# ──────────────────────────────────────────────────────────────────────────────

def setup_logging(log_file="sensor_client.log"):
    """Configura logging a archivo y consola"""
    logger = logging.getLogger()
    logger.setLevel(logging.DEBUG)
    
    formatter = logging.Formatter(
        '[%(asctime)s] [%(levelname)s] %(message)s',
        datefmt='%Y-%m-%d %H:%M:%S'
    )
    
    console = logging.StreamHandler()
    console.setFormatter(formatter)
    logger.addHandler(console)
    
    file_handler = logging.FileHandler(log_file)
    file_handler.setFormatter(formatter)
    logger.addHandler(file_handler)
    
    return logger

logger = setup_logging()

# ──────────────────────────────────────────────────────────────────────────────
# SIMULADOR DE SENSOR BASE
# ──────────────────────────────────────────────────────────────────────────────

class Sensor(ABC):
    """Clase abstracta para sensores"""
    
    def __init__(self, sensor_id, sensor_type, unit, min_val, max_val, normal_avg):
        self.sensor_id = sensor_id
        self.sensor_type = sensor_type
        self.unit = unit
        self.min_val = min_val
        self.max_val = max_val
        self.normal_avg = normal_avg
        self.last_value = normal_avg
        self.sock = None
        self.running = False
        
    def connect(self, host, port):
        """Conecta al servidor"""
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.connect((host, port))
            logger.info(f"[{self.sensor_id}] Conectado a {host}:{port}")
            return True
        except Exception as e:
            logger.error(f"[{self.sensor_id}] Error conectando: {str(e)}")
            return False
    
    def register(self):
        """Registra el sensor en el servidor"""
        try:
            msg = f"REGISTER SENSOR {self.sensor_id} {self.sensor_type} {self.unit}\n"
            self.sock.send(msg.encode('utf-8'))
            logger.info(f"[{self.sensor_id}] → {msg.strip()}")
            
            response = self.sock.recv(BUFFER_SIZE).decode('utf-8').strip()
            logger.info(f"[{self.sensor_id}] ← {response}")
            
            if "OK" in response:
                logger.info(f"[{self.sensor_id}] Registro exitoso")
                return True
            else:
                logger.error(f"[{self.sensor_id}] Registro fallido")
                return False
        except Exception as e:
            logger.error(f"[{self.sensor_id}] Error registrando: {str(e)}")
            return False
    
    @abstractmethod
    def simulate_value(self):
        """Simula un nuevo valor de sensor (implementar en subclass)"""
        pass
    
    def send_measurement(self):
        """Envía una medición al servidor"""
        try:
            value = self.simulate_value()
            timestamp = datetime.utcnow().strftime("%Y-%m-%dT%H:%M:%SZ")
            msg = f"MEASURE {self.sensor_id} {value:.2f} {timestamp}\n"
            
            self.sock.send(msg.encode('utf-8'))
            logger.debug(f"[{self.sensor_id}] → {msg.strip()}")
            
            response = self.sock.recv(BUFFER_SIZE).decode('utf-8').strip()
            logger.debug(f"[{self.sensor_id}] ← {response}")
            
            return True
        except Exception as e:
            logger.error(f"[{self.sensor_id}] Error enviando medición: {str(e)}")
            return False
    
    def run(self, host, port):
        """Ciclo principal de ejecución del sensor"""
        if not self.connect(host, port):
            return
        
        if not self.register():
            return
        
        self.running = True
        logger.info(f"[{self.sensor_id}] Iniciando mediciones cada {MEASURE_INTERVAL}s")
        
        try:
            while self.running:
                time.sleep(MEASURE_INTERVAL)
                self.send_measurement()
        except KeyboardInterrupt:
            logger.info(f"[{self.sensor_id}] Interrupcion detectada")
        finally:
            self.disconnect()
    
    def disconnect(self):
        """Desconecta el sensor"""
        try:
            msg = f"DISCONNECT {self.sensor_id}\n"
            self.sock.send(msg.encode('utf-8'))
            logger.info(f"[{self.sensor_id}] → {msg.strip()}")
            
            response = self.sock.recv(BUFFER_SIZE).decode('utf-8').strip()
            logger.info(f"[{self.sensor_id}] ← {response}")
            
            self.sock.close()
            logger.info(f"[{self.sensor_id}] Desconectado")
        except Exception as e:
            logger.error(f"[{self.sensor_id}] Error desconectando: {str(e)}")
            if self.sock:
                self.sock.close()
        
        self.running = False

# ──────────────────────────────────────────────────────────────────────────────
# SENSORES ESPECÍFICOS
# ──────────────────────────────────────────────────────────────────────────────

class TemperatureSensor(Sensor):
    """Simula un sensor de temperatura"""
    def __init__(self, sensor_id):
        super().__init__(
            sensor_id=sensor_id,
            sensor_type="temperature",
            unit="celsius",
            min_val=-10,
            max_val=80,
            normal_avg=25
        )
    
    def simulate_value(self):
        """Simula temperatura con variación gradual"""
        self.last_value += random.uniform(-0.5, 0.5)
        self.last_value = max(self.min_val, min(self.max_val, self.last_value))
        
        # Ocasionalmente generar anomalía
        if random.random() < 0.05:
            self.last_value += random.uniform(20, 40)
        
        return self.last_value

class HumiditySensor(Sensor):
    """Simula un sensor de humedad"""
    def __init__(self, sensor_id):
        super().__init__(
            sensor_id=sensor_id,
            sensor_type="humidity",
            unit="%",
            min_val=0,
            max_val=95,
            normal_avg=60
        )
    
    def simulate_value(self):
        """Simula humedad"""
        self.last_value += random.uniform(-2, 2)
        self.last_value = max(self.min_val, min(self.max_val, self.last_value))
        return self.last_value

class PressureSensor(Sensor):
    """Simula un sensor de presión"""
    def __init__(self, sensor_id):
        super().__init__(
            sensor_id=sensor_id,
            sensor_type="pressure",
            unit="hPa",
            min_val=900,
            max_val=1100,
            normal_avg=1013
        )
    
    def simulate_value(self):
        """Simula presión atmosférica"""
        self.last_value += random.uniform(-1, 1)
        self.last_value = max(self.min_val, min(self.max_val, self.last_value))
        return self.last_value

class VibrationSensor(Sensor):
    """Simula un sensor de vibración"""
    def __init__(self, sensor_id):
        super().__init__(
            sensor_id=sensor_id,
            sensor_type="vibration",
            unit="m/s²",
            min_val=0,
            max_val=10,
            normal_avg=1.5
        )
    
    def simulate_value(self):
        """Simula vibración"""
        self.last_value += random.uniform(-0.1, 0.1)
        self.last_value = max(self.min_val, min(self.max_val, self.last_value))
        
        # Ocasionalmente generar anomalía
        if random.random() < 0.03:
            self.last_value = random.uniform(8, 12)
        
        return self.last_value

class EnergySensor(Sensor):
    """Simula un sensor de consumo energético"""
    def __init__(self, sensor_id):
        super().__init__(
            sensor_id=sensor_id,
            sensor_type="energy",
            unit="W",
            min_val=0,
            max_val=5000,
            normal_avg=1500
        )
    
    def simulate_value(self):
        """Simula consumo energético"""
        self.last_value += random.uniform(-100, 100)
        self.last_value = max(self.min_val, min(self.max_val, self.last_value))
        
        # Picos ocasionales
        if random.random() < 0.08:
            self.last_value = random.uniform(4500, 5500)
        
        return self.last_value

# ──────────────────────────────────────────────────────────────────────────────
# CLIENTE PRINCIPAL
# ──────────────────────────────────────────────────────────────────────────────

def main():
    if len(sys.argv) > 1:
        host = sys.argv[1]
    else:
        host = DEFAULT_HOST
    
    if len(sys.argv) > 2:
        port = int(sys.argv[2])
    else:
        port = DEFAULT_PORT
    
    logger.info("=" * 70)
    logger.info(f"Cliente Sensor IoT iniciando")
    logger.info(f"Servidor: {host}:{port}")
    logger.info("=" * 70)
    
    # Crear sensores
    sensors = [
        TemperatureSensor("temp-01"),
        TemperatureSensor("temp-02"),
        HumiditySensor("humid-01"),
        PressureSensor("press-01"),
        VibrationSensor("vibra-01"),
        EnergySensor("energy-01"),
    ]
    
    logger.info(f"Creados {len(sensors)} sensores")
    
    # Iniciar cada sensor en un thread
    threads = []
    for sensor in sensors:
        thread = threading.Thread(
            target=sensor.run,
            args=(host, port),
            daemon=False
        )
        thread.start()
        threads.append(thread)
        time.sleep(0.5)  # Pequeño delay entre conexiones
    
    logger.info("Todos los sensores iniciados")
    
    # Esperar a que terminen
    try:
        for thread in threads:
            thread.join()
    except KeyboardInterrupt:
        logger.info("Cerrando clientes...")
        for sensor in sensors:
            sensor.running = False

if __name__ == "__main__":
    main()
