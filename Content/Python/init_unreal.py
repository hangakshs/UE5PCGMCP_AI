"""
UE5 Startup Script for MCP Integration
UE5 에디터 시작 시 자동으로 File Watcher Service를 시작합니다.
"""
import unreal
import sys
import subprocess
import atexit
from pathlib import Path


# 전역 변수 - 파일 감시 서비스 프로세스
_file_watcher_process = None


def start_file_watcher_service():
    """파일 감시 서비스를 백그라운드 프로세스로 시작"""
    global _file_watcher_process

    # 이미 실행 중이면 스킵
    if _file_watcher_process is not None and _file_watcher_process.poll() is None:
        unreal.log_warning("File Watcher Service already running")
        return

    try:
        # 프로젝트 루트 경로
        project_root = Path(unreal.Paths.project_dir())
        mcp_server_dir = project_root / "MCPServer"
        file_watcher_script = mcp_server_dir / "file_watcher_service.py"

        # 스크립트 존재 확인
        if not file_watcher_script.exists():
            unreal.log_error("=" * 70)
            unreal.log_error("❌ File Watcher Service script not found!")
            unreal.log_error(f"   Expected location: {file_watcher_script}")
            unreal.log_error("")
            unreal.log_error("   MANUAL START REQUIRED:")
            unreal.log_error(f"   1. Open terminal/PowerShell")
            unreal.log_error(f"   2. Run: {project_root / 'StartFileWatcher.bat'}")
            unreal.log_error("   OR double-click StartFileWatcher.bat in project folder")
            unreal.log_error("=" * 70)
            return

        # Python 실행 파일 찾기
        python_exe = sys.executable

        # 파일 감시 서비스 시작 (백그라운드 프로세스)
        _file_watcher_process = subprocess.Popen(
            [python_exe, str(file_watcher_script)],
            cwd=str(mcp_server_dir),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            creationflags=subprocess.CREATE_NO_WINDOW if sys.platform == 'win32' else 0
        )

        # 프로세스 시작 대기 및 확인
        import time
        time.sleep(0.5)

        if _file_watcher_process.poll() is not None:
            # 프로세스가 즉시 종료됨 - 에러 발생
            stderr = _file_watcher_process.stderr.read().decode('utf-8', errors='ignore')
            unreal.log_error("=" * 70)
            unreal.log_error("❌ File Watcher Service failed to start!")
            unreal.log_error(f"   Error: {stderr}")
            unreal.log_error("")
            unreal.log_error("   MANUAL START REQUIRED:")
            unreal.log_error(f"   Run: {project_root / 'StartFileWatcher.bat'}")
            unreal.log_error("=" * 70)
            _file_watcher_process = None
            return

        unreal.log("=" * 70)
        unreal.log("🚀 File Watcher Service Started Automatically!")
        unreal.log("=" * 70)
        unreal.log(f"   Process ID: {_file_watcher_process.pid}")
        unreal.log(f"   Script: {file_watcher_script}")
        unreal.log(f"   Command Dir: {project_root / 'Intermediate' / 'MCP_Commands'}")
        unreal.log("=" * 70)
        unreal.log("✅ NLPPCG System Ready!")
        unreal.log("   You can now generate forests using natural language commands!")
        unreal.log("   Example: '밀집된 소나무 숲'")
        unreal.log("=" * 70)
        unreal.log("⚠️  NOTE: File Watcher Service will stop when UE5 Editor closes")
        unreal.log("=" * 70)

    except Exception as e:
        unreal.log_error("=" * 70)
        unreal.log_error("❌ Failed to start File Watcher Service!")
        unreal.log_error(f"   Error: {e}")
        unreal.log_error("")
        unreal.log_error("   MANUAL START REQUIRED:")
        unreal.log_error("   Double-click 'StartFileWatcher.bat' in project folder")
        unreal.log_error("   OR see '빠른_문제해결.md' for help")
        unreal.log_error("=" * 70)
        import traceback
        unreal.log_error(traceback.format_exc())


def stop_file_watcher_service():
    """파일 감시 서비스 중지"""
    global _file_watcher_process

    if _file_watcher_process is None:
        return

    try:
        if _file_watcher_process.poll() is None:  # 프로세스가 실행 중인 경우
            _file_watcher_process.terminate()
            try:
                _file_watcher_process.wait(timeout=5)
                unreal.log("✅ File Watcher Service stopped successfully")
            except subprocess.TimeoutExpired:
                _file_watcher_process.kill()
                unreal.log("⚠️ File Watcher Service forcefully terminated")

        _file_watcher_process = None

    except Exception as e:
        unreal.log_error(f"Error stopping File Watcher Service: {e}")


# Python 경로에 Content/Python 추가
content_python = Path(unreal.Paths.project_content_dir()) / "Python"
if str(content_python) not in sys.path:
    sys.path.insert(0, str(content_python))
    unreal.log(f"Added to Python path: {content_python}")


# 에디터 종료 시 파일 감시 서비스 자동 중지
atexit.register(stop_file_watcher_service)


# 파일 감시 서비스 자동 시작
try:
    start_file_watcher_service()
except Exception as e:
    unreal.log_error(f"Error in init_unreal.py: {e}")
    import traceback
    unreal.log_error(traceback.format_exc())


unreal.log("UE5 MCP Integration initialized")
