#!/bin/bash
# Script de compilación del servidor IoT
# Nota: Este script compila SOLO en Linux/Unix (incluyendo AWS EC2)
# Para Windows, consultar FINAL_DEPLOYMENT.md

echo "=================================="
echo "Compilando Servidor IoT (C++)"
echo "=================================="

# Detectar sistema operativo
if [[ "$OSTYPE" == "linux-gnu"* ]] || [[ "$OSTYPE" == "darwin"* ]]; then
    # Linux o macOS
    echo "✓ Sistema operativo compatible: $OSTYPE"
else
    echo "⚠️  Este script está optimizado para Linux/macOS"
    echo "Para Windows: Usar WSL2, Git Bash, o compilar en AWS EC2"
fi

# Verificar si g++ está disponible
if ! command -v g++ &> /dev/null; then
    echo "❌ Error: g++ no está instalado"
    echo "Instalar:"
    echo "  Ubuntu/Debian: sudo apt install build-essential"
    echo "  macOS: brew install gcc"
    echo "  AWS EC2: sudo yum install gcc-c++ (Amazon Linux) o sudo apt install build-essential (Ubuntu)"
    exit 1
fi

echo "✓ g++ encontrado: $(g++ --version | head -1)"

# Compilar - En Linux no necesitamos -lws2_32
echo ""
echo "Compilando server.cpp..."
g++ -std=c++17 -pthread -Wall -Wextra -o server server.cpp

if [ $? -eq 0 ]; then
    echo "✓ Compilación exitosa"
    echo "  Binario: ./server"
    echo ""
    echo "Ejecutar con:"
    echo "  ./server 8080 server.log"
    echo ""
    echo "Detener con: Ctrl+C"
    exit 0
else
    echo "❌ Error en compilación"
    exit 1
fi
