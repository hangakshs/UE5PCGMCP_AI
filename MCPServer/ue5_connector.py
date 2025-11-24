"""
UE5 Connector - MCP 서버와 Unreal Engine 5 연결
Cursor의 MCP 명령을 UE5로 전달
"""
import json
import sys
from pathlib import Path
from typing import Dict, Any, Optional

class UE5Connector:
    """Unreal Engine 5와 통신하는 커넥터"""

    def __init__(self, project_root: Optional[Path] = None):
        self.is_ue5_available = self._check_ue5_environment()

        # 프로젝트 루트 경로 설정
        if project_root is None:
            # 현재 스크립트 위치에서 프로젝트 루트 추론
            current_file = Path(__file__).resolve()
            # MCPServer/ue5_connector.py -> 프로젝트 루트
            project_root = current_file.parent.parent

        self.file_comm = FileBasedCommunication(project_root)

    def _check_ue5_environment(self) -> bool:
        """UE5 Python 환경 확인"""
        try:
            import unreal
            return True
        except ImportError:
            return False

    def execute_forest_command(self, params: Dict[str, Any]) -> Dict[str, Any]:
        """
        숲 생성 명령을 UE5에서 실행

        Args:
            params: 숲 생성 파라미터

        Returns:
            실행 결과 딕셔너리
        """
        # UE5 Python이 아닌 경우 파일 기반 통신 사용
        if not self.is_ue5_available:
            return self.file_comm.send_command(params)

        # UE5 Python 환경인 경우 직접 실행
        try:
            import unreal

            # ForestPCGManagerLibrary 블루프린트 라이브러리 가져오기
            forest_lib = unreal.ForestPCGManagerLibrary

            # 명령 생성
            command_text = self._params_to_command_text(params)

            # UE5에서 숲 생성 함수 호출
            world = unreal.EditorLevelLibrary.get_editor_world()
            success = forest_lib.generate_forest_from_nlp(
                world_context_object=world,
                command=command_text,
                spawn_location=unreal.Vector(0, 0, 0)
            )

            if success:
                return {
                    "success": True,
                    "message": f"UE5에서 숲 생성 완료: {command_text}",
                    "parameters": params
                }
            else:
                return {
                    "success": False,
                    "error": "UE5에서 숲 생성 실패 - ForestPCGManager를 찾을 수 없습니다.",
                    "parameters": params
                }

        except Exception as e:
            return {
                "success": False,
                "error": f"UE5 실행 중 오류: {str(e)}",
                "parameters": params
            }

    def _params_to_command_text(self, params: Dict[str, Any]) -> str:
        """파라미터를 자연어 명령으로 변환 (역변환)"""
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

    def clear_forest(self) -> Dict[str, Any]:
        """UE5에서 숲 제거"""
        # UE5 Python이 아닌 경우 파일 기반 통신 사용
        if not self.is_ue5_available:
            clear_command = {
                "action": "clear_forest",
                "parameters": {}
            }
            return self.file_comm.send_command_raw(clear_command)

        # UE5 Python 환경인 경우 직접 실행
        try:
            import unreal

            forest_lib = unreal.ForestPCGManagerLibrary
            world = unreal.EditorLevelLibrary.get_editor_world()

            success = forest_lib.clear_all_forests(world_context_object=world)

            return {
                "success": success,
                "message": "모든 숲 제거 완료" if success else "숲 제거 실패"
            }

        except Exception as e:
            return {
                "success": False,
                "error": f"숲 제거 중 오류: {str(e)}"
            }


# 파일 기반 통신 (UE5 에디터 외부에서 사용)
class FileBasedCommunication:
    """파일을 통한 UE5 통신 (대안 방법)"""

    def __init__(self, project_root: Path):
        self.project_root = project_root
        self.command_file = project_root / "Intermediate" / "MCP_Commands" / "pending_command.json"
        self.result_file = project_root / "Intermediate" / "MCP_Commands" / "result.json"

        # 디렉토리 생성
        self.command_file.parent.mkdir(parents=True, exist_ok=True)

    def send_command(self, params: Dict[str, Any]) -> Dict[str, Any]:
        """명령을 파일로 저장 (UE5가 읽어서 처리)"""
        command_data = {
            "action": "create_forest",
            "parameters": params
        }
        return self.send_command_raw(command_data)

    def check_ue5_command(self) -> Optional[Dict[str, Any]]:
        """UE5에서 생성한 명령 파일 확인"""
        ue5_command_file = self.command_file.parent / "ue5_command.json"

        if not ue5_command_file.exists():
            return None

        try:
            with open(ue5_command_file, 'r', encoding='utf-8') as f:
                command_data = json.load(f)

            # 파일 삭제 (중복 처리 방지)
            ue5_command_file.unlink()

            return command_data
        except Exception as e:
            print(f"Error reading UE5 command: {e}")
            return None

    def send_command_raw(self, command_data: Dict[str, Any]) -> Dict[str, Any]:
        """Raw 명령을 UE5로 전송 (mcp_response.json으로 저장)"""
        try:
            # UE5가 읽을 응답 파일에 저장
            response_file = self.command_file.parent / "mcp_response.json"

            with open(response_file, 'w', encoding='utf-8') as f:
                json.dump(command_data, f, indent=2, ensure_ascii=False)

            return {
                "success": True,
                "message": f"✅ 명령이 UE5로 전송되었습니다.",
                "file": str(response_file),
                "action": command_data.get("action", "unknown")
            }
        except Exception as e:
            return {
                "success": False,
                "error": f"파일 저장 실패: {str(e)}"
            }


def test_connector():
    """테스트"""
    connector = UE5Connector()
    print(f"UE5 환경 사용 가능: {connector.is_ue5_available}")

    test_params = {
        "tree_type": "pine",
        "density": "dense",
        "size": "medium",
        "area_size": 5000.0,
        "min_distance": 150.0,
        "max_distance": 300.0,
        "randomness": 0.3,
        "scale_multiplier": 1.0
    }

    result = connector.execute_forest_command(test_params)
    print(f"실행 결과: {json.dumps(result, indent=2, ensure_ascii=False)}")


if __name__ == "__main__":
    test_connector()
