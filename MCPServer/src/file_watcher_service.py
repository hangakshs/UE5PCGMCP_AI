"""
UE5 Forest Generation File Watcher Service
UE5와 파일 기반으로 실시간 통신하는 독립 서비스
Cursor MCP와 별개로 동작하며 UE5 에디터가 실행 중일 때만 필요합니다.
"""
import json
import time
import sys
from pathlib import Path
from typing import Optional, Dict, Any
from datetime import datetime
import logging

# 로깅 설정
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler('file_watcher.log'),
        logging.StreamHandler(sys.stdout)
    ]
)
logger = logging.getLogger(__name__)


class UE5FileWatcherService:
    """UE5와 파일 통신을 위한 감시 서비스"""

    def __init__(self, project_root: Optional[Path] = None):
        # 프로젝트 루트 경로 설정
        if project_root is None:
            current_file = Path(__file__).resolve()
            project_root = current_file.parent.parent

        self.project_root = project_root
        self.comm_dir = project_root / "Intermediate" / "MCP_Commands"
        self.ue5_command_file = self.comm_dir / "ue5_command.json"
        self.mcp_response_file = self.comm_dir / "mcp_response.json"

        # 디렉토리 생성
        self.comm_dir.mkdir(parents=True, exist_ok=True)

        # NLP 핸들러 초기화
        try:
            from nlp_handler import ForestNLPHandler
            self.nlp_handler = ForestNLPHandler()
            logger.info("✅ NLP Handler initialized")
        except Exception as e:
            logger.error(f"❌ Failed to initialize NLP Handler: {e}")
            self.nlp_handler = None

        # 마지막 처리 시간 (중복 방지)
        self.last_processed_hash: Optional[str] = None
        self.is_running = False

        logger.info(f"📁 File Watcher initialized")
        logger.info(f"   Project Root: {self.project_root}")
        logger.info(f"   Command Dir: {self.comm_dir}")

    def start(self, poll_interval: float = 0.5):
        """
        서비스 시작

        Args:
            poll_interval: 파일 확인 간격 (초)
        """
        self.is_running = True
        logger.info(f"🚀 File Watcher Service started (poll interval: {poll_interval}s)")
        logger.info("   Waiting for commands from UE5...")
        logger.info("   Press Ctrl+C to stop")

        try:
            while self.is_running:
                self._check_and_process_command()
                time.sleep(poll_interval)
        except KeyboardInterrupt:
            logger.info("\n⏹️  Service stopped by user")
            self.is_running = False
        except Exception as e:
            logger.error(f"❌ Service error: {e}")
            raise

    def stop(self):
        """서비스 중지"""
        self.is_running = False
        logger.info("⏹️  Service stopped")

    def _check_and_process_command(self):
        """UE5 명령 파일 확인 및 처리"""
        # 명령 파일이 존재하는지 확인
        if not self.ue5_command_file.exists():
            return

        try:
            # 파일 읽기
            with open(self.ue5_command_file, 'r', encoding='utf-8') as f:
                command_data = json.load(f)

            # 중복 처리 방지 (파일 내용 해시)
            content_hash = hash(json.dumps(command_data, sort_keys=True))
            if content_hash == self.last_processed_hash:
                return

            self.last_processed_hash = content_hash

            # 명령 처리
            command = command_data.get('command', '')
            timestamp = command_data.get('timestamp', 0)

            logger.info(f"\n{'='*60}")
            logger.info(f"📨 Received command from UE5")
            logger.info(f"   Command: {command}")
            logger.info(f"   Timestamp: {timestamp}")

            # NLP 파싱 및 응답 생성
            response = self._process_command(command)

            # 응답 파일 작성
            self._write_response(response)

            # 명령 파일 삭제 (처리 완료)
            self.ue5_command_file.unlink()
            logger.info(f"✅ Command processed successfully")
            logger.info(f"{'='*60}\n")

        except json.JSONDecodeError as e:
            logger.error(f"❌ Invalid JSON in command file: {e}")
        except Exception as e:
            logger.error(f"❌ Error processing command: {e}")
            import traceback
            logger.error(traceback.format_exc())

    def _process_command(self, command: str) -> Dict[str, Any]:
        """
        명령 처리 및 응답 생성

        Args:
            command: UE5에서 받은 자연어 명령

        Returns:
            UE5로 보낼 응답 데이터
        """
        # 특수 명령 처리
        if command.lower() in ['clear', 'remove', '제거', '삭제']:
            return {
                "action": "clear_forest",
                "parameters": {
                    "area_size": 0.0
                }
            }

        # NLP 파싱
        if self.nlp_handler is None:
            logger.error("❌ NLP Handler not available")
            return self._create_error_response("NLP Handler가 초기화되지 않았습니다")

        try:
            params = self.nlp_handler.parse_forest_command(command)

            logger.info(f"   Parsed parameters:")
            logger.info(f"      Tree Type: {params.get('tree_type', 'N/A')}")
            logger.info(f"      Density: {params.get('density', 'N/A')}")
            logger.info(f"      Area Size: {params.get('area_size', 'N/A')} cm²")
            logger.info(f"      Min Distance: {params.get('min_distance', 'N/A')} cm")

            return {
                "action": "create_forest",
                "parameters": params,
                "timestamp": time.time(),
                "original_command": command
            }

        except Exception as e:
            logger.error(f"❌ NLP parsing failed: {e}")
            return self._create_error_response(f"명령 파싱 실패: {str(e)}")

    def _create_error_response(self, error_message: str) -> Dict[str, Any]:
        """에러 응답 생성"""
        return {
            "action": "error",
            "error": error_message,
            "parameters": {
                "area_size": 0.0
            }
        }

    def _write_response(self, response: Dict[str, Any]):
        """응답 파일 작성"""
        try:
            with open(self.mcp_response_file, 'w', encoding='utf-8') as f:
                json.dump(response, f, indent=2, ensure_ascii=False)

            logger.info(f"   Response written to: {self.mcp_response_file}")
            logger.info(f"   Action: {response.get('action', 'N/A')}")

        except Exception as e:
            logger.error(f"❌ Failed to write response file: {e}")


def main():
    """메인 진입점"""
    print("""
╔══════════════════════════════════════════════════════════╗
║   UE5 Forest Generation - File Watcher Service          ║
║   Standalone service for UE5 ↔ Python communication     ║
╚══════════════════════════════════════════════════════════╝
    """)

    # 프로젝트 루트 확인
    current_file = Path(__file__).resolve()
    project_root = current_file.parent.parent

    print(f"📂 Project Root: {project_root}")
    print(f"📁 Communication Dir: {project_root / 'Intermediate' / 'MCP_Commands'}")
    print()

    # 서비스 시작
    service = UE5FileWatcherService(project_root)
    service.start(poll_interval=0.3)  # 300ms 간격으로 확인


if __name__ == "__main__":
    main()
