"""
파일 기반 통신 테스트
UE5와 MCP 서버 간의 파일 통신을 테스트합니다.
"""
import json
import time
from pathlib import Path
from nlp_handler import ForestNLPHandler
from ue5_connector import UE5Connector


def test_file_communication():
    """파일 기반 통신 테스트"""
    print("=== File-Based Communication Test ===\n")

    # 핸들러 초기화
    nlp_handler = ForestNLPHandler()
    ue5_connector = UE5Connector()

    # 테스트 명령
    test_commands = [
        "밀집된 소나무 숲",
        "성긴 참나무 숲 1000평방미터",
        "보통 자작나무 숲"
    ]

    for i, command in enumerate(test_commands, 1):
        print(f"\n--- Test {i}: {command} ---")

        # NLP 파싱
        params = nlp_handler.parse_forest_command(command)
        print(f"Parsed parameters: {json.dumps(params, indent=2, ensure_ascii=False)}")

        # UE5로 명령 전송
        result = ue5_connector.execute_forest_command(params)
        print(f"Send result: {result.get('message', 'Unknown')}")

        # 응답 파일 확인
        response_file = Path(ue5_connector.file_comm.command_file.parent) / "mcp_response.json"
        if response_file.exists():
            with open(response_file, 'r', encoding='utf-8') as f:
                response_data = json.load(f)
            print(f"Response file created: {response_file}")
            print(f"Action: {response_data.get('action', 'N/A')}")
            print("✅ Test passed")
        else:
            print("❌ Response file not created")

        time.sleep(0.5)

    print("\n=== All tests completed ===")


if __name__ == "__main__":
    test_file_communication()
