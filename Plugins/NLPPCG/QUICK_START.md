# NLPPCG Quick Start Guide

## 🚀 빠른 시작

### 1. 플러그인 빌드

```bash
# 비주얼 스튜디오에서
솔루션 탐색기 → NLPPCG 프로젝트 우클릭 → Build
```

### 2. 언리얼 에디터에서 액터 배치

1. **Place Actors** 패널 열기 (Shift + 1)
2. **NLPPCG** 카테고리 찾기
3. **ForestPCGManager** 액터를 레벨에 드래그 & 드롭

### 3. 자동 초기화 확인

**Window → Developer Tools → Output Log**에서 다음 메시지 확인:

```
LogTemp: Warning: ForestPCGManager: Auto-created MCP Client (Debug Mode: ON)
LogTemp: Warning: ForestPCGManager: MCP Client bound to Forest Manager
LogTemp: Warning: === NLPPCG System Ready ===
LogTemp: Warning: You can now generate forests using natural language commands!
LogTemp: Warning: Example: '밀집된 소나무 숲'
```

### 4. 테스트

#### 방법 1: 커서 (MCP 서버)

```
"중간 크기의 소나무 숲 생성해줘"
```

#### 방법 2: 블루프린트

```
Event BeginPlay
  ↓
Get Actor of Class (ForestPCGManager)
  ↓
Generate Forest From NLP
  Command: "밀집된 소나무 숲"
```

## 📋 자동 초기화 기능

ForestPCGManager를 레벨에 배치하면:

- ✅ MCPClient 자동 생성
- ✅ 델리게이트 자동 바인딩
- ✅ 디버그 모드 활성화
- ✅ 기본 나무 메시 초기화 (큐브)

## 🎨 나무 메시 커스터마이징

1. **ForestPCGManager** 선택
2. **Details** 패널에서 **Forest → Tree Type Meshes** 확장
3. 각 나무 타입별로 메시 변경:
   - `pine` - 소나무
   - `oak` - 참나무
   - `birch` - 자작나무
   - `maple` - 단풍나무
   - `generic_tree` - 일반 나무

## ⚙️ 설정 옵션

### ForestPCGManager 속성

- `bAutoCreateMCPClient` - MCPClient 자동 생성 (기본: true)
- `bSavePCGGraphAsAsset` - PCG 그래프 자동 저장 (기본: true)
- `TreeMeshes` - 나무 타입별 메시 매핑
- `MCPClient` - 수동으로 연결할 MCPClient (선택사항)

## 🔍 문제 해결

### 숲이 생성되지 않는 경우

1. **로그 확인**: "=== NLPPCG System Ready ===" 메시지가 있는지 확인
2. **액터 확인**: World Outliner에서 `ForestPCGManager`와 `AutoMCPClient` 확인
3. **델리게이트 확인**: 로그에 "MCP Client bound to Forest Manager" 메시지 확인

### MCPClient가 자동 생성되지 않는 경우

1. `bAutoCreateMCPClient` 속성이 true인지 확인
2. 레벨에 ForestPCGManager가 배치되어 있는지 확인
3. 에디터 로그에서 에러 메시지 확인

### 메시가 보이지 않는 경우

기본적으로 엔진 기본 큐브 메시가 사용됩니다. 실제 나무 메시로 변경하려면:

1. ForestPCGManager 선택
2. Details → Forest → Tree Type Meshes
3. 원하는 나무 타입의 메시 변경

## 📝 지원되는 명령어

```
"밀집된 소나무 숲"
"성긴 참나무 숲 500평방미터"
"큰 자작나무들로 빽빽한 숲"
"작은 단풍나무 숲"
"보통 밀도의 나무 숲 1000평방미터"
```

## 🎯 고급 기능

- [자동 설정 가이드](AUTO_SETUP_GUIDE.md)
- [고급 기능 가이드](ADVANCED_FEATURES.md)
- [UE5.7 호환성](UE5.7_COMPATIBILITY.md)
