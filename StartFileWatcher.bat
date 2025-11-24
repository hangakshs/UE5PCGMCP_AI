@echo off
REM File Watcher Service Launcher for UE5 Forest Generation
REM This script starts the File Watcher Service in a new window

echo.
echo ╔══════════════════════════════════════════════════════════╗
echo ║   UE5 Forest Generation - File Watcher Service          ║
echo ║   Starting service for UE5 ↔ Python communication       ║
echo ╚══════════════════════════════════════════════════════════╝
echo.

cd /d "%~dp0MCPServer"

REM Check if Python is available
python --version >nul 2>&1
if errorlevel 1 (
    echo ❌ ERROR: Python not found!
    echo    Please install Python 3.7+ and add it to PATH
    pause
    exit /b 1
)

echo 🔧 Checking Python dependencies...
python -c "import sys; from nlp_handler import ForestNLPHandler; print('✅ Dependencies OK')" 2>nul
if errorlevel 1 (
    echo ⚠️  Installing required packages...
    pip install -r requirements.txt
)

echo.
echo 🚀 Starting File Watcher Service...
echo    Press Ctrl+C to stop
echo.
echo ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
echo.

REM Run the File Watcher Service
python file_watcher_service.py

echo.
echo ⏹️  File Watcher Service stopped
pause
