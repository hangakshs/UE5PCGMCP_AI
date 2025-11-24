# MCP 서버 설정 가이드

이 가이드는 Claude Desktop 또는 Cursor에서 PCG Forest MCP 서버를 설정하는 방법을 설명합니다.

## 목차

1. [Claude Desktop 설정](#claude-desktop-설정)
2. [Cursor 설정](#cursor-설정)
3. [MCP 서버 실행](#mcp-서버-실행)
4. [테스트](#테스트)
5. [트러블슈팅](#트러블슈팅)

---

## Claude Desktop 설정

### 1. MCP 서버 준비

```bash
# 프로젝트 디렉토리로 이동
cd /path/to/UE5PCGMCP_AI/MCPServer

# Python 의존성 설치
pip install -r requirements.txt
```

### 2. Claude Desktop 설정 파일 수정

#### Windows
```
%APPDATA%\Claude\claude_desktop_config.json
```

#### macOS
```
~/Library/Application Support/Claude/claude_desktop_config.json
```

#### Linux
```
~/.config/Claude/claude_desktop_config.json
```

### 3. 설정 파일 작성

`claude_desktop_config.json` 파일을 다음과 같이 작성합니다:

```json
{
  "mcpServers": {
    "pcg-forest": {
      "command": "python",
      "args": [
        "/절대/경로/UE5PCGMCP_AI/MCPServer/server.py"
      ],
      "env": {
        "PYTHONPATH": "/절대/경로/UE5PCGMCP_AI/MCPServer"
      }
    }
  }
}
```

**중요**: `/절대/경로/`를 실제 프로젝트 경로로 변경하세요!

예시:
- Windows: `C:/Projects/UE5PCGMCP_AI/MCPServer/server.py`
- macOS/Linux: `/home/user/UE5PCGMCP_AI/MCPServer/server.py`

### 4. Python 가상환경 사용 시 (권장)

```json
{
  "mcpServers": {
    "pcg-forest": {
      "command": "/절대/경로/venv/bin/python",
      "args": [
        "/절대/경로/UE5PCGMCP_AI/MCPServer/server.py"
      ]
    }
  }
}
```

Windows의 경우:
```json
{
  "mcpServers": {
    "pcg-forest": {
      "command": "C:/Projects/UE5PCGMCP_AI/venv/Scripts/python.exe",
      "args": [
        "C:/Projects/UE5PCGMCP_AI/MCPServer/server.py"
      ]
    }
  }
}
```

### 5. Claude Desktop 재시작

1. Claude Desktop을 완전히 종료합니다
2. Claude Desktop을 다시 시작합니다
3. 새 대화를 시작하면 MCP 서버가 자동으로 연결됩니다

### 6. 연결 확인

Claude Desktop에서 다음과 같이 테스트합니다:

```
"사용 가능한 MCP 도구를 보여줘"
```

응답에 다음 도구들이 표시되어야 합니다:
- `create_forest`: 숲 생성
- `clear_forest`: 숲 제거
- `modify_forest`: 숲 수정

---

## Cursor 설정

### 1. MCP 서버 준비

```bash
cd /path/to/UE5PCGMCP_AI/MCPServer
pip install -r requirements.txt
```

### 2. Cursor 설정 파일 위치

#### Windows
```
%APPDATA%\Cursor\User\globalStorage\saoudrizwan.claude-dev\settings\cline_mcp_settings.json
```

#### macOS
```
~/Library/Application Support/Cursor/User/globalStorage/saoudrizwan.claude-dev/settings/cline_mcp_settings.json
```

#### Linux
```
~/.config/Cursor/User/globalStorage/saoudrizwan.claude-dev/settings/cline_mcp_settings.json
```

### 3. 설정 파일 작성

`cline_mcp_settings.json` 파일을 다음과 같이 작성합니다:

```json
{
  "mcpServers": {
    "pcg-forest": {
      "command": "python",
      "args": [
        "/절대/경로/UE5PCGMCP_AI/MCPServer/server.py"
      ],
      "env": {
        "PYTHONPATH": "/절대/경로/UE5PCGMCP_AI/MCPServer"
      },
      "disabled": false,
      "alwaysAllow": []
    }
  }
}
```

### 4. Cursor에서 MCP 활성화

1. Cursor를 열고 `Cmd/Ctrl + Shift + P`를 누릅니다
2. "MCP: Manage Servers"를 검색합니다
3. "pcg-forest" 서버가 표시되는지 확인합니다
4. 서버를 활성화합니다

### 5. Cursor 재시작

Cursor를 완전히 종료하고 다시 시작합니다.

---

## MCP 서버 실행

### 방법 1: Claude/Cursor가 자동 실행 (권장)

Claude Desktop이나 Cursor가 설정 파일을 읽고 자동으로 MCP 서버를 시작합니다.

### 방법 2: 수동 실행

개발 및 디버깅 시에는 수동으로 실행할 수 있습니다:

```bash
cd MCPServer
python server.py
```

이 경우 Claude/Cursor 설정을 stdio 대신 HTTP로 변경해야 합니다.

---

## 테스트

### Claude Desktop에서 테스트

```
create_forest 도구를 사용해서 "밀집된 소나무 숲 만들어줘"라는 명령을 실행해줘
```

**예상 응답:**

```
밀집된 보통 크기의 소나무 숲을 약 0㎡ 영역에 생성합니다.
나무 간 최소 거리: 150cm, 최대 거리: 300cm
임의성: 0.3

--- Unreal Engine Command ---
{
  "action": "create_forest",
  "parameters": {
    "tree_type": "pine",
    "density": "dense",
    "min_distance": 150.0,
    "max_distance": 300.0,
    ...
  }
}
```

### Cursor에서 테스트

채팅 창에서:

```
@pcg-forest 성긴 참나무 숲을 500평방미터에 생성해줘
```

### 다양한 테스트 명령어

```
"큰 나무들로 빽빽한 숲 만들어줘"
"작은 자작나무 숲 만들어줘"
"거대한 단풍나무 숲 1000평방미터"
"숲 제거해줘"
```

---

## 트러블슈팅

### 문제 1: MCP 서버가 시작되지 않음

**원인**: Python 경로가 잘못되었거나 의존성이 설치되지 않음

**해결**:
```bash
# Python 경로 확인
which python  # macOS/Linux
where python  # Windows

# 의존성 재설치
pip install -r requirements.txt
```

### 문제 2: "MCP server not found" 오류

**원인**: 설정 파일 경로가 잘못되었거나 JSON 형식 오류

**해결**:
1. 설정 파일 경로가 올바른지 확인
2. JSON 형식 검증: https://jsonlint.com
3. 절대 경로 사용 (상대 경로 X)
4. Windows의 경우 백슬래시 이스케이프: `C:\\Projects\\...` 또는 슬래시 사용: `C:/Projects/...`

### 문제 3: "Permission denied" 오류

**원인**: 파일 실행 권한 문제

**해결** (macOS/Linux):
```bash
chmod +x MCPServer/server.py
```

### 문제 4: Import 오류

**원인**: MCP SDK가 설치되지 않음

**해결**:
```bash
pip install mcp>=0.9.0
```

### 문제 5: 서버가 응답하지 않음

**원인**: 서버 크래시 또는 무한 루프

**해결**:

**디버그 모드로 실행**:
```bash
cd MCPServer
python server.py 2>&1 | tee server.log
```

**로그 확인**:
```bash
cat server.log
```

### 문제 6: Claude/Cursor에서 도구가 보이지 않음

**원인**: 서버가 제대로 연결되지 않음

**해결**:
1. Claude/Cursor 완전히 재시작
2. 설정 파일 다시 확인
3. 수동으로 서버 실행 후 로그 확인

---

## 고급 설정

### 1. 로깅 활성화

`server.py` 실행 시 로깅:

```python
# server.py 상단에 추가
import logging
logging.basicConfig(
    level=logging.DEBUG,
    format='%(asctime)s - %(levelname)s - %(message)s',
    filename='mcp_server.log'
)
```

### 2. 여러 MCP 서버 동시 사용

```json
{
  "mcpServers": {
    "pcg-forest": {
      "command": "python",
      "args": ["/path/to/forest_server.py"]
    },
    "other-tool": {
      "command": "python",
      "args": ["/path/to/other_server.py"]
    }
  }
}
```

### 3. 환경 변수 설정

```json
{
  "mcpServers": {
    "pcg-forest": {
      "command": "python",
      "args": ["/path/to/server.py"],
      "env": {
        "PYTHONPATH": "/path/to/MCPServer",
        "LOG_LEVEL": "DEBUG",
        "SERVER_PORT": "8000"
      }
    }
  }
}
```

---

## 참고 자료

### MCP 공식 문서
- [MCP Specification](https://spec.modelcontextprotocol.io/)
- [MCP Python SDK](https://github.com/anthropics/mcp-python-sdk)

### Claude Desktop
- [Claude Desktop MCP Guide](https://docs.anthropic.com/claude/docs/mcp)

### Cursor
- [Cursor MCP Integration](https://cursor.sh/docs/mcp)

### 프로젝트 문서
- [README.md](../README.md): 프로젝트 개요
- [USAGE.md](../USAGE.md): 사용 가이드
- [ARCHITECTURE.md](../ARCHITECTURE.md): 시스템 아키텍처

---

## 빠른 체크리스트

설정이 완료되었는지 확인하세요:

- [ ] Python 3.11+ 설치됨
- [ ] `pip install -r requirements.txt` 실행됨
- [ ] 설정 파일 경로가 올바름 (claude_desktop_config.json 또는 cline_mcp_settings.json)
- [ ] JSON 형식이 올바름 (콤마, 중괄호 확인)
- [ ] 절대 경로 사용 (상대 경로 X)
- [ ] Claude Desktop 또는 Cursor 재시작됨
- [ ] 테스트 명령어로 확인됨

모두 체크되었다면 준비 완료입니다! 🎉

---

## 추가 도움

문제가 계속되면:

1. GitHub Issues에 문의
2. 로그 파일 첨부
3. 운영체제 및 버전 명시
4. 설정 파일 내용 공유 (민감한 정보 제거)
