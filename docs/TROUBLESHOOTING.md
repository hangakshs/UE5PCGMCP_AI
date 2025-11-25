# 숲 생성 안 됨 문제 해결 가이드

## 문제 증상
- 숲 생성 명령을 실행해도 아무 일도 일어나지 않음
- 로그에 명령 처리 메시지가 없음

## 원인
File Watcher Service가 자동으로 시작되지 않았습니다.

## 해결 방법

### 방법 1: Python 콘솔에서 수동 시작 (가장 빠름)

1. UE5 에디터에서 **Output Log** 창을 엽니다
2. **Cmd** 탭으로 전환합니다
3. 다음 명령어를 입력하고 Enter:

```python
import sys
from pathlib import Path
project_dir = Path(r"<프로젝트경로>/UnrealProject")  # 실제 프로젝트 경로로 변경
sys.path.insert(0, str(project_dir / "Content" / "Python"))
import init_unreal
```

4. 로그에 다음과 같은 메시지가 나타나면 성공:
```
🚀 File Watcher Service Started Automatically!
```

### 방법 2: 별도 터미널에서 File Watcher Service 실행

1. **새 PowerShell 또는 CMD 창**을 엽니다
2. 프로젝트의 MCPServer 폴더로 이동:
```bash
cd F:\Project\Portfolio_MCP_PCG\MCPServer
```

3. File Watcher Service 실행:
```bash
python file_watcher_service.py
```

4. 다음과 같은 메시지가 나타나면 성공:
```
╔══════════════════════════════════════════════════════════╗
║   UE5 Forest Generation - File Watcher Service          ║
║   Standalone service for UE5 ↔ Python communication     ║
╚══════════════════════════════════════════════════════════╝

🚀 File Watcher Service started (poll interval: 0.3s)
   Waiting for commands from UE5...
```

5. **이 창은 UE5 에디터를 사용하는 동안 열어두세요**

### 방법 3: UE5 Python Plugin 설정 (영구적 해결)

1. 프로젝트 루트에 `Config` 폴더 생성 (없는 경우)
2. `Config/DefaultEngine.ini` 파일 생성/수정
3. 다음 섹션 추가:

```ini
[Python]
+StartupScripts=Content/Python/init_unreal.py
```

4. UE5 에디터를 재시작합니다

## 작동 확인

File Watcher Service가 실행되면:

1. UE5 Output Log에서 다음 명령 실행:
```python
import unreal
forest_lib = unreal.ForestPCGManagerLibrary
world = unreal.EditorLevelLibrary.get_editor_world()
forest_lib.generate_forest_from_nlp(world, "밀집된 소나무 숲", unreal.Vector(0,0,0))
```

2. File Watcher 터미널에서 다음과 같은 로그가 나타나야 합니다:
```
============================================================
📨 Received command from UE5
   Command: 밀집된 소나무 숲
```

3. UE5 뷰포트에서 ForestPCGManager 액터 위치에 나무들이 생성되어야 합니다

## 추가 확인사항

### File Watcher Service 로그 확인
- `MCPServer/file_watcher.log` 파일을 확인하여 에러 메시지가 있는지 확인

### UE5 Output Log 확인
다음 메시지들이 나타나야 정상입니다:
```
LogTemp: Warning: ForestPCGManager: MCP Client bound to Forest Manager
LogTemp: Warning: === NLPPCG System Ready ===
```

### ForestPCGManager 액터 확인
1. World Outliner에서 `ForestPCGManager` 액터가 있는지 확인
2. Details 패널에서:
   - `bUseFileCommunication`: ✅ 체크
   - `bAutoCreateMCPClient`: ✅ 체크
   - `Tree Meshes`: 나무 타입별 메시 설정 (기본값: Cube)

## 여전히 작동하지 않는 경우

1. Python 환경 확인:
```bash
cd F:\Project\Portfolio_MCP_PCG\MCPServer
python -c "from nlp_handler import ForestNLPHandler; print('OK')"
```

2. 필요한 Python 패키지 설치:
```bash
pip install -r requirements.txt
```

3. UE5 에디터를 완전히 닫고 재시작

4. 프로젝트를 다시 컴파일:
   - Visual Studio에서 솔루션 빌드
   - 또는 `.uproject` 파일 우클릭 → "Generate Visual Studio Project Files"

## 도움이 필요한 경우

로그 파일을 확인하세요:
- `MCPServer/file_watcher.log` - File Watcher Service 로그
- UE5 Output Log - UE5 에디터 로그
- `Intermediate/MCP_Commands/` - 명령 파일 디렉토리 확인
