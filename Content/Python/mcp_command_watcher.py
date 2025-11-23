"""
MCP Command Watcher for UE5
MCP 서버의 명령을 감시하고 UE5에서 자동 실행

UE5 에디터에서 이 스크립트를 실행하거나 startup script로 등록하세요.
"""
import unreal
import json
import time
from pathlib import Path


class MCPCommandWatcher:
    """MCP 명령 파일을 감시하고 실행"""

    def __init__(self):
        # 프로젝트 루트 경로
        self.project_root = Path(unreal.Paths.project_dir())
        self.command_dir = self.project_root / "Intermediate" / "MCP_Commands"
        self.command_file = self.command_dir / "pending_command.json"
        self.processing_file = self.command_dir / "processing_command.json"
        self.result_file = self.command_dir / "result.json"

        # 디렉토리 생성
        self.command_dir.mkdir(parents=True, exist_ok=True)

        unreal.log("MCP Command Watcher initialized")
        unreal.log(f"Watching directory: {self.command_dir}")

    def check_and_execute(self):
        """새 명령 확인 및 실행"""
        if not self.command_file.exists():
            return

        try:
            # 명령 파일 읽기
            with open(self.command_file, 'r', encoding='utf-8') as f:
                command_data = json.load(f)

            # 처리 중 표시 (파일 이동)
            self.command_file.rename(self.processing_file)

            unreal.log(f"Processing MCP command: {command_data.get('action', 'unknown')}")

            # 명령 실행
            result = self.execute_command(command_data)

            # 결과 저장
            with open(self.result_file, 'w', encoding='utf-8') as f:
                json.dump(result, f, indent=2, ensure_ascii=False)

            # 처리 완료 - 파일 삭제
            self.processing_file.unlink()

            unreal.log(f"Command executed. Result: {result.get('success', False)}")

        except Exception as e:
            error_msg = f"Error executing MCP command: {str(e)}"
            unreal.log_error(error_msg)

            # 에러 결과 저장
            error_result = {
                "success": False,
                "error": error_msg
            }
            with open(self.result_file, 'w', encoding='utf-8') as f:
                json.dump(error_result, f, indent=2, ensure_ascii=False)

            # 처리 파일 삭제
            if self.processing_file.exists():
                self.processing_file.unlink()

    def execute_command(self, command_data: dict) -> dict:
        """명령 실행"""
        action = command_data.get("action", "")
        parameters = command_data.get("parameters", {})

        if action == "create_forest":
            return self.create_forest(parameters)
        elif action == "clear_forest":
            return self.clear_forest()
        elif action == "modify_forest":
            return self.modify_forest(parameters)
        else:
            return {
                "success": False,
                "error": f"Unknown action: {action}"
            }

    def create_forest(self, params: dict) -> dict:
        """숲 생성"""
        try:
            # ForestPCGManagerLibrary 가져오기
            forest_lib = unreal.ForestPCGManagerLibrary

            # 명령 텍스트 생성
            command_text = self._params_to_command(params)

            # 현재 월드 가져오기
            world = unreal.EditorLevelLibrary.get_editor_world()

            # 숲 생성
            success = forest_lib.generate_forest_from_nlp(
                world_context_object=world,
                command=command_text,
                spawn_location=unreal.Vector(0, 0, 0)
            )

            if success:
                return {
                    "success": True,
                    "message": f"Forest created: {command_text}",
                    "parameters": params
                }
            else:
                return {
                    "success": False,
                    "error": "Failed to create forest - ForestPCGManager not found",
                    "parameters": params
                }

        except Exception as e:
            return {
                "success": False,
                "error": f"Exception in create_forest: {str(e)}",
                "parameters": params
            }

    def clear_forest(self) -> dict:
        """숲 제거"""
        try:
            forest_lib = unreal.ForestPCGManagerLibrary
            world = unreal.EditorLevelLibrary.get_editor_world()

            success = forest_lib.clear_all_forests(world_context_object=world)

            return {
                "success": success,
                "message": "All forests cleared" if success else "Failed to clear forests"
            }

        except Exception as e:
            return {
                "success": False,
                "error": f"Exception in clear_forest: {str(e)}"
            }

    def modify_forest(self, params: dict) -> dict:
        """숲 수정 (현재는 재생성으로 구현)"""
        # 기존 숲 제거
        clear_result = self.clear_forest()
        if not clear_result.get("success"):
            return clear_result

        # 새로운 파라미터로 재생성
        return self.create_forest(params)

    def _params_to_command(self, params: dict) -> str:
        """파라미터를 명령 텍스트로 변환"""
        density_map = {
            'dense': '밀집된',
            'medium': '보통',
            'sparse': '성긴'
        }

        tree_map = {
            'pine': '소나무',
            'oak': '참나무',
            'birch': '자작나무',
            'maple': '단풍나무',
            'generic_tree': '나무'
        }

        density = density_map.get(params.get('density', 'medium'), '보통')
        tree = tree_map.get(params.get('tree_type', 'generic_tree'), '나무')

        return f"{density} {tree} 숲"


# 전역 watcher 인스턴스
_watcher = None


def start_watching():
    """명령 감시 시작"""
    global _watcher
    _watcher = MCPCommandWatcher()
    unreal.log("MCP Command Watcher started")


def check_commands():
    """명령 확인 (주기적으로 호출)"""
    global _watcher
    if _watcher is None:
        start_watching()

    _watcher.check_and_execute()


def stop_watching():
    """명령 감시 중지"""
    global _watcher
    _watcher = None
    unreal.log("MCP Command Watcher stopped")


# UE5 에디터에서 직접 실행할 때
if __name__ == "__main__":
    start_watching()
    unreal.log("MCP Command Watcher ready. Call check_commands() periodically.")
