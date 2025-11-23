# UE5 PCG MCP 통합 가이드

Cursor IDE의 MCP(Model Context Protocol) 서버를 통해 자연어로 UE5에서 숲을 생성하는 시스템입니다.

## 🎯 시스템 아키텍처

```
Cursor IDE
    ↓
MCP Server (Python)
    ↓
Command File (JSON)
    ↓
UE5 Python Watcher
    ↓
ForestPCGManager
    ↓
숲 생성! 🌲🌲🌲
```

---

## 📋 설정 방법

### 1. Cursor에서 MCP 서버 설정

**Cursor 설정 파일** (`.cursor/mcp_config.json` 또는 Cursor 설정):

```json
{
  "mcpServers": {
    "pcg-forest": {
      "command": "python",
      "args": [
        "/home/user/UE5PCGMCP_AI/MCPServer/server.py"
      ],
      "cwd": "/home/user/UE5PCGMCP_AI/MCPServer"
    }
  }
}
```

> **주의**: 경로를 실제 프로젝트 경로로 변경하세요!

### 2. MCP 서버 의존성 설치

```bash
cd /home/user/UE5PCGMCP_AI/MCPServer
pip install -r requirements.txt
```

필수 패키지:
- `mcp>=0.9.0`
- `httpx>=0.27.0`
- `pydantic>=2.0.0`

### 3. UE5 에디터에서 Python Watcher 시작

UE5 에디터를 열고 **Python 콘솔**을 엽니다:

**방법 1: Python 콘솔에서 직접 실행**

```python
import start_mcp_watcher
start_mcp_watcher.start()
```

**방법 2: Output Log 확인**

시작하면 다음 메시지가 표시됩니다:

```
======================================================================
🎮 MCP Command Watcher Started!
======================================================================
✅ Auto-polling enabled (every 5 seconds)
📁 Watching: Intermediate/MCP_Commands/pending_command.json
🔧 Now you can send commands from Cursor IDE!
======================================================================
```

---

## 🚀 사용 방법

### Cursor에서 명령 보내기

Cursor IDE에서 다음과 같이 입력하세요:

```
중간 규모의 숲 생성해줘
```

또는:

```
밀집된 소나무 숲을 500평방미터에 만들어줘
```

또는:

```
성긴 참나무 숲 생성해줘
```

### MCP 도구 직접 호출

Cursor에서 MCP 도구를 직접 사용할 수도 있습니다:

**create_forest 도구:**
```
Command: "밀집된 소나무 숲"
```

**clear_forest 도구:**
```
모든 숲 제거
```

---

## 📂 생성되는 파일

### 명령 파일
- **위치**: `Intermediate/MCP_Commands/pending_command.json`
- **역할**: Cursor에서 보낸 명령을 UE5로 전달
- **자동 삭제**: 처리 후 자동 삭제됨

### 처리 중 파일
- **위치**: `Intermediate/MCP_Commands/processing_command.json`
- **역할**: 현재 처리 중인 명령 표시

### 결과 파일
- **위치**: `Intermediate/MCP_Commands/result.json`
- **역할**: 실행 결과 저장

---

## 🎮 UE5 Python Watcher 명령어

### 시작
```python
import start_mcp_watcher
start_mcp_watcher.start()
```

### 중지
```python
start_mcp_watcher.stop()
```

### 상태 확인
```python
start_mcp_watcher.status()
```

### 수동으로 명령 확인
```python
import mcp_command_watcher
mcp_command_watcher.check_commands()
```

---

## 🌲 숲 생성 파라미터

### 나무 종류
- `소나무` (pine)
- `참나무` (oak)
- `자작나무` (birch)
- `단풍나무` (maple)
- `나무` (generic_tree)

### 밀도
- `밀집된` / `빽빽한` / `많은` → dense
- `성긴` / `듬성한` / `적은` → sparse
- `보통` / `일반` → medium

### 크기
- `거대한` → huge
- `큰` → large
- `보통` → medium
- `작은` → small

### 면적
- `500평방미터` → 500㎡
- `1000㎡` → 1000㎡

---

## 🔧 문제 해결

### MCP Watcher가 시작되지 않는 경우

**증상**: `ModuleNotFoundError: No module named 'mcp_command_watcher'`

**해결**:
```python
import sys
from pathlib import Path
import unreal

content_python = Path(unreal.Paths.project_content_dir()) / "Python"
sys.path.insert(0, str(content_python))

# 다시 시도
import start_mcp_watcher
start_mcp_watcher.start()
```

### 명령이 처리되지 않는 경우

**확인 사항**:
1. UE5 Python Watcher가 실행 중인지 확인
   ```python
   start_mcp_watcher.status()
   ```

2. 명령 파일이 생성되었는지 확인
   ```
   Intermediate/MCP_Commands/pending_command.json
   ```

3. UE5 Output Log 확인 (에러 메시지 확인)

### ForestPCGManager를 찾을 수 없는 경우

**증상**: `ForestPCGManager not found`

**해결**:
1. World Outliner에서 `ForestPCGManager` 확인
2. 없으면 레벨에 배치:
   - Content Browser에서 `ForestPCGManager` 검색
   - 레벨에 드래그 & 드롭

---

## 📝 예제 명령어

```python
# Cursor에서 보낼 수 있는 명령어들:

"밀집된 소나무 숲 만들어줘"
"성긴 참나무 숲을 500평방미터에 생성해줘"
"큰 나무들로 빽빽한 숲 만들어줘"
"작은 자작나무 숲 만들어줘"
"보통 단풍나무 숲 생성해줘"
"거대한 나무로 성긴 숲 만들어줘"
```

---

## 🎨 고급 설정

### Tree Mesh 변경

ForestPCGManager의 Details 패널에서:
1. `Tree Meshes` 맵 확장
2. 각 나무 타입에 Static Mesh 할당
   - `pine` → 소나무 메시
   - `oak` → 참나무 메시
   - `birch` → 자작나무 메시
   - `maple` → 단풍나무 메시

### PCG 그래프 커스터마이징

Content Browser에서 생성된 PCG 그래프를 열어 수정 가능:
- `Content/PCG/ForestGraph_xxxxx`

---

## ✅ 테스트

### 1. MCP 서버 테스트
```bash
cd /home/user/UE5PCGMCP_AI/MCPServer
python server.py
```

### 2. NLP 파서 테스트
```bash
python nlp_handler.py
```

### 3. UE5 Connector 테스트
```bash
python ue5_connector.py
```

---

## 🐛 디버그 로그

### UE5 Output Log에서 확인

```
LogTemp: MCP Command Watcher initialized
LogTemp: Processing MCP command: create_forest
LogTemp: Forest created: 밀집된 소나무 숲
LogTemp: PCG Forest generated with parameters: Density=dense, MinDist=150.0
```

### 파일 기반 디버깅

명령 파일 수동 생성:
```json
// Intermediate/MCP_Commands/pending_command.json
{
  "action": "create_forest",
  "parameters": {
    "tree_type": "pine",
    "density": "dense",
    "size": "medium",
    "area_size": 5000.0,
    "min_distance": 150.0,
    "max_distance": 300.0,
    "randomness": 0.3,
    "scale_multiplier": 1.0
  }
}
```

---

## 📞 지원

문제가 발생하면:
1. UE5 Output Log 확인
2. `Intermediate/MCP_Commands/result.json` 확인
3. MCP 서버 로그 확인

---

**Happy Forest Generation! 🌲🌳🌴**
