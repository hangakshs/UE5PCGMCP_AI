# 🚀 빠른 시작 가이드 - UE5 PCG MCP 통합

## 1️⃣ UE5 에디터 설정 (필수)

### A. Python Watcher 시작

UE5 에디터를 열고 **Python 콘솔**을 엽니다:
- **Window → Developer Tools → Output Log**
- 하단의 **Cmd** 드롭다운에서 **Python** 선택

다음 코드를 입력하고 Enter:

```python
import start_mcp_watcher
start_mcp_watcher.start()
```

**성공 메시지:**
```
======================================================================
🎮 MCP Command Watcher Started!
======================================================================
✅ Auto-polling enabled (every 5 seconds)
📁 Watching: Intermediate/MCP_Commands/pending_command.json
🔧 Now you can send commands from Cursor IDE!
======================================================================
```

> ⚠️ **중요**: UE5 에디터를 닫으면 Watcher도 중지됩니다. 다시 시작하려면 위 코드를 재실행하세요.

---

## 2️⃣ Cursor IDE 설정

### A. MCP 서버 설정 파일

Cursor의 MCP 설정 파일에 다음 추가:

**파일 위치**: `.cursor/mcp_config.json` 또는 Cursor 설정

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

> 💡 **경로 변경**: 프로젝트가 다른 위치에 있으면 절대 경로를 수정하세요.

### B. Python 패키지 설치

터미널에서 실행:

```bash
cd /home/user/UE5PCGMCP_AI/MCPServer
pip install -r requirements.txt
```

---

## 3️⃣ 사용 방법

### Cursor에서 명령 보내기

Cursor IDE 채팅에서 다음과 같이 입력:

```
중간 규모의 숲 생성해줘
```

또는:

```
밀집된 소나무 숲 만들어줘
```

또는:

```
성긴 참나무 숲을 500평방미터에 생성해줘
```

### Cursor 응답 예시

```
📍 명령: 중간 규모의 숲 생성해줘

✅ 파싱 결과:
보통 보통 크기의 나무 숲을 약 500㎡ 영역에 생성합니다.
나무 간 최소 거리: 200cm, 최대 거리: 500cm
임의성: 0.5

🎮 UE5 실행 결과: ✅ 성공
   ✅ 명령이 UE5로 전송되었습니다.

--- 생성된 파라미터 ---
{
  "tree_type": "generic_tree",
  "density": "medium",
  "size": "medium",
  "area_size": 5000.0,
  "min_distance": 200.0,
  "max_distance": 500.0,
  "randomness": 0.5,
  "scale_multiplier": 1.0
}
```

### UE5에서 확인

5초 이내에 UE5 에디터의 **Output Log**에 다음이 표시됩니다:

```
LogTemp: Processing MCP command: create_forest
LogTemp: Forest created: 보통 나무 숲
LogTemp: PCG Forest generated with parameters: Density=medium, MinDist=200.0
```

레벨의 **World Outliner**에서 숲이 생성된 것을 확인할 수 있습니다! 🌲

---

## 🎮 지원되는 명령어

### 나무 종류
- 소나무
- 참나무
- 자작나무
- 단풍나무
- 나무 (일반)

### 밀도
- 밀집된 / 빽빽한 / 많은
- 성긴 / 듬성한 / 적은
- 보통 / 일반

### 크기
- 거대한
- 큰
- 작은
- 보통

### 예제 명령어

```
"밀집된 소나무 숲 만들어줘"
"성긴 참나무 숲을 500평방미터에 생성해줘"
"큰 나무들로 빽빽한 숲 만들어줘"
"작은 자작나무 숲 만들어줘"
"거대한 단풍나무 숲 생성해줘"
```

---

## 🛑 숲 제거

Cursor에서:

```
숲 제거해줘
```

또는 UE5 Python 콘솔에서:

```python
import unreal
world = unreal.EditorLevelLibrary.get_editor_world()
unreal.ForestPCGManagerLibrary.clear_all_forests(world_context_object=world)
```

---

## 🔧 문제 해결

### "명령이 처리되지 않아요"

**체크리스트:**
1. ✅ UE5 에디터가 실행 중인가요?
2. ✅ Python Watcher가 시작되었나요?
   ```python
   start_mcp_watcher.status()
   ```
3. ✅ ForestPCGManager가 레벨에 있나요?
   - World Outliner에서 확인
   - 없으면 Content Browser에서 레벨에 배치

### "Python 모듈을 찾을 수 없어요"

Python 경로 추가:

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

### "Cursor에서 MCP 서버가 안 보여요"

1. Cursor 재시작
2. MCP 설정 파일 경로 확인
3. Python 패키지 설치 확인:
   ```bash
   pip list | grep mcp
   ```

---

## 📁 파일 위치

### 명령 파일
- `Intermediate/MCP_Commands/pending_command.json` - Cursor→UE5 명령

### Python 스크립트
- `Content/Python/start_mcp_watcher.py` - 자동 감시 스크립트
- `Content/Python/mcp_command_watcher.py` - 명령 처리기
- `Content/Python/init_unreal.py` - Startup 스크립트 (선택)

### MCP 서버
- `MCPServer/server.py` - MCP 서버
- `MCPServer/nlp_handler.py` - 자연어 처리
- `MCPServer/ue5_connector.py` - UE5 연결

---

## 🎨 고급: Tree Mesh 변경

ForestPCGManager Details 패널에서:

1. **Tree Meshes** 맵 확장
2. 각 타입에 Static Mesh 할당:
   - `pine` → 소나무 메시
   - `oak` → 참나무 메시
   - `birch` → 자작나무 메시
   - `maple` → 단풍나무 메시
   - `generic_tree` → 일반 나무 메시

---

## 📞 더 많은 정보

전체 가이드: `MCP_INTEGRATION_GUIDE.md`

---

**이제 Cursor로 UE5에서 숲을 생성할 수 있습니다! 🌲🌳🌴**
