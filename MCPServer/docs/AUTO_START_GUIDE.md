# 파일 감시 서비스 자동 시작 가이드

## 개요

이제 **File Watcher Service가 UE5 에디터 시작 시 자동으로 실행**됩니다!
`start_file_watcher.bat`를 수동으로 실행할 필요가 없습니다.

## 작동 방식

### 1. UE5 에디터 시작

UE5 에디터가 시작되면 Python 스타트업 스크립트가 자동 실행됩니다:

```
Content/Python/init_unreal.py
```

### 2. 파일 감시 서비스 자동 시작

스타트업 스크립트가 백그라운드 프로세스로 File Watcher Service를 시작합니다:

```python
subprocess.Popen([python, "MCPServer/file_watcher_service.py"])
```

### 3. 자동 통신

- **UE5 → MCP**: 명령을 `Intermediate/MCP_Commands/ue5_command.json`에 저장
- **File Watcher**: 파일 감시 서비스가 명령을 읽고 NLP 파싱
- **MCP → UE5**: 파싱된 파라미터를 `Intermediate/MCP_Commands/mcp_response.json`에 저장
- **UE5**: MCPClient가 응답을 읽고 PCG 숲 생성

### 4. 에디터 종료 시 자동 정리

UE5 에디터가 종료되면 File Watcher Service도 자동으로 중지됩니다.

## 로그 확인

UE5 에디터의 **Output Log**에서 다음 메시지를 확인할 수 있습니다:

```
======================================================================
🚀 File Watcher Service Started Automatically!
======================================================================
   Process ID: [PID]
   Script: <프로젝트경로>/MCPServer/src/file_watcher_service.py
   Command Dir: <프로젝트경로>/Intermediate/MCP_Commands
======================================================================
✅ NLPPCG System Ready!
   You can now generate forests using natural language commands!
   Example: '밀집된 소나무 숲'
======================================================================
⚠️  NOTE: File Watcher Service will stop when UE5 Editor closes
======================================================================
```

## 수동 시작 (옵션)

자동 시작이 작동하지 않는 경우, 여전히 수동으로 시작할 수 있습니다:

```bash
cd MCPServer
start_file_watcher.bat
```

## 문제 해결

### 1. "File Watcher Service script not found" 에러

- `MCPServer/file_watcher_service.py` 파일이 존재하는지 확인
- 프로젝트 구조가 올바른지 확인

### 2. Python 모듈 import 에러

- UE5의 Python 환경에 필요한 모듈이 설치되어 있는지 확인
- `MCPServer/nlp_handler.py`가 존재하는지 확인

### 3. 명령 응답이 없음

- Output Log에서 File Watcher Service 시작 메시지 확인
- `Intermediate/MCP_Commands/` 폴더에 파일이 생성/삭제되는지 확인
- `MCPServer/file_watcher.log` 파일 확인

## 구조 요약

```
Portfolio_MCP_PCG/
├── Content/
│   └── Python/
│       └── init_unreal.py          # ✅ 자동 시작 스크립트
├── MCPServer/
│   ├── file_watcher_service.py     # 파일 감시 서비스
│   ├── nlp_handler.py              # NLP 파싱
│   └── start_file_watcher.bat      # (수동 시작용)
├── Intermediate/
│   └── MCP_Commands/               # 통신 디렉토리
│       ├── ue5_command.json        # UE5 → MCP
│       └── mcp_response.json       # MCP → UE5
└── Plugins/
    └── NLPPCG/
        └── Source/
            └── NLPPCG/
                ├── MCPClient.cpp          # 파일 통신 처리
                └── ForestPCGManager.cpp   # PCG 숲 생성
```

## 장점

✅ **자동화**: 별도의 배치 파일 실행 불필요
✅ **편리함**: UE5 에디터만 열면 바로 사용 가능
✅ **안정성**: 에디터 종료 시 자동 정리
✅ **투명성**: Output Log에서 모든 상태 확인 가능

## 다음 단계

1. UE5 에디터 열기
2. Output Log에서 자동 시작 메시지 확인
3. Cursor IDE에서 숲 생성 명령 입력
4. 에디터에서 즉시 숲이 생성되는 것 확인!
