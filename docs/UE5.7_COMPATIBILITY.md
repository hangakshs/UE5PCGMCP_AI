# UE5.7 호환성 가이드

UE5.7에서는 PCG API가 일부 변경되어, 수동 설정이 필요한 부분이 있습니다.

## Static Mesh 설정

### 문제
UE5.7에서는 `FPCGStaticMeshSpawnerEntry` 타입이 제거되어, C++ 코드에서 메시를 자동으로 설정할 수 없습니다.

### 해결 방법

#### 옵션 1: PCG Graph 에디터에서 수동 설정 (권장)

1. **ForestPCGManager 배치**
   - 레벨에 ForestPCGManager Actor 배치

2. **PCG Graph 열기**
   - ForestPCGManager 선택
   - Details 패널 → PCG Component → Graph
   - "Open PCG Graph Editor" 클릭

3. **Static Mesh Spawner 노드 설정**
   - Graph에서 "Static Mesh Spawner" 노드 선택
   - Details 패널에서 Mesh 속성 설정:
     - **Mesh Selector Type**: `ByAttribute` 또는 `StaticList`
     - **Static Meshes**: 사용할 나무 메시 추가
     - 예: `/Engine/BasicShapes/Cube` (테스트용)

4. **저장**
   - Ctrl+S로 그래프 저장

#### 옵션 2: 블루프린트에서 설정

```
Event BeginPlay
  |
  v
Get ForestPCGManager
  |
  v
Get PCGComponent
  |
  v
Get Graph
  |
  v
Get Node By Name: "Static Mesh Spawner"
  |
  v
Set Mesh (StaticMeshSpawnerSettings의 Mesh 속성 설정)
```

#### 옵션 3: TreeMeshLibrary 사용 (고급)

고급 기능의 TreeMeshLibrary를 사용하면 자동으로 메시를 선택할 수 있습니다:

1. **TreeMeshLibrary 데이터 에셋 생성**
   - Content Browser → Data Asset → TreeMeshLibrary
   - 나무 종류별 메시 등록

2. **ForestPCGManager에 연결**
   - TreeMeshLibrary 프로퍼티에 생성한 데이터 에셋 설정

자세한 내용은 [ADVANCED_FEATURES.md](ADVANCED_FEATURES.md#2-나무-메시-자동-선택)를 참조하세요.

## 알려진 UE5.7 변경사항

### 1. Seed 변수
- **변경**: `UPCGSettings`에 이미 `Seed` 변수가 정의됨
- **영향**: 자식 클래스에서 Seed 재정의 불가
- **해결**: 부모 클래스의 Seed 사용

### 2. RemoveAllNodes()
- **변경**: `UPCGGraph::RemoveAllNodes()` 메서드 제거
- **영향**: 그래프 초기화 시 오류
- **해결**: `GetNodes()` + `RemoveNodes()` 사용

### 3. RootComponent 타입
- **변경**: `UPCGComponent`를 `RootComponent`에 직접 할당 불가
- **영향**: Actor 생성 시 컴파일 오류
- **해결**: `Cast<USceneComponent>()` 사용

### 4. FPCGStaticMeshSpawnerEntry
- **변경**: 구조체 제거 또는 변경
- **영향**: 메시 자동 설정 불가
- **해결**: PCG Graph 에디터에서 수동 설정

## 빠른 시작 (UE5.7)

```cpp
// 1. ForestPCGManager 생성
AForestPCGManager* Manager = GetWorld()->SpawnActor<AForestPCGManager>();

// 2. 자연어로 숲 생성 (포인트만 생성됨)
Manager->GenerateForestFromNLP(TEXT("밀집된 소나무 숲"));

// 3. PCG Graph 에디터에서 Static Mesh Spawner 노드의 메시 설정
//    (한 번만 설정하면 이후 자동 적용)
```

## 문제 해결

### Q: 숲이 생성되지 않아요
A: Output Log를 확인하세요. "Please configure Static Mesh Spawner node manually" 메시지가 있다면, PCG Graph 에디터에서 메시를 설정해야 합니다.

### Q: 메시를 프로그래밍 방식으로 설정하고 싶어요
A: UE5.7에서는 제한적입니다. TreeMeshLibrary 시스템을 사용하거나, 블루프린트에서 동적으로 설정하는 방법을 권장합니다.

### Q: 이전 UE5 버전과 호환되나요?
A: UE5.0-5.6에서는 자동 메시 설정이 작동할 수 있지만, 테스트되지 않았습니다. UE5.7용으로 최적화되어 있습니다.

## 참고 자료

- [UE5.7 PCG Release Notes](https://docs.unrealengine.com/5.7/en-US/procedural-content-generation-release-notes/)
- [ADVANCED_FEATURES.md](ADVANCED_FEATURES.md)
- [USAGE.md](USAGE.md)
