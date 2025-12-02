"""
MCP Command Watcher for UE5

MCP 서버의 명령 파일(pending_command.json)을 감시하고 UE5에서 자동 실행

역할:
- 주기적으로 pending_command.json 확인
- 명령 파일 발견 시 파싱 및 실행
- ForestPCGManagerLibrary 호출하여 숲 생성/제거/수정
- 결과를 result.json에 저장

사용법:
UE5 Python 콘솔에서:
>>> import mcp_command_watcher
>>> mcp_command_watcher.start_watching()
>>> mcp_command_watcher.check_commands()  # 주기적 호출 필요
"""
import unreal
import json
from pathlib import Path
from typing import Any, Dict


class MCPCommandWatcher:
    """
    MCP 명령 파일 감시 및 실행 클래스

    파일 통신 프로토콜:
    1. MCP 서버가 pending_command.json 생성
    2. Watcher가 파일 감지 -> processing_command.json으로 이동
    3. 명령 실행
    4. 결과를 result.json에 저장
    5. processing_command.json 삭제
    """

    def __init__(self):
        """프로젝트 경로 및 명령 파일 경로 초기화"""
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
        return self._generate_forest_from_parameters(params, clear_existing=False)

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
        """숲 수정 (기존 숲 제거 후 재생성)"""
        return self._generate_forest_from_parameters(params, clear_existing=True)

    def _generate_forest_from_parameters(self, params: dict, clear_existing: bool) -> dict:
        """PCG 파라미터를 그대로 적용하여 숲 생성/수정"""
        try:
            forest_lib = unreal.ForestPCGManagerLibrary
            world = unreal.EditorLevelLibrary.get_editor_world()
            manager = forest_lib.get_or_create_forest_pcg_manager(
                world_context_object=world,
                spawn_location=unreal.Vector(0, 0, 0)
            )

            if manager is None:
                return {
                    "success": False,
                    "error": "ForestPCGManager is not available in the current level.",
                    "parameters": params
                }

            forest_params = self._dict_to_pcg_parameters(params)

            if clear_existing:
                manager.clear_forest()

            manager.generate_forest_from_parameters(forest_params)

            applied = self._pcg_parameters_to_dict(forest_params)

            return {
                "success": True,
                "message": "Forest generated from explicit parameters",
                "parameters": params,
                "applied_parameters": applied
            }

        except Exception as e:
            return {
                "success": False,
                "error": f"Exception in _generate_forest_from_parameters: {str(e)}",
                "parameters": params
            }

    def _dict_to_pcg_parameters(self, params: dict) -> unreal.PCGForestParameters:
        """dict -> PCGForestParameters 변환"""
        forest_params = unreal.PCGForestParameters()

        forest_params.tree_type = params.get('tree_type', 'generic_tree')

        tree_types = params.get('tree_types') or []
        if isinstance(tree_types, str):
            tree_types = [tree_types]
        forest_params.tree_types = list(tree_types)

        forest_params.density = params.get('density', 'medium')
        forest_params.size = params.get('size', 'medium')

        forest_params.area_size = self._get_float(params, 'area_size', 1000000.0)
        forest_params.min_distance = self._get_float(params, 'min_distance', 200.0)
        forest_params.max_distance = self._get_float(params, 'max_distance', 500.0)
        forest_params.randomness = self._get_float(params, 'randomness', 0.5)
        forest_params.scale_multiplier = self._get_float(params, 'scale_multiplier', 1.0)
        forest_params.density_multiplier = self._get_float(params, 'density_multiplier', 1.0)

        return forest_params

    def _pcg_parameters_to_dict(self, forest_params: unreal.PCGForestParameters) -> Dict[str, Any]:
        """적용된 파라미터를 dict로 변환 (결과 저장용)"""
        return {
            "tree_type": forest_params.tree_type,
            "tree_types": list(forest_params.tree_types),
            "density": forest_params.density,
            "size": forest_params.size,
            "area_size": forest_params.area_size,
            "min_distance": forest_params.min_distance,
            "max_distance": forest_params.max_distance,
            "randomness": forest_params.randomness,
            "scale_multiplier": forest_params.scale_multiplier,
            "density_multiplier": forest_params.density_multiplier
        }

    def _get_float(self, params: dict, key: str, default: float) -> float:
        """안전한 float 변환"""
        value = params.get(key, default)
        try:
            return float(value)
        except (TypeError, ValueError):
            return default


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
