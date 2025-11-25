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

# 로깅 설정 - Windows 콘솔 인코딩 문제 해결
import io

# Windows 콘솔 인코딩 설정
if sys.platform == 'win32':
    # Windows 콘솔을 UTF-8로 강제 설정
    if sys.stdout.encoding != 'utf-8':
        sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
    if sys.stderr.encoding != 'utf-8':
        sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding='utf-8', errors='replace')

logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler('file_watcher.log', encoding='utf-8'),
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
            logger.warning("⚠️ Project root not provided, using auto-detection")

        self.project_root = Path(project_root).resolve()  # 절대 경로로 변환
        self.comm_dir = self.project_root / "Intermediate" / "MCP_Commands"
        self.ue5_command_file = self.comm_dir / "ue5_command.json"
        self.mcp_response_file = self.comm_dir / "mcp_response.json"

        # 디렉토리 생성 및 확인
        logger.info("=" * 70)
        logger.info("📁 Initializing File Watcher...")
        logger.info("=" * 70)
        logger.info(f"   Project Root: {self.project_root}")
        logger.info(f"   Project Root (absolute): {self.project_root.resolve()}")
        logger.info(f"   Command Dir: {self.comm_dir}")
        logger.info(f"   Command File: {self.ue5_command_file}")
        logger.info(f"   Response File: {self.mcp_response_file}")
        logger.info(f"   Command Dir exists: {self.comm_dir.exists()}")

        if not self.comm_dir.exists():
            logger.info(f"   Creating Command Dir...")
            self.comm_dir.mkdir(parents=True, exist_ok=True)
            logger.info(f"   Created: {self.comm_dir.exists()}")
        else:
            logger.info(f"   Command Dir already exists")

        # NLP 핸들러 초기화
        try:
            # src 디렉토리를 Python 경로에 추가
            src_dir = Path(__file__).parent
            if str(src_dir) not in sys.path:
                sys.path.insert(0, str(src_dir))
                logger.info(f"   Added to sys.path: {src_dir}")

            from nlp_handler import ForestNLPHandler
            self.nlp_handler = ForestNLPHandler()
            logger.info("✅ NLP Handler initialized successfully")
        except ImportError as e:
            logger.error(f"❌ Failed to import NLP Handler: {e}")
            logger.error(f"   Current working directory: {Path.cwd()}")
            logger.error(f"   Python path: {sys.path}")
            logger.error(f"   Attempting to import from: {src_dir}")
            self.nlp_handler = None
        except Exception as e:
            logger.error(f"❌ Failed to initialize NLP Handler: {e}")
            logger.error(f"   Error type: {type(e).__name__}")
            import traceback
            logger.error(traceback.format_exc())
            self.nlp_handler = None

        # 마지막 처리 시간 (중복 방지)
        self.last_processed_hash: Optional[str] = None
        self.is_running = False

        logger.info(f"✅ File Watcher initialized")
        logger.info(f"   NLP Handler: {'Available' if self.nlp_handler else 'NOT Available'}")

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

        logger.info(f"\n{'='*70}")
        logger.info(f"📨 Command file detected!")
        logger.info(f"   File: {self.ue5_command_file}")
        logger.info(f"   File size: {self.ue5_command_file.stat().st_size} bytes")

        try:
            # 파일 읽기 (여러 인코딩 시도 - Windows 호환성)
            file_content = None
            for encoding in ['utf-8-sig', 'utf-8', 'utf-16-le', 'utf-16-be']:
                try:
                    with open(self.ue5_command_file, 'r', encoding=encoding) as f:
                        file_content = f.read()
                        logger.info(f"   File read with encoding: {encoding}")
                        break
                except UnicodeDecodeError:
                    continue

            if file_content is None:
                logger.error(f"❌ Could not read file with any known encoding")
                logger.error(f"{'='*70}\n")
                return

            logger.info(f"   File content: {file_content}")

            # JSON 파싱
            command_data = json.loads(file_content)
            logger.info(f"   ✅ JSON parsed successfully")

            # 중복 처리 방지 (파일 내용 해시)
            content_hash = hash(json.dumps(command_data, sort_keys=True))
            if content_hash == self.last_processed_hash:
                logger.info(f"   ⚠️  Duplicate command (already processed), skipping")
                logger.info(f"{'='*70}\n")
                return

            self.last_processed_hash = content_hash

            # 명령 처리
            command = command_data.get('command', '')
            timestamp = command_data.get('timestamp', 0)

            logger.info(f"   Command: '{command}'")
            logger.info(f"   Timestamp: {timestamp}")

            # NLP 파싱 및 응답 생성
            logger.info(f"   Processing command with NLP Handler...")
            response = self._process_command(command)
            logger.info(f"   ✅ Command processed")
            logger.info(f"   Response action: {response.get('action', 'N/A')}")

            # 응답 파일 작성
            logger.info(f"   Writing response file...")
            self._write_response(response)

            # 명령 파일 삭제 (처리 완료)
            logger.info(f"   Deleting command file...")
            self.ue5_command_file.unlink()
            logger.info(f"✅ Command file deleted")
            logger.info(f"✅ Processing complete!")
            logger.info(f"{'='*70}\n")

        except json.JSONDecodeError as e:
            logger.error(f"❌ Invalid JSON in command file: {e}")
            logger.error(f"   File content: {file_content if 'file_content' in locals() else 'Could not read'}")
            logger.error(f"{'='*70}\n")
        except Exception as e:
            logger.error(f"❌ Error processing command: {e}")
            logger.error(f"   Error type: {type(e).__name__}")
            import traceback
            logger.error(traceback.format_exc())
            logger.error(f"{'='*70}\n")

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

    # 명령줄 인수에서 프로젝트 루트 읽기
    import argparse
    parser = argparse.ArgumentParser(description='UE5 File Watcher Service')
    parser.add_argument('--project-root', type=str, help='UE5 Project root directory')
    args = parser.parse_args()

    # 프로젝트 루트 결정
    if args.project_root:
        project_root = Path(args.project_root)
        logger.info(f"📂 Project Root (from command line): {project_root}")
    else:
        # 폴백: 스크립트 위치 기준으로 계산
        current_file = Path(__file__).resolve()
        project_root = current_file.parent.parent
        logger.info(f"📂 Project Root (auto-detected): {project_root}")

    print(f"📂 Project Root: {project_root}")
    print(f"📁 Communication Dir: {project_root / 'Intermediate' / 'MCP_Commands'}")
    print()

    # 경로 유효성 확인
    if not project_root.exists():
        logger.error(f"❌ Project root does not exist: {project_root}")
        logger.error("   Please check the path and try again")
        return

    # 서비스 시작
    service = UE5FileWatcherService(project_root)
    service.start(poll_interval=0.3)  # 300ms 간격으로 확인


if __name__ == "__main__":
    main()
