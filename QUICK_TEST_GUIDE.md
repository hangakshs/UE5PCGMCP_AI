# 🌲 빠른 테스트 가이드 (UE5.7 호환)

## 숲 생성 테스트 방법

### ✅ 방법 1: Python 테스트 스크립트 실행 (권장)

UE5 에디터 Output Log의 Cmd 입력창에 다음 명령 입력:

```
py "F:/Project/Portfolio_MCP_PCG/Content/Python/test_forest_generation.py"
```

> **주의:** 경로는 실제 프로젝트 경로로 변경하세요!

---

### ✅ 방법 2: Python 콘솔에서 직접 실행 (UE5.7 호환)

**1단계: Shift + F1** (Python 콘솔 열기)

**2단계: 아래 코드 복사하여 실행:**

```python
import unreal

# UE5.7 호환 방법 - EditorActorSubsystem 사용
editor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = editor_subsystem.get_all_level_actors()
managers = [actor for actor in all_actors if isinstance(actor, unreal.ForestPCGManager)]

if managers:
    unreal.log("✅ ForestPCGManager 찾음!")
    managers[0].generate_forest_from_nlp("밀집된 소나무 숲")
else:
    unreal.log_error("❌ ForestPCGManager를 레벨에 배치하세요!")
```

**또는 GameplayStatics 사용 (더 간단):**

```python
import unreal

world = unreal.EditorLevelLibrary.get_editor_world()
managers = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ForestPCGManager)

if managers and len(managers) > 0:
    unreal.log("✅ ForestPCGManager 찾음!")
    managers[0].generate_forest_from_nlp("밀집된 소나무 숲")
else:
    unreal.log_error("❌ ForestPCGManager를 레벨에 배치하세요!")
```

---

### ✅ 방법 3: 한 줄 명령 (가장 빠름)

Output Log의 Cmd 입력창에:

```
py import unreal; world = unreal.EditorLevelLibrary.get_editor_world(); mgrs = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ForestPCGManager); mgrs[0].generate_forest_from_nlp("밀집된 소나무 숲") if mgrs and len(mgrs) > 0 else unreal.log_error("No ForestPCGManager found!")
```

---

## ⚠️ 주의사항

### ❌ UE5.7에서 작동하지 않는 명령:

```python
# 이 명령은 UE5.6 이하에서만 작동합니다!
unreal.EditorLevelLibrary.get_all_actors_of_class(...)  # ← 에러 발생!
```

**에러 메시지:**
```
AttributeError: type object 'EditorLevelLibrary' has no attribute 'get_all_actors_of_class'
```

### ✅ UE5.7에서 사용할 올바른 방법:

```python
# 방법 1: EditorActorSubsystem (에디터 전용)
editor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = editor_subsystem.get_all_level_actors()
managers = [a for a in all_actors if isinstance(a, unreal.ForestPCGManager)]

# 방법 2: GameplayStatics (에디터 + 런타임)
world = unreal.EditorLevelLibrary.get_editor_world()
managers = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ForestPCGManager)
```

---

## 📊 정상 동작 확인

테스트 명령 실행 후 Output Log에서 다음 순서로 메시지를 확인하세요:

1. ✅ `🚀 Sending Command via File`
2. ✅ `✅ Command file created successfully!`
3. ✅ `📥 Response File Detected!`
4. ✅ `🌲 Forest Parameters Received!`
5. ✅ `🚀 Executing PCG->Generate()...`
6. ✅ `✅ PCG Forest Generation Complete!`

뷰포트에 나무(Cube)가 생성되면 성공!

---

## 🔧 문제 해결

### 문제: "No ForestPCGManager found!"

**해결:**
1. Place Modes > All Classes
2. "ForestPCGManager" 검색
3. 레벨에 드래그 앤 드롭

### 문제: 명령은 전송되는데 숲이 생성되지 않음

**상세 디버깅:**
- [DEBUGGING_GUIDE_숲생성문제해결.md](./DEBUGGING_GUIDE_숲생성문제해결.md) 참조

### 문제: File Watcher Service가 시작되지 않음

**확인:**
Output Log에서 다음 메시지 찾기:
```
LogPython: 🚀 File Watcher Service Started Automatically!
LogPython:    Process ID: [숫자]
```

**없으면:**
1. UE5 에디터 재시작
2. 또는 수동 시작: `MCPServer/scripts/StartFileWatcher.bat`

---

## 📝 추가 테스트 명령어

```python
# 시스템 상태 확인
import test_forest_generation
test_forest_generation.check_system_status()

# 여러 가지 숲 테스트
managers[0].generate_forest_from_nlp("밀집된 소나무 숲")
managers[0].generate_forest_from_nlp("성긴 참나무 숲")
managers[0].generate_forest_from_nlp("보통 밀도 자작나무 숲")
```

---

## 🎯 요약

**가장 빠른 테스트 방법:**

1. Output Log 열기 (Window > Developer Tools > Output Log)
2. Cmd 입력창에 다음 입력:
   ```
   py "F:/Project/Portfolio_MCP_PCG/Content/Python/test_forest_generation.py"
   ```
3. Output Log에서 `✅ PCG Forest Generation Complete!` 확인
4. 뷰포트에서 생성된 나무 확인

**더 자세한 정보:**
- 상세 디버깅: [DEBUGGING_GUIDE_숲생성문제해결.md](./DEBUGGING_GUIDE_숲생성문제해결.md)
- 시스템 구조: [ARCHITECTURE.md](./ARCHITECTURE.md)
