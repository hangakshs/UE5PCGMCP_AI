"""
UE5 Startup Script for MCP Integration

UE5 에디터 시작 시 자동으로 File Watcher Service를 시작합니다.

주의:
- 현재는 C++ 플러그인 모듈(NLPPCGModule)에서 File Watcher를 시작하므로
  이 스크립트에서는 중복 실행을 방지하기 위해 시작하지 않습니다.
- Python 경로 설정만 수행합니다.
"""
import unreal
import sys
import subprocess
import atexit
from pathlib import Path


# 전역 변수 - 파일 감시 서비스 프로세스
_file_watcher_process = None


def start_file_watcher_service():
    """
    파일 감시 서비스를 백그라운드 프로세스로 시작

    Note: 이 함수는 수동 시작을 위해 남겨두었지만,
    현재는 C++ 모듈에서 자동 시작됩니다.
    """
    global _file_watcher_process

    # 이미 실행 중이면 스킵
    if _file_watcher_process is not None and _file_watcher_process.poll() is None:
        unreal.log_warning("File Watcher Service already running")
        return

    try:
        # 프로젝트 루트 경로
        project_root = Path(unreal.Paths.project_dir())
        mcp_server_dir = project_root / "MCPServer"
        file_watcher_script = mcp_server_dir / "src" / "file_watcher_service.py"

        # 스크립트 존재 확인
        if not file_watcher_script.exists():
            unreal.log_error("=" * 70)
            unreal.log_error("❌ File Watcher Service script not found!")
            unreal.log_error(f"   Expected location: {file_watcher_script}")
            unreal.log_error("")
            unreal.log_error("   MANUAL START REQUIRED:")
            unreal.log_error(f"   1. Open terminal/PowerShell")
            unreal.log_error(f"   2. Run: {project_root / 'MCPServer' / 'scripts' / 'StartFileWatcher.bat'}")
            unreal.log_error("   OR double-click StartFileWatcher.bat in MCPServer/scripts folder")
            unreal.log_error("=" * 70)
            return

        # Python 실행 파일 찾기
        python_exe = sys.executable

        # 파일 감시 서비스 시작 (백그라운드 프로세스)
        # stdout/stderr를 캡처하여 UE5 로그에 표시
        # 프로젝트 루트를 명령줄 인수로 전달

        # Windows에서 창이 뜨지 않도록 설정
        startupinfo = None
        if sys.platform == 'win32':
            startupinfo = subprocess.STARTUPINFO()
            startupinfo.dwFlags |= subprocess.STARTF_USESHOWWINDOW
            startupinfo.wShowWindow = 0  # SW_HIDE

        _file_watcher_process = subprocess.Popen(
            [python_exe, str(file_watcher_script), '--project-root', str(project_root)],
            cwd=str(mcp_server_dir),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            stdin=subprocess.DEVNULL,
            startupinfo=startupinfo,
            creationflags=subprocess.CREATE_NO_WINDOW if sys.platform == 'win32' else 0
        )

        unreal.log(f"🚀 Starting File Watcher with project root: {project_root}")

        # 프로세스 시작 대기 및 확인
        import time
        time.sleep(1.0)  # 조금 더 기다림

        if _file_watcher_process.poll() is not None:
            # 프로세스가 즉시 종료됨 - 에러 발생
            stdout = _file_watcher_process.stdout.read().decode('utf-8', errors='ignore')
            stderr = _file_watcher_process.stderr.read().decode('utf-8', errors='ignore')

            unreal.log_error("=" * 70)
            unreal.log_error("❌ File Watcher Service failed to start!")

            if stdout:
                unreal.log_error("   STDOUT:")
                for line in stdout.strip().split('\n'):
                    unreal.log_error(f"      {line}")

            if stderr:
                unreal.log_error("   STDERR:")
                for line in stderr.strip().split('\n'):
                    unreal.log_error(f"      {line}")

            unreal.log_error("")
            unreal.log_error("   MANUAL START REQUIRED:")
            unreal.log_error(f"   Run: {project_root / 'MCPServer' / 'scripts' / 'StartFileWatcher.bat'}")
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
    """
    파일 감시 서비스 중지

    에디터 종료 시 자동으로 호출됩니다 (atexit 등록됨)
    """
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
# NOTE: C++ 플러그인 모듈(NLPPCGModule)에서 이미 시작하므로 여기서는 시작하지 않음
# 중복 실행 방지를 위해 주석 처리
# try:
#     start_file_watcher_service()
# except Exception as e:
#     unreal.log_error(f"Error in init_unreal.py: {e}")
#     import traceback
#     unreal.log_error(traceback.format_exc())

unreal.log("=" * 70)
unreal.log("UE5 MCP Integration initialized")
unreal.log("File Watcher Service is managed by NLPPCG C++ Module")
unreal.log("=" * 70)
