"""
MCP Server for Unreal Engine PCG Forest Generation
언리얼 엔진과 통신하여 자연어로 PCG 숲 생성을 제어하는 MCP 서버
"""
import asyncio
import json
from typing import Any, Sequence
from mcp.server import Server
from mcp.types import Tool, TextContent, ImageContent, EmbeddedResource
from mcp.server.stdio import stdio_server
from nlp_handler import ForestNLPHandler


class PCGForestMCPServer:
    def __init__(self):
        self.app = Server("pcg-forest-server")
        self.nlp_handler = ForestNLPHandler()
        self.setup_handlers()

    def setup_handlers(self):
        """MCP 서버 핸들러 설정"""

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
        """숲 생성 명령 처리"""
        command = arguments.get("command", "")

        if not command:
            return [TextContent(
                type="text",
                text="오류: 명령이 제공되지 않았습니다."
            )]

        try:
            # NLP로 파라미터 파싱
            params = self.nlp_handler.parse_forest_command(command)

            # 사용자 응답 생성
            response_text = self.nlp_handler.generate_response(params)

            # 언리얼 엔진에 전달할 JSON 생성
            ue_command = {
                "action": "create_forest",
                "parameters": params
            }

            result_text = f"{response_text}\n\n--- Unreal Engine Command ---\n"
            result_text += json.dumps(ue_command, indent=2, ensure_ascii=False)

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
        """숲 제거 명령 처리"""
        ue_command = {
            "action": "clear_forest",
            "parameters": {}
        }

        result_text = "모든 PCG 숲을 제거합니다.\n\n--- Unreal Engine Command ---\n"
        result_text += json.dumps(ue_command, indent=2, ensure_ascii=False)

        return [TextContent(
            type="text",
            text=result_text
        )]

    async def handle_modify_forest(self, arguments: dict) -> Sequence[TextContent]:
        """숲 수정 명령 처리"""
        command = arguments.get("command", "")

        if not command:
            return [TextContent(
                type="text",
                text="오류: 수정 명령이 제공되지 않았습니다."
            )]

        try:
            # 기존 파싱 로직 재사용
            params = self.nlp_handler.parse_forest_command(command)
            params['action'] = 'modify_forest'

            response_text = f"숲을 수정합니다: {command}"

            ue_command = {
                "action": "modify_forest",
                "parameters": params
            }

            result_text = f"{response_text}\n\n--- Unreal Engine Command ---\n"
            result_text += json.dumps(ue_command, indent=2, ensure_ascii=False)

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


async def main():
    """메인 진입점"""
    server = PCGForestMCPServer()
    await server.run()


if __name__ == "__main__":
    asyncio.run(main())
