"""
MCP Server for Unreal Engine PCG Forest Generation

이 서버는 MCP(Model Context Protocol)를 통해 Claude/Cursor와 통신하여
자연어 명령을 Unreal Engine 5의 PCG 숲 생성 파라미터로 변환합니다.

주요 기능:
- create_forest: 자연어로 숲 생성
- clear_forest: 모든 PCG 숲 제거
- modify_forest: 기존 숲 수정

파일 기반 통신:
- MCP 명령 -> UE5 파일 감시 서비스 -> UE5 에디터
"""
import asyncio
import json
import sys
from copy import deepcopy
from pathlib import Path
from typing import Any, Sequence, Dict
from mcp.server import Server
from mcp.types import Tool, TextContent, ImageContent, EmbeddedResource
from mcp.server.stdio import stdio_server
from nlp_handler import ForestNLPHandler
from ue5_connector import UE5Connector

# Windows에서 UTF-8 인코딩 설정 (이모지 출력을 위해)
if sys.platform == 'win32':
    try:
        sys.stdout.reconfigure(encoding='utf-8')
        sys.stderr.reconfigure(encoding='utf-8')
    except AttributeError:
        # Python 3.6 이하에서는 reconfigure가 없음
        import io
        sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')
        sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding='utf-8')


class PCGForestMCPServer:
    """MCP 프로토콜 기반 PCG 숲 생성 서버"""
    def __init__(self):
        """서버 초기화 및 핸들러, 커넥터 설정"""
        self.app = Server("pcg-forest-server")
        self.nlp_handler = ForestNLPHandler()  # 자연어 파싱 핸들러
        self.ue5_connector = UE5Connector()     # UE5 통신 커넥터
        self.setup_handlers()
        self.last_parameters: Dict[str, Any] | None = None
        self.state_file = self.ue5_connector.file_comm.command_file.parent / "last_parameters.json"
        self._load_last_parameters()

    def setup_handlers(self):
        """MCP 프로토콜 핸들러 등록 (도구 목록, 도구 호출)"""

        @self.app.list_tools()
        async def list_tools() -> list[Tool]:
            """사용 가능한 도구 목록 반환"""
            return [
                Tool(
                    name="create_forest",
                    description="자연어 명령으로 PCG 기반 숲을 생성합니다. "
                                "예: '밀집된 소나무 숲 만들어줘', '성긴 참나무 숲 500평방미터'",
                    inputSchema={
                        "type": "object",
                        "properties": {
                            "command": {
                                "type": "string",
                                "description": "숲 생성에 대한 자연어 명령"
                            }
                        },
                        "required": ["command"]
                    }
                ),
                Tool(
                    name="clear_forest",
                    description="현재 생성된 PCG 숲을 제거합니다.",
                    inputSchema={
                        "type": "object",
                        "properties": {},
                    }
                ),
                Tool(
                    name="modify_forest",
                    description="기존 숲의 밀도나 크기를 수정합니다.",
                    inputSchema={
                        "type": "object",
                        "properties": {
                            "command": {
                                "type": "string",
                                "description": "수정 명령 (예: '더 빽빽하게', '나무를 크게')"
                            }
                        },
                        "required": ["command"]
                    }
                )
            ]

        @self.app.call_tool()
        async def call_tool(name: str, arguments: Any) -> Sequence[TextContent | ImageContent | EmbeddedResource]:
            """도구 호출 처리"""

            if name == "create_forest":
                return await self.handle_create_forest(arguments)
            elif name == "clear_forest":
                return await self.handle_clear_forest()
            elif name == "modify_forest":
                return await self.handle_modify_forest(arguments)
            else:
                raise ValueError(f"Unknown tool: {name}")

    async def handle_create_forest(self, arguments: dict) -> Sequence[TextContent]:
        """
        숲 생성 명령 처리

        Args:
            arguments: {"command": "자연어 명령"}

        Returns:
            실행 결과 텍스트 콘텐츠
        """
        command = arguments.get("command", "")

        if not command:
            return [TextContent(
                type="text",
                text="오류: 명령이 제공되지 않았습니다."
            )]

        try:
            # NLP로 파라미터 파싱
            params = self.nlp_handler.parse_forest_command(command)
            overrides = params.pop('_overrides', [])

            # 사용자 응답 생성
            response_text = self.nlp_handler.generate_response(params)

            # UE5에서 직접 실행
            execution_result = self.ue5_connector.execute_forest_command(params)

            if execution_result.get("success"):
                self.last_parameters = deepcopy(params)
                self._persist_last_parameters()

            # 결과 텍스트 생성
            result_text = f"[명령] {command}\n\n"
            result_text += f"[파싱 결과]\n{response_text}\n\n"

            if execution_result.get("success"):
                result_text += f"[UE5 실행 결과] 성공\n"
                result_text += f"   {execution_result.get('message', '')}\n"
            else:
                result_text += f"[UE5 실행 결과] 실패\n"
                result_text += f"   오류: {execution_result.get('error', 'Unknown error')}\n"

            result_text += f"\n--- 생성된 파라미터 ---\n"
            result_text += json.dumps(params, indent=2, ensure_ascii=False)

            return [TextContent(
                type="text",
                text=result_text
            )]

        except Exception as e:
            return [TextContent(
                type="text",
                text=f"오류 발생: {str(e)}"
            )]

    async def handle_clear_forest(self) -> Sequence[TextContent]:
        """
        숲 제거 명령 처리

        Returns:
            실행 결과 텍스트 콘텐츠
        """
        try:
            # UE5에서 직접 실행
            execution_result = self.ue5_connector.clear_forest()

            result_text = "[명령] 모든 PCG 숲을 제거합니다.\n\n"

            if execution_result.get("success"):
                result_text += f"[UE5 실행 결과] 성공\n"
                result_text += f"   {execution_result.get('message', '')}\n"
            else:
                result_text += f"[UE5 실행 결과] 실패\n"
                result_text += f"   오류: {execution_result.get('error', 'Unknown error')}\n"

            return [TextContent(
                type="text",
                text=result_text
            )]

        except Exception as e:
            return [TextContent(
                type="text",
                text=f"오류 발생: {str(e)}"
            )]

    async def handle_modify_forest(self, arguments: dict) -> Sequence[TextContent]:
        """
        숲 수정 명령 처리

        Args:
            arguments: {"command": "수정 명령"}

        Returns:
            실행 결과 텍스트 콘텐츠
        """
        command = arguments.get("command", "")

        if not command:
            return [TextContent(
                type="text",
                text="오류: 수정 명령이 제공되지 않았습니다."
            )]

        try:
            # 기존 파싱 로직 재사용
            params = self.nlp_handler.parse_forest_command(command)
            overrides = set(params.pop('_overrides', []))
            params['action'] = 'modify_forest'

            if self.last_parameters is None:
                return [TextContent(
                    type="text",
                    text="오류: 수정할 기존 숲 정보가 없습니다. 먼저 숲을 생성해 주세요."
                )]

            if not overrides:
                return [TextContent(
                    type="text",
                    text="오류: 수정할 속성이 감지되지 않았습니다. (예: '밀도를 2배로' 같이 변경할 값을 명시해 주세요.)"
                )]

            merged_params = deepcopy(self.last_parameters)
            merged_params['action'] = 'modify_forest'

            for key in overrides:
                if key in params:
                    merged_params[key] = params[key]

            # tree_type과 tree_types 동기화
            if 'tree_types' in overrides and 'tree_types' not in merged_params:
                merged_params['tree_types'] = []

            params = merged_params

            response_text = self.nlp_handler.generate_response(params)

            # UE5로 명령 전송
            ue_command = {
                "action": "modify_forest",
                "parameters": params
            }
            
            # 파일 기반 통신으로 전송
            execution_result = self.ue5_connector.file_comm.send_command_raw(ue_command)

            if execution_result.get("success"):
                self.last_parameters = deepcopy(params)
                self._persist_last_parameters()

            result_text = f"[명령] {command}\n\n"
            result_text += f"[파싱 결과]\n{response_text}\n\n"

            if execution_result.get("success"):
                result_text += f"[UE5 실행 결과] 성공\n"
                result_text += f"   {execution_result.get('message', '')}\n"
            else:
                result_text += f"[UE5 실행 결과] 실패\n"
                result_text += f"   오류: {execution_result.get('error', 'Unknown error')}\n"

            result_text += f"\n--- 생성된 파라미터 ---\n"
            result_text += json.dumps(params, indent=2, ensure_ascii=False)

            return [TextContent(
                type="text",
                text=result_text
            )]

        except Exception as e:
            return [TextContent(
                type="text",
                text=f"오류 발생: {str(e)}"
            )]

    async def run(self):
        """서버 실행"""
        async with stdio_server() as (read_stream, write_stream):
            await self.app.run(
                read_stream,
                write_stream,
                self.app.create_initialization_options()
            )

    def _persist_last_parameters(self):
        try:
            self.state_file.parent.mkdir(parents=True, exist_ok=True)
            with open(self.state_file, 'w', encoding='utf-8') as f:
                json.dump(self.last_parameters or {}, f, indent=2, ensure_ascii=False)
        except Exception as e:
            print(f"[WARN] Failed to persist last parameters: {e}")

    def _load_last_parameters(self):
        try:
            if self.state_file.exists():
                with open(self.state_file, 'r', encoding='utf-8') as f:
                    data = json.load(f)
                    if isinstance(data, dict) and data:
                        self.last_parameters = data
        except Exception as e:
            print(f"[WARN] Failed to load last parameters: {e}")


async def main():
    """메인 진입점"""
    server = PCGForestMCPServer()
    await server.run()


if __name__ == "__main__":
    asyncio.run(main())
