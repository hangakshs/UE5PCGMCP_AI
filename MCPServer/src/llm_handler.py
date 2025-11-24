"""
LLM Handler for Advanced NLP
Claude/GPT를 사용한 고급 자연어 처리
"""
import json
from typing import Dict, Any, Optional
import os


class LLMForestHandler:
    """LLM을 사용한 자연어 처리 핸들러"""

    def __init__(self, api_key: Optional[str] = None, use_llm: bool = False):
        """
        초기화

        Args:
            api_key: Anthropic API 키 (환경 변수 ANTHROPIC_API_KEY 사용 가능)
            use_llm: LLM 사용 여부 (False면 기본 파서 사용)
        """
        self.use_llm = use_llm
        self.api_key = api_key or os.environ.get('ANTHROPIC_API_KEY')

        if self.use_llm and self.api_key:
            try:
                import anthropic
                self.client = anthropic.Anthropic(api_key=self.api_key)
            except ImportError:
                print("Warning: anthropic 패키지가 설치되지 않았습니다. 기본 파서를 사용합니다.")
                self.use_llm = False
        else:
            self.use_llm = False

    def parse_with_llm(self, user_input: str) -> Dict[str, Any]:
        """
        Claude를 사용하여 자연어 명령 파싱

        Args:
            user_input: 사용자 입력 (예: "아름다운 가을 숲을 만들고 싶어요")

        Returns:
            PCG 파라미터 딕셔너리
        """
        if not self.use_llm:
            raise RuntimeError("LLM이 비활성화되어 있습니다")

        prompt = f"""다음 자연어 명령을 PCG 숲 생성 파라미터로 변환해주세요.

사용자 입력: "{user_input}"

다음 JSON 형식으로 파라미터를 추출하세요:
{{
  "tree_type": "pine|oak|birch|maple|generic_tree",
  "density": "dense|medium|sparse",
  "size": "tiny|small|medium|large|huge",
  "area_size": <면적(cm²)>,
  "min_distance": <최소 거리(cm)>,
  "max_distance": <최대 거리(cm)>,
  "randomness": <0~1 사이 값>,
  "scale_multiplier": <스케일 배수>
}}

추가 가이드라인:
- "아름다운", "신비로운" 등의 감성적 표현은 적절한 밀도와 크기로 변환
- "가을 숲"은 단풍나무(maple), "겨울 숲"은 소나무(pine) 선호
- 면적이 명시되지 않으면 5000(cm²) 사용
- 밀도에 따라 min_distance, max_distance 자동 조정:
  - dense: 150-300
  - medium: 200-500
  - sparse: 400-800

JSON만 출력하세요 (추가 설명 없이):"""

        try:
            message = self.client.messages.create(
                model="claude-3-5-sonnet-20241022",
                max_tokens=1024,
                messages=[{
                    "role": "user",
                    "content": prompt
                }]
            )

            # JSON 추출
            content = message.content[0].text

            # JSON 부분만 추출 (마크다운 코드 블록 제거)
            if "```json" in content:
                content = content.split("```json")[1].split("```")[0]
            elif "```" in content:
                content = content.split("```")[1].split("```")[0]

            params = json.loads(content.strip())
            params['action'] = 'create_forest'

            return params

        except Exception as e:
            print(f"LLM 파싱 실패: {e}")
            # 폴백: 기본 파서 사용
            from nlp_handler import ForestNLPHandler
            fallback_handler = ForestNLPHandler()
            return fallback_handler.parse_forest_command(user_input)

    def generate_description(self, params: Dict[str, Any]) -> str:
        """
        파라미터로부터 자연스러운 설명 생성

        Args:
            params: PCG 파라미터

        Returns:
            자연스러운 설명 문자열
        """
        if not self.use_llm:
            # 기본 설명
            return f"{params['density']} {params['tree_type']} 숲 ({params['area_size']}cm²)"

        prompt = f"""다음 PCG 숲 생성 파라미터를 자연스러운 한국어로 설명해주세요:

{json.dumps(params, indent=2, ensure_ascii=False)}

사용자에게 보여줄 친근하고 자연스러운 설명을 2-3문장으로 작성하세요.
예: "밀집된 소나무 숲을 생성합니다. 나무들은 150-300cm 간격으로 배치되며, 자연스러운 무작위성(0.3)이 적용됩니다. 약 0.5㎡ 영역에 걸쳐 생성됩니다."

설명만 출력하세요:"""

        try:
            message = self.client.messages.create(
                model="claude-3-5-sonnet-20241022",
                max_tokens=512,
                messages=[{
                    "role": "user",
                    "content": prompt
                }]
            )

            return message.content[0].text.strip()

        except Exception as e:
            print(f"설명 생성 실패: {e}")
            return f"{params['density']} {params['tree_type']} 숲"


def test_llm_handler():
    """LLM 핸들러 테스트"""
    # API 키 필요 (환경 변수 또는 직접 입력)
    handler = LLMForestHandler(use_llm=True)

    if not handler.use_llm:
        print("LLM이 비활성화되어 있습니다. ANTHROPIC_API_KEY를 설정하세요.")
        return

    test_inputs = [
        "아름다운 가을 숲을 만들고 싶어요",
        "어두운 침엽수림 생성해줘",
        "넓은 공원에 나무를 듬성듬성 심어줘",
        "신비로운 자작나무 숲을 2000평방미터에"
    ]

    for user_input in test_inputs:
        print(f"\n입력: {user_input}")
        print("-" * 80)

        params = handler.parse_with_llm(user_input)
        print(f"파라미터:\n{json.dumps(params, indent=2, ensure_ascii=False)}")

        description = handler.generate_description(params)
        print(f"\n설명: {description}")


if __name__ == "__main__":
    test_llm_handler()
