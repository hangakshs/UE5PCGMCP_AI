# 숲 생성 문제 해결 요약

## 발견된 문제

### 1. **MCPClient가 시뮬레이션 모드로 작동**
   - `MCPClient.cpp`의 라인 64-94에서 실제 HTTP 통신 코드가 주석 처리됨
   - 하드코딩된 시뮬레이션 응답만 사용
   - 결과: 어떤 명령을 보내도 항상 같은 "밀집된 소나무 숲" 파라미터만 생성

### 2. **MCP 서버 통신 방식 불일치**
   - MCP 서버는 `stdio_server()` 사용 (표준 입출력 기반)
   - MCPClient는 HTTP 통신 시도
   - 결과: 서버와 클라이언트 간 통신 불가

### 3. **손상된 외부 액터 파일**
   - `/Game/__ExternalActors__/Maps/M_MCP_PCG/` 경로의 손상된 액터 파일
   - ForestPCGManager 로드 실패

## 적용된 해결 방법

### 1. **파일 기반 통신 구현**

#### UE5 측 (MCPClient.cpp/h)
```cpp
// 새로운 기능:
- bUseFileCommunication: 파일 통신 사용 플래그 (기본값: true)
- FilePollingInterval: 파일 폴링 간격 (기본값: 0.5초)
- Tick(): 주기적으로 응답 파일 확인
- SendCommandViaFile(): 명령을 ue5_command.json에 저장
- CheckCommandFile(): mcp_response.json 확인 및 처리
```

**파일 경로:**
- 명령 파일: `{ProjectDir}/Intermediate/MCP_Commands/ue5_command.json`
- 응답 파일: `{ProjectDir}/Intermediate/MCP_Commands/mcp_response.json`

#### MCP 서버 측 (ue5_connector.py)
```python
# 새로운 기능:
- check_ue5_command(): UE5의 명령 파일 확인
- send_command_raw(): mcp_response.json에 응답 저장
```

### 2. **통신 흐름**

```
┌─────────────────────────────────────────────────────────────┐
│                    파일 기반 통신 흐름                       │
└─────────────────────────────────────────────────────────────┘

1. Cursor → MCP Server
   - create_forest 툴 호출
   - NLP 파싱

2. MCP Server → UE5
   - mcp_response.json 생성
   - {action, parameters} 저장

3. UE5 (MCPClient Tick)
   - 0.5초마다 mcp_response.json 확인
   - 파일 발견 시 ProcessForestCommand() 호출
   - OnForestGenerated 델리게이트 브로드캐스트

4. UE5 (ForestPCGManager)
   - OnForestParametersReceived() 호출
   - SetupPCGGraph() 실행
   - PCG 생성
```

### 3. **HTTP 통신 옵션 유지**

MCPClient는 두 가지 통신 모드를 지원합니다:

```cpp
// 파일 기반 (기본)
bUseFileCommunication = true;

// HTTP 기반 (향후 사용)
bUseFileCommunication = false;
ServerURL = "http://localhost:8000";
```

## 사용 방법

### 1. **프로젝트 리빌드**

```bash
# Windows
cd "<프로젝트경로>"
"<UnrealEngine경로>/Engine/Build/BatchFiles/Build.bat" Portfolio_MCP_PCGEditor Win64 Development "<프로젝트경로>/UnrealProject/Portfolio_MCP_PCG.uproject"
```

### 2. **UE5 에디터에서 확인**

1. 에디터 열기
2. M_MCP_PCG 맵 열기
3. World Outliner에서 기존 ForestPCGManager 제거 (손상된 경우)
4. 새 ForestPCGManager 액터 배치
5. Details 패널 확인:
   - `bUseFileCommunication`: ✅ Checked
   - `bAutoCreateMCPClient`: ✅ Checked
   - `bDebugMode`: ✅ Checked

### 3. **Cursor에서 테스트**

Cursor에서 다음 명령 실행:

```
밀집된 소나무 숲 만들어줘
```

또는:

```
성긴 참나무 숲 500평방미터
```

### 4. **로그 확인**

UE5 에디터의 Output Log에서 다음 메시지 확인:

```
✅ Command sent via file: .../Intermediate/MCP_Commands/ue5_command.json
📥 Received MCP response from file
Forest parameters parsed: Type=pine, Density=dense, MinDist=150.0
PCG Forest generated with parameters: Density=dense, MinDist=150.0
```

## 테스트

### MCP 서버 테스트

```bash
cd MCPServer
python test_file_communication.py
```

예상 출력:
```
=== File-Based Communication Test ===

--- Test 1: 밀집된 소나무 숲 ---
Parsed parameters: {
  "tree_type": "pine",
  "density": "dense",
  ...
}
Send result: ✅ 명령이 UE5로 전송되었습니다.
Response file created: .../mcp_response.json
Action: create_forest
✅ Test passed
```

## 변경된 파일

1. **Plugins/NLPPCG/Source/NLPPCG/Public/MCPClient.h**
   - 파일 통신 관련 속성 추가
   - Tick() 함수 선언
   - 파일 통신 헬퍼 함수 선언

2. **Plugins/NLPPCG/Source/NLPPCG/Private/MCPClient.cpp**
   - 시뮬레이션 모드 제거
   - 파일 통신 구현
   - Tick() 함수 구현
   - SendCommandViaFile(), CheckCommandFile() 구현

3. **MCPServer/ue5_connector.py**
   - send_command_raw() 수정 (mcp_response.json 저장)
   - check_ue5_command() 추가

4. **MCPServer/test_file_communication.py** (신규)
   - 파일 통신 테스트 스크립트

## 다음 단계

1. ✅ 코드 수정 완료
2. ⏳ **프로젝트 리빌드 필요**
3. ⏳ UE5 에디터에서 테스트
4. ⏳ 실제 숲 생성 확인

## 문제 해결

### "숲이 여전히 생성되지 않음"

1. **로그 확인**:
   ```
   LogTemp: ✅ Command sent via file: ...
   LogTemp: 📥 Received MCP response from file
   ```

2. **파일 확인**:
   ```bash
   # Windows
   dir "<프로젝트경로>\UnrealProject\Intermediate\MCP_Commands"
   ```

3. **MCPClient 설정 확인**:
   - `bUseFileCommunication` = true
   - `bDebugMode` = true

4. **ForestPCGManager 확인**:
   - 맵에 정상적으로 배치되었는지
   - MCPClient와 바인딩되었는지

### "mcp_response.json이 생성되지 않음"

MCP 서버가 실행 중인지 확인하고 Cursor에서 명령 재시도

### "파일이 생성되지만 읽히지 않음"

- FilePollingInterval 확인 (기본 0.5초)
- Tick이 활성화되었는지 확인
- 로그에서 "Received MCP response" 메시지 확인
