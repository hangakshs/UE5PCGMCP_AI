"""
MCP Command Watcher with Auto-Polling
자동으로 주기적으로 MCP 명령을 확인하는 스크립트

UE5 에디터의 Python 콘솔에서 실행:
>>> import start_mcp_watcher
>>> start_mcp_watcher.start()
"""
import unreal
import sys
from pathlib import Path


# Python 경로 설정
content_python = Path(unreal.Paths.project_content_dir()) / "Python"
if str(content_python) not in sys.path:
    sys.path.insert(0, str(content_python))


# Watcher 임포트
from mcp_command_watcher import MCPCommandWatcher


# 전역 변수
_watcher_instance = None
_timer_handle = None


def start():
    """MCP 명령 감시 시작 (5초마다 자동 확인)"""
    global _watcher_instance, _timer_handle

    if _watcher_instance is not None:
        unreal.log_warning("MCP Watcher already running. Stop it first.")
        return

    # Watcher 인스턴스 생성
    _watcher_instance = MCPCommandWatcher()

    unreal.log("=" * 70)
    unreal.log("🎮 MCP Command Watcher Started!")
    unreal.log("=" * 70)
    unreal.log("✅ Auto-polling enabled (every 5 seconds)")
    unreal.log("📁 Watching: Intermediate/MCP_Commands/pending_command.json")
    unreal.log("🔧 Now you can send commands from Cursor IDE!")
    unreal.log("=" * 70)
    unreal.log("Commands:")
    unreal.log("  - start_mcp_watcher.start()  : Start watcher")
    unreal.log("  - start_mcp_watcher.stop()   : Stop watcher")
    unreal.log("  - start_mcp_watcher.status() : Check status")
    unreal.log("=" * 70)

    # 타이머 시작 (5초마다 check_tick 호출)
    _timer_handle = unreal.register_slate_post_tick_callback(_check_tick)

    # 즉시 한 번 확인
    _check_commands()


def stop():
    """MCP 명령 감시 중지"""
    global _watcher_instance, _timer_handle

    if _watcher_instance is None:
        unreal.log_warning("MCP Watcher not running")
        return

    # 타이머 중지
    if _timer_handle is not None:
        unreal.unregister_slate_post_tick_callback(_timer_handle)
        _timer_handle = None

    _watcher_instance = None
    unreal.log("🛑 MCP Command Watcher stopped")


def status():
    """현재 상태 확인"""
    if _watcher_instance is None:
        unreal.log("❌ MCP Watcher is NOT running")
        unreal.log("   Run: start_mcp_watcher.start()")
    else:
        unreal.log("✅ MCP Watcher is RUNNING")
        unreal.log(f"   Watching: {_watcher_instance.command_dir}")


# 내부 변수 (타이머용)
_last_check_time = 0.0
_check_interval = 5.0  # 5초


def _check_tick(delta_time: float):
    """매 프레임 호출되는 콜백"""
    global _last_check_time

    _last_check_time += delta_time

    if _last_check_time >= _check_interval:
        _last_check_time = 0.0
        _check_commands()


def _check_commands():
    """명령 확인 및 실행"""
    global _watcher_instance

    if _watcher_instance is None:
        return

    try:
        _watcher_instance.check_and_execute()
    except Exception as e:
        unreal.log_error(f"Error in MCP command check: {e}")


# 모듈 로드 시 자동 시작 (선택적)
# start()


if __name__ == "__main__":
    start()
