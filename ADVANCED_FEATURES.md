# 고급 기능 가이드

이 문서는 NLP PCG Forest Generator의 고급 기능들을 설명합니다.

## 목차

1. [LLM 통합 (Claude/GPT)](#1-llm-통합-claudegpt)
2. [나무 메시 자동 선택](#2-나무-메시-자동-선택)
3. [지형 기반 배치](#3-지형-기반-배치)
4. [바이옴 시스템](#4-바이옴-시스템)

---

## 1. LLM 통합 (Claude/GPT)

### 개요

기본 NLP 파서 대신 Claude 같은 대형 언어 모델을 사용하여 더 자연스럽고 복잡한 명령을 처리할 수 있습니다.

### 설정

#### 1. API 키 설정

```bash
# 환경 변수로 설정 (권장)
export ANTHROPIC_API_KEY="your-api-key-here"

# 또는 코드에서 직접 설정
```

#### 2. LLM 핸들러 사용

```python
from llm_handler import LLMForestHandler

# LLM 활성화
handler = LLMForestHandler(use_llm=True)

# 자연어 명령 파싱
params = handler.parse_with_llm("아름다운 가을 숲을 만들고 싶어요")

# 결과:
# {
#   "tree_type": "maple",  # "가을 숲" → 단풍나무
#   "density": "medium",
#   "size": "medium",
#   ...
# }
```

### 지원되는 고급 명령

#### 감성적 표현

```
"아름다운 가을 숲을 만들고 싶어요"
→ 단풍나무(maple), 중간 밀도

"어두운 침엽수림을 생성해줘"
→ 소나무(pine), 밀집(dense)

"신비로운 자작나무 숲"
→ 자작나무(birch), 성긴(sparse), 큰 크기
```

#### 계절/분위기 기반

```
"겨울 숲"
→ 소나무(pine) 위주

"봄 숲"
→ 자작나무(birch) 위주

"가을 단풍 숲"
→ 단풍나무(maple) 위주
```

#### 복잡한 요구사항

```
"넓은 공원에 나무를 듬성듬성 심고 싶어요. 사람들이 산책할 수 있도록"
→ sparse 밀도, 큰 간격

"산 중턱에 울창한 숲을 만들어서 야생동물 서식지로 사용하려고 해요"
→ dense 밀도, 다양한 크기
```

### MCP 서버 통합

`server.py`에서 LLM 핸들러 사용:

```python
from llm_handler import LLMForestHandler

class PCGForestMCPServer:
    def __init__(self, use_llm=False):
        self.app = Server("pcg-forest-server")
        self.nlp_handler = ForestNLPHandler()

        # LLM 핸들러 추가
        if use_llm:
            self.llm_handler = LLMForestHandler(use_llm=True)
        else:
            self.llm_handler = None

    async def handle_create_forest(self, arguments: dict):
        command = arguments.get("command", "")

        # LLM 사용 가능하면 LLM으로, 아니면 기본 파서로
        if self.llm_handler and self.llm_handler.use_llm:
            params = self.llm_handler.parse_with_llm(command)
        else:
            params = self.nlp_handler.parse_forest_command(command)

        # ... 나머지 처리
```

### 비용 최적화

LLM API 호출은 비용이 발생하므로:

1. **하이브리드 모드**: 간단한 명령은 기본 파서, 복잡한 명령만 LLM
2. **캐싱**: 같은 명령은 결과 캐시
3. **배치 처리**: 여러 명령을 한 번에 처리

```python
class HybridHandler:
    def __init__(self):
        self.basic_handler = ForestNLPHandler()
        self.llm_handler = LLMForestHandler(use_llm=True)
        self.cache = {}

    def parse(self, command):
        # 캐시 확인
        if command in self.cache:
            return self.cache[command]

        # 간단한 명령 감지 (키워드 매칭)
        simple_keywords = ['소나무', '참나무', '자작나무', '밀집', '성긴']
        if any(keyword in command for keyword in simple_keywords):
            # 기본 파서 사용 (무료)
            result = self.basic_handler.parse_forest_command(command)
        else:
            # LLM 사용 (유료)
            result = self.llm_handler.parse_with_llm(command)

        # 캐싱
        self.cache[command] = result
        return result
```

---

## 2. 나무 메시 자동 선택

### 개요

나무 종류에 따라 적절한 메시를 자동으로 선택하는 시스템입니다.

### Tree Mesh Library 생성

#### 1. 데이터 에셋 생성

1. Content Browser에서 우클릭
2. **Miscellaneous** → **Data Asset**
3. **TreeMeshLibrary** 선택
4. 이름: `DA_TreeMeshLibrary`

#### 2. 메시 등록

```cpp
// 예시: 소나무 메시 등록
FTreeMeshEntry PineEntry;
PineEntry.TreeType = TEXT("pine");
PineEntry.Meshes.Add(TSoftObjectPtr<UStaticMesh>(TEXT("/Game/Trees/Pine_01")));
PineEntry.Meshes.Add(TSoftObjectPtr<UStaticMesh>(TEXT("/Game/Trees/Pine_02")));
PineEntry.Meshes.Add(TSoftObjectPtr<UStaticMesh>(TEXT("/Game/Trees/Pine_03")));
PineEntry.DefaultScale = FVector(1.0f, 1.0f, 1.2f); // 소나무는 약간 크게
PineEntry.ScaleVariation = FVector(0.3f, 0.3f, 0.4f);
PineEntry.Weight = 1.0f;
```

#### 3. 계절별 메시

```cpp
// 가을 단풍나무
FTreeMeshEntry MapleAutumn;
MapleAutumn.TreeType = TEXT("maple");
MapleAutumn.Season = TEXT("Autumn");
MapleAutumn.Meshes.Add(...); // 빨간 잎 메시
MapleAutumn.Weight = 2.0f; // 높은 가중치

// 여름 단풍나무
FTreeMeshEntry MapleSummer;
MapleSummer.TreeType = TEXT("maple");
MapleSummer.Season = TEXT("Summer");
MapleSummer.Meshes.Add(...); // 녹색 잎 메시
MapleSummer.Weight = 1.0f;
```

### 사용 방법

#### 블루프린트

```
ForestPCGManager
  -> TreeMeshLibrary: DA_TreeMeshLibrary (설정)
  -> GenerateForestFromNLP: "소나무 숲"
```

자동으로 `DA_TreeMeshLibrary`에서 "pine" 타입의 메시들을 랜덤 선택합니다.

#### C++

```cpp
// TreeMeshLibrary 로드
UTreeMeshLibrary* Library = LoadObject<UTreeMeshLibrary>(
    nullptr,
    TEXT("/Game/Data/DA_TreeMeshLibrary")
);

// 랜덤 스트림
FRandomStream RandomStream(12345);

// 소나무 메시 가져오기
UStaticMesh* PineMesh = Library->GetRandomMesh(TEXT("pine"), RandomStream);

// 스케일 정보 가져오기
FVector DefaultScale, ScaleVariation;
Library->GetScaleInfo(TEXT("pine"), DefaultScale, ScaleVariation);

// 포인트에 적용
FPCGPoint Point;
Point.Transform.SetScale3D(
    DefaultScale + FVector(
        RandomStream.FRandRange(-ScaleVariation.X, ScaleVariation.X),
        RandomStream.FRandRange(-ScaleVariation.Y, ScaleVariation.Y),
        RandomStream.FRandRange(-ScaleVariation.Z, ScaleVariation.Z)
    )
);
```

### 동적 메시 전환

LOD(Level of Detail)나 거리에 따라 메시 전환:

```cpp
// 거리 기반 메시 선택
UStaticMesh* SelectMeshByDistance(float Distance, UTreeMeshLibrary* Library)
{
    if (Distance < 1000.0f)
    {
        // 가까우면 고품질 메시
        return Library->GetMeshesByType("pine", "")[0].Meshes[0].LoadSynchronous();
    }
    else if (Distance < 5000.0f)
    {
        // 중간 거리면 중간 품질
        return Library->GetMeshesByType("pine", "")[0].Meshes[1].LoadSynchronous();
    }
    else
    {
        // 멀면 저품질 빌보드
        return Library->GetMeshesByType("pine", "")[0].Meshes[2].LoadSynchronous();
    }
}
```

---

## 3. 지형 기반 배치

### 개요

경사, 고도 등 지형 데이터를 분석하여 자연스러운 나무 배치를 생성합니다.

### PCG 그래프 구성

```
ForestGenerator
    ↓
TerrainAdapter (새로 추가!)
    ↓
StaticMeshSpawner
```

### Terrain Adapter 설정

#### 1. 노드 추가

PCG 그래프에서:
1. **ForestGenerator** 출력에 우클릭
2. **Add Node** → **TerrainAdapter**
3. **TerrainAdapter** → **StaticMeshSpawner** 연결

#### 2. 설정 파라미터

```cpp
// TerrainAdapter 설정
UPCGTerrainAdapterSettings* TerrainSettings = NewObject<UPCGTerrainAdapterSettings>();

// 경사 필터: 0~30도만 허용 (가파른 곳 제외)
TerrainSettings->TerrainFilter.MinSlope = 0.0f;
TerrainSettings->TerrainFilter.MaxSlope = 30.0f;

// 고도 필터: 해발 0~1000m
TerrainSettings->TerrainFilter.MinAltitude = 0.0f;
TerrainSettings->TerrainFilter.MaxAltitude = 100000.0f; // 1000m = 100000cm

// 경사에 따른 자동 조정
TerrainSettings->TerrainFilter.bAdjustDensityBySlope = true;
TerrainSettings->TerrainFilter.bAdjustScaleBySlope = true;

// 지형에 정렬
TerrainSettings->TerrainFilter.bAlignToTerrain = true;
TerrainSettings->TerrainFilter.AlignmentStrength = 0.7f;
```

### 효과

#### 경사 필터링

```
평지 (0-15도): 밀집된 나무
완만한 경사 (15-30도): 중간 밀도
가파른 경사 (30도+): 나무 없음 (바위/절벽)
```

#### 자동 밀도 조정

```cpp
// 경사가 클수록 밀도 감소
float DensityMultiplier = 1.0f - (Slope / 45.0f) * 0.5f;

// 예:
// Slope = 0도 → Multiplier = 1.0 (100%)
// Slope = 22.5도 → Multiplier = 0.75 (75%)
// Slope = 45도 → Multiplier = 0.5 (50%)
```

#### 자동 스케일 조정

```cpp
// 경사가 클수록 나무 작게 (풍압, 토양 등)
float ScaleMultiplier = 1.0f - (Slope / 45.0f) * 0.2f;

// 예:
// Slope = 0도 → Scale = 1.0x
// Slope = 45도 → Scale = 0.8x
```

#### 지형 정렬

나무가 경사면을 따라 자연스럽게 회전:

```cpp
// 지형 법선 방향으로 회전
FQuat TerrainRotation = FRotationMatrix::MakeFromZ(TerrainNormal).ToQuat();

// AlignmentStrength로 보간 (0.7 = 70% 정렬)
FQuat FinalRotation = FQuat::Slerp(OriginalRotation, TerrainRotation, 0.7f);
```

### 고도별 나무 종 변화

```cpp
// 고도에 따라 다른 나무 종 선택
FString SelectTreeByAltitude(float Altitude)
{
    if (Altitude < 30000.0f) // 0-300m
    {
        return TEXT("oak"); // 저지대: 참나무
    }
    else if (Altitude < 80000.0f) // 300-800m
    {
        return TEXT("pine"); // 중간 고도: 소나무
    }
    else
    {
        return TEXT("birch"); // 고산: 자작나무
    }
}
```

---

## 4. 바이옴 시스템

### 개요

실제 생태계를 모방한 바이옴(생물군계) 시스템으로, 침엽수림, 활엽수림, 혼합림 등을 생성합니다.

### 지원되는 바이옴

#### 1. 침엽수림 (Coniferous Forest)

- **나무 종**: 소나무 70%, 기타 30%
- **밀도**: Dense
- **기후**: -20°C ~ 15°C, 강수량 500mm
- **특징**: 어두운 숲, 침엽수 우세

#### 2. 활엽수림 (Deciduous Forest)

- **나무 종**: 참나무 40%, 단풍나무 30%, 자작나무 30%
- **밀도**: Medium
- **층위 구조**: 교목층 60%, 아교목층 30%, 관목층 10%
- **기후**: -5°C ~ 25°C, 강수량 800mm
- **특징**: 계절 변화, 다층 구조

#### 3. 혼합림 (Mixed Forest)

- **나무 종**: 소나무, 참나무, 자작나무, 단풍나무 각 25%
- **밀도**: Medium
- **층위 구조**: 활성화
- **기후**: -10°C ~ 20°C, 강수량 700mm
- **특징**: 침엽수와 활엽수 혼합

#### 4. 타이가 (Taiga)

- **나무 종**: 소나무 90%, 자작나무 10%
- **밀도**: Medium (넓은 간격)
- **기후**: -40°C ~ 10°C, 강수량 400mm
- **특징**: 추운 지역, 낮은 다양성

### 사용 방법

#### 바이옴 데이터 에셋 생성

1. Content Browser → Data Asset → **BiomeDataAsset**
2. 이름: `DA_Biomes`
3. 기본 바이옴들이 자동 생성됨

#### 바이옴으로 숲 생성

```cpp
// 바이옴 데이터 로드
UBiomeDataAsset* BiomeData = LoadObject<UBiomeDataAsset>(...);

// 활엽수림 설정 가져오기
FBiomeSettings DeciduousForest = BiomeData->GetBiomeSettings(
    EBiomeType::DeciduousForest
);

// PCG 파라미터 생성 (층위별로 분리됨)
TArray<FPCGForestParameters> LayerParameters;
UBiomeForestGenerator::GenerateParametersFromBiome(
    DeciduousForest,
    LayerParameters
);

// 각 층위별로 숲 생성
for (int32 i = 0; i < LayerParameters.Num(); i++)
{
    FPCGForestParameters& Params = LayerParameters[i];

    // i == 0: 교목층 (큰 나무)
    // i == 1: 아교목층 (중간 나무)
    // i == 2: 관목층 (작은 나무/관목)

    GenerateForestLayer(Params);
}
```

#### 기후 기반 바이옴 선택

```cpp
// 현재 위치의 기후 데이터
float Temperature = 15.0f; // 섭씨
float Precipitation = 800.0f; // mm

// 적합한 바이옴 찾기
EBiomeType SuitableBiome = UBiomeForestGenerator::FindSuitableBiome(
    Temperature,
    Precipitation,
    BiomeData
);

// 결과: DeciduousForest (활엽수림)
```

#### 자연어 명령으로 바이옴 생성

MCP 서버에서 바이옴 키워드 인식:

```python
# nlp_handler.py에 추가
self.biome_keywords = {
    '침엽수림': 'coniferous_forest',
    '활엽수림': 'deciduous_forest',
    '혼합림': 'mixed_forest',
    '타이가': 'taiga'
}

# 파싱
if '활엽수림' in command:
    params['biome'] = 'deciduous_forest'
```

사용자 명령:

```
"활엽수림을 만들어줘"
→ 참나무, 단풍나무, 자작나무가 혼합된 숲

"타이가 생성해줘"
→ 소나무 위주의 추운 지역 숲

"침엽수림 500평방미터"
→ 밀집된 소나무 숲
```

### 층위 구조 (Stratification)

실제 숲처럼 여러 층으로 구성:

```
교목층 (Canopy Layer)
  ├─ 높이: 15-30m
  ├─ 나무: 큰 참나무, 소나무
  └─ 비율: 60%

아교목층 (Understory Layer)
  ├─ 높이: 5-15m
  ├─ 나무: 중간 크기 단풍나무, 자작나무
  └─ 비율: 30%

관목층 (Shrub Layer)
  ├─ 높이: 0-5m
  ├─ 식물: 관목, 작은 나무
  └─ 비율: 10%
```

### 커스텀 바이옴 생성

```cpp
// 새 바이옴 정의
FBiomeSettings CustomBiome;
CustomBiome.BiomeType = EBiomeType::MixedForest;
CustomBiome.BiomeName = TEXT("My Custom Forest");

// 나무 종 구성
FTreeSpeciesDistribution Species1;
Species1.TreeType = TEXT("pine");
Species1.Proportion = 0.6f;
Species1.bDominantSpecies = true;
CustomBiome.SpeciesDistribution.Add(Species1);

FTreeSpeciesDistribution Species2;
Species2.TreeType = TEXT("birch");
Species2.Proportion = 0.4f;
CustomBiome.SpeciesDistribution.Add(Species2);

// 기본 파라미터
CustomBiome.BaseParameters.Density = TEXT("dense");
CustomBiome.BaseParameters.MinDistance = 180.0f;
CustomBiome.BaseParameters.MaxDistance = 350.0f;

// 층위 구조
CustomBiome.bEnableStratification = true;
CustomBiome.CanopyProportion = 0.7f;
CustomBiome.UnderstoryProportion = 0.2f;
CustomBiome.ShrubProportion = 0.1f;

// 기후
CustomBiome.MinTemperature = -15.0f;
CustomBiome.MaxTemperature = 18.0f;
CustomBiome.AnnualPrecipitation = 600.0f;

// BiomeDataAsset에 추가
BiomeData->Biomes.Add(CustomBiome);
```

---

## 종합 사용 예시

### 시나리오: 산악 지형에 현실적인 숲 생성

```cpp
// 1. 바이옴 선택 (기후 기반)
UBiomeDataAsset* BiomeData = LoadBiomeData();
EBiomeType Biome = UBiomeForestGenerator::FindSuitableBiome(
    12.0f,   // 12°C
    900.0f,  // 900mm 강수량
    BiomeData
);
// 결과: MixedForest (혼합림)

// 2. 바이옴에서 파라미터 생성
FBiomeSettings BiomeSettings = BiomeData->GetBiomeSettings(Biome);
TArray<FPCGForestParameters> LayerParams;
UBiomeForestGenerator::GenerateParametersFromBiome(BiomeSettings, LayerParams);

// 3. PCG 그래프 구성
// ForestGenerator → TerrainAdapter → MeshSpawner (교목층)
// ForestGenerator → TerrainAdapter → MeshSpawner (아교목층)
// ForestGenerator → TerrainAdapter → MeshSpawner (관목층)

// 4. 각 층위별로 생성
for (int32 i = 0; i < LayerParams.Num(); i++)
{
    UPCGForestGeneratorSettings* ForestGen = CreateForestGenerator(LayerParams[i]);

    // 지형 어댑터 추가
    UPCGTerrainAdapterSettings* TerrainAdapter = CreateTerrainAdapter();
    TerrainAdapter->TerrainFilter.MinSlope = 0.0f;
    TerrainAdapter->TerrainFilter.MaxSlope = (i == 0) ? 25.0f : 35.0f; // 교목층은 덜 가파른 곳에만
    TerrainAdapter->TerrainFilter.bAdjustDensityBySlope = true;
    TerrainAdapter->TerrainFilter.bAlignToTerrain = true;

    // 메시 스포너 (TreeMeshLibrary 사용)
    UPCGStaticMeshSpawnerSettings* MeshSpawner = CreateMeshSpawner();
    MeshSpawner->MeshLibrary = LoadTreeMeshLibrary();

    // 그래프 연결 및 실행
    ConnectAndExecute(ForestGen, TerrainAdapter, MeshSpawner);
}
```

### 결과

- **교목층**: 큰 소나무와 참나무가 0-25도 경사의 평지에 밀집
- **아교목층**: 중간 크기 단풍나무와 자작나무가 0-35도 경사에 중간 밀도
- **관목층**: 작은 나무들이 넓은 범위에 산재
- **지형 적응**: 모든 나무가 경사면을 따라 자연스럽게 정렬
- **메시 다양성**: 각 나무 종마다 여러 메시 변종 사용
- **기후 적합성**: 12°C, 900mm 기후에 맞는 혼합림 생성

---

## 성능 최적화

### 1. 레벨 스트리밍

큰 숲을 여러 서브레벨로 분할:

```cpp
// 100x100m 타일로 분할
for (int32 X = 0; X < 10; X++)
{
    for (int32 Y = 0; Y < 10; Y++)
    {
        FVector TileLocation = FVector(X * 10000, Y * 10000, 0);
        CreateForestTile(TileLocation, BiomeSettings);
    }
}
```

### 2. 비동기 생성

```cpp
// 백그라운드 스레드에서 PCG 실행
AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [this, Params]()
{
    TArray<FPCGPoint> Points = GeneratePoints(Params);

    // 메인 스레드에서 메시 스폰
    AsyncTask(ENamedThreads::GameThread, [this, Points]()
    {
        SpawnMeshes(Points);
    });
});
```

### 3. LOD 자동 전환

```cpp
// 거리에 따라 메시 품질 조정
HISM->SetStaticMesh(SelectMeshByDistance(Distance, MeshLibrary));
HISM->SetNumCustomDataFloats(1); // LOD 레벨 저장
```

---

## 참고 자료

- [PCG 공식 문서](https://docs.unrealengine.com/5.0/procedural-content-generation/)
- [Claude API 문서](https://docs.anthropic.com/claude/reference/)
- [생태학 기초](https://en.wikipedia.org/wiki/Biome)
- [Poisson Disk Sampling](https://www.cs.ubc.ca/~rbridson/docs/bridson-siggraph07-poissondisk.pdf)

---

## 다음 단계

1. **실시간 기후 시스템**: 계절 변화, 날씨 효과
2. **동물 서식지**: 바이옴에 따른 동물 스폰
3. **불 시뮬레이션**: 산불 확산 및 재생
4. **멀티플레이어 동기화**: 네트워크 최적화
5. **VR/AR 지원**: 몰입형 숲 체험
