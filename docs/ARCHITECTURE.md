# 시스템 아키텍처

## 전체 구조

```
┌─────────────────────────────────────────────────────────────┐
│                     사용자 (자연어 입력)                        │
└────────────────────────┬────────────────────────────────────┘
                         │
                         │ "밀집된 소나무 숲 만들어줘"
                         ▼
┌─────────────────────────────────────────────────────────────┐
│                  Unreal Engine 5 (클라이언트)                 │
│  ┌─────────────────────────────────────────────────────┐   │
│  │          ForestPCGManager (Actor)                    │   │
│  │  - 숲 생성 관리                                       │   │
│  │  - PCG 그래프 동적 구성                               │   │
│  │  - MCP 클라이언트 연동                                │   │
│  └──────────┬──────────────────────────┬────────────────┘   │
│             │                          │                     │
│             ▼                          ▼                     │
│  ┌──────────────────┐      ┌──────────────────────┐        │
│  │   MCPClient      │      │   PCGComponent        │        │
│  │   (Actor)        │      │   - PCG 실행 엔진     │        │
│  │  - HTTP 통신     │      │   - 포인트 생성       │        │
│  │  - JSON-RPC      │      │   - 메시 스폰         │        │
│  └────┬─────────────┘      └──┬───────────────────┘        │
│       │                       │                             │
└───────┼───────────────────────┼─────────────────────────────┘
        │                       │
        │ HTTP/JSON-RPC         │ PCG Graph
        ▼                       ▼
┌──────────────────┐   ┌──────────────────────────┐
│  MCP Server      │   │  PCG Nodes               │
│  (Python)        │   │  ┌────────────────────┐ │
│                  │   │  │ ForestGenerator    │ │
│  ┌────────────┐ │   │  │ (Poisson Disk)     │ │
│  │ NLP Handler│ │   │  └─────┬──────────────┘ │
│  │ - 파싱     │ │   │        │                 │
│  │ - 파라미터 │ │   │        ▼                 │
│  │   생성     │ │   │  ┌────────────────────┐ │
│  └────────────┘ │   │  │ StaticMeshSpawner  │ │
│                  │   │  │ (메시 인스턴싱)    │ │
└──────────────────┘   │  └────────────────────┘ │
                       └──────────────────────────┘
```

## 데이터 흐름

### 1. 자연어 → PCG 파라미터

```
"밀집된 소나무 숲 만들어줘"
    │
    ▼ [ForestPCGManager]
GenerateForestFromNLP(command)
    │
    ▼ [MCPClient]
SendCommand(command)
    │
    ▼ [HTTP Request]
{
  "jsonrpc": "2.0",
  "method": "tools/call",
  "params": {
    "name": "create_forest",
    "arguments": {
      "command": "밀집된 소나무 숲 만들어줘"
    }
  }
}
    │
    ▼ [MCP Server - NLP Handler]
parse_forest_command(command)
    │
    ▼ [파싱 결과]
{
  "tree_type": "pine",
  "density": "dense",
  "min_distance": 150.0,
  "max_distance": 300.0,
  "randomness": 0.3,
  ...
}
    │
    ▼ [HTTP Response]
{
  "action": "create_forest",
  "parameters": { ... }
}
    │
    ▼ [MCPClient]
ProcessForestCommand(response)
    │
    ▼ [델리게이트]
OnForestGenerated.Broadcast(params)
    │
    ▼ [ForestPCGManager]
OnForestParametersReceived(params)
    │
    ▼
SetupPCGGraph(params)
```

### 2. PCG 그래프 실행

```
SetupPCGGraph(params)
    │
    ▼
PCGGraph 생성/초기화
    │
    ├─> ForestGeneratorNode 생성
    │   └─> Parameters 설정
    │
    ├─> StaticMeshSpawnerNode 생성
    │   └─> Mesh 설정
    │
    └─> 노드 연결
        │
        ▼
PCGComponent->Generate()
    │
    ▼ [ForestGeneratorElement::Execute]
Poisson Disk Sampling
    │
    ├─> 첫 포인트 생성
    ├─> Active List에 추가
    └─> While (Active List):
        ├─> 랜덤 포인트 선택
        ├─> N번 시도:
        │   └─> MinDist~MaxDist에 새 포인트
        └─> 유효하면 추가, 아니면 제거
    │
    ▼ [PointData 생성]
TArray<FPCGPoint> (포인트 배열)
    │
    ▼ [StaticMeshSpawner::Execute]
각 포인트에 메시 스폰 (Instancing)
    │
    ▼ [렌더링]
HISM 컴포넌트로 최적화된 렌더링
```

## 주요 컴포넌트 상세

### 1. MCP Python 서버

#### nlp_handler.py

**클래스**: `ForestNLPHandler`

**주요 메서드**:
- `parse_forest_command(text)`: 자연어 → 파라미터
- `generate_response(params)`: 사용자 응답 생성

**파싱 알고리즘**:
```python
1. 사전 정의된 키워드 매칭
   - 나무 종류: {'소나무': 'pine', '참나무': 'oak', ...}
   - 밀도: {'빽빽': 'dense', '성긴': 'sparse', ...}
   - 크기: {'큰': 'large', '작은': 'small', ...}

2. 정규표현식으로 수치 추출
   - 면적: r'(\d+(?:\.\d+)?)\s*(?:평방미터|제곱미터|m2|㎡)'

3. 파라미터 조정
   - 밀도에 따라 min_distance, max_distance 자동 설정
   - 크기에 따라 scale_multiplier 설정
```

#### server.py

**클래스**: `PCGForestMCPServer`

**MCP 도구**:
1. `create_forest`: 숲 생성
2. `clear_forest`: 숲 제거
3. `modify_forest`: 숲 수정

**프로토콜**: JSON-RPC 2.0 over stdio

### 2. Unreal Engine 플러그인

#### PCGForestGenerator (PCG 노드)

**클래스**:
- `UPCGForestGeneratorSettings`: 노드 설정
- `FPCGForestGeneratorElement`: 실행 로직

**알고리즘**: Poisson Disk Sampling

```cpp
// 의사 코드
function PoissonDiskSampling():
    grid = 2D 그리드 (크기: BoundsSize / MinDist)
    activeList = []
    points = []

    // 첫 포인트
    p0 = RandomPointInBounds()
    activeList.add(p0)
    points.add(p0)
    grid[GetGridCell(p0)].add(0)

    while activeList is not empty:
        idx = Random(0, activeList.size)
        point = activeList[idx]

        found = false
        for attempt in 0..MaxAttempts:
            // MinDist ~ MaxDist 거리에 새 포인트
            angle = Random(0, 2π)
            dist = Random(MinDist, MaxDist)
            newPoint = point + (cos(angle), sin(angle)) * dist

            if IsValidPoint(newPoint, grid, MinDist):
                activeList.add(newPoint)
                points.add(newPoint)
                grid[GetGridCell(newPoint)].add(points.size - 1)
                found = true
                break

        if not found:
            activeList.remove(idx)

    return points
```

**시간 복잡도**: O(n), n = 생성된 포인트 수

**공간 복잡도**: O(n + grid_size²)

#### MCPClient (HTTP 클라이언트)

**클래스**: `AMCPClient`

**주요 기능**:
- HTTP 요청/응답 처리
- JSON 직렬화/역직렬화
- 델리게이트 브로드캐스트

**통신 방식**:
```cpp
// 현재: 시뮬레이션 모드 (서버 없이 동작)
// 실제: HTTP POST to ServerURL/rpc

Request:
POST /rpc HTTP/1.1
Content-Type: application/json

{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "tools/call",
  "params": {
    "name": "create_forest",
    "arguments": {
      "command": "..."
    }
  }
}

Response:
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": {
    "content": [{
      "type": "text",
      "text": "{\"action\": \"create_forest\", \"parameters\": {...}}"
    }]
  }
}
```

#### ForestPCGManager (관리 Actor)

**클래스**: `AForestPCGManager`

**주요 책임**:
1. PCG 그래프 동적 생성
2. MCP 클라이언트 관리
3. 델리게이트 바인딩
4. 메시 리소스 관리

**PCG 그래프 구조**:
```
Input (없음)
  │
  ▼
┌─────────────────────┐
│ ForestGenerator     │
│ - Parameters        │
│ - BoundsSize        │
│ - Seed              │
└──────────┬──────────┘
           │ Point Data
           ▼
┌─────────────────────┐
│ StaticMeshSpawner   │
│ - Meshes[]          │
│ - InstancePacking   │
└──────────┬──────────┘
           │ Mesh Instances
           ▼
        Output
```

## 성능 특성

### 포인트 생성

- **복잡도**: O(n), n = 포인트 수
- **병목**: 거리 체크 (그리드 최적화로 완화)
- **확장성**: ~10,000 포인트까지 실시간 (<100ms)

### 메시 렌더링

- **기술**: Hierarchical Instanced Static Mesh (HISM)
- **드로우콜**: 1개 (같은 메시 사용 시)
- **메모리**: O(n), 인스턴스 데이터만 저장
- **확장성**: 수십만 인스턴스 렌더링 가능

### 통신 오버헤드

- **현재 (시뮬레이션)**: ~0ms
- **실제 (HTTP)**: ~10-50ms (로컬 네트워크)
- **최적화**: 배치 요청, 응답 캐싱 가능

## 확장 포인트

### 1. 새로운 나무 종류 추가

```python
# nlp_handler.py
self.tree_types['벚나무'] = 'cherry'
```

### 2. 커스텀 분포 알고리즘

```cpp
// PCGForestGenerator.cpp
// ExecuteInternal() 메서드 수정
// Perlin Noise, Blue Noise 등 적용 가능
```

### 3. LLM 통합

```python
# nlp_handler.py
import anthropic

def parse_forest_command_with_llm(text):
    client = anthropic.Anthropic()
    response = client.messages.create(
        model="claude-3-5-sonnet-20241022",
        messages=[{
            "role": "user",
            "content": f"Extract forest parameters from: {text}"
        }]
    )
    # JSON 파싱 후 반환
```

### 4. 바이옴 시스템

```cpp
// BiomeSettings.h
USTRUCT()
struct FBiomeSettings
{
    UPROPERTY()
    TMap<FString, float> TreeTypeWeights;

    UPROPERTY()
    FPCGForestParameters BaseParameters;
};

// 바이옴별로 다른 나무 조합
```

### 5. 지형 적응

```cpp
// PCGForestGenerator.cpp
// 지형 높이/경사 샘플링
FVector SurfaceNormal = GetTerrainNormal(Point);
if (SurfaceNormal.Z < 0.7f) // 너무 가파름
{
    continue; // 포인트 스킵
}
```

## 보안 고려사항

### 1. 입력 검증

```python
# nlp_handler.py
def validate_command(text):
    if len(text) > 1000:
        raise ValueError("Command too long")
    if contains_malicious_patterns(text):
        raise ValueError("Invalid command")
```

### 2. 리소스 제한

```cpp
// PCGForestGenerator.cpp
const int32 MaxPoints = 10000;
if (Points.Num() >= MaxPoints)
{
    break; // 포인트 수 제한
}
```

### 3. 네트워크 보안

```cpp
// MCPClient.cpp
// HTTPS 사용
Request->SetURL(TEXT("https://") + ServerURL);

// 토큰 인증
Request->SetHeader(TEXT("Authorization"), TEXT("Bearer ") + AuthToken);
```

## 테스트 전략

### 1. 단위 테스트

```python
# test_nlp.py
def test_parse_command():
    handler = ForestNLPHandler()
    params = handler.parse_forest_command("밀집된 소나무 숲")
    assert params['tree_type'] == 'pine'
    assert params['density'] == 'dense'
```

### 2. 통합 테스트

```cpp
// ForestPCGManagerTest.cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FForestIntegrationTest, ...)

bool FForestIntegrationTest::RunTest(...)
{
    AForestPCGManager* Manager = SpawnManager();
    Manager->GenerateForestFromNLP(TEXT("소나무 숲"));
    // PCG 완료 대기
    // 포인트 수 검증
    return true;
}
```

### 3. 성능 테스트

```python
# benchmark.py
def benchmark_parsing():
    for _ in range(10000):
        parse_forest_command("밀집된 숲")
    # 평균 시간 측정
```

## 배포 고려사항

### 1. MCP 서버

```bash
# Docker 컨테이너로 배포
FROM python:3.11-slim
COPY MCPServer /app
RUN pip install -r requirements.txt
CMD ["python", "server.py"]
```

### 2. 언리얼 플러그인

```
# Marketplace 배포 또는
# GitHub Release로 소스 배포
# 빌드된 바이너리 제공
```

### 3. 설정 관리

```ini
# DefaultGame.ini
[/Script/NLPPCG.MCPClient]
ServerURL=http://localhost:8000
DebugMode=False
```

## 모니터링 및 로깅

```cpp
// 구조화된 로깅
DEFINE_LOG_CATEGORY(LogNLPPCG);

UE_LOG(LogNLPPCG, Log, TEXT("Forest generated: %d points, %.1fms"),
    PointCount, GenerationTime);
```

## 라이선스 및 저작권

- Unreal Engine: Epic Games 라이선스
- MCP SDK: Anthropic 라이선스
- 플러그인 코드: MIT License
