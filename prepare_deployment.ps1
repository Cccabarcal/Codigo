# ════════════════════════════════════════════════════════════════════════════
# Script: prepare_deployment.ps1
# Descripción: Prepara el proyecto para deployment en AWS (Windows PowerShell)
# Uso: .\prepare_deployment.ps1
# ════════════════════════════════════════════════════════════════════════════

Write-Host "════════════════════════════════════════════════════════════════" -ForegroundColor Blue
Write-Host "  Preparación del Proyecto para Despliegue en AWS              " -ForegroundColor Blue
Write-Host "  Sistema IoT Distribuido - Proyecto I                         " -ForegroundColor Blue
Write-Host "════════════════════════════════════════════════════════════════" -ForegroundColor Blue
Write-Host ""

# Colors
$Success = 'Green'
$Warning = 'Yellow'
$Error = 'Red'
$Info = 'Cyan'

# ════════════════════════════════════════════════════════════════════════════
# 1. LIMPIAR ARCHIVOS TEMPORALES
# ════════════════════════════════════════════════════════════════════════════

Write-Host "📋 PASO 1: Limpiando archivos temporales..." -ForegroundColor $Warning

Write-Host "  • Eliminando archivos .log..." -ForegroundColor Gray
Get-ChildItem -Filter "*.log" -ErrorAction SilentlyContinue | Remove-Item -Force

Write-Host "  • Eliminando versiones temporales..." -ForegroundColor Gray
@("server_simple.cpp", "server_debug.cpp", "server_test.cpp") | ForEach-Object {
    if (Test-Path $_) { Remove-Item $_ -Force }
}

Write-Host "  • Limpiando Python cache..." -ForegroundColor Gray
Get-ChildItem -Recurse -Directory -Filter "__pycache__" -ErrorAction SilentlyContinue | 
    Remove-Item -Recurse -Force

Get-ChildItem -Recurse -Filter "*.pyc" -ErrorAction SilentlyContinue | Remove-Item -Force
Get-ChildItem -Recurse -Filter ".pytest_cache" -ErrorAction SilentlyContinue | Remove-Item -Recurse -Force

Write-Host "  ✓ Archivos temporales eliminados" -ForegroundColor $Success
Write-Host ""

# ════════════════════════════════════════════════════════════════════════════
# 2. VERIFICAR ESTRUCTURA DE ARCHIVOS
# ════════════════════════════════════════════════════════════════════════════

Write-Host "📋 PASO 2: Verificando estructura de archivos..." -ForegroundColor $Warning

$RequiredFiles = @(
    "server.cpp",
    "Logger.h",
    "protocol.h",
    "auth_service.py",
    "sensor_client.py",
    "web_interface.py",
    "operator_client.py",
    "users.json",
    "requirements.txt",
    "PROTOCOLO.md",
    "README.md",
    "DESPLIEGUE_AWS.md",
    "build.sh",
    ".gitignore"
)

$Missing = 0
foreach ($file in $RequiredFiles) {
    if (Test-Path $file) {
        Write-Host "  ✓ $file" -ForegroundColor $Success
    } else {
        Write-Host "  ✗ FALTA: $file" -ForegroundColor $Error
        $Missing++
    }
}

if ($Missing -gt 0) {
    Write-Host "❌ Error: Faltan $Missing archivos requeridos" -ForegroundColor $Error
    exit 1
}

Write-Host "  ✓ Todos los archivos requeridos presentes" -ForegroundColor $Success
Write-Host ""

# ════════════════════════════════════════════════════════════════════════════
# 3. INFORMACIÓN DE GIT
# ════════════════════════════════════════════════════════════════════════════

Write-Host "📋 PASO 3: Verificando estado de git..." -ForegroundColor $Warning

try {
    $GitVersion = & git --version 2>$null
    Write-Host "  • $GitVersion" -ForegroundColor Gray
    
    $Status = & git status --porcelain 2>$null
    $Changes = ($Status | Measure-Object -Line).Lines
    
    Write-Host "  • Cambios pendientes: $Changes" -ForegroundColor Gray
    
    if ($Changes -gt 0) {
        Write-Host ""
        Write-Host "  Cambios pendientes:" -ForegroundColor $Warning
        & git status --short | Select-Object -First 10 | ForEach-Object {
            Write-Host "    $_" -ForegroundColor Gray
        }
    }
    
    Write-Host "  ✓ Repositorio git válido" -ForegroundColor $Success
} catch {
    Write-Host "  ⚠️  No se puede verificar git" -ForegroundColor $Warning
}
Write-Host ""

# ════════════════════════════════════════════════════════════════════════════
# 4. ESTADÍSTICAS DEL PROYECTO
# ════════════════════════════════════════════════════════════════════════════

Write-Host "📋 PASO 4: Estadísticas del proyecto..." -ForegroundColor $Warning

$CppLines = (Get-Content server.cpp -ErrorAction SilentlyContinue | Measure-Object -Line).Lines
$PythonFiles = @("auth_service.py", "sensor_client.py", "web_interface.py", "operator_client.py")
$PythonLines = 0
foreach ($file in $PythonFiles) {
    if (Test-Path $file) {
        $PythonLines += (Get-Content $file | Measure-Object -Line).Lines
    }
}

Write-Host "  • server.cpp: $CppLines líneas de código C++" -ForegroundColor Gray
Write-Host "  • Python services: $PythonLines líneas totales" -ForegroundColor Gray
Write-Host "  • Documentación: PROTOCOLO.md, README.md, DESPLIEGUE_AWS.md" -ForegroundColor Gray
Write-Host ""

# ════════════════════════════════════════════════════════════════════════════
# 5. RESUMEN Y SIGUIENTES PASOS
# ════════════════════════════════════════════════════════════════════════════

Write-Host "╔════════════════════════════════════════════════════════════╗" -ForegroundColor $Success
Write-Host "║  ✅ PROYECTO LISTO PARA DESPLIEGUE EN AWS                  ║" -ForegroundColor $Success
Write-Host "╚════════════════════════════════════════════════════════════╝" -ForegroundColor $Success
Write-Host ""

Write-Host "📋 Siguientes pasos:" -ForegroundColor $Warning
Write-Host ""
Write-Host "1. Revisar cambios pendientes:" -ForegroundColor $Info
Write-Host "   git status" -ForegroundColor Gray
Write-Host ""
Write-Host "2. Hacer commit:" -ForegroundColor $Info
Write-Host "   git add -A" -ForegroundColor Gray
Write-Host "   git commit -m 'Proyecto I final: Sistema IoT Distribuido'" -ForegroundColor Gray
Write-Host ""
Write-Host "3. En AWS, clonar y ejecutar:" -ForegroundColor $Info
Write-Host "   git clone <REPO_URL>" -ForegroundColor Gray
Write-Host "   cd Codigo" -ForegroundColor Gray
Write-Host "   bash build.sh" -ForegroundColor Gray
Write-Host "   ./server 8080 server.log &" -ForegroundColor Gray
Write-Host "   python3 auth_service.py &" -ForegroundColor Gray
Write-Host "   python3 sensor_client.py localhost 8080 &" -ForegroundColor Gray
Write-Host "   python3 web_interface.py" -ForegroundColor Gray
Write-Host ""
Write-Host "   Acceder a: http://54.242.32.222:5000" -ForegroundColor $Success
Write-Host ""
