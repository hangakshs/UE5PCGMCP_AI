#!/bin/bash
# UE5 Forest Generation - File Watcher Service Startup Script (Linux/Mac)
# 이 스크립트를 실행하면 UE5와 통신하는 파일 감시 서비스가 시작됩니다.

echo "================================================================"
echo "   UE5 Forest Generation - File Watcher Service"
echo "   Starting standalone file communication service..."
echo "================================================================"
echo ""

# Python 가상환경 활성화 (있는 경우)
if [ -d ".venv" ]; then
    echo "Activating virtual environment..."
    source .venv/bin/activate
elif [ -d "venv" ]; then
    echo "Activating virtual environment..."
    source venv/bin/activate
else
    echo "No virtual environment found, using system Python"
fi

# Python 버전 확인
python3 --version
echo ""

# 서비스 시작
echo "Starting File Watcher Service..."
echo "Press Ctrl+C to stop the service"
echo ""
python3 src/file_watcher_service.py

# 종료 시
echo ""
echo "Service stopped."
