#!/bin/bash
# File Watcher Service Launcher for UE5 Forest Generation
# This script starts the File Watcher Service

echo ""
echo "╔══════════════════════════════════════════════════════════╗"
echo "║   UE5 Forest Generation - File Watcher Service          ║"
echo "║   Starting service for UE5 ↔ Python communication       ║"
echo "╚══════════════════════════════════════════════════════════╝"
echo ""

# Change to MCPServer directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR/MCPServer"

# Check if Python is available
if ! command -v python3 &> /dev/null; then
    echo "❌ ERROR: Python3 not found!"
    echo "   Please install Python 3.7+ and add it to PATH"
    exit 1
fi

echo "🔧 Checking Python dependencies..."
python3 -c "import sys; from nlp_handler import ForestNLPHandler; print('✅ Dependencies OK')" 2>/dev/null
if [ $? -ne 0 ]; then
    echo "⚠️  Installing required packages..."
    pip3 install -r requirements.txt
fi

echo ""
echo "🚀 Starting File Watcher Service..."
echo "   Press Ctrl+C to stop"
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Run the File Watcher Service
python3 file_watcher_service.py

echo ""
echo "⏹️  File Watcher Service stopped"
