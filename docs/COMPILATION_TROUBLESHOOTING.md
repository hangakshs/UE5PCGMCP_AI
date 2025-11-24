# 컴파일 문제 해결 가이드

## UE5.7 PCG 관련 컴파일 오류

### 증상

다음과 같은 컴파일 오류가 발생하는 경우:

```
'FPCGStaticMeshSpawnerEntry': 선언되지 않은 식별자입니다.
'MeshEntry': 선언되지 않은 식별자입니다.
'bUseAttribute': 'TObjectPtr<UPCGMeshSelectorBase>'의 멤버가 아닙니다.
'StaticMeshEntries': 'TObjectPtr<UPCGMeshSelectorBase>'의 멤버가 아닙니다.
```

### 원인

1. **오래된 빌드 캐시**: Intermediate, Binaries, Saved 폴더에 캐시된 이전 빌드 파일
2. **생성된 코드 캐시**: UHT(Unreal Header Tool)가 생성한 .generated.h 파일이 오래됨
3. **IDE 캐시**: Visual Studio 또는 Rider의 IntelliSense 캐시가 오래됨

### 해결 방법

#### 1단계: 빌드 캐시 정리

프로젝트 루트 디렉토리에서 다음 폴더들을 삭제:

```bash
# Linux/Mac
rm -rf UnrealProject/Intermediate
rm -rf UnrealProject/Binaries
rm -rf UnrealProject/Saved
rm -rf UnrealProject/.vs
rm -rf UnrealProject/Plugins/NLPPCG/Intermediate
rm -rf UnrealProject/Plugins/NLPPCG/Binaries
```

```powershell
# Windows PowerShell
Remove-Item -Recurse -Force UnrealProject/Intermediate
Remove-Item -Recurse -Force UnrealProject/Binaries
Remove-Item -Recurse -Force UnrealProject/Saved
Remove-Item -Recurse -Force UnrealProject/.vs
Remove-Item -Recurse -Force UnrealProject/Plugins/NLPPCG/Intermediate
Remove-Item -Recurse -Force UnrealProject/Plugins/NLPPCG/Binaries
```

#### 2단계: 프로젝트 파일 재생성

**.uproject 파일이 있는 경우:**

```bash
# Windows
Right-click on .uproject -> "Generate Visual Studio project files"

# Linux
/path/to/UE5/Engine/Build/BatchFiles/Linux/GenerateProjectFiles.sh -project="/path/to/YourProject.uproject" -game
```

**플러그인만 개발하는 경우:**

프로젝트 파일 재생성은 선택사항입니다.

#### 3단계: 전체 리빌드

**Unreal Editor에서:**
1. Editor 열기
2. **편집(Edit) > 프로젝트 설정(Project Settings)**
3. **플러그인(Plugins)** 탭에서 NLPPCG 플러그인 비활성화
4. Editor 재시작
5. 플러그인 다시 활성화
6. Editor 재시작 (자동 컴파일됨)

**Visual Studio에서:**
1. Solution 열기
2. **빌드(Build) > 솔루션 정리(Clean Solution)**
3. **빌드(Build) > 솔루션 다시 빌드(Rebuild Solution)**

**명령줄에서 (Linux):**

```bash
cd /path/to/UnrealEngine
./Engine/Build/BatchFiles/Linux/Build.sh NLPPCGEditor Linux Development -Project="/path/to/YourProject.uproject"
```

#### 4단계: IDE 캐시 정리

**Visual Studio:**
- **도구(Tools) > 옵션(Options) > 텍스트 편집기(Text Editor) > C/C++ > 고급(Advanced)**
- **IntelliSense 데이터베이스 다시 검색(Rescan Solution)** 클릭

**JetBrains Rider:**
- **파일(File) > 캐시 무효화/재시작(Invalidate Caches/Restart)**

## UE5.7 PCG API 변경사항

### 제거된 타입 및 멤버

현재 NLPPCG 플러그인은 다음 UE5.7 변경사항을 이미 반영했습니다:

1. ✅ **FPCGStaticMeshSpawnerEntry 제거**
   - **해결**: 직접 사용하지 않음, PCG Graph Editor에서 설정

2. ✅ **UPCGGraph::RemoveAllNodes() 제거**
   - **해결**: `GetNodes()` + `RemoveNodes()` 사용 (ForestPCGManager.cpp:160-165)

3. ✅ **MeshSelectorParameters Read-Only**
   - **해결**: 프로그래밍 방식 설정 시도하지 않음, 에디터 설정 권장

### 올바른 UE5.7 코드 예제

```cpp
// Static Mesh Spawner 생성 (UE5.7 호환)
UPCGStaticMeshSpawnerSettings* SpawnerSettings =
    NewObject<UPCGStaticMeshSpawnerSettings>(PCGGraph);

// 기본 속성만 설정 (MeshSelector는 에디터에서 설정)
SpawnerSettings->OutAttributeName = FName("TreeInstance");
SpawnerSettings->bApplyMeshBoundsToPoints = true;

// 메시 설정은 PCG Graph Editor에서 수동으로:
// 1. Static Mesh Spawner 노드 선택
// 2. Mesh Selector Type 설정
// 3. Mesh Entries 추가
```

## 추가 문제 해결

### Q: 여전히 컴파일 오류가 발생합니다

**A**: 다음을 시도해보세요:

1. **엔진 버전 확인**
   ```bash
   # UnrealEditor --version
   ```
   UE5.7 이상인지 확인

2. **플러그인 종속성 확인**
   `NLPPCG.Build.cs`에 PCG 모듈이 포함되어 있는지 확인:
   ```csharp
   PublicDependencyModuleNames.AddRange(new string[] {
       "Core",
       "CoreUObject",
       "Engine",
       "PCG",  // 이것이 있어야 함
       // ...
   });
   ```

3. **전체 엔진 재빌드** (최후의 수단)
   ```bash
   cd /path/to/UnrealEngine
   ./Setup.sh
   ./GenerateProjectFiles.sh
   make
   ```

### Q: 에디터에서 플러그인이 로드되지 않습니다

**A**: Output Log를 확인하세요:
- **Windows**: `%LOCALAPPDATA%/UnrealEngine/[Version]/Saved/Logs/`
- **Linux**: `~/.config/Epic/UnrealEngine/[Version]/Saved/Logs/`

일반적인 오류:
- **모듈을 찾을 수 없음**: Binaries 폴더 재생성 필요
- **심볼 해결 실패**: 전체 리빌드 필요

### Q: PCG 그래프가 생성되지 않습니다

**A**: ForestPCGManager 설정 확인:

1. **PCG Component 활성화**
   ```cpp
   PCGComponent->bActivated = true;
   ```

2. **Generation Trigger 설정**
   ```cpp
   PCGComponent->GenerationTrigger = EPCGComponentGenerationTrigger::GenerateOnDemand;
   ```

3. **메시 설정**
   - PCG Graph Editor 열기
   - Static Mesh Spawner 노드 설정
   - 최소 1개 이상의 메시 추가

## 도움말 리소스

- [UE5.7_COMPATIBILITY.md](UE5.7_COMPATIBILITY.md) - UE5.7 호환성 상세 가이드
- [USAGE.md](USAGE.md) - 기본 사용법
- [ADVANCED_FEATURES.md](ADVANCED_FEATURES.md) - 고급 기능

## 버그 리포트

위 단계를 모두 시도했지만 문제가 해결되지 않는 경우:

1. **Output Log 수집**
2. **컴파일 오류 전체 메시지 복사**
3. **환경 정보 기록**:
   - OS 및 버전
   - Unreal Engine 버전 (정확한 빌드 번호)
   - 컴파일러 버전
4. **GitHub Issues에 리포트**
