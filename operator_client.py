#!/usr/bin/env python3
"""
Cliente Operador con Interfaz Gráfica para Sistema IoT
Permite monitorear sensores en tiempo real y recibir alertas
Uso: python3 operator_client.py <host> <puerto>
"""

import tkinter as tk
from tkinter import ttk, messagebox, scrolledtext
import socket
import threading
import time
import logging
from datetime import datetime
import sys

# ──────────────────────────────────────────────────────────────────────────────
# CONFIGURACIÓN
# ──────────────────────────────────────────────────────────────────────────────

DEFAULT_HOST = "localhost"
DEFAULT_PORT = 8080
BUFFER_SIZE = 1024

# ──────────────────────────────────────────────────────────────────────────────
# SETUP DE LOGGING
# ──────────────────────────────────────────────────────────────────────────────

logger = logging.getLogger()
logger.setLevel(logging.DEBUG)

formatter = logging.Formatter(
    '[%(asctime)s] [%(levelname)s] %(message)s',
    datefmt='%Y-%m-%d %H:%M:%S'
)

console = logging.StreamHandler()
console.setFormatter(formatter)
logger.addHandler(console)

file_handler = logging.FileHandler("operator_client.log")
file_handler.setFormatter(formatter)
logger.addHandler(file_handler)

# ──────────────────────────────────────────────────────────────────────────────
# CLIENTE OPERADOR
# ──────────────────────────────────────────────────────────────────────────────

class OperatorClient:
    """Cliente Operador para conexión y comunicación con servidor IoT"""
    
    def __init__(self, host, port):
        self.host = host
        self.port = port
        self.sock = None
        self.connected = False
        self.authenticated = False
        self.username = None
        self.sensors = {}
        self.measurements = {}
        
    def connect(self):
        """Conecta al servidor"""
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.connect((self.host, self.port))
            self.connected = True
            logger.info(f"Conectado a {self.host}:{self.port}")
            return True
        except Exception as e:
            logger.error(f"Error conectando: {str(e)}")
            messagebox.showerror("Error", f"No se pudo conectar: {str(e)}")
            return False
    
    def login(self, username, password):
        """Autentica usuario"""
        try:
            msg = f"LOGIN {username} {password}\n"
            self.sock.send(msg.encode('utf-8'))
            response = self.sock.recv(BUFFER_SIZE).decode('utf-8').strip()
            
            logger.info(f"← {response}")
            
            if response.startswith("OK"):
                self.authenticated = True
                self.username = username
                logger.info(f"Autenticado como {username}")
                return True
            else:
                messagebox.showerror("Error", "Credenciales inválidas")
                return False
        except Exception as e:
            logger.error(f"Error en login: {str(e)}")
            messagebox.showerror("Error", f"Error de autenticación: {str(e)}")
            return False
    
    def list_sensors(self):
        """Obtiene lista de sensores activos"""
        try:
            msg = "LIST SENSORS\n"
            self.sock.send(msg.encode('utf-8'))
            response = self.sock.recv(BUFFER_SIZE).decode('utf-8').strip()
            
            logger.info(f"← {response}")
            
            if response.startswith("SENSORS"):
                sensor_data = response.replace("SENSORS ", "")
                if sensor_data:
                    self.sensors = {}
                    for pair in sensor_data.split(","):
                        sensor_id, sensor_type = pair.split(":")
                        self.sensors[sensor_id] = sensor_type
                return True
            return False
        except Exception as e:
            logger.error(f"Error listando sensores: {str(e)}")
            return False
    
    def get_measurements(self, sensor_id, count=10):
        """Obtiene mediciones históri cas de un sensor"""
        try:
            msg = f"GET MEASURE {sensor_id} {count}\n"
            self.sock.send(msg.encode('utf-8'))
            
            measurements = []
            for _ in range(count):
                try:
                    response = self.sock.recv(BUFFER_SIZE).decode('utf-8').strip()
                    if response.startswith("DATA"):
                        measurements.append(response)
                    else:
                        break
                except socket.timeout:
                    break
            
            logger.info(f"Recibidas {len(measurements)} mediciones de {sensor_id}")
            return measurements
        except Exception as e:
            logger.error(f"Error obteniendo mediciones: {str(e)}")
            return []
    
    def receive_alerts(self, callback):
        """Escucha alertas en background (thread)"""
        while self.connected:
            try:
                data = self.sock.recv(BUFFER_SIZE).decode('utf-8').strip()
                if not data:
                    break
                
                if data.startswith("ALERT"):
                    logger.warning(f"ALERTA: {data}")
                    callback(data)
                elif data.startswith("DATA"):
                    logger.debug(f"← {data}")
                    callback(data)
                elif data.startswith("OK"):
                    logger.debug(f"← {data}")
                
            except Exception as e:
                if self.connected:
                    logger.error(f"Error recibiendo: {str(e)}")
                break
    
    def disconnect(self):
        """Desconecta del servidor"""
        try:
            if self.authenticated:
                msg = "LOGOUT\n"
                self.sock.send(msg.encode('utf-8'))
                response = self.sock.recv(BUFFER_SIZE).decode('utf-8').strip()
                logger.info(f"← {response}")
            
            self.sock.close()
            self.connected = False
            self.authenticated = False
            logger.info("Desconectado")
        except Exception as e:
            logger.error(f"Error desconectando: {str(e)}")

# ──────────────────────────────────────────────────────────────────────────────
# INTERFAZ GRÁFICA
# ──────────────────────────────────────────────────────────────────────────────

class OperatorGUI:
    """Interfaz gráfica del operador"""
    
    def __init__(self, root, host, port):
        self.root = root
        self.client = OperatorClient(host, port)
        self.alert_thread = None
        self.setup_ui()
        
    def setup_ui(self):
        """Configura la interfaz"""
        self.root.title("Centro de Monitoreo IoT")
        self.root.geometry("900x700")
        self.root.configure(bg="#f0f0f0")
        
        # ── PANEL DE LOGIN ──
        self.login_frame = ttk.Frame(self.root)
        self.login_frame.pack(fill="both", expand=True)
        
        ttk.Label(self.login_frame, text="Centro de Monitoreo - IoT System", 
                  font=("Arial", 18, "bold")).pack(pady=20)
        
        ttk.Label(self.login_frame, text="Usuario:").pack()
        self.username_entry = ttk.Entry(self.login_frame, width=30)
        self.username_entry.pack(pady=5)
        self.username_entry.insert(0, "carlos")
        
        ttk.Label(self.login_frame, text="Contraseña:").pack()
        self.password_entry = ttk.Entry(self.login_frame, width=30, show="*")
        self.password_entry.pack(pady=5)
        self.password_entry.insert(0, "password123")
        
        ttk.Button(self.login_frame, text="Conectar y Login",
                   command=self.do_login).pack(pady=20)
        
        # ── PANEL PRINCIPAL (oculto inicialmente) ──
        self.main_frame = ttk.Frame(self.root)
        
        # Barra superior
        top_bar = ttk.Frame(self.main_frame)
        top_bar.pack(fill="x", padx=10, pady=10)
        
        ttk.Label(top_bar, text="Sensores Activos:", font=("Arial", 12, "bold")).pack(side="left")
        ttk.Button(top_bar, text="Actualizar Lista", 
                   command=self.refresh_sensors).pack(side="right", padx=5)
        ttk.Button(top_bar, text="Logout", 
                   command=self.do_logout).pack(side="right")
        
        # Panel sensores
        sensors_frame = ttk.LabelFrame(self.main_frame, text="Sensores Registrados")
        sensors_frame.pack(fill="x", padx=10, pady=5)
        
        self.sensors_listbox = tk.Listbox(sensors_frame, height=6)
        self.sensors_listbox.pack(fill="both", expand=True, padx=5, pady=5)
        self.sensors_listbox.bind('<<ListboxSelect>>', self.on_sensor_select)
        
        scrollbar = ttk.Scrollbar(sensors_frame, orient="vertical", command=self.sensors_listbox.yview)
        scrollbar.pack(side="right", fill="y")
        self.sensors_listbox.config(yscrollcommand=scrollbar.set)
        
        # Panel mediciones
        measure_frame = ttk.LabelFrame(self.main_frame, text="Últimas Mediciones")
        measure_frame.pack(fill="both", expand=True, padx=10, pady=5)
        
        self.measurements_text = scrolledtext.ScrolledText(
            measure_frame, height=10, width=100, state="disabled"
        )
        self.measurements_text.pack(fill="both", expand=True, padx=5, pady=5)
        
        # Panel alertas
        alerts_frame = ttk.LabelFrame(self.main_frame, text="Alertas en Tiempo Real")
        alerts_frame.pack(fill="both", expand=True, padx=10, pady=5)
        
        self.alerts_text = scrolledtext.ScrolledText(
            alerts_frame, height=8, width=100, bg="#ffe6e6", state="disabled"
        )
        self.alerts_text.pack(fill="both", expand=True, padx=5, pady=5)
        
        ttk.Button(self.main_frame, text="Limpiar Alertas",
                   command=self.clear_alerts).pack(pady=5)
    
    def do_login(self):
        """Ejecuta login"""
        username = self.username_entry.get()
        password = self.password_entry.get()
        
        if not username or not password:
            messagebox.showwarning("Advertencia", "Ingrese usuario y contraseña")
            return
        
        if not self.client.connect():
            return
        
        if not self.client.login(username, password):
            self.client.disconnect()
            return
        
        # Mostrar panel principal
        self.login_frame.pack_forget()
        self.main_frame.pack(fill="both", expand=True)
        
        # Actualizar lista de sensores
        self.refresh_sensors()
        
        # Iniciar thread de alertas
        self.alert_thread = threading.Thread(
            target=self.client.receive_alerts,
            args=(self.on_alert,),
            daemon=True
        )
        self.alert_thread.start()
        logger.info("Thread de alertas iniciado")
    
    def refresh_sensors(self):
        """Actualiza lista de sensores"""
        if self.client.list_sensors():
            self.sensors_listbox.delete(0, tk.END)
            for sensor_id, sensor_type in self.client.sensors.items():
                self.sensors_listbox.insert(tk.END, f"{sensor_id} ({sensor_type})")
            logger.info(f"Sensores actualizados: {len(self.client.sensors)}")
        else:
            messagebox.showerror("Error", "No se pudo actualizar la lista de sensores")
    
    def on_sensor_select(self, event):
        """Cuando selecciona un sensor"""
        selection = self.sensors_listbox.curselection()
        if not selection:
            return
        
        sensor_text = self.sensors_listbox.get(selection[0])
        sensor_id = sensor_text.split(" ")[0]
        
        measurements = self.client.get_measurements(sensor_id, 10)
        
        self.measurements_text.config(state="normal")
        self.measurements_text.delete(1.0, tk.END)
        self.measurements_text.insert(tk.END, f"=== Mediciones de {sensor_id} ===\n\n")
        
        for m in measurements:
            self.measurements_text.insert(tk.END, f"{m}\n")
        
        self.measurements_text.config(state="disabled")
    
    def on_alert(self, message):
        """Recibe alerta en tiempo real"""
        self.alerts_text.config(state="normal")
        timestamp = datetime.now().strftime("%H:%M:%S")
        
        if message.startswith("ALERT"):
            self.alerts_text.insert(tk.END, f"[{timestamp}] ⚠️  {message}\n", "alert")
            self.alerts_text.see(tk.END)
        
        self.alerts_text.config(state="disabled")
        logger.info(f"Alerta mostrada: {message}")
    
    def clear_alerts(self):
        """Limpia panel de alertas"""
        self.alerts_text.config(state="normal")
        self.alerts_text.delete(1.0, tk.END)
        self.alerts_text.config(state="disabled")
    
    def do_logout(self):
        """Desconecta"""
        self.client.disconnect()
        messagebox.showinfo("Logout", "Desconectado correctamente")
        self.root.quit()
    
    def on_closing(self):
        """Al cerrar la ventana"""
        if self.client.connected:
            self.client.disconnect()
        self.root.destroy()

# ──────────────────────────────────────────────────────────────────────────────
# PUNTO DE ENTRADA
# ──────────────────────────────────────────────────────────────────────────────

def main():
    host = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_HOST
    port = int(sys.argv[2]) if len(sys.argv) > 2 else DEFAULT_PORT
    
    logger.info("=" * 70)
    logger.info(f"Cliente Operador IoT - Servidor: {host}:{port}")
    logger.info("=" * 70)
    
    root = tk.Tk()
    gui = OperatorGUI(root, host, port)
    root.protocol("WM_DELETE_WINDOW", gui.on_closing)
    root.mainloop()

if __name__ == "__main__":
    main()
