#!/bin/bash
# ════════════════════════════════════════════════════════════════════════════
# SCRIPT: prepare_deployment.sh
# Descripción: Prepara el proyecto para deployment en AWS
# Uso: bash prepare_deployment.sh
# ════════════════════════════════════════════════════════════════════════════

set -e  # Exit on error

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'  # No Color

echo -e "${BLUE}"
echo "╔════════════════════════════════════════════════════════════╗"
echo "║  Preparación del Proyecto para Despliegue en AWS          ║"
echo "║  Sistema IoT Distribuido - Proyecto I                     ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo -e "${NC}"

# ════════════════════════════════════════════════════════════════════════════
# 1. LIMPIAR ARCHIVOS TEMPORALES
# ════════════════════════════════════════════════════════════════════════════

echo -e "${YELLOW}📋 PASO 1: Limpiando archivos temporales...${NC}"

# Eliminar logs
echo "  • Eliminando archivos .log..."
rm -f *.log server.log auth_service.log sensor_client.log web_interface.log operator_client.log

# Eliminar archivos de depuración
echo "  • Eliminando versiones temporales..."
rm -f server_simple.cpp server_debug.cpp server_test.cpp

# Limpiar Python cache
echo "  • Limpiando Python cache..."
find . -type d -name __pycache__ -exec rm -rf {} + 2>/dev/null || true
find . -type f -name "*.pyc" -delete 2>/dev/null || true
find . -type f -name "*.pyo" -delete 2>/dev/null || true
find . -type f -name ".pytest_cache" -delete 2>/dev/null || true

# Eliminar .DS_Store de macOS
find . -type f -name ".DS_Store" -delete 2>/dev/null || true

echo -e "${GREEN}  ✓ Archivos temporales eliminados${NC}"

# ════════════════════════════════════════════════════════════════════════════
# 2. VERIFICAR ESTRUCTURA DE ARCHIVOS
# ════════════════════════════════════════════════════════════════════════════

echo -e "${YELLOW}📋 PASO 2: Verificando estructura de archivos...${NC}"

FILES_REQUIRED=(
    "server.cpp"
    "Logger.h"
    "protocol.h"
    "auth_service.py"
    "sensor_client.py"
    "web_interface.py"
    "operator_client.py"
    "users.json"
    "requirements.txt"
    "PROTOCOLO.md"
    "README.md"
    "DESPLIEGUE_AWS.md"
    "build.sh"
    ".gitignore"
)

MISSING=0
for file in "${FILES_REQUIRED[@]}"; do
    if [ -f "$file" ]; then
        echo "  ✓ $file"
    else
        echo "  ${RED}✗ FALTA: $file${NC}"
        MISSING=$((MISSING + 1))
    fi
done

if [ $MISSING -gt 0 ]; then
    echo -e "${RED}❌ Error: Faltan $MISSING archivos requeridos${NC}"
    exit 1
fi

echo -e "${GREEN}  ✓ Todos los archivos requeridos presentes${NC}"

# ════════════════════════════════════════════════════════════════════════════
# 3. INTENTAR COMPILACIÓN LOCAL
# ════════════════════════════════════════════════════════════════════════════

echo -e "${YELLOW}📋 PASO 3: Verificando compilación del servidor...${NC}"

if command -v g++ &> /dev/null; then
    echo "  • g++ encontrado: $(g++ --version | head -1)"
    echo "  • Compilando server.cpp..."
    
    if g++ -std=c++17 -pthread -Wall -Wextra -o server server.cpp 2>/tmp/compile_errors.txt; then
        FILE_SIZE=$(wc -c < server)
        echo -e "${GREEN}  ✓ Compilación exitosa (${FILE_SIZE} bytes)${NC}"
    else
        echo -e "${RED}  ✗ Error en compilación:${NC}"
        cat /tmp/compile_errors.txt
        exit 1
    fi
else
    echo -e "${YELLOW}  ⚠️  g++ no instalado, compilación será en AWS${NC}"
fi

# ════════════════════════════════════════════════════════════════════════════
# 4. VERIFICAR DEPENDENCIAS PYTHON
# ════════════════════════════════════════════════════════════════════════════

echo -e "${YELLOW}📋 PASO 4: Verificando dependencias Python...${NC}"

if command -v python3 &> /dev/null; then
    PYTHON_VERSION=$(python3 --version)
    echo "  • $PYTHON_VERSION"
    
    echo "  • Verificando módulos..."
    python3 -c "import socket; print('    ✓ socket')" 2>/dev/null || echo "    ${RED}✗ socket${NC}"
    python3 -c "import threading; print('    ✓ threading')" 2>/dev/null || echo "    ${RED}✗ threading${NC}"
    python3 -c "import json; print('    ✓ json')" 2>/dev/null || echo "    ${RED}✗ json${NC}"
    
    echo -e "${GREEN}  ✓ Python configurado${NC}"
else
    echo -e "${YELLOW}  ⚠️  Python no disponible localmente${NC}"
fi

# ════════════════════════════════════════════════════════════════════════════
# 5. VERIFICAR GIT
# ════════════════════════════════════════════════════════════════════════════

echo -e "${YELLOW}📋 PASO 5: Verificando estado de git...${NC}"

if command -v git &> /dev/null; then
    echo "  • Git instalado: $(git --version)"
    
    if git status &> /dev/null; then
        CHANGES=$(git status --porcelain | wc -l)
        echo "  • Cambios pendientes: $CHANGES"
        
        if [ $CHANGES -gt 0 ]; then
            echo ""
            echo -e "${YELLOW}  Cambios pendientes:${NC}"
            git status --short | head -10
        fi
        
        echo -e "${GREEN}  ✓ Repositorio git válido${NC}"
    else
        echo -e "${RED}  ✗ No es un repositorio git${NC}"
        exit 1
    fi
else
    echo -e "${YELLOW}  ⚠️  Git no instalado${NC}"
fi

# ════════════════════════════════════════════════════════════════════════════
# 6. RESUMEN Y SIGUIENTES PASOS
# ════════════════════════════════════════════════════════════════════════════

echo ""
echo -e "${GREEN}╔════════════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║  ✅ PROYECTO LISTO PARA DESPLIEGUE                         ║${NC}"
echo -e "${GREEN}╚════════════════════════════════════════════════════════════╝${NC}"

echo ""
echo -e "${YELLOW}📋 Siguientes pasos:${NC}"
echo ""
echo "1. Revisar cambios pendientes:"
echo "   git status"
echo ""
echo "2. Hacer commit:"
echo "   git add -A"
echo "   git commit -m 'Proyecto I final: Sistema IoT Distribuido'"
echo ""
echo "3. En AWS, clonar y ejecutar:"
echo "   git clone <REPO_URL>"
echo "   cd Codigo"
echo "   bash build.sh"
echo "   ./server 8080 server.log &"
echo "   python3 auth_service.py &"
echo "   python3 sensor_client.py 54.242.32.222 8080 &"
echo "   python3 web_interface.py"
echo ""
echo -e "${BLUE}Acceder a la interfaz web: http://54.242.32.222:5000${NC}"
echo ""
