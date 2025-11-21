# 사용 가이드

## 빠른 시작 (5분 안에 숲 생성하기)

### 1단계: Python 서버 실행 (선택사항)

```bash
cd MCPServer
pip install -r requirements.txt
python server.py
```

**참고**: 현재는 시뮬레이션 모드로도 동작하므로 서버 없이도 테스트 가능합니다.

### 2단계: 언리얼 프로젝트 설정

1. 새 UE5 프로젝트 생성 (또는 기존 프로젝트)
2. 프로젝트 폴더에 `Plugins` 디렉토리 생성
3. `Plugins/NLPPCG` 전체 복사
4. 프로젝트 재시작

### 3단계: 레벨에 ForestPCGManager 배치

1. **콘텐츠 브라우저**에서 C++ 클래스 필터 활성화
2. **Place Actors** 패널에서 "ForestPCGManager" 검색
3. 레벨에 드래그 앤 드롭

### 4단계: 블루프린트로 숲 생성

#### 옵션 A: 레벨 블루프린트 사용

1. **블루프린트** → **레벨 블루프린트 열기**
2. 이벤트 그래프에 다음 노드 추가:

```
Event BeginPlay
  |
  v
Get Actor Of Class (ForestPCGManager)
  |
  v
GenerateForestFromNLP
  Command: "밀집된 소나무 숲 만들어줘"
```

3. **컴파일** → **저장**
4. **플레이** 버튼 클릭

#### 옵션 B: 위젯 블루프린트로 UI 생성

1. **Content Browser** → **우클릭** → **User Interface** → **Widget Blueprint**
2. 이름: `WBP_ForestChat`
3. **캔버스**에 다음 추가:
   - Text Box (이름: `TB_Command`)
   - Button (이름: `BTN_Generate`)
   - Text Block (레이블: "숲 생성")

4. **그래프**:

```
BTN_Generate - On Clicked
  |
  v
Get Text (TB_Command)
  |
  v
Get Actor Of Class (ForestPCGManager)
  |
  v
GenerateForestFromNLP
  Command: <TB_Command의 텍스트>
```

5. **레벨 블루프린트**에서 위젯 생성:

```
Event BeginPlay
  |
  v
Create Widget (WBP_ForestChat)
  |
  v
Add to Viewport
```

## 자연어 명령 예시

### 기본 명령

```
"숲 만들어줘"
"소나무 숲 생성"
"참나무 나무들 배치해줘"
```

### 밀도 조정

```
"빽빽한 숲 만들어줘"
"밀집된 소나무 숲"
"성긴 참나무 숲"
"듬성듬성 나무 배치"
```

### 크기 조정

```
"큰 나무들로 숲 만들어줘"
"작은 나무 숲"
"거대한 참나무들"
```

### 면적 지정

```
"500평방미터에 숲 만들어줘"
"1000제곱미터 소나무 숲"
"100m2 밀집된 숲"
```

### 복합 명령

```
"밀집된 큰 소나무 숲을 1000평방미터에 생성해줘"
"성긴 작은 자작나무 숲 500㎡"
"거대한 참나무들로 빽빽한 숲을 2000평방미터에"
```

## C++ 사용 예시

### 1. 헤더에 포함

```cpp
#include "ForestPCGManager.h"
#include "MCPClient.h"
```

### 2. 멤버 변수 선언

```cpp
UPROPERTY()
AForestPCGManager* ForestManager;
```

### 3. BeginPlay에서 초기화

```cpp
void AMyGameMode::BeginPlay()
{
    Super::BeginPlay();

    // ForestPCGManager 스폰
    FActorSpawnParameters SpawnParams;
    ForestManager = GetWorld()->SpawnActor<AForestPCGManager>(
        AForestPCGManager::StaticClass(),
        FVector::ZeroVector,
        FRotator::ZeroRotator,
        SpawnParams
    );
}
```

### 4. 숲 생성

```cpp
void AMyGameMode::CreateForest()
{
    if (ForestManager)
    {
        ForestManager->GenerateForestFromNLP(TEXT("밀집된 소나무 숲 1000평방미터"));
    }
}
```

### 5. 델리게이트 바인딩

```cpp
void AMyGameMode::BeginPlay()
{
    Super::BeginPlay();

    // ... ForestManager 생성 코드 ...

    // MCP 클라이언트 델리게이트 바인딩
    if (ForestManager && ForestManager->MCPClient)
    {
        ForestManager->MCPClient->OnMCPResponse.AddDynamic(
            this,
            &AMyGameMode::OnMCPResponseReceived
        );

        ForestManager->MCPClient->OnForestGenerated.AddDynamic(
            this,
            &AMyGameMode::OnForestGenerated
        );
    }
}

UFUNCTION()
void AMyGameMode::OnMCPResponseReceived(const FString& Response)
{
    UE_LOG(LogTemp, Log, TEXT("MCP Response: %s"), *Response);
}

UFUNCTION()
void AMyGameMode::OnForestGenerated(const FPCGForestParameters& Parameters)
{
    UE_LOG(LogTemp, Log, TEXT("Forest generated with density: %s"), *Parameters.Density);
}
```

## 파라미터 직접 제어

```cpp
// 파라미터 구조체 생성
FPCGForestParameters Params;
Params.TreeType = TEXT("oak");
Params.Density = TEXT("dense");
Params.Size = TEXT("large");
Params.AreaSize = 10000.0f; // 100㎡ in cm²
Params.MinDistance = 150.0f;
Params.MaxDistance = 300.0f;
Params.Randomness = 0.3f;
Params.ScaleMultiplier = 1.5f;

// 직접 생성
ForestManager->GenerateForestFromParameters(Params);
```

## 커스텀 메시 사용

### 블루프린트

1. ForestPCGManager 선택
2. Details 패널에서 **Tree Mesh** 찾기
3. 원하는 Static Mesh 할당

### C++

```cpp
// Static Mesh 로드
UStaticMesh* CustomTreeMesh = LoadObject<UStaticMesh>(
    nullptr,
    TEXT("/Game/Models/Trees/Pine_Tree.Pine_Tree")
);

// 할당
ForestManager->TreeMesh = CustomTreeMesh;

// 숲 생성
ForestManager->GenerateForestFromNLP(TEXT("소나무 숲"));
```

## 런타임 동적 생성

### 플레이어 위치 기반

```cpp
void AMyCharacter::SpawnForestNearPlayer()
{
    FVector PlayerLocation = GetActorLocation();
    FVector ForestLocation = PlayerLocation + FVector(1000.0f, 0.0f, 0.0f);

    // ForestManager 이동
    ForestManager->SetActorLocation(ForestLocation);

    // 숲 생성
    ForestManager->GenerateForestFromNLP(TEXT("성긴 참나무 숲"));
}
```

### 시간 기반 변화

```cpp
void AMyGameMode::UpdateForestByTimeOfDay()
{
    float Hour = GetTimeOfDay();

    if (Hour >= 6.0f && Hour < 12.0f)
    {
        // 아침: 성긴 숲
        ForestManager->GenerateForestFromNLP(TEXT("성긴 숲"));
    }
    else if (Hour >= 12.0f && Hour < 18.0f)
    {
        // 오후: 보통 숲
        ForestManager->GenerateForestFromNLP(TEXT("보통 숲"));
    }
    else
    {
        // 밤: 밀집된 숲
        ForestManager->GenerateForestFromNLP(TEXT("밀집된 어두운 숲"));
    }
}
```

## 여러 숲 관리

```cpp
TArray<AForestPCGManager*> ForestManagers;

void CreateMultipleForests()
{
    for (int32 i = 0; i < 5; i++)
    {
        FVector Location = FVector(i * 10000.0f, 0.0f, 0.0f);

        AForestPCGManager* Manager = GetWorld()->SpawnActor<AForestPCGManager>(
            AForestPCGManager::StaticClass(),
            Location,
            FRotator::ZeroRotator
        );

        ForestManagers.Add(Manager);

        // 각기 다른 숲
        switch (i)
        {
            case 0: Manager->GenerateForestFromNLP(TEXT("소나무 숲")); break;
            case 1: Manager->GenerateForestFromNLP(TEXT("참나무 숲")); break;
            case 2: Manager->GenerateForestFromNLP(TEXT("자작나무 숲")); break;
            case 3: Manager->GenerateForestFromNLP(TEXT("밀집된 숲")); break;
            case 4: Manager->GenerateForestFromNLP(TEXT("성긴 숲")); break;
        }
    }
}
```

## 성능 최적화

### 1. LOD 설정

```cpp
// TreeMesh에 LOD 설정
TreeMesh->SetLODGroup(NAME_TreesAndFoliage);
```

### 2. 컬링 거리

```cpp
// Static Mesh Spawner 설정에서
SpawnerSettings->CullStartDistance = 5000.0f;
SpawnerSettings->CullEndDistance = 10000.0f;
```

### 3. 인스턴싱

PCG는 자동으로 Hierarchical Instanced Static Mesh를 사용하므로 최적화되어 있습니다.

## 디버깅 팁

### 1. 비주얼 디버깅

```cpp
// ForestPCGManager 선택 상태에서
PCGComponent->bShowDebug = true;
```

### 2. 로그 레벨 조정

```cpp
// DefaultEngine.ini
[Core.Log]
LogTemp=VeryVerbose
```

### 3. PCG 그래프 시각화

1. ForestPCGManager 선택
2. Details 패널 → PCG Component
3. "Open PCG Graph" 클릭

## 트러블슈팅

### "숲이 생성되지 않아요"

1. Output Log 확인
2. ForestPCGManager가 레벨에 있는지 확인
3. TreeMesh가 할당되어 있는지 확인
4. PCGComponent의 Generate 호출 여부 확인

### "메시가 보이지 않아요"

1. TreeMesh 프로퍼티 확인
2. Static Mesh Spawner 설정 확인
3. 카메라 위치 확인 (숲이 생성된 위치에 있는지)

### "MCP 서버 연결 안 돼요"

현재 시뮬레이션 모드로 동작하므로 서버 없이도 작동합니다.
실제 서버 연결 필요 시 MCPClient.cpp 주석 참조.

## 추가 리소스

- [PCG 공식 문서](https://docs.unrealengine.com/5.0/en-US/procedural-content-generation-overview/)
- [MCP 프로토콜](https://spec.modelcontextprotocol.io/)
- GitHub Issues: 버그 리포트 및 기능 요청
