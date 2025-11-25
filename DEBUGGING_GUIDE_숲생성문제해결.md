# 🌲 숲 생성 문제 해결 가이드

## 문제: 숲이 생성되지 않음

### 시스템 구조 이해

```
[1] 사용자 명령 입력
     ↓
[2] ForestPCGManager.GenerateForestFromNLP(Command)
     ↓
[3] MCPClient.SendCommand(Command)
     ↓
[4] ue5_command.json 파일 생성
     ↓
[5] File Watcher Service 감지
     ↓
[6] NLP Handler로 파싱
     ↓
[7] mcp_response.json 파일 생성
     ↓
[8] MCPClient.CheckCommandFile() (Tick)
     ↓
[9] mcp_response.json 읽기 및 파싱
     ↓
[10] OnForestGenerated 델리게이트 호출
     ↓
[11] ForestPCGManager.OnForestParametersReceived()
     ↓
[12] PCG 그래프 생성 및 실행
     ↓
[13] 숲 생성 완료!
```

---

## 1단계: 시스템 초기화 확인

### ✅ 확인 항목

#### 1.1 File Watcher Service 실행 확인

**Output Log에서 확인:**
```
LogPython: 🚀 File Watcher Service Started Automatically!
LogPython:    Process ID: [숫자]
```

**❌ 없으면:**
- Python 스크립트 에러 확인
- `MCPServer/src/file_watcher_service.py` 파일 존재 확인
- 수동 시작: `MCPServer/scripts/StartFileWatcher.bat` 실행

#### 1.2 ForestPCGManager 존재 확인

**Output Log에서 확인:**
```
LogTemp: Warning: ✅ NLPPCG System Ready!
LogTemp: Warning:    ForestPCGManager: [이름]
LogTemp: Warning:    MCPClient: [이름]
```

**❌ 없으면:**
1. 레벨에 ForestPCGManager 배치
   - Place Modes > All Classes > ForestPCGManager 검색
   - 뷰포트에 드래그 앤 드롭

2. Python 테스트 스크립트 실행:
   ```
   py "<프로젝트경로>/UnrealProject/Content/Python/test_forest_generation.py"
   ```

#### 1.3 MCPClient 설정 확인

**Output Log에서 확인:**
```
LogTemp: Warning: 🔧 MCPClient::BeginPlay() called
LogTemp: Warning:    bUseFileCommunication: TRUE
LogTemp: Warning:    bDebugMode: TRUE
```

**필수 설정:**
- `bUseFileCommunication` = TRUE
- `bDebugMode` = TRUE (디버깅용)
- `FilePollingInterval` = 0.5 (권장)

---

## 2단계: 명령 전송 테스트

### 방법 1: Python 테스트 스크립트 사용 (권장)

**UE5 에디터에서:**
1. `Shift + F1` (Python 콘솔 열기)
2. 다음 명령 실행:
   ```python
   import test_forest_generation
   test_forest_generation.check_system_status()
   test_forest_generation.test_forest_generation()
   ```

3. 또는 Cmd 입력창에:
   ```
   py "<프로젝트경로>/UnrealProject/Content/Python/test_forest_generation.py"
   ```

### 방법 2: Blueprint 사용

1. ForestPCGManager 액터 선택
2. Details 패널에서 `GenerateForestFromNLP` 호출
3. Command: "밀집된 소나무 숲" 입력

### 방법 3: 직접 파일 생성 (File Watcher 테스트)

**명령 파일 수동 생성:**
- 위치: `<프로젝트경로>/UnrealProject/Intermediate/MCP_Commands/ue5_command.json`
- 내용:
  ```json
  {
    "command": "밀집된 소나무 숲",
    "timestamp": 1732479881.0
  }
  ```

---

## 3단계: 각 단계별 로그 확인

### 📝 정상 동작 시 Output Log 순서:

#### [1] 명령 전송
```
LogTemp: Warning: 🚀 Sending Command via File
LogTemp: Warning:    Command: 밀집된 소나무 숲
LogTemp: Warning:    Target File: F:/Project/.../ue5_command.json
LogTemp: Warning: ✅ Command file created successfully!
```

#### [2] File Watcher 감지 (file_watcher.log 확인)
```
2025-11-24 23:04:50 - INFO - 📨 Command file detected!
2025-11-24 23:04:50 - INFO -    Command: '밀집된 소나무 숲'
2025-11-24 23:04:50 - INFO -    Processing command with NLP Handler...
2025-11-24 23:04:50 - INFO -    ✅ Command processed
2025-11-24 23:04:50 - INFO -    Writing response file...
2025-11-24 23:04:50 - INFO -    ✅ Command file deleted
```

#### [3] MCPClient 응답 읽기
```
LogTemp: Warning: 📥 Response File Detected!
LogTemp: Warning:    File: F:/Project/.../mcp_response.json
LogTemp: Warning: ✅ Processing action: create_forest
```

#### [4] ForestPCGManager 처리
```
LogTemp: Warning: 🌲 Forest Parameters Received!
LogTemp: Warning:    Tree Type: pine
LogTemp: Warning:    Density: dense
LogTemp: Warning: 🔧 Setting up PCG Graph...
LogTemp: Warning: 🚀 Executing PCG->Generate()...
LogTemp: Warning: ✅ PCG Forest Generation Complete!
```

---

## 4단계: 문제별 해결 방법

### ❌ 문제 1: 명령 파일이 생성되지 않음

**증상:**
- Output Log에 "🚀 Sending Command via File" 없음

**원인:**
- `GenerateForestFromNLP()` 함수가 호출되지 않음
- MCPClient가 없거나 비활성화됨

**해결:**
1. ForestPCGManager가 레벨에 있는지 확인
2. Python 테스트 스크립트 실행 (방법 1)
3. MCPClient 설정 확인:
   - `bUseFileCommunication` = TRUE
   - Details 패널에서 확인

### ❌ 문제 2: File Watcher가 파일을 감지하지 못함

**증상:**
- `ue5_command.json` 파일은 생성됨
- File Watcher 로그 없음

**원인:**
- File Watcher Service가 실행되지 않음
- 디렉토리 경로 문제

**해결:**
1. File Watcher 프로세스 확인:
   ```
   tasklist | findstr python
   ```
   - PID가 Output Log의 PID와 일치하는지 확인

2. File Watcher 로그 확인:
   - 위치: `MCPServer/file_watcher.log`
   - 최신 로그 확인

3. 수동 재시작:
   ```cmd
   cd F:\Project\Portfolio_MCP_PCG\MCPServer
   python src\file_watcher_service.py
   ```

### ❌ 문제 3: MCPClient가 응답 파일을 읽지 못함

**증상:**
- `mcp_response.json` 파일은 생성됨
- Output Log에 "📥 Response File Detected!" 없음

**원인:**
- MCPClient의 Tick이 작동하지 않음
- `CheckCommandFile()` 호출 문제

**해결:**
1. MCPClient 설정 확인:
   ```cpp
   PrimaryActorTick.bCanEverTick = true;  // Tick 활성화
   ```

2. Output Log에서 Tick 동작 확인:
   ```
   LogTemp: Warning: 🔄 MCPClient::Tick() - First tick called!
   LogTemp: Log: 🔄 MCPClient::Tick() - Polling for response file...
   ```

3. 파일 폴링 간격 확인:
   - `FilePollingInterval` = 0.5 (기본값)
   - 너무 크면 응답이 느려짐

### ❌ 문제 4: PCG 그래프가 생성되지 않음

**증상:**
- "🌲 Forest Parameters Received!" 로그는 있음
- 하지만 PCG 그래프 생성 실패

**원인:**
- PCGComponent가 없거나 비활성화됨
- BoundsComponent 문제

**해결:**
1. ForestPCGManager 확인:
   - Details > PCGComponent 존재 확인
   - BoundsComponent 크기 확인

2. PCG 로그 확인:
   ```
   LogTemp: Warning: 🔧 Setting up PCG Graph...
   LogTemp: Warning: ✅ Updated BoundsComponent
   LogTemp: Warning: 🚀 Executing PCG->Generate()...
   ```

3. PCG 수동 실행:
   - ForestPCGManager 선택
   - Details > Generate 버튼 클릭

---

## 5단계: 디렉토리 및 파일 구조 확인

### 필수 디렉토리:
```
<프로젝트경로>/
├── UnrealProject/
│   ├── Intermediate/
│   │   └── MCP_Commands/           ← 이 디렉토리 필수!
│   │       ├── ue5_command.json    ← UE5가 생성
│   │       └── mcp_response.json   ← File Watcher가 생성
│   └── Content/
│       └── Python/
│           ├── init_unreal.py
│           └── test_forest_generation.py
│
└── MCPServer/
    ├── src/
    │   ├── file_watcher_service.py
    │   └── nlp_handler.py
    ├── scripts/
    │   └── StartFileWatcher.bat
    └── file_watcher.log        ← 로그 파일
```

### 디렉토리 생성 확인:
```python
# Python 콘솔에서 실행
from pathlib import Path
import unreal

project = Path(unreal.Paths.project_dir())
cmd_dir = project / "Intermediate" / "MCP_Commands"

print(f"Directory: {cmd_dir}")
print(f"Exists: {cmd_dir.exists()}")

if not cmd_dir.exists():
    cmd_dir.mkdir(parents=True, exist_ok=True)
    print("✅ Created!")
```

---

## 6단계: 로그 수집

### 문제 보고 시 필요한 정보:

1. **UE5 Output Log:**
   - Window > Developer Tools > Output Log
   - 전체 로그 복사 (Ctrl+A, Ctrl+C)

2. **File Watcher Log:**
   - `MCPServer/file_watcher.log`
   - 마지막 50줄

3. **디렉토리 상태:**
   ```cmd
   dir /s F:\Project\Portfolio_MCP_PCG\Intermediate\MCP_Commands
   ```

4. **프로세스 목록:**
   ```cmd
   tasklist | findstr python
   ```

5. **시스템 상태:**
   - Python 테스트 스크립트의 `check_system_status()` 결과

---

## 7단계: 완전 재시작 절차

모든 것이 작동하지 않을 때:

### 1. UE5 에디터 종료
   - File Watcher Service도 자동 종료됨

### 2. 잔여 프로세스 종료
   ```cmd
   taskkill /F /IM python.exe
   ```

### 3. 디렉토리 초기화
   ```cmd
   rd /s /q "<프로젝트경로>\UnrealProject\Intermediate\MCP_Commands"
   ```

### 4. UE5 에디터 재시작
   - init_unreal.py가 자동 실행됨
   - File Watcher Service가 자동 시작됨

### 5. 테스트 스크립트 실행
   ```
   py "<프로젝트경로>/UnrealProject/Content/Python/test_forest_generation.py"
   ```

---

## 빠른 체크리스트

숲이 생성되지 않을 때 순서대로 확인:

- [ ] File Watcher Service 실행 중? (Output Log 확인)
- [ ] ForestPCGManager가 레벨에 있음?
- [ ] MCPClient 설정: `bUseFileCommunication` = TRUE?
- [ ] `Intermediate/MCP_Commands` 디렉토리 존재?
- [ ] Python 테스트 스크립트 실행해봤음?
- [ ] Output Log에 명령 전송 로그 있음? ("🚀 Sending Command")
- [ ] `file_watcher.log`에 처리 로그 있음?
- [ ] `mcp_response.json` 파일 생성됨?
- [ ] MCPClient가 응답 읽었음? ("📥 Response File Detected")
- [ ] ForestPCGManager가 파라미터 받았음? ("🌲 Forest Parameters Received")

---

## 추가 도움말

### File Watcher 로그 실시간 모니터링:
```cmd
cd F:\Project\Portfolio_MCP_PCG\MCPServer
powershell Get-Content file_watcher.log -Wait -Tail 20
```

### 수동 명령 전송 (디버깅용):
```python
# UE5 Python 콘솔에서
import unreal

# UE5.7에서는 EditorActorSubsystem 사용
editor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = editor_subsystem.get_all_level_actors()
managers = [actor for actor in all_actors if isinstance(actor, unreal.ForestPCGManager)]

if managers:
    managers[0].generate_forest_from_nlp("밀집된 소나무 숲")

# 또는 GameplayStatics 사용 (런타임에서도 동작)
world = unreal.EditorLevelLibrary.get_editor_world()
managers = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ForestPCGManager)
if managers and len(managers) > 0:
    managers[0].generate_forest_from_nlp("밀집된 소나무 숲")
```

### MCP_Commands 디렉토리 수동 생성:
```cmd
mkdir "<프로젝트경로>\UnrealProject\Intermediate\MCP_Commands"
```

---

## 정상 동작 확인 방법

1. **Python 테스트 스크립트 실행:**
   ```
   py "<프로젝트경로>/UnrealProject/Content/Python/test_forest_generation.py"
   ```

2. **Output Log에서 다음 순서로 메시지 확인:**
   - ✅ "🚀 Sending Command via File"
   - ✅ "📥 Response File Detected!"
   - ✅ "🌲 Forest Parameters Received!"
   - ✅ "🚀 Executing PCG->Generate()..."
   - ✅ "✅ PCG Forest Generation Complete!"

3. **뷰포트에서 나무 확인:**
   - ForestPCGManager 주변에 나무(Cube) 생성됨
   - World Outliner에서 "PCG_[이름]" 액터 확인

---

**문제가 계속되면:**
- 전체 로그 수집 (Output Log, file_watcher.log)
- GitHub Issues에 보고
- 또는 빠른_문제해결.md 참조
