# 자동 초기화 시스템 가이드 (Auto Initialization System Guide)

## 개요

NLPPCG 플러그인은 이제 **자동 초기화 시스템**을 포함하고 있습니다. 레벨이 로드되면 자동으로 필요한 모든 액터와 연결이 설정됩니다.

## 자동으로 생성되는 컴포넌트

레벨이 에디터에서 열리거나 게임이 실행될 때 다음이 자동으로 생성됩니다:

1. **MCPClient** (`AutoMCPClient`)
   - MCP 서버와 통신하는 클라이언트
   - 디버그 모드 기본 활성화
   - 시뮬레이션 모드로 작동 (실제 MCP 서버 없이도 테스트 가능)

2. **ForestPCGManager** (`AutoForestPCGManager`)
   - PCG 기반 숲 생성 관리자
   - 기본 큐브 메시로 설정된 나무 타입들
   - 자동으로 MCPClient와 연결됨

## 사용 방법

### 1. 기본 사용 (아무 설정 없이)

1. 언리얼 에디터에서 레벨 열기
2. 로그 확인: `=== NLPPCG System Ready ===` 메시지 확인
3. 완료! 이제 MCP 서버에서 숲을 생성할 수 있습니다

### 2. 블루프린트에서 테스트

```
Event BeginPlay
  ↓
Get World Subsystem (NLPPCG World Subsystem)
  ↓
Test Forest Generation
  Command: "밀집된 소나무 숲"
```

### 3. C++에서 테스트

```cpp
#include "NLPPCGWorldSubsystem.h"

// Get the subsystem
UNLPPCGWorldSubsystem* Subsystem = GetWorld()->GetSubsystem<UNLPPCGWorldSubsystem>();

if (Subsystem)
{
    // Test forest generation
    Subsystem->TestForestGeneration(TEXT("밀집된 소나무 숲"));
}
```

### 4. 콘솔 명령으로 테스트

에디터에서 다음 블루프린트 노드를 실행:

```
Get NLPPCG World Subsystem
  ↓
Test Forest Generation: "중간 크기의 자작나무 숲"
```

## 로그 확인

자동 초기화가 성공하면 다음과 같은 로그가 출력됩니다:

```
LogTemp: Warning: UNLPPCGWorldSubsystem::GetOrCreateMCPClient - Created new MCP Client (Debug Mode: ON)
LogTemp: Warning: UNLPPCGWorldSubsystem::GetOrCreateForestPCGManager - Created new Forest PCG Manager
LogTemp: Warning: UNLPPCGWorldSubsystem::BindMCPClientToManager - MCP Client bound to Forest PCG Manager
LogTemp: Warning: === NLPPCG System Ready ===
```

숲 생성 시 다음과 같은 로그가 출력됩니다:

```
LogTemp: Log: Sending command to MCP: 밀집된 소나무 숲
LogTemp: Warning: === Forest Generation Triggered ===
LogTemp: Warning: Tree Type: pine
LogTemp: Warning: Density: dense
LogTemp: Warning: Area Size: 5000.0
```

## MCP 서버 연결 (선택사항)

현재는 시뮬레이션 모드로 작동하지만, 실제 MCP 서버를 연결하려면:

1. `MCPClient.cpp` 파일 열기
2. 62-70번째 줄의 주석 해제 (실제 HTTP 요청 코드)
3. 72-94번째 줄 삭제 또는 주석 처리 (시뮬레이션 모드 코드)
4. Python MCP 서버 실행:
   ```bash
   cd MCPServer
   python server.py
   ```

## 문제 해결

### 자동 초기화가 작동하지 않는 경우

1. **로그 확인**: `UNLPPCGWorldSubsystem::Initialize` 메시지가 있는지 확인
2. **월드 타입 확인**: 에디터 월드 또는 게임 월드에서만 작동
3. **플러그인 활성화 확인**: 프로젝트 설정에서 NLPPCG 플러그인 활성화 확인

### 숲이 생성되지 않는 경우

1. **델리게이트 바인딩 확인**: 로그에서 "MCP Client bound to Forest PCG Manager" 메시지 확인
2. **ForestPCGManager 확인**: 월드 아웃라이너에서 `AutoForestPCGManager` 액터 확인
3. **디버그 로그 확인**: `=== Forest Generation Triggered ===` 메시지 확인

## 수동 설정 비교

### 이전 (수동 설정)
1. MCPClient 액터를 레벨에 배치
2. ForestPCGManager 액터를 레벨에 배치
3. MCPClient의 OnForestGenerated 델리게이트를 ForestPCGManager의 GenerateForest에 바인딩
4. MCP 서버 URL 설정
5. 테스트

### 현재 (자동 설정)
1. 레벨 열기
2. 테스트

## 추가 정보

- 자동 초기화 시스템은 에디터와 게임 모드 모두에서 작동합니다
- 이미 레벨에 MCPClient 또는 ForestPCGManager가 있다면 새로 생성하지 않고 기존 것을 사용합니다
- 디버그 모드가 기본으로 활성화되어 모든 작업이 로그에 기록됩니다
- 시뮬레이션 모드로 MCP 서버 없이도 테스트 가능합니다
