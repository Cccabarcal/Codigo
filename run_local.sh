#!/bin/bash
# Script para ejecutar todo el sistema localmente
# Uso: ./run_local.sh

echo "╔════════════════════════════════════════════════════════════╗"
echo "║   Sistema IoT - Ejecución Local (Desarrollo)              ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# Colores
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Verificar compilación
if [ ! -f "server" ]; then
    echo -e "${YELLOW}⚠️  Servidor no compilado, compilando...${NC}"
    bash build.sh
    if [ $? -ne 0 ]; then
        echo -e "${RED}❌ Compilación fallida${NC}"
        exit 1
    fi
fi

echo ""
echo -e "${GREEN}✓ Sistema listo para ejecutar${NC}"
echo ""
echo "╔════════════════════════════════════════════════════════════╗"
echo "║   INSTRUCCIONES - Abrir en 5 terminales diferentes:        ║"
echo "╠════════════════════════════════════════════════════════════╣"
echo ""
echo -e "${YELLOW}Terminal 1 - Servidor IoT (C++):${NC}"
echo "  ./server 8080 server.log"
echo ""
echo -e "${YELLOW}Terminal 2 - Servicio de Autenticación (Python):${NC}"
echo "  python3 auth_service.py"
echo ""
echo -e "${YELLOW}Terminal 3 - Clientes Sensor (Python):${NC}"
echo "  python3 sensor_client.py localhost 8080"
echo ""
echo -e "${YELLOW}Terminal 4 - Cliente Operador con GUI (Python):${NC}"
echo "  python3 operator_client.py localhost 8080"
echo ""
echo -e "${YELLOW}Terminal 5 (Opcional) - Interfaz Web (Python):${NC}"
echo "  python3 web_interface.py"
echo "  Luego abrir navegador: http://localhost:5000"
echo ""
echo "╚════════════════════════════════════════════════════════════╝"
echo ""
echo -e "${GREEN}Usuarios de prueba:${NC}"
echo "  Usuario: carlos       | Contraseña: password123  | Rol: admin"
echo "  Usuario: maria        | Contraseña: maria456     | Rol: operator"
echo "  Usuario: juan         | Contraseña: juan789      | Rol: admin"
echo ""
