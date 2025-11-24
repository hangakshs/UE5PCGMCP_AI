# PCG Forest MCP Server

자연어로 언리얼 엔진 PCG 숲 생성을 제어하는 MCP 서버입니다.

## 설치

```bash
cd MCPServer
pip install -r requirements.txt
```

## 실행

```bash
python server.py
```

## 사용 예시

### 숲 생성 명령어

- "밀집된 소나무 숲 만들어줘"
- "성긴 참나무 숲을 500평방미터에 생성해줘"
- "큰 나무들로 빽빽한 숲 만들어줘"
- "작은 자작나무 숲 만들어줘"

### 지원되는 키워드

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

## MCP 도구

1. **create_forest**: 자연어로 숲 생성
2. **clear_forest**: 숲 제거
3. **modify_forest**: 기존 숲 수정
