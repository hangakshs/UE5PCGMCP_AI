# Python MCP 서버 - 상세 문서

## 목차
- [개요](#개요)
- [모듈 구조](#모듈-구조)
- [server.py - MCP 서버 메인](#serverpy---mcp-서버-메인)
- [nlp_handler.py - 자연어 파싱](#nlp_handlerpy---자연어-파싱)
- [ue5_connector.py - UE5 통신](#ue5_connectorpy---ue5-통신)
- [file_watcher_service.py - 파일 감시](#file_watcher_servicepy---파일-감시)
- [통신 프로토콜](#통신-프로토콜)
- [설정 및 배포](#설정-및-배포)

---

## 개요

Python MCP 서버는 Claude/Cursor IDE와 Unreal Engine 5 사이의 브리지 역할을 수행합니다.

### 핵심 기능
1. **MCP 프로토콜 구현**: Claude/Cursor와 표준 MCP로 통신
2. **자연어 파싱**: 한국어 명령을 PCG 파라미터로 변환
3. **파일 기반 통신**: UE5와 JSON 파일로 데이터 교환
4. **독립 서비스**: File Watcher로 UE5 에디터 외부에서도 동작

### 기술 스택
- **Python 3.11+**
- **MCP SDK**: Model Context Protocol 구현
- **asyncio**: 비동기 I/O
- **JSON**: 데이터 직렬화

---

## 모듈 구조

```
MCPServer/
├── src/
│   ├── server.py                 # MCP 서버 메인 (asyncio 기반)
│   ├── nlp_handler.py            # 자연어 파싱 (키워드 기반)
│   ├── ue5_connector.py          # UE5 통신 (파일/직접)
│   ├── file_watcher_service.py   # 파일 감시 서비스 (독립 프로세스)
│   └── llm_handler.py            # LLM 기반 파싱 (옵션)
├── tests/
│   └── test_nlp_handler.py
└── pyproject.toml                # Poetry 의존성
```

---

## server.py - MCP 서버 메인

### 클래스: `PCGForestMCPServer`

**역할**: MCP 프로토콜 서버 구현 및 도구(Tool) 제공

**주요 속성**:
```python
class PCGForestMCPServer:
    def __init__(self):
        self.app = Server("pcg-forest-server")  # MCP 서버 인스턴스
        self.nlp_handler = ForestNLPHandler()   # 자연어 파싱 핸들러
        self.ue5_connector = UE5Connector()     # UE5 통신 커넥터
        self.last_parameters: Dict[str, Any] | None = None  # 마지막 파라미터
        self.state_file = Path("last_parameters.json")  # 상태 저장 파일
```

### MCP 도구 (Tools)

#### 1. `create_forest`

**설명**: 자연어 명령으로 PCG 숲 생성

**입력 스키마**:
```json
{
  "type": "object",
  "properties": {
    "command": {
      "type": "string",
      "description": "숲 생성에 대한 자연어 명령"
    }
  },
  "required": ["command"]
}
```

**예시 명령**:
- "밀집된 소나무 숲 만들어줘"
- "성긴 참나무 숲 500평방미터"
- "다양한 나무로 큰 숲 만들어줘"

**처리 흐름**:
```python
async def handle_create_forest(self, arguments: dict) -> Sequence[TextContent]:
    command = arguments.get("command", "")

    # 1. NLP로 파라미터 파싱
    params = self.nlp_handler.parse_forest_command(command)
    overrides = params.pop('_overrides', [])

    # 2. 사용자 응답 생성
    response_text = self.nlp_handler.generate_response(params)

    # 3. UE5에서 실행
    execution_result = self.ue5_connector.execute_forest_command(params)

    # 4. 성공 시 파라미터 저장
    if execution_result.get("success"):
        self.last_parameters = deepcopy(params)
        self._persist_last_parameters()

    # 5. 결과 텍스트 반환
    result_text = f"[명령] {command}\n\n"
    result_text += f"[파싱 결과]\n{response_text}\n\n"
    result_text += f"[UE5 실행 결과] {'성공' if execution_result.get('success') else '실패'}\n"
    result_text += f"\n--- 생성된 파라미터 ---\n{json.dumps(params, indent=2, ensure_ascii=False)}"

    return [TextContent(type="text", text=result_text)]
```

**출력 예시**:
```
[명령] 밀집된 소나무 숲 만들어줘

[파싱 결과]
나무 종류: 소나무 (pine)
밀도: 밀집 (dense, 밀도 배율 2.0x)
영역 크기: 100.0 m² (1000000.0 cm²)

[UE5 실행 결과] 성공
   [성공] 명령이 UE5로 전송되었습니다.

--- 생성된 파라미터 ---
{
  "action": "create_forest",
  "tree_type": "pine",
  "tree_types": [],
  "density": "dense",
  "density_multiplier": 2.0,
  "area_size": 1000000.0,
  "min_distance": 200.0,
  "max_distance": 500.0,
  "randomness": 0.3,
  "scale_multiplier": 1.0
}
```

#### 2. `clear_forest`

**설명**: 현재 생성된 PCG 숲 제거

**입력 스키마**: 없음 (빈 객체)

**처리 흐름**:
```python
async def handle_clear_forest(self) -> Sequence[TextContent]:
    # UE5에서 숲 제거 실행
    execution_result = self.ue5_connector.clear_forest()

    result_text = "[명령] 모든 PCG 숲을 제거합니다.\n\n"
    result_text += f"[UE5 실행 결과] {'성공' if execution_result.get('success') else '실패'}\n"

    return [TextContent(type="text", text=result_text)]
```

#### 3. `modify_forest` (v1.2.0)

**설명**: 기존 숲의 밀도나 크기 수정

**입력 스키마**:
```json
{
  "type": "object",
  "properties": {
    "command": {
      "type": "string",
      "description": "수정 명령 (예: '더 빽빽하게', '나무를 크게')"
    }
  },
  "required": ["command"]
}
```

**예시 명령**:
- "밀도를 2로 변경"
- "더 빽빽하게"
- "나무를 크게"

**처리 흐름**:
```python
async def handle_modify_forest(self, arguments: dict) -> Sequence[TextContent]:
    command = arguments.get("command", "")

    # 1. 파싱 (create_forest와 동일한 로직 재사용)
    params = self.nlp_handler.parse_forest_command(command)
    overrides = set(params.pop('_overrides', []))
    params['action'] = 'modify_forest'

    # 2. 이전 파라미터 존재 확인
    if self.last_parameters is None:
        return [TextContent(type="text", text="오류: 수정할 기존 숲 정보가 없습니다.")]

    # 3. 변경된 속성 확인
    if not overrides:
        return [TextContent(type="text", text="오류: 수정할 속성이 감지되지 않았습니다.")]

    # 4. 파라미터 병합 (변경된 것만 덮어쓰기)
    merged_params = deepcopy(self.last_parameters)
    merged_params['action'] = 'modify_forest'

    for key in overrides:
        if key in params:
            merged_params[key] = params[key]

    params = merged_params

    # 5. UE5로 전송
    ue_command = {
        "action": "modify_forest",
        "parameters": params
    }
    execution_result = self.ue5_connector.file_comm.send_command_raw(ue_command)

    # 6. 성공 시 파라미터 저장
    if execution_result.get("success"):
        self.last_parameters = deepcopy(params)
        self._persist_last_parameters()

    return [TextContent(type="text", text=result_text)]
```

**파라미터 병합 예시**:
```python
# 이전 파라미터 (last_parameters)
{
  "tree_type": "pine",
  "density_multiplier": 1.0,
  "area_size": 5000000.0,
  "scale_multiplier": 1.0
}

# 명령: "밀도를 2로 변경"
# 파싱 결과 (overrides = {"density_multiplier"})
{
  "density_multiplier": 2.0
}

# 병합 결과
{
  "tree_type": "pine",           # 유지
  "density_multiplier": 2.0,     # 변경
  "area_size": 5000000.0,        # 유지
  "scale_multiplier": 1.0,       # 유지
  "action": "modify_forest"
}
```

### 상태 관리

**파라미터 지속성** (v1.2.0):
```python
def _persist_last_parameters(self):
    """마지막 파라미터를 파일에 저장"""
    try:
        self.state_file.parent.mkdir(parents=True, exist_ok=True)
        with open(self.state_file, 'w', encoding='utf-8') as f:
            json.dump(self.last_parameters or {}, f, indent=2, ensure_ascii=False)
    except Exception as e:
        print(f"[WARN] Failed to persist last parameters: {e}")

def _load_last_parameters(self):
    """저장된 파라미터 로드 (초기화 시)"""
    try:
        if self.state_file.exists():
            with open(self.state_file, 'r', encoding='utf-8') as f:
                data = json.load(f)
                if isinstance(data, dict) and data:
                    self.last_parameters = data
    except Exception as e:
        print(f"[WARN] Failed to load last parameters: {e}")
```

### 서버 실행

```python
async def run(self):
    """MCP 서버 실행 (stdio 통신)"""
    async with stdio_server() as (read_stream, write_stream):
        await self.app.run(
            read_stream,
            write_stream,
            self.app.create_initialization_options()
        )

async def main():
    """메인 진입점"""
    server = PCGForestMCPServer()
    await server.run()

if __name__ == "__main__":
    asyncio.run(main())
```

---

## nlp_handler.py - 자연어 파싱

### 클래스: `ForestNLPHandler`

**역할**: 한국어 명령을 PCG 파라미터로 변환 (키워드 기반)

**주요 속성**:
```python
class ForestNLPHandler:
    def __init__(self):
        # 나무 타입 매핑 (한국어 → 영어)
        self.tree_types = {
            '단풍나무': 'maple',
            '소나무': 'pine',
            '참나무': 'oak',
            '자작나무': 'birch',
            '떡갈나무': 'oak',
            '나무': 'generic_tree'
        }

        # 밀도 키워드
        self.density_keywords = {
            '빽빽': 'dense',
            '밀집': 'dense',
            '많은': 'dense',
            '듬성': 'sparse',
            '성긴': 'sparse',
            '적은': 'sparse',
            '보통': 'medium',
            '일반': 'medium'
        }

        # 크기 키워드
        self.size_keywords = {
            '거대한': 'huge',
            '큰': 'large',
            '작은': 'small',
            '보통': 'medium',
            '일반': 'medium'
        }
```

### 메서드: `parse_forest_command()`

**기능**: 자연어 명령 → PCG 파라미터 딕셔너리

**알고리즘**:
```python
def parse_forest_command(self, text: str) -> Dict[str, Any]:
    # 1. 기본 파라미터 초기화
    params = {
        'action': 'create_forest',
        'tree_type': 'generic_tree',
        'tree_types': [],
        'density': 'medium',
        'size': 'medium',
        'area_size': 1000000.0,  # 100㎡ (1000000 cm²)
        'randomness': 0.5,
        'min_distance': 200.0,  # 2m
        'max_distance': 500.0,  # 5m
        'density_multiplier': 1.0,
        'scale_multiplier': 1.0
    }
    explicit_overrides = set()  # 사용자가 명시적으로 지정한 파라미터 추적

    # 2. 혼합 숲 감지 (v1.2.0)
    mixed_keywords = ['다양한', '다양하게', '다양', '섞인', '여러 종류',
                      '여러가지', '혼합', '다종', '믹스']
    is_mixed_forest = any(keyword in text for keyword in mixed_keywords)

    if is_mixed_forest:
        params['tree_types'] = ['pine', 'oak', 'birch', 'maple']
        params['tree_type'] = 'mixed'
        explicit_overrides.update({'tree_type', 'tree_types'})
    else:
        # 3. 단일 나무 타입 파싱
        for korean, english in self.tree_types.items():
            if korean in text:
                params['tree_type'] = english
                explicit_overrides.add('tree_type')
                break

    # 4. 밀도 키워드 파싱
    for keyword, density in self.density_keywords.items():
        if keyword in text:
            params['density'] = density
            explicit_overrides.add('density')
            break

    # 5. 크기 키워드 파싱
    for keyword, size in self.size_keywords.items():
        if keyword in text:
            params['size'] = size
            explicit_overrides.add('size')
            break

    # 6. 명시적 스케일 배율 파싱 (v1.2.0)
    # 패턴: "스케일 0.5배", "크기를 2배", "사이즈 0.8"
    scale_match = re.search(
        r'(?:스케일|scale|크기|사이즈|나무\s*크기|배율)\s*(?:만|만큼|정도)?\s*(?:를|을|으로|로|만|은|는|이|가)?\s*(\d+(?:\.\d+)?)\s*배',
        text.lower()
    )
    if not scale_match:
        scale_match = re.search(
            r'(?:스케일|scale|크기|사이즈|나무\s*크기|배율)\s*[:=]?\s*(\d+(?:\.\d+)?)',
            text.lower()
        )
    if scale_match:
        explicit_scale = float(scale_match.group(1))
        params['scale_multiplier'] = explicit_scale
        explicit_overrides.add('scale_multiplier')

    # 7. 명시적 밀도 배율 파싱 (v1.2.0)
    # 패턴: "밀도 2", "밀도를 2로", "밀도=2", "density 2.0"
    density_mult_match = re.search(
        r'(?:밀도|density)\s*(?:만|만큼)?\s*[를을]?\s*(\d+(?:\.\d+)?)\s*(?:배)?\s*(?:로|으로|만큼|정도로|가량|정도)?',
        text.lower()
    )
    if not density_mult_match:
        density_mult_match = re.search(
            r'(?:밀도|density)\s*(?:만|만큼)?\s*(\d+(?:\.\d+)?)\s*배',
            text.lower()
        )
    if density_mult_match:
        explicit_density = float(density_mult_match.group(1))
        params['density_multiplier'] = explicit_density
        explicit_overrides.add('density_multiplier')

    # 8. 면적 파싱
    area_match = re.search(r'(\d+(?:\.\d+)?)\s*(?:평방미터|제곱미터|m2|㎡)', text)
    if area_match:
        area = float(area_match.group(1))
        params['area_size'] = area * 10000.0  # m² → cm²
        explicit_overrides.add('area_size')

    # 9. 밀도에 따른 자동 조정 (명시적 지정이 없는 경우만)
    if 'density_multiplier' not in explicit_overrides:
        if params['density'] == 'dense':
            params['randomness'] = 0.3
            params['density_multiplier'] = 2.0  # 2배 밀도
        elif params['density'] == 'sparse':
            params['randomness'] = 0.7
            params['density_multiplier'] = 0.5  # 0.5배 밀도
        else:  # medium
            params['density_multiplier'] = 1.0

    # 10. 크기에 따른 스케일 조정 (명시적 지정이 없는 경우만)
    if 'scale_multiplier' not in explicit_overrides:
        scale_multipliers = {
            'tiny': 0.5,
            'small': 0.75,
            'medium': 1.0,
            'large': 1.25,
            'huge': 1.5
        }
        params['scale_multiplier'] = scale_multipliers.get(params['size'], 1.0)

    # 11. _overrides 필드 추가 (modify 명령에 사용)
    params['_overrides'] = list(explicit_overrides)

    return params
```

**파싱 예시**:

| 입력 명령 | 파싱 결과 |
|----------|----------|
| "밀집된 소나무 숲" | `tree_type: "pine", density: "dense", density_multiplier: 2.0` |
| "성긴 참나무 숲 500㎡" | `tree_type: "oak", density: "sparse", density_multiplier: 0.5, area_size: 5000000.0` |
| "다양한 나무로 큰 숲" | `tree_types: ["pine", "oak", "birch", "maple"], size: "large", scale_multiplier: 1.25` |
| "밀도를 2로 변경" | `density_multiplier: 2.0, _overrides: ["density_multiplier"]` |
| "스케일 0.5배" | `scale_multiplier: 0.5, _overrides: ["scale_multiplier"]` |

### 메서드: `generate_response()`

**기능**: 파라미터를 사용자 친화적인 텍스트로 변환

```python
def generate_response(self, params: Dict[str, Any]) -> str:
    """파라미터를 자연어 응답으로 변환"""
    tree_name_map = {
        'pine': '소나무',
        'oak': '참나무',
        'birch': '자작나무',
        'maple': '단풍나무',
        'mixed': '다양한 나무',
        'generic_tree': '일반 나무'
    }

    density_name_map = {
        'dense': '밀집',
        'medium': '보통',
        'sparse': '성긴'
    }

    tree_name = tree_name_map.get(params.get('tree_type', 'generic_tree'), '일반 나무')
    density_name = density_name_map.get(params.get('density', 'medium'), '보통')
    area_m2 = params.get('area_size', 1000000.0) / 10000.0
    density_mult = params.get('density_multiplier', 1.0)
    scale_mult = params.get('scale_multiplier', 1.0)

    response = f"나무 종류: {tree_name} ({params.get('tree_type', 'generic_tree')})\n"
    response += f"밀도: {density_name} ({params.get('density', 'medium')}, 밀도 배율 {density_mult}x)\n"
    response += f"영역 크기: {area_m2:.1f} m² ({params.get('area_size', 1000000.0):.1f} cm²)\n"

    if scale_mult != 1.0:
        response += f"스케일: {scale_mult}x\n"

    if params.get('tree_types'):
        response += f"혼합 나무: {', '.join(params['tree_types'])}\n"

    return response
```

---

## ue5_connector.py - UE5 통신

### 클래스: `UE5Connector`

**역할**: MCP 서버와 UE5 간 통신 (파일 기반 / 직접 호출)

**주요 속성**:
```python
class UE5Connector:
    def __init__(self, project_root: Optional[Path] = None):
        self.is_ue5_available = self._check_ue5_environment()  # UE5 Python 환경 체크

        # 프로젝트 루트 자동 감지
        if project_root is None:
            current_file = Path(__file__).resolve()
            project_root = current_file.parent.parent.parent  # MCPServer/src → 프로젝트 루트

        self.file_comm = FileBasedCommunication(project_root)
```

### 메서드: `execute_forest_command()`

**기능**: 숲 생성 명령 실행 (환경에 따라 파일/직접)

```python
def execute_forest_command(self, params: Dict[str, Any]) -> Dict[str, Any]:
    # UE5 Python 환경이 아닌 경우 → 파일 기반 통신
    if not self.is_ue5_available:
        return self.file_comm.send_command(params)

    # UE5 Python 환경인 경우 → 직접 실행
    try:
        return self._execute_direct_in_ue5(params)
    except Exception as e:
        return {
            "success": False,
            "error": f"UE5 실행 중 오류: {str(e)}",
            "parameters": params
        }
```

### 메서드: `_execute_direct_in_ue5()` (UE5 Python 전용)

**기능**: UE5 Python 환경에서 직접 PCG 실행

```python
def _execute_direct_in_ue5(self, params: Dict[str, Any]) -> Dict[str, Any]:
    import unreal

    # ForestPCGManager 가져오기/생성
    forest_lib = unreal.ForestPCGManagerLibrary
    world = unreal.EditorLevelLibrary.get_editor_world()
    manager = forest_lib.get_or_create_forest_pcg_manager(
        world_context_object=world,
        spawn_location=unreal.Vector(0, 0, 0)
    )

    if manager is None:
        return {
            "success": False,
            "error": "ForestPCGManager를 찾을 수 없습니다."
        }

    # Dict → unreal.PCGForestParameters 변환
    forest_params = self._dict_to_pcg_parameters(unreal, params)

    # PCG 실행
    manager.generate_forest_from_parameters(forest_params)

    return {
        "success": True,
        "message": "UE5에서 PCG 파라미터를 직접 적용했습니다.",
        "parameters": params
    }
```

### 클래스: `FileBasedCommunication`

**역할**: 파일 기반 통신 (MCP 서버 ↔ UE5 에디터)

**주요 속성**:
```python
class FileBasedCommunication:
    def __init__(self, project_root: Path):
        self.project_root = project_root
        self.command_file = project_root / "Intermediate" / "MCP_Commands" / "pending_command.json"
        self.result_file = project_root / "Intermediate" / "MCP_Commands" / "result.json"

        # 디렉토리 생성
        self.command_file.parent.mkdir(parents=True, exist_ok=True)
```

### 메서드: `send_command_raw()`

**기능**: Raw 명령을 UE5로 전송 (mcp_response.json 저장)

```python
def send_command_raw(self, command_data: Dict[str, Any]) -> Dict[str, Any]:
    try:
        # UE5가 읽을 파일에 저장
        response_file = self.command_file.parent / "mcp_response.json"

        with open(response_file, 'w', encoding='utf-8') as f:
            json.dump(command_data, f, indent=2, ensure_ascii=False)

        return {
            "success": True,
            "message": "[성공] 명령이 UE5로 전송되었습니다.",
            "file": str(response_file),
            "action": command_data.get("action", "unknown")
        }
    except Exception as e:
        return {
            "success": False,
            "error": f"파일 저장 실패: {str(e)}"
        }
```

---

## file_watcher_service.py - 파일 감시

### 클래스: `UE5FileWatcherService`

**역할**: UE5와 독립적으로 실행되는 파일 감시 서비스

**주요 속성**:
```python
class UE5FileWatcherService:
    def __init__(self, project_root: Optional[Path] = None):
        self.project_root = Path(project_root).resolve()
        self.comm_dir = self.project_root / "Intermediate" / "MCP_Commands"
        self.ue5_command_file = self.comm_dir / "ue5_command.json"  # UE5 → 서비스
        self.mcp_response_file = self.comm_dir / "mcp_response.json"  # 서비스 → UE5
        self.last_parameters_file = self.comm_dir / "last_parameters.json"  # v1.2.0

        # NLP 핸들러 초기화
        from nlp_handler import ForestNLPHandler
        self.nlp_handler = ForestNLPHandler()

        # 중복 처리 방지
        self.last_processed_hash: Optional[str] = None
        self.is_running = False
```

### 메서드: `start()`

**기능**: 서비스 시작 (폴링 루프)

```python
def start(self, poll_interval: float = 0.5):
    """
    서비스 시작

    Args:
        poll_interval: 파일 확인 간격 (초)
    """
    self.is_running = True
    logger.info(f"🚀 File Watcher Service started (poll interval: {poll_interval}s)")

    try:
        while self.is_running:
            self._check_and_process_command()
            time.sleep(poll_interval)
    except KeyboardInterrupt:
        logger.info("\n⏹️  Service stopped by user")
```

### 메서드: `_check_and_process_command()` (v1.2.0 주요 개선)

**기능**: 명령 파일 확인 및 처리

```python
def _check_and_process_command(self):
    # 1. 파일 존재 확인
    if not self.ue5_command_file.exists():
        return

    # 2. 파일 읽기
    try:
        with open(self.ue5_command_file, 'r', encoding='utf-8') as f:
            command_data = json.load(f)
    except Exception as e:
        logger.error(f"❌ Failed to read command file: {e}")
        return

    # 3. 중복 처리 방지 (파일 해시 체크)
    content_hash = hashlib.md5(json.dumps(command_data, sort_keys=True).encode()).hexdigest()
    if content_hash == self.last_processed_hash:
        return  # 이미 처리한 명령

    # 4. 명령 처리
    command_text = command_data.get('command', '')
    logger.info(f"📥 Command received: {command_text}")

    # ⭐ v1.2.0: Modify 키워드 감지
    modify_keywords = ['변경', '수정', '조정', '바꿔', '바꾸', 'change', 'modify', 'update']
    is_modify = any(keyword in command_text for keyword in modify_keywords)

    # 5. NLP 파싱
    params = self.nlp_handler.parse_forest_command(command_text)
    overrides = set(params.pop('_overrides', []))

    # ⭐ v1.2.0: Modify 명령 처리
    if is_modify:
        last_params = self._load_last_parameters()

        if last_params is None:
            logger.warning("⚠️ No last parameters found, treating as create command")
            is_modify = False
        elif not overrides:
            response_data = self._create_error_response(
                "수정할 속성이 감지되지 않았습니다. (예: '밀도를 2로' 같이 변경할 값을 명시해 주세요.)"
            )
            self._write_response(response_data)
            self.last_processed_hash = content_hash
            self.ue5_command_file.unlink()
            return
        else:
            # 파라미터 병합 (변경된 것만 덮어쓰기)
            merged_params = last_params.copy()
            for key in overrides:
                if key in params:
                    merged_params[key] = params[key]
            params = merged_params
            logger.info(f"🔄 Merged parameters with last_parameters.json")

    # 6. 응답 파일 생성
    response_data = {
        "action": "modify_forest" if is_modify else "create_forest",
        "parameters": params,
        "timestamp": datetime.now().isoformat(),
        "original_command": command_text
    }

    # ⭐ v1.2.0: 파라미터 저장
    self._save_last_parameters(params)

    # 7. mcp_response.json 저장
    self._write_response(response_data)

    # 8. 원본 파일 삭제
    self.last_processed_hash = content_hash
    self.ue5_command_file.unlink()

    logger.info(f"✅ Command processed successfully")
```

### 메서드: `_load_last_parameters()` (v1.2.0)

**기능**: 마지막 파라미터 로드

```python
def _load_last_parameters(self) -> Optional[Dict[str, Any]]:
    """마지막 파라미터 로드 (modify 명령용)"""
    if not self.last_parameters_file.exists():
        return None

    try:
        with open(self.last_parameters_file, 'r', encoding='utf-8') as f:
            return json.load(f)
    except Exception as e:
        logger.error(f"Failed to load last parameters: {e}")
        return None
```

### 메서드: `_save_last_parameters()` (v1.2.0)

**기능**: 마지막 파라미터 저장

```python
def _save_last_parameters(self, params: Dict[str, Any]):
    """마지막 파라미터 저장 (modify 명령용)"""
    try:
        # action, _overrides 제외하고 저장
        params_to_save = params.copy()
        params_to_save.pop('action', None)
        params_to_save.pop('_overrides', None)

        with open(self.last_parameters_file, 'w', encoding='utf-8') as f:
            json.dump(params_to_save, f, indent=2, ensure_ascii=False)

        logger.info(f"💾 Saved last parameters to {self.last_parameters_file}")
    except Exception as e:
        logger.error(f"Failed to save last parameters: {e}")
```

---

## 통신 프로토콜

### 1. MCP 프로토콜 (Claude/Cursor ↔ MCP 서버)

**통신 방식**: stdio (표준 입출력)

**메시지 형식**: JSON-RPC 2.0

**도구 호출 예시**:
```json
// Request (Claude → MCP 서버)
{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "tools/call",
  "params": {
    "name": "create_forest",
    "arguments": {
      "command": "밀집된 소나무 숲 만들어줘"
    }
  }
}

// Response (MCP 서버 → Claude)
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": {
    "content": [
      {
        "type": "text",
        "text": "[명령] 밀집된 소나무 숲 만들어줘\n\n[파싱 결과]\n나무 종류: 소나무 (pine)\n..."
      }
    ]
  }
}
```

### 2. 파일 기반 프로토콜 (MCP 서버 ↔ UE5)

**통신 방식**: JSON 파일

**파일 위치**: `프로젝트/Intermediate/MCP_Commands/`

#### 파일 종류:

1. **mcp_response.json** (MCP 서버 → UE5)
```json
{
  "action": "create_forest",
  "parameters": {
    "tree_type": "pine",
    "tree_types": [],
    "density": "dense",
    "density_multiplier": 2.0,
    "area_size": 1000000.0,
    "min_distance": 200.0,
    "max_distance": 500.0,
    "randomness": 0.3,
    "scale_multiplier": 1.0
  },
  "timestamp": "2025-01-20T15:30:45"
}
```

2. **ue5_command.json** (UE5 → File Watcher)
```json
{
  "command": "밀도를 2로 변경",
  "timestamp": "2025-01-20T15:30:45"
}
```

3. **last_parameters.json** (v1.2.0, 파라미터 지속성)
```json
{
  "tree_type": "pine",
  "tree_types": [],
  "density": "dense",
  "density_multiplier": 2.0,
  "area_size": 5000000.0,
  "min_distance": 200.0,
  "max_distance": 500.0,
  "randomness": 0.3,
  "scale_multiplier": 1.0
}
```

---

## 설정 및 배포

### 1. 의존성 설치 (Poetry)

**pyproject.toml**:
```toml
[tool.poetry]
name = "pcg-forest-mcp-server"
version = "1.2.0"
description = "MCP Server for UE5 PCG Forest Generation"

[tool.poetry.dependencies]
python = "^3.11"
mcp = "^1.0.0"

[build-system]
requires = ["poetry-core"]
build-backend = "poetry.core.masonry.api"
```

**설치**:
```bash
cd MCPServer
poetry install
```

### 2. MCP 서버 실행

**직접 실행** (테스트용):
```bash
cd MCPServer/src
python server.py
```

**Cursor/Claude 설정**:
```json
// Cursor settings.json 또는 Claude Desktop config
{
  "mcpServers": {
    "pcg-forest": {
      "command": "python",
      "args": [
        "F:/Project/Portfolio_MCP_PCG/MCPServer/src/server.py"
      ],
      "cwd": "F:/Project/Portfolio_MCP_PCG"
    }
  }
}
```

### 3. File Watcher 실행

**독립 프로세스로 실행**:
```bash
cd MCPServer/src
python file_watcher_service.py --project-root "F:/Project/Portfolio_MCP_PCG"
```

**백그라운드 실행** (Windows):
```bash
start /B python file_watcher_service.py --project-root "F:/Project/Portfolio_MCP_PCG"
```

**로그 확인**:
```bash
# 로그 파일 위치
tail -f F:/Project/Portfolio_MCP_PCG/Intermediate/MCP_Commands/file_watcher.log
```

---

## 버전 히스토리

### v1.2.0 (2025-01-XX)
- ✅ `modify_forest` 도구 추가
- ✅ 파라미터 지속성 (`last_parameters.json`)
- ✅ File Watcher의 modify 키워드 감지 및 파라미터 병합
- ✅ 혼합 숲 키워드 확장 ("다양하게", "믹스")
- ✅ `_overrides` 필드로 명시적 파라미터 추적
- ✅ 명시적 밀도/스케일 배율 파싱 강화

### v1.1.0
- NLP 핸들러 키워드 확장
- File Watcher 서비스 안정성 개선
- UTF-8 인코딩 문제 해결

### v1.0.0
- 초기 릴리즈
- MCP 프로토콜 구현
- 키워드 기반 자연어 파싱
- 파일 기반 통신

---

## 트러블슈팅

### 1. MCP 서버가 시작되지 않음

**증상**: Cursor/Claude에서 "Failed to start MCP server" 오류

**해결**:
1. Python 경로 확인: `which python` / `where python`
2. 의존성 설치 확인: `poetry install`
3. 수동 실행 테스트: `python server.py`
4. 로그 확인

### 2. File Watcher가 명령을 감지하지 못함

**증상**: UE5에서 명령을 전송해도 응답 없음

**해결**:
1. File Watcher 실행 확인: 프로세스 리스트 확인
2. 로그 확인: `file_watcher.log` 파일 열기
3. 파일 경로 확인: `ue5_command.json` 경로가 올바른지 확인
4. 권한 확인: `Intermediate/MCP_Commands/` 쓰기 권한

### 3. Modify 명령이 작동하지 않음 (v1.2.0)

**증상**: "밀도를 2로 변경" 입력 시 새 숲 생성

**해결**:
1. `last_parameters.json` 파일 존재 확인
2. File Watcher 버전 확인 (v1.2.0 이상)
3. modify 키워드 사용 확인: "변경", "수정", "조정" 등
4. 로그에서 "Modify command detected" 메시지 확인

---

## 참고 자료

- **MCP 프로토콜**: https://modelcontextprotocol.io
- **Python asyncio**: https://docs.python.org/3/library/asyncio.html
- **Poetry**: https://python-poetry.org/docs/
