#!/bin/bash
# Script de compilación del servidor IoT
# Uso: ./build.sh

echo "=================================="
echo "Compilando Servidor IoT (C++)"
echo "=================================="

# Verificar si g++ está disponible
if ! command -v g++ &> /dev/null; then
    echo "❌ Error: g++ no está instalado"
    echo "Instalar:"
    echo "  Ubuntu/Debian: sudo apt install build-essential"
    echo "  macOS: brew install gcc"
    echo "  Windows: Descargar de mingw.org"
    exit 1
fi

echo "✓ g++ encontrado: $(g++ --version | head -1)"

# Compilar
echo ""
echo "Compilando server.cpp..."
g++ -std=c++17 -pthread -Wall -o server server.cpp

if [ $? -eq 0 ]; then
    echo "✓ Compilación exitosa"
    echo ""
    echo "Ejecutar con:"
    echo "  ./server 8080 server.log"
    exit 0
else
    echo "❌ Error en compilación"
    exit 1
fi
