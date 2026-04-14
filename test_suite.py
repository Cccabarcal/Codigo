#!/usr/bin/env python3
"""
Test Suite para Sistema IoT
Valida comunicación entre clientes y servidor

Uso: python3 test_suite.py
"""

import socket
import time
import sys
import logging

logging.basicConfig(
    level=logging.INFO,
    format='[%(levelname)s] %(message)s'
)
logger = logging.getLogger()

# ──────────────────────────────────────────────────────────────────────────────
# CONFIGURACIÓN
# ──────────────────────────────────────────────────────────────────────────────

IOT_HOST = "localhost"
IOT_PORT = 8080
AUTH_HOST = "localhost"
AUTH_PORT = 9000
BUFFER_SIZE = 1024

# ──────────────────────────────────────────────────────────────────────────────
# TESTS
# ──────────────────────────────────────────────────────────────────────────────

def test_auth_service():
    """Test 1: Servicio de autenticación"""
    logger.info("=" * 60)
    logger.info("TEST 1: Servicio de Autenticación (Puerto 9000)")
    logger.info("=" * 60)
    
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((AUTH_HOST, AUTH_PORT))
        
        # Test credenciales válidas
        logger.info("Enviando: AUTH carlos password123")
        sock.send(b"AUTH carlos password123\n")
        response = sock.recv(BUFFER_SIZE).decode('utf-8').strip()
        logger.info(f"Respuesta: {response}")
        
        if response.startswith("OK"):
            logger.info("✓ Auth test PASÓ")
            return True
        else:
            logger.error("✗ Auth test FALLÓ - Respuesta inesperada")
            return False
            
    except Exception as e:
        logger.error(f"✗ Auth test FALLÓ: {str(e)}")
        return False
    finally:
        try:
            sock.close()
        except:
            pass

def test_sensor_registration():
    """Test 2: Registro de sensor"""
    logger.info("")
    logger.info("=" * 60)
    logger.info("TEST 2: Registro de Sensor")
    logger.info("=" * 60)
    
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((IOT_HOST, IOT_PORT))
        
        # Registrar sensor
        msg = "REGISTER SENSOR test-sensor temperature celsius\n"
        logger.info(f"Enviando: {msg.strip()}")
        sock.send(msg.encode('utf-8'))
        response = sock.recv(BUFFER_SIZE).decode('utf-8').strip()
        logger.info(f"Respuesta: {response}")
        
        if "OK REGISTERED" in response:
            logger.info("✓ Sensor registration test PASÓ")
            return True
        else:
            logger.error("✗ Sensor registration test FALLÓ")
            return False
            
    except Exception as e:
        logger.error(f"✗ Sensor registration test FALLÓ: {str(e)}")
        return False
    finally:
        try:
            sock.close()
        except:
            pass

def test_measure_send():
    """Test 3: Envío de medición"""
    logger.info("")
    logger.info("=" * 60)
    logger.info("TEST 3: Envío de Medición")
    logger.info("=" * 60)
    
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((IOT_HOST, IOT_PORT))
        
        # Registrar
        sock.send(b"REGISTER SENSOR temp-test temperature celsius\n")
        response = sock.recv(BUFFER_SIZE)
        logger.info(f"Registro: {response.decode('utf-8').strip()}")
        
        # Enviar medición
        msg = "MEASURE temp-test 25.5 2026-04-14T10:30:45Z\n"
        logger.info(f"Enviando medición: {msg.strip()}")
        sock.send(msg.encode('utf-8'))
        response = sock.recv(BUFFER_SIZE).decode('utf-8').strip()
        logger.info(f"Respuesta: {response}")
        
        if response == "OK":
            logger.info("✓ Measure send test PASÓ")
            return True
        else:
            logger.error("✗ Measure send test FALLÓ")
            return False
            
    except Exception as e:
        logger.error(f"✗ Measure send test FALLÓ: {str(e)}")
        return False
    finally:
        try:
            sock.close()
        except:
            pass

def test_operator_login():
    """Test 4: Login de operador"""
    logger.info("")
    logger.info("=" * 60)
    logger.info("TEST 4: Login de Operador")
    logger.info("=" * 60)
    
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((IOT_HOST, IOT_PORT))
        
        # Login
        msg = "LOGIN carlos password123\n"
        logger.info(f"Enviando: {msg.strip()}")
        sock.send(msg.encode('utf-8'))
        response = sock.recv(BUFFER_SIZE).decode('utf-8').strip()
        logger.info(f"Respuesta: {response}")
        
        if response.startswith("OK"):
            logger.info("✓ Operator login test PASÓ")
            return True
        else:
            logger.error("✗ Operator login test FALLÓ")
            return False
            
    except Exception as e:
        logger.error(f"✗ Operator login test FALLÓ: {str(e)}")
        return False
    finally:
        try:
            sock.close()
        except:
            pass

def test_list_sensors():
    """Test 5: Listar sensores"""
    logger.info("")
    logger.info("=" * 60)
    logger.info("TEST 5: Listar Sensores")
    logger.info("=" * 60)
    
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((IOT_HOST, IOT_PORT))
        
        # Login
        sock.send(b"LOGIN carlos password123\n")
        response = sock.recv(BUFFER_SIZE)
        logger.info(f"Login: {response.decode('utf-8').strip()}")
        
        # List
        sock.send(b"LIST SENSORS\n")
        response = sock.recv(BUFFER_SIZE).decode('utf-8').strip()
        logger.info(f"Respuesta: {response}")
        
        if response.startswith("SENSORS"):
            logger.info("✓ List sensors test PASÓ")
            return True
        else:
            logger.error("✗ List sensors test FALLÓ")
            return False
            
    except Exception as e:
        logger.error(f"✗ List sensors test FALLÓ: {str(e)}")
        return False
    finally:
        try:
            sock.close()
        except:
            pass

def test_disconnect():
    """Test 6: Desconexión de sensor"""
    logger.info("")
    logger.info("=" * 60)
    logger.info("TEST 6: Desconexión de Sensor")
    logger.info("=" * 60)
    
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((IOT_HOST, IOT_PORT))
        
        # Registrar
        sock.send(b"REGISTER SENSOR disco-test temperature celsius\n")
        response = sock.recv(BUFFER_SIZE)
        logger.info(f"Registro: {response.decode('utf-8').strip()}")
        
        # Desconectar
        sock.send(b"DISCONNECT disco-test\n")
        response = sock.recv(BUFFER_SIZE).decode('utf-8').strip()
        logger.info(f"Respuesta: {response}")
        
        if "Bye" in response or "OK" in response:
            logger.info("✓ Disconnect test PASÓ")
            return True
        else:
            logger.error("✗ Disconnect test FALLÓ")
            return False
            
    except Exception as e:
        logger.error(f"✗ Disconnect test FALLÓ: {str(e)}")
        return False
    finally:
        try:
            sock.close()
        except:
            pass

# ──────────────────────────────────────────────────────────────────────────────
# RUNNER
# ──────────────────────────────────────────────────────────────────────────────

def run_all_tests():
    """Ejecuta todos los tests"""
    logger.info("")
    logger.info("╔════════════════════════════════════════════════════════════╗")
    logger.info("║       TEST SUITE - Sistema IoT                             ║")
    logger.info("╚════════════════════════════════════════════════════════════╝")
    logger.info("")
    logger.info(f"Servidor IoT: {IOT_HOST}:{IOT_PORT}")
    logger.info(f"Auth Service: {AUTH_HOST}:{AUTH_PORT}")
    logger.info("")
    
    time.sleep(1)
    
    results = {
        "Auth Service": test_auth_service(),
        "Sensor Registration": test_sensor_registration(),
        "Measure Send": test_measure_send(),
        "Operator Login": test_operator_login(),
        "List Sensors": test_list_sensors(),
        "Disconnect": test_disconnect(),
    }
    
    # Resumen
    logger.info("")
    logger.info("=" * 60)
    logger.info("RESUMEN DE TESTS")
    logger.info("=" * 60)
    
    passed = sum(1 for v in results.values() if v)
    total = len(results)
    
    for test_name, result in results.items():
        status = "✓ PASÓ" if result else "✗ FALLÓ"
        logger.info(f"{test_name}: {status}")
    
    logger.info("")
    logger.info(f"Total: {passed}/{total} tests pasaron")
    
    if passed == total:
        logger.info("✓ TODOS LOS TESTS PASARON")
        return True
    else:
        logger.error("✗ ALGUNOS TESTS FALLARON")
        return False

if __name__ == "__main__":
    try:
        success = run_all_tests()
        sys.exit(0 if success else 1)
    except KeyboardInterrupt:
        logger.info("Pruebas interrumpidas por usuario")
        sys.exit(1)
