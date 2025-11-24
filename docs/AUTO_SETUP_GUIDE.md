# 자동 설정 가이드

ForestPCGManager의 자동 설정 및 배치 기능 사용 가이드입니다.

## 🎯 주요 기능

### 1. 나무 메시 자동 등록

ForestPCGManager는 `nlp_handler.py`의 `tree_types`와 동일한 나무 타입을 자동으로 등록합니다.

#### 자동 등록되는 나무 타입

- `pine` - 소나무
- `oak` - 참나무
- `birch` - 자작나무
- `maple` - 단풍나무
- `generic_tree` - 일반 나무

#### 기본 메시

모든 타입은 기본적으로 `/Engine/BasicShapes/Cube` 메시로 설정됩니다.

### 2. 에디터에서 메시 변경

1. **ForestPCGManager 선택**
   - 레벨에서 ForestPCGManager Actor 선택

2. **Details 패널에서 Tree Type Meshes 섹션 찾기**
   - `Forest` > `Tree Meshes` > `Tree Type Meshes`

3. **각 나무 타입의 메시 변경**
   ```
   Pine → 소나무 Static Mesh 선택
   Oak → 참나무 Static Mesh 선택
   Birch → 자작나무 Static Mesh 선택
   Maple → 단풍나무 Static Mesh 선택
   Generic Tree → 기본 나무 Static Mesh 선택
   ```

4. **저장**
   - ForestPCGManager의 설정이 저장됩니다

### 3. PCG 그래프 자동 저장

ForestPCGManager는 생성된 PCG 그래프를 자동으로 에셋으로 저장합니다.

#### 설정

- **Save PCG Graph as Asset**: 자동 저장 활성화 (기본값: true)
- **Graph Asset Path**: 저장 경로 (기본값: `/Game/PCG/Graphs/`)

#### 저장되는 파일

```
/Game/PCG/Graphs/PCG_ForestGraph_YYYYMMDD_HHMMSS.uasset
```

예: `PCG_ForestGraph_20241221_153045.uasset`

---

## 🚀 빠른 사용법

### 방법 1: 블루프린트 (자동 배치)

가장 간단한 방법입니다. ForestPCGManager가 없으면 자동으로 생성됩니다.

```
Event BeginPlay
  |
  v
Generate Forest From NLP (NLPPCG Library)
  Command: "밀집된 소나무 숲"
  Spawn Location: (0, 0, 0)
```

**장점**:
- ForestPCGManager 수동 배치 불필요
- 자동으로 설정됨
- 한 줄로 숲 생성 가능

### 방법 2: C++ (자동 배치)

```cpp
#include "ForestPCGManagerLibrary.h"

void AMyGameMode::CreateForest()
{
    UForestPCGManagerLibrary::GenerateForestFromNLP(
        this,
        TEXT("밀집된 소나무 숲"),
        FVector(0, 0, 0)
    );
}
```

### 방법 3: 수동 배치 + 자동 설정

1. **ForestPCGManager 배치**
   - Place Actors → NLPPCG → ForestPCGManager
   - 레벨에 드래그 앤 드롭

2. **자동으로 설정됨**
   - 나무 메시 맵 자동 초기화 (모두 Cube)
   - PCG 그래프 자동 저장 활성화

3. **메시 변경 (선택사항)**
   - Details 패널에서 각 나무 타입의 메시 변경

4. **숲 생성**
   ```
   GenerateForestFromNLP: "밀집된 소나무 숲"
   ```

---

## 📝 자세한 사용 예시

### 예시 1: 게임 시작 시 자동 숲 생성

#### 블루프린트

```
Event BeginPlay
  |
  v
Get Or Create Forest PCG Manager (NLPPCG Library)
  Spawn Location: (0, 0, 0)
  |
  v
GenerateForestFromNLP
  Command: "성긴 참나무 숲 500평방미터"
```

#### C++

```cpp
void AMyGameMode::BeginPlay()
{
    Super::BeginPlay();

    // ForestPCGManager 가져오거나 생성
    AForestPCGManager* Manager = UForestPCGManagerLibrary::GetOrCreateForestPCGManager(
        this,
        FVector(0, 0, 0)
    );

    if (Manager)
    {
        Manager->GenerateForestFromNLP(TEXT("성긴 참나무 숲 500평방미터"));
    }
}
```

### 예시 2: 여러 숲 생성

```cpp
// 각기 다른 위치에 다양한 숲 생성
TArray<FVector> ForestLocations = {
    FVector(0, 0, 0),
    FVector(10000, 0, 0),
    FVector(0, 10000, 0)
};

TArray<FString> ForestCommands = {
    TEXT("밀집된 소나무 숲"),
    TEXT("성긴 참나무 숲"),
    TEXT("큰 자작나무 숲")
};

for (int32 i = 0; i < ForestLocations.Num(); i++)
{
    UForestPCGManagerLibrary::GenerateForestFromNLP(
        this,
        ForestCommands[i],
        ForestLocations[i]
    );
}
```

### 예시 3: 메시 프로그래밍 방식으로 설정

```cpp
AForestPCGManager* Manager = UForestPCGManagerLibrary::GetOrCreateForestPCGManager(this);

if (Manager)
{
    // 소나무 메시 설정
    UStaticMesh* PineMesh = LoadObject<UStaticMesh>(
        nullptr,
        TEXT("/Game/Trees/Pine_Tree.Pine_Tree")
    );

    if (PineMesh)
    {
        Manager->TreeMeshes.Add(TEXT("pine"), TSoftObjectPtr<UStaticMesh>(PineMesh));
    }

    // 참나무 메시 설정
    UStaticMesh* OakMesh = LoadObject<UStaticMesh>(
        nullptr,
        TEXT("/Game/Trees/Oak_Tree.Oak_Tree")
    );

    if (OakMesh)
    {
        Manager->TreeMeshes.Add(TEXT("oak"), TSoftObjectPtr<UStaticMesh>(OakMesh));
    }

    // 숲 생성
    Manager->GenerateForestFromNLP(TEXT("소나무와 참나무 혼합 숲"));
}
```

### 예시 4: 모든 PCG 그래프 저장

```cpp
// 레벨의 모든 ForestPCGManager의 그래프를 에셋으로 저장
int32 SavedCount = UForestPCGManagerLibrary::SaveAllPCGGraphsAsAssets(this);
UE_LOG(LogTemp, Log, TEXT("Saved %d PCG graphs"), SavedCount);
```

---

## 🔧 고급 설정

### 1. 나무 메시 초기화 강제 실행

에디터에서 버튼으로 실행:

1. ForestPCGManager 선택
2. Details 패널 → `Initialize Default Tree Meshes` 버튼 클릭

코드로 실행:

```cpp
Manager->InitializeDefaultTreeMeshes();
```

### 2. PCG 그래프 수동 저장

에디터에서:

1. ForestPCGManager 선택
2. Details 패널 → `Save PCG Graph as Asset` 버튼 클릭

코드로:

```cpp
if (Manager->SavePCGGraphAsAsset())
{
    UE_LOG(LogTemp, Log, TEXT("PCG Graph saved successfully"));
}
```

### 3. 저장 경로 변경

에디터에서:

1. ForestPCGManager 선택
2. Details 패널 → PCG → `Graph Asset Path` 변경
3. 예: `/Game/MyProject/PCG/`

코드로:

```cpp
Manager->GraphAssetPath = TEXT("/Game/MyProject/PCG/");
Manager->SavePCGGraphAsAsset();
```

### 4. 자동 저장 비활성화

```cpp
Manager->bSavePCGGraphAsAsset = false;
```

---

## 📊 Details 패널 구조

ForestPCGManager를 선택하면 다음과 같은 섹션들이 표시됩니다:

```
ForestPCGManager (Details)
├─ PCG
│  ├─ PCG Component (자동 생성됨)
│  ├─ PCG Graph Asset (저장된 그래프 참조)
│  ├─ Save PCG Graph as Asset (체크박스)
│  └─ Graph Asset Path (저장 경로)
│
├─ Forest
│  └─ Tree Meshes
│     └─ Tree Type Meshes (Map)
│        ├─ pine → Static Mesh
│        ├─ oak → Static Mesh
│        ├─ birch → Static Mesh
│        ├─ maple → Static Mesh
│        └─ generic_tree → Static Mesh
│
├─ Forest (Legacy)
│  └─ Default Tree Mesh (Legacy) (하위 호환성)
│
└─ MCP
   ├─ MCP Client (참조)
   └─ Auto Create MCP Client (체크박스)
```

---

## ⚡ 성능 팁

### 1. 메시 로딩 최적화

TSoftObjectPtr를 사용하므로 메시는 필요할 때만 로드됩니다:

```cpp
// 메시는 GetTreeMeshForType() 호출 시 로드됨
UStaticMesh* Mesh = Manager->GetTreeMeshForType(TEXT("pine"));
```

### 2. 그래프 재사용

한 번 저장된 PCG 그래프는 재사용 가능:

```cpp
// 저장된 그래프를 다른 Manager에 할당
AForestPCGManager* Manager2 = ...;
Manager2->PCGGraphAsset = Manager1->PCGGraphAsset;
```

### 3. 배치 생성

여러 숲을 한 번에 생성할 때:

```cpp
// 비동기로 생성하지 않고 순차적으로 생성
for (const FString& Command : Commands)
{
    Manager->GenerateForestFromNLP(Command);
    // 각 생성 사이에 딜레이 추가 권장
}
```

---

## 🐛 문제 해결

### Q: 나무 메시가 큐브로 표시돼요

**A**: 기본값이 큐브이므로, Details 패널에서 실제 나무 메시로 변경하세요.

1. ForestPCGManager 선택
2. Forest → Tree Meshes → Tree Type Meshes
3. 각 타입의 메시 변경

### Q: PCG 그래프가 저장되지 않아요

**A**: 다음을 확인하세요:

1. `Save PCG Graph as Asset`이 체크되어 있는지
2. `Graph Asset Path` 경로가 유효한지
3. 에디터에서 실행 중인지 (런타임에는 저장 안됨)

### Q: ForestPCGManager가 자동 생성되지 않아요

**A**: `GetOrCreateForestPCGManager` 또는 `GenerateForestFromNLP` 함수를 사용하세요:

```cpp
// 이 함수들은 자동으로 생성합니다
UForestPCGManagerLibrary::GenerateForestFromNLP(this, TEXT("숲"), FVector::ZeroVector);
```

### Q: 메시 설정이 저장되지 않아요

**A**: ForestPCGManager를 에셋으로 저장하세요:

1. World Outliner에서 ForestPCGManager 선택
2. 우클릭 → Convert to Blueprint Class
3. 블루프린트로 저장

---

## 📚 관련 문서

- [README.md](README.md) - 프로젝트 개요
- [USAGE.md](USAGE.md) - 상세 사용 가이드
- [ADVANCED_FEATURES.md](ADVANCED_FEATURES.md) - 고급 기능 (LLM, 바이옴 등)
- [UE5.7_COMPATIBILITY.md](UE5.7_COMPATIBILITY.md) - UE5.7 호환성 가이드

---

## 🎉 빠른 체크리스트

시작하기 전에 확인:

- [ ] 플러그인 빌드 성공
- [ ] 플러그인 활성화됨
- [ ] (선택) 나무 Static Mesh 에셋 준비

숲 생성:

- [ ] `GenerateForestFromNLP` 블루프린트 노드 호출
- [ ] 또는 `UForestPCGManagerLibrary::GenerateForestFromNLP()` C++ 호출
- [ ] 자연어 명령 입력 (예: "밀집된 소나무 숲")

메시 설정 (선택사항):

- [ ] ForestPCGManager 선택
- [ ] Details → Forest → Tree Meshes
- [ ] 각 타입의 메시 변경
- [ ] 저장

완료! 🎊
