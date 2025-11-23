"""
UE5 Startup Script for MCP Integration
UE5 에디터 시작 시 자동으로 MCP 명령 감시자를 시작합니다.
"""
import unreal
import sys
from pathlib import Path


# Python 경로에 Content/Python 추가
content_python = Path(unreal.Paths.project_content_dir()) / "Python"
if str(content_python) not in sys.path:
    sys.path.insert(0, str(content_python))
    unreal.log(f"Added to Python path: {content_python}")


# MCP Command Watcher 가져오기 및 시작
try:
    from mcp_command_watcher import start_watching, check_commands

    # Watcher 시작
    start_watching()
    unreal.log("=" * 60)
    unreal.log("MCP Command Watcher Started Successfully!")
    unreal.log("=" * 60)
    unreal.log("You can now send forest generation commands from Cursor IDE")
    unreal.log("Command file location: Intermediate/MCP_Commands/pending_command.json")
    unreal.log("=" * 60)

    # 타이머로 주기적으로 명령 확인 (5초마다)
    # UE5에서 EditorUtilityWidget이나 Blueprint로 타이머 설정이 필요합니다
    # 여기서는 수동으로 check_commands()를 호출해야 합니다

except ImportError as e:
    unreal.log_error(f"Failed to import mcp_command_watcher: {e}")
except Exception as e:
    unreal.log_error(f"Error starting MCP Command Watcher: {e}")


unreal.log("UE5 MCP Integration initialized")
