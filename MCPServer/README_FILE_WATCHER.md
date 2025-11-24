# UE5 Forest Generation - File Watcher Service

## 개요

UE5 에디터와 Python NLP 시스템 간의 실시간 파일 기반 통신을 위한 독립 서비스입니다.

## 주요 기능

- ✅ UE5에서 자연어 명령을 파일로 전송
- ✅ Python이 명령을 파싱하여 PCG 파라미터 생성
- ✅ 결과를 파일로 UE5에 전달
- ✅ 실시간 로깅 및 모니터링
- ✅ Cursor MCP와 독립적으로 동작

## 사용 방법

### 1. 서비스 시작 (필수)

UE5 에디터를 사용하기 **전에** 이 서비스를 먼저 시작해야 합니다.

#### Windows
```bash
cd MCPServer
start_file_watcher.bat
```

#### Linux/Mac
```bash
cd MCPServer
chmod +x start_file_watcher.sh
./start_file_watcher.sh
```

#### 또는 Python 직접 실행
```bash
cd MCPServer
python file_watcher_service.py
```

### 2. UE5 에디터 열기

서비스가 실행 중인 상태에서 UE5 프로젝트를 엽니다.

### 3. 숲 생성 테스트

UE5 에디터에서:
1. ForestPCGManager 액터를 레벨에 배치
2. Details 패널에서 "Generate Forest From NLP" 함수 호출
3. 명령어 입력 예시:
   - "밀집된 소나무 숲"
   - "성긴 참나무 숲 1000평방미터"
   - "보통 자작나무 숲"

### 4. 서비스 모니터링

터미널/커맨드 창에서 실시간 로그를 확인할 수 있습니다:

```
📨 Received command from UE5
   Command: 밀집된 소나무 숲
   Timestamp: 1234567890.123
   Parsed parameters:
      Tree Type: pine
      Density: dense
      Area Size: 10000000.0 cm²
      Min Distance: 150.0 cm
   Response written to: .../mcp_response.json
   Action: create_forest
✅ Command processed successfully
```

## 파일 통신 구조

```
UE5 Editor                    File Watcher Service
    |                                |
    |  ue5_command.json             |
    |------------------------------->|
    |                                |
    |         (NLP Processing)       |
    |                                |
    |  mcp_response.json            |
    |<-------------------------------|
    |                                |
  (PCG 생성)
```

### 통신 파일 위치

- **Command File**: `Intermediate/MCP_Commands/ue5_command.json`
- **Response File**: `Intermediate/MCP_Commands/mcp_response.json`
- **Log File**: `MCPServer/file_watcher.log`

## 명령어 형식

### UE5 → Python (ue5_command.json)
```json
{
  "command": "밀집된 소나무 숲",
  "timestamp": 1234567890.123
}
```

### Python → UE5 (mcp_response.json)
```json
{
  "action": "create_forest",
  "parameters": {
    "tree_type": "pine",
    "density": "dense",
    "size": "medium",
    "area_size": 10000000.0,
    "min_distance": 150.0,
    "max_distance": 300.0,
    "randomness": 0.3,
    "scale_multiplier": 1.0
  },
  "timestamp": 1234567890.456,
  "original_command": "밀집된 소나무 숲"
}
```

## 문제 해결

### 숲이 생성되지 않는 경우

1. **File Watcher Service가 실행 중인지 확인**
   - 터미널/커맨드 창에서 "Waiting for commands from UE5..." 메시지 확인

2. **파일 경로 확인**
   - `Intermediate/MCP_Commands/` 디렉토리가 존재하는지 확인
   - 파일 권한 문제가 없는지 확인

3. **로그 확인**
   - `MCPServer/file_watcher.log` 파일에서 에러 메시지 확인
   - UE5 Output Log에서 "MCP" 관련 로그 확인

4. **Python 의존성 확인**
   ```bash
   pip install -r requirements.txt
   ```

### 일반적인 에러

#### "NLP Handler not available"
- `nlp_handler.py`가 제대로 임포트되지 않음
- Python 경로 설정 확인
- 필요한 패키지 설치 확인

#### "Failed to write response file"
- 파일 쓰기 권한 문제
- 디스크 공간 확인
- 안티바이러스 소프트웨어 확인

## 개발 모드

더 상세한 로그를 보려면:

```python
# file_watcher_service.py 상단의 로깅 레벨 변경
logging.basicConfig(
    level=logging.DEBUG,  # INFO에서 DEBUG로 변경
    ...
)
```

## Cursor MCP vs File Watcher Service

### Cursor MCP (server.py)
- Cursor IDE에서 호출
- stdio 모드로 동작
- Cursor의 AI 어시스턴트와 대화형 인터페이스

### File Watcher Service (file_watcher_service.py)
- **독립 실행형 서비스**
- UE5 에디터와 직접 통신
- **숲 생성에 필수**
- 24/7 백그라운드 실행 가능

## 자동 시작 (선택사항)

UE5 에디터와 함께 자동으로 시작하려면:

1. Windows 작업 스케줄러 사용
2. 시작 프로그램에 등록
3. 또는 UE5 Python 스타트업 스크립트에서 subprocess로 실행

## 성능 최적화

- **Polling Interval**: 기본 0.3초, 필요에 따라 조정 가능
- **로그 파일**: 정기적으로 정리 권장
- **메모리**: 약 50MB 미만 사용

## 라이선스

MIT License - 자유롭게 사용, 수정, 배포 가능
