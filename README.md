# UE5 NLP PCG Forest Generator

자연어 채팅으로 언리얼 엔진 5에서 PCG(Procedural Content Generation) 기반 숲을 자동 생성하는 시스템입니다.

## 🌟 주요 기능

- **자연어 인터페이스**: 한글로 "밀집된 소나무 숲 만들어줘"와 같은 명령으로 숲 생성
- **MCP 서버 연동**: Python MCP 서버가 자연어를 PCG 파라미터로 변환
- **PCG 기반 생성**: Poisson Disk Sampling을 사용한 자연스러운 나무 배치
- **실시간 수정**: 밀도, 크기, 면적 등을 자연어로 동적 조정

## 📁 프로젝트 구조

```
UE5PCGMCP_AI/
│
├── UnrealProject/              # 🎮 언리얼 프로젝트 파일
│   ├── Config/                # 엔진 설정 파일
│   │   └── DefaultEngine.ini
│   ├── Content/               # 에셋 및 Python 스크립트
│   │   └── Python/           # 언리얼용 Python 스크립트
│   │       ├── init_unreal.py
│   │       ├── mcp_command_watcher.py
│   │       └── start_mcp_watcher.py
│   └── Plugins/               # 언리얼 플러그인
│       └── NLPPCG/           # NLP PCG 플러그인
│           ├── NLPPCG.uplugin
│           ├── Source/NLPPCG/
│           │   ├── Public/
│           │   │   ├── NLPPCGModule.h
│           │   │   ├── PCGForestGenerator.h
│           │   │   ├── MCPClient.h
│           │   │   └── ForestPCGManager.h
│           │   └── Private/
│           │       ├── NLPPCGModule.cpp
│           │       ├── PCGForestGenerator.cpp
│           │       ├── MCPClient.cpp
│           │       └── ForestPCGManager.cpp
│           └── Resources/
│
├── MCPServer/                  # 🐍 Python MCP 서버
│   ├── src/                   # 소스 코드
│   │   ├── server.py         # MCP 서버 메인
│   │   ├── nlp_handler.py    # 자연어 처리
│   │   ├── llm_handler.py    # LLM 통합
│   │   ├── ue5_connector.py  # UE5 연결
│   │   └── file_watcher_service.py  # 파일 감시 서비스
│   ├── scripts/               # 실행 스크립트
│   │   ├── start_file_watcher.bat
│   │   └── start_file_watcher.sh
│   ├── tests/                 # 테스트 파일
│   │   ├── test_nlp.py
│   │   └── test_file_communication.py
│   ├── docs/                  # MCP 서버 문서
│   │   ├── AUTO_START_GUIDE.md
│   │   ├── README_FILE_WATCHER.md
│   │   └── README.md
│   └── requirements.txt       # Python 의존성
│
├── docs/                       # 📚 프로젝트 문서
│   ├── ADVANCED_FEATURES.md   # 고급 기능
│   ├── ARCHITECTURE.md        # 아키텍처
│   ├── AUTO_SETUP_GUIDE.md    # 자동 설정
│   ├── MCP_INTEGRATION_GUIDE.md
│   ├── MCP_SETUP.md
│   ├── QUICK_START.md
│   ├── TROUBLESHOOTING.md
│   ├── USAGE.md
│   ├── UE5.7_COMPATIBILITY.md
│   ├── 빠른_문제해결.md
│   └── 빠른_시작_가이드.md
│
├── scripts/                    # ⚙️ 프로젝트 스크립트
│   ├── StartFileWatcher.bat
│   └── StartFileWatcher.sh
│
├── README.md                   # 메인 README
├── FIX_SUMMARY.md
└── .gitignore
```

## 🚀 시작하기

### ✨ 간편 시작 (NEW! - 자동 시작 모드)

**파일 감시 서비스가 UE5 에디터 시작 시 자동으로 실행됩니다!**

1. UE5 에디터 열기
2. Output Log에서 다음 메시지 확인:
   ```
   🚀 File Watcher Service Started Automatically!
   ✅ NLPPCG System Ready!
   ```
3. Cursor IDE에서 숲 생성 명령 입력
4. 완료! 별도의 서버 실행 불필요

> 📘 자세한 내용: [자동 시작 가이드](MCPServer/docs/AUTO_START_GUIDE.md)

### 1. Python MCP 서버 설정 (선택사항 - MCP 서버 모드)

**File Watcher Service (자동 모드)**를 사용하는 경우 이 단계를 건너뛸 수 있습니다.

```bash
cd MCPServer
pip install -r requirements.txt
python src/server.py
```

**MCP 클라이언트 연동**: Claude Desktop이나 Cursor에서 MCP 서버를 사용하려면 [MCP 설정 가이드](docs/MCP_SETUP.md)를 참조하세요.

### 2. 언리얼 엔진 설정

1. 언리얼 엔진 5.7 프로젝트 생성
2. `UnrealProject/Plugins/NLPPCG` 폴더를 프로젝트의 `Plugins` 디렉토리에 복사
3. 프로젝트를 빌드 (.uproject 우클릭 -> Generate Visual Studio project files)
4. 언리얼 에디터에서 플러그인 활성화 (Edit -> Plugins -> "NLP PCG Forest Generator" 검색)

**⚠️ UE5.7 사용자**: PCG Graph 에디터에서 Static Mesh Spawner 노드의 메시를 수동으로 설정해야 합니다. 자세한 내용은 [UE5.7 호환성 가이드](docs/UE5.7_COMPATIBILITY.md)를 참조하세요.

**✨ NEW! 자동 초기화**: ForestPCGManager를 레벨에 배치하면 자동으로 MCPClient가 생성되고 연결됩니다! [빠른 시작 가이드](UnrealProject/Plugins/NLPPCG/QUICK_START.md)를 참조하세요.

**✨ 자동 설정**: 나무 메시가 자동으로 등록되며 에디터에서 수정 가능합니다! [자동 설정 가이드](docs/AUTO_SETUP_GUIDE.md)를 참조하세요.

### 3. 레벨에서 사용하기

**⚡ 가장 쉬운 방법 (추천):**
1. **Place Actors** (Shift + 1) → **NLPPCG** → **ForestPCGManager**를 레벨에 드래그
2. 로그에서 `=== NLPPCG System Ready ===` 메시지 확인
3. 커서 또는 블루프린트에서 숲 생성 명령 실행
4. 완료! MCPClient가 자동으로 생성되고 연결됩니다!

#### 방법 1: 블루프린트 (자동 배치 - 권장)

ForestPCGManager가 없으면 자동으로 생성됩니다!

```
Event BeginPlay
  |
  v
Generate Forest From NLP (NLPPCG Library)
  Command: "밀집된 소나무 숲 만들어줘"
  Spawn Location: (0, 0, 0)
```

#### 방법 2: 수동 배치

1. **ForestPCGManager 액터 배치**
   - Place Actors → NLPPCG → ForestPCGManager
   - 레벨에 드래그 앤 드롭
   - 나무 메시 자동 등록됨 (기본값: Cube)

2. **자연어 명령 실행**
   - 블루프린트에서 `GenerateForestFromNLP` 노드 호출
   - Command 입력: "밀집된 소나무 숲 만들어줘"

#### 방법 3: C++ (자동 배치)

```cpp
#include "ForestPCGManagerLibrary.h"

// 자동으로 ForestPCGManager 생성 + 숲 생성
UForestPCGManagerLibrary::GenerateForestFromNLP(
    this,
    TEXT("성긴 참나무 숲을 500평방미터에 생성해줘"),
    FVector(0, 0, 0)
);
```

## 💬 지원되는 자연어 명령

### 기본 명령어

```
"밀집된 소나무 숲 만들어줘"
"성긴 참나무 숲을 500평방미터에 생성해줘"
"큰 나무들로 빽빽한 숲 만들어줘"
"작은 자작나무 숲 만들어줘"
"보통 밀도의 단풍나무 숲 1000평방미터"
```

### 키워드

**나무 종류:**
- 소나무, 참나무, 자작나무, 단풍나무, 떡갈나무, 나무

**밀도:**
- 빽빽, 밀집, 많은 (dense)
- 듬성, 성긴, 적은 (sparse)
- 보통, 일반 (medium)

**크기:**
- 큰, 거대한 (large/huge)
- 작은 (small/tiny)
- 보통, 일반 (medium)

**면적:**
- 숫자 + "평방미터", "제곱미터", "m2", "㎡"

## 🔧 기술 스택

### MCP Python 서버
- **MCP SDK**: Model Context Protocol 구현
- **자연어 처리**: 정규표현식 기반 한글 파싱
- **HTTP/JSON-RPC**: 언리얼과 통신

### 언리얼 플러그인
- **UE5 PCG**: Procedural Content Generation 프레임워크
- **Poisson Disk Sampling**: 자연스러운 나무 분포
- **HTTP 모듈**: MCP 서버 통신
- **Static Mesh Spawning**: 인스턴싱 기반 메시 배치

## 📊 PCG 알고리즘

### Poisson Disk Sampling
- 최소/최대 거리 기반 포인트 배치
- 그리드 최적화로 O(n) 성능
- 자연스러운 무작위 분포

### 파라미터 매핑
```
밀도 (Density):
  - dense: 150-300cm 간격, randomness 0.3
  - medium: 200-500cm 간격, randomness 0.5
  - sparse: 400-800cm 간격, randomness 0.7

크기 (Size):
  - tiny: 0.5x 스케일
  - small: 0.75x 스케일
  - medium: 1.0x 스케일
  - large: 1.5x 스케일
  - huge: 2.0x 스케일
```

## 🎮 사용 예시

### 시나리오 1: 게임 레벨 디자인
```cpp
// 스폰 지점 주변에 숲 생성
ForestManager->SetActorLocation(SpawnPoint);
ForestManager->GenerateForestFromNLP(TEXT("밀집된 소나무 숲 1000평방미터"));
```

### 시나리오 2: 런타임 환경 생성
```cpp
// 플레이어 진행도에 따라 동적으로 숲 생성
if (PlayerProgress > 50)
{
    ForestManager->GenerateForestFromNLP(TEXT("거대한 나무들로 어두운 숲"));
}
```

### 시나리오 3: 프로토타이핑
```
블루프린트 이벤트:
  Event BeginPlay
    -> GenerateForestFromNLP: "보통 참나무 숲"
    -> Delay 5초
    -> GenerateForestFromNLP: "더 빽빽하게"
```

## 🛠️ 커스터마이징

### 1. 나무 메시 변경

```cpp
// ForestPCGManager의 TreeMesh 프로퍼티 설정
ForestManager->TreeMesh = MyCustomTreeMesh;
```

### 2. 새로운 나무 종류 추가

`MCPServer/src/nlp_handler.py`:
```python
self.tree_types = {
    '소나무': 'pine',
    '참나무': 'oak',
    '벚나무': 'cherry',  # 추가
    # ...
}
```

### 3. PCG 노드 확장

```cpp
// PCGForestGenerator.h/.cpp 수정
// 새로운 분포 알고리즘 추가
```

## 🐛 트러블슈팅

### MCP 서버 연결 실패
- 현재 **시뮬레이션 모드**로 동작 (MCPClient.cpp 참조)
- 실제 서버 사용 시 `MCPClient.cpp`의 주석 해제

### PCG 생성 안 됨
- PCG 컴포넌트가 활성화되어 있는지 확인
- TreeMesh가 올바르게 설정되었는지 확인
- 로그 확인: `Output Log` 창에서 "NLPPCG" 검색

### 메시가 보이지 않음
- 기본 큐브 메시 사용 중
- `ForestPCGManager`의 TreeMesh 프로퍼티에 원하는 메시 할당

## 📝 코드 파일 참조

### 핵심 클래스

- **MCPServer/src/nlp_handler.py**: 자연어 → PCG 파라미터 변환
- **MCPServer/src/server.py**: MCP 서버 구현
- **PCGForestGenerator.h/cpp**: PCG 포인트 생성 (UnrealProject/Plugins/NLPPCG/Source/NLPPCG)
- **MCPClient.h/cpp**: MCP 통신 클라이언트 (UnrealProject/Plugins/NLPPCG/Source/NLPPCG)
- **ForestPCGManager.h/cpp**: 숲 관리 액터 (UnrealProject/Plugins/NLPPCG/Source/NLPPCG)

## 🔮 고급 기능 (구현 완료)

✅ **구현된 기능** ([상세 가이드](docs/ADVANCED_FEATURES.md))

- [x] **LLM 통합 (Claude API)** - 감성적 표현과 복잡한 자연어 처리
  - "아름다운 가을 숲", "신비로운 자작나무 숲" 등 고급 명령 지원
  - 계절, 분위기 기반 자동 파라미터 생성
  - [llm_handler.py](MCPServer/src/llm_handler.py)

- [x] **나무 메시 자동 선택** - TreeMeshLibrary 시스템
  - 나무 종류별 메시 자동 매핑
  - 계절별 메시 변종 지원
  - 가중치 기반 랜덤 선택
  - [TreeMeshLibrary.h/cpp](UnrealProject/Plugins/NLPPCG/Source/NLPPCG/Public/TreeMeshLibrary.h)

- [x] **지형 기반 배치** - 경사, 고도 분석
  - 경사 필터링 (0-45도 범위 설정 가능)
  - 경사에 따른 자동 밀도/스케일 조정
  - 지형 법선에 나무 정렬
  - [PCGTerrainAdapter.h/cpp](UnrealProject/Plugins/NLPPCG/Source/NLPPCG/Public/PCGTerrainAdapter.h)

- [x] **바이옴 시스템** - 실제 생태계 모방
  - 침엽수림, 활엽수림, 혼합림, 타이가 등 프리셋
  - 층위 구조 (교목층, 아교목층, 관목층)
  - 기후 조건 기반 바이옴 자동 선택
  - [BiomeSystem.h/cpp](UnrealProject/Plugins/NLPPCG/Source/NLPPCG/Public/BiomeSystem.h)

📋 **향후 개선 사항**

- [ ] 시즌/날씨에 따른 동적 변화
- [ ] 멀티플레이어 동기화
- [ ] 동물 서식지 통합
- [ ] 불 시뮬레이션 및 재생 시스템

## 📄 라이선스

MIT License

## 👥 기여

이슈와 PR을 환영합니다!

## 📧 문의

프로젝트 관련 문의사항은 이슈를 통해 남겨주세요.
