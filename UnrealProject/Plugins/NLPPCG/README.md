# NLPPCG - Natural Language PCG Forest Generator Plugin

언리얼 엔진 5용 자연어 PCG 숲 생성 플러그인

## 플러그인 구조

### Public 헤더 파일

#### NLPPCGModule.h
플러그인 모듈 정의

#### PCGForestGenerator.h
- `FPCGForestParameters`: 숲 생성 파라미터 구조체
- `UPCGForestGeneratorSettings`: PCG 노드 설정
- `FPCGForestGeneratorElement`: Poisson Disk Sampling 알고리즘 구현

#### MCPClient.h
- `AMCPClient`: MCP 서버와 통신하는 Actor
- MCP JSON-RPC 프로토콜 구현
- HTTP 요청/응답 처리

#### ForestPCGManager.h
- `AForestPCGManager`: 숲 생성/제거를 관리하는 메인 Actor
- PCG 그래프 동적 생성
- MCP 클라이언트 연동

## 클래스 다이어그램

```
AForestPCGManager
  ├── UPCGComponent (PCG 실행)
  ├── AMCPClient (MCP 통신)
  └── UStaticMesh* TreeMesh (나무 메시)
       │
       ├─> GenerateForestFromNLP(Command)
       │     └─> MCPClient->SendCommand(Command)
       │           └─> OnForestGenerated 델리게이트
       │                 └─> SetupPCGGraph(Parameters)
       │
       └─> PCG Graph:
             [ForestGenerator] -> [StaticMeshSpawner] -> Output
```

## 사용 방법

### 1. 블루프린트에서

```
// ForestPCGManager 배치
Place Actor -> NLPPCG -> ForestPCGManager

// 이벤트 그래프
Event BeginPlay
  -> Get ForestPCGManager Reference
  -> GenerateForestFromNLP
       Command: "밀집된 소나무 숲 만들어줘"
```

### 2. C++에서

```cpp
#include "ForestPCGManager.h"

// 스폰
AForestPCGManager* Manager = GetWorld()->SpawnActor<AForestPCGManager>();

// 숲 생성
Manager->GenerateForestFromNLP(TEXT("성긴 참나무 숲 500평방미터"));

// 직접 파라미터로 생성
FPCGForestParameters Params;
Params.TreeType = TEXT("oak");
Params.Density = TEXT("dense");
Params.MinDistance = 150.0f;
Params.MaxDistance = 300.0f;
Manager->GenerateForestFromParameters(Params);

// 제거
Manager->ClearForest();
```

## PCG 노드 사용

### PCG 그래프에서 직접 사용

1. PCG Component 추가
2. PCG Graph 생성
3. "Forest Generator" 노드 추가
4. Parameters 설정
5. Static Mesh Spawner 연결

## 델리게이트

### AMCPClient

```cpp
// MCP 응답 수신
UPROPERTY(BlueprintAssignable)
FOnMCPResponse OnMCPResponse;

// 숲 파라미터 수신
UPROPERTY(BlueprintAssignable)
FOnForestGenerated OnForestGenerated;
```

### 블루프린트 바인딩

```
MCPClient Reference
  -> OnForestGenerated
       -> Bind Event to OnForestGenerated
            -> Print String: "숲 생성됨"
```

## 파라미터 설명

### FPCGForestParameters

| 파라미터 | 타입 | 설명 | 기본값 |
|---------|------|------|--------|
| TreeType | FString | 나무 종류 | "generic_tree" |
| Density | FString | 밀도 (dense/medium/sparse) | "medium" |
| Size | FString | 크기 (tiny/small/medium/large/huge) | "medium" |
| AreaSize | float | 영역 크기 (cm²) | 5000.0 |
| MinDistance | float | 최소 거리 (cm) | 200.0 |
| MaxDistance | float | 최대 거리 (cm) | 500.0 |
| Randomness | float | 임의성 (0~1) | 0.5 |
| ScaleMultiplier | float | 스케일 배수 | 1.0 |

## 알고리즘

### Poisson Disk Sampling

```cpp
1. 랜덤 첫 포인트 생성
2. Active List에 추가
3. While (Active List가 비지 않음):
     a. 랜덤으로 Active List에서 포인트 선택
     b. 해당 포인트 주변에 N번 시도:
        - MinDist~MaxDist 거리에 새 포인트 생성
        - 다른 포인트와 MinDist 이상 떨어져 있으면 추가
     c. N번 실패 시 Active List에서 제거
4. 생성된 포인트들을 PCG 포인트로 변환
```

## 시뮬레이션 모드

현재 MCPClient는 시뮬레이션 모드로 동작합니다 (실제 HTTP 통신 없음).

실제 MCP 서버 사용 시 `MCPClient.cpp`의 주석을 해제하세요:

```cpp
// MCPClient.cpp, SendCommand() 함수

// 주석 해제:
Request->SetURL(ServerURL + TEXT("/rpc"));
Request->SetVerb(TEXT("POST"));
Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
Request->SetContentAsString(RequestBody);
Request->ProcessRequest();

// 시뮬레이션 코드 제거
```

## 디버깅

### 로그 출력

```cpp
// Output Log 창에서 확인
LogTemp: MCP Client initialized
LogTemp: Sending command to MCP: 밀집된 소나무 숲
LogTemp: Generated 234 tree points
LogTemp: PCG Forest generated
```

### 일반적인 문제

**문제**: PCG가 생성되지 않음
**해결**:
- PCGComponent가 활성화되어 있는지 확인
- Generate() 호출 여부 확인

**문제**: 메시가 보이지 않음
**해결**:
- TreeMesh 프로퍼티에 메시 할당
- Static Mesh Spawner 설정 확인

**문제**: MCP 연결 실패
**해결**:
- 현재 시뮬레이션 모드 사용 중
- 실제 서버 필요 시 주석 해제

## 의존성

- Unreal Engine 5.0+
- PCG Plugin (엔진 내장)
- HTTP Module
- Json Module
- JsonUtilities Module

## 빌드

```bash
# Visual Studio project 생성
Generate Visual Studio project files

# 빌드
Development Editor 구성으로 빌드
```

## 테스트

### 단위 테스트 (C++)

```cpp
// 테스트 코드 예시
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPCGForestTest, "NLPPCG.ForestGenerator",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPCGForestTest::RunTest(const FString& Parameters)
{
    UPCGForestGeneratorSettings* Settings = NewObject<UPCGForestGeneratorSettings>();
    Settings->ForestParameters.MinDistance = 200.0f;

    // 테스트 로직
    return true;
}
```

## 라이선스

MIT License
