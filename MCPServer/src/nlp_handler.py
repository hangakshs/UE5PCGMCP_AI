"""
NLP Handler for Forest PCG Generation

자연어를 PCG 파라미터로 변환하는 모듈

주요 기능:
- 한국어 키워드 기반 파싱 (LLM 없이 동작)
- 나무 타입, 밀도, 크기, 면적 추출
- 파라미터 자동 조정 (밀도/크기에 따른 거리 설정)
"""
import re
import json
import sys
from typing import Dict, Any, List

# Windows에서 UTF-8 인코딩 설정 (이모지 출력을 위해)
if sys.platform == 'win32':
    try:
        sys.stdout.reconfigure(encoding='utf-8')
        sys.stderr.reconfigure(encoding='utf-8')
    except AttributeError:
        # Python 3.6 이하에서는 reconfigure가 없음
        import io
        sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')
        sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding='utf-8')


class ForestNLPHandler:
    """
    자연어 명령을 숲 생성 파라미터로 변환하는 핸들러

    지원 나무 타입: 소나무, 참나무, 자작나무, 단풍나무
    밀도: 빽빽/밀집 (dense), 보통 (medium), 듬성/성긴 (sparse)
    크기: 거대한/큰/보통/작은 (huge/large/medium/small)
    """

    def __init__(self):
        """키워드 매핑 딕셔너리 초기화"""
        # 나무 타입 한->영 매핑 (긴 키워드를 먼저 배치하여 정확한 매칭)
        self.tree_types = {
            '단풍나무': 'maple',
            '소나무': 'pine',
            '참나무': 'oak',
            '자작나무': 'birch',
            '떡갈나무': 'oak',
            '나무': 'generic_tree'
        }

        # 밀도 키워드 매핑
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

        # 크기 키워드 매핑
        self.size_keywords = {
            '거대한': 'huge',
            '큰': 'large',
            '작은': 'small',
            '보통': 'medium',
            '일반': 'medium'
        }

    def parse_forest_command(self, text: str) -> Dict[str, Any]:
        """
        자연어 명령을 PCG 파라미터로 변환

        예시:
        - "밀집된 소나무 숲 만들어줘" -> dense pine forest
        - "성긴 참나무 숲을 500평방미터에 생성해줘" -> sparse oak forest 500m²
        - "큰 나무들로 빽빽한 숲 만들어줘" -> dense large tree forest
        """

        params = {
            'action': 'create_forest',
            'tree_type': 'generic_tree',
            'tree_types': [],  # 기본값은 빈 배열 (단일 나무 타입)
            'density': 'medium',
            'size': 'medium',
            'area_size': 1000000.0,  # 기본 100㎡ (1000000 cm²)
            'randomness': 0.5,
            'min_distance': 200.0,  # cm (2m) - 보통 밀도 기준 최소 거리
            'max_distance': 500.0,  # cm (5m) - 보통 밀도 기준 최대 거리
            'density_multiplier': 1.0  # 기본 밀도 (1.0 = 메시 바운드 크기만큼 배치)
        }
        explicit_overrides = set()
        text_lower = text.lower()

        # 다양한 나무가 섞인 숲인지 확인
        mixed_keywords = ['다양한', '다양하게', '다양', '섞인', '여러 종류', '여러가지', '혼합', '다종', '믹스']
        is_mixed_forest = any(keyword in text for keyword in mixed_keywords)

        if is_mixed_forest:
            # 여러 나무 타입 사용 (소나무, 참나무, 자작나무, 단풍나무)
            params['tree_types'] = ['pine', 'oak', 'birch', 'maple']
            params['tree_type'] = 'mixed'  # 호환성을 위해
            explicit_overrides.update({'tree_type', 'tree_types'})
        else:
            # 단일 나무 종류 파싱
            for korean, english in self.tree_types.items():
                if korean in text:
                    params['tree_type'] = english
                    explicit_overrides.add('tree_type')
                    break

        # 밀도 파싱
        for keyword, density in self.density_keywords.items():
            if keyword in text:
                params['density'] = density
                explicit_overrides.add('density')
                break

        # 크기 파싱
        for keyword, size in self.size_keywords.items():
            if keyword in text:
                params['size'] = size
                explicit_overrides.add('size')
                break

        # 명시적 스케일 배율 파싱 (예: "스케일 0.5배", "크기를 2배", "사이즈 0.8")
        scale_match = re.search(
            r'(?:스케일|scale|크기|사이즈|나무\s*크기|배율)\s*(?:만|만큼|정도)?\s*(?:를|을|으로|로|만|은|는|이|가)?\s*(\d+(?:\.\d+)?)\s*배',
            text_lower
        )
        if not scale_match:
            # "사이즈 0.1", "크기 0.5" 같은 패턴도 감지
            scale_match = re.search(
                r'(?:스케일|scale|크기|사이즈|나무\s*크기|배율)\s*[:=]?\s*(\d+(?:\.\d+)?)',
                text_lower
            )
        if scale_match:
            explicit_scale = float(scale_match.group(1))
            params['scale_multiplier'] = explicit_scale
            explicit_overrides.add('scale_multiplier')
            print(f"[DEBUG] Parsed explicit scale: {explicit_scale}x")

        # 명시적 밀도 배율 파싱 (예: "밀도 1.5", "밀도를 2로", "밀도 0.5", "density 2.0")
        # "밀도를 2로", "밀도 2", "밀도=2", "밀도:2", "밀도를2로" 등 다양한 패턴 지원
        # "밀도" 다음에 조사(를/을/로/으로)와 공백/구두점을 무시하고 숫자를 찾음
        # 여러 패턴 시도: "밀도를 2로", "밀도 2", "밀도=2", "밀도:2", "밀도를2로"
        density_mult_match = re.search(
            r'(?:밀도|density)\s*(?:만|만큼)?\s*[를을]?\s*(\d+(?:\.\d+)?)\s*(?:배)?\s*(?:로|으로|만큼|정도로|가량|정도)?',
            text_lower
        )
        if not density_mult_match:
            density_mult_match = re.search(
                r'(?:밀도|density)\s*(?:만|만큼)?\s*(\d+(?:\.\d+)?)\s*배',
                text_lower
            )
        if not density_mult_match:
            # "밀도를2로", "밀도만2배" 같이 공백이 없는 경우도 처리
            density_mult_match = re.search(
                r'(?:밀도|density)\s*(?:만)?[를을로으로]?(\d+(?:\.\d+)?)',
                text_lower
            )
        if density_mult_match:
            explicit_density = float(density_mult_match.group(1))
            params['density_multiplier'] = explicit_density
            explicit_overrides.add('density_multiplier')
            print(f"[DEBUG] Parsed explicit density multiplier: {explicit_density}x")

        # 면적 파싱 (숫자 추출)
        area_match = re.search(r'(\d+(?:\.\d+)?)\s*(?:평방미터|제곱미터|m2|㎡)', text)
        if area_match:
            area = float(area_match.group(1))
            params['area_size'] = area * 10000.0  # m² to cm²
            explicit_overrides.add('area_size')

        # 밀도에 따른 밀도 배율 설정 (명시적 밀도 배율이 지정되지 않은 경우만)
        # 밀도는 수치로 정리:
        # - 밀도 1.0: 메시 바운드 크기만큼 배치
        # - 밀도 2.0: 밀도 1보다 2배 많은 나무 객체 수 배치
        # - 밀도 0.5: 밀도 1보다 0.5배 나무 객체 수 배치
        # 나무 간 간격은 전체 숲 크기에 비례하여 C++ 코드에서 자동 계산됨
        if not density_mult_match:  # 명시적으로 "밀도 X" 형태로 지정하지 않은 경우
            if params['density'] == 'dense':
                params['randomness'] = 0.3
                params['density_multiplier'] = 2.0  # 밀집: 2배 밀도 (2배 많은 나무)
                # 간격은 C++에서 자동으로 1/√2로 줄어듦
            elif params['density'] == 'sparse':
                params['randomness'] = 0.7
                params['density_multiplier'] = 0.5  # 성긴: 0.5배 밀도 (0.5배 나무)
                # 간격은 C++에서 자동으로 √2로 늘어남
            else:  # medium (기본 밀도)
                # 보통 밀도는 1.0 유지 (메시 바운드 크기만큼 배치)
                # min_distance=200cm(2m), max_distance=500cm(5m) 유지
                params['density_multiplier'] = 1.0

        # 크기에 따른 스케일 조정 (명시적 스케일이 없는 경우에만)
        if 'scale_multiplier' not in params:
            scale_multipliers = {
                'tiny': 0.5,
                'small': 0.75,
                'medium': 1.0,
                'large': 1.5,
                'huge': 2.0
            }
            params['scale_multiplier'] = scale_multipliers.get(params['size'], 1.0)

        # 크기별 면적 조정
        if params['size'] in ['small', 'large', 'huge']:
            if params['area_size'] == 1000000.0:  # 기본값인 경우
                if params['size'] == 'small':
                    params['area_size'] = 500000.0  # 50㎡
                elif params['size'] == 'large':
                    params['area_size'] = 3000000.0  # 300㎡
                elif params['size'] == 'huge':
                    params['area_size'] = 6000000.0  # 600㎡

        params['_overrides'] = list(explicit_overrides)
        return params

    def generate_response(self, params: Dict[str, Any]) -> str:
        """사용자에게 보여줄 응답 생성"""

        density_kr = {
            'dense': '밀집된',
            'medium': '보통',
            'sparse': '성긴'
        }

        size_kr = {
            'tiny': '아주 작은',
            'small': '작은',
            'medium': '보통 크기의',
            'large': '큰',
            'huge': '거대한'
        }

        tree_kr = {
            'pine': '소나무',
            'oak': '참나무',
            'birch': '자작나무',
            'maple': '단풍나무',
            'generic_tree': '나무'
        }

        density = density_kr.get(params['density'], '보통')
        size = size_kr.get(params['size'], '보통 크기의')
        
        # 여러 나무 타입이 있는 경우
        if 'tree_types' in params and params['tree_types']:
            tree_names = [tree_kr.get(t, t) for t in params['tree_types']]
            tree = '와 '.join(tree_names) + '가 섞인'
        else:
            tree = tree_kr.get(params['tree_type'], '나무')
        
        area_m2 = params['area_size'] / 10000.0

        response = f"{density} {size} {tree} 숲을 약 {area_m2:.0f}㎡ 영역에 생성합니다.\n"
        response += f"밀도 배율: {params['density_multiplier']:.1f}x "
        if params['density_multiplier'] == 1.0:
            response += "(메시 바운드 크기만큼 배치)\n"
        elif params['density_multiplier'] > 1.0:
            response += f"({params['density_multiplier']:.1f}배 많은 나무 배치)\n"
        else:
            response += f"({params['density_multiplier']:.1f}배 나무 배치)\n"
        response += f"나무 간 거리: 최소 {params['min_distance']:.0f}cm(2m), "
        response += f"최대 {params['max_distance']:.0f}cm(5m) "
        response += f"(밀도에 따라 자동 조정됨)\n"
        response += f"임의성: {params['randomness']:.1f}"

        return response


def test_nlp_handler():
    """테스트 함수"""
    handler = ForestNLPHandler()

    test_commands = [
        "밀집된 소나무 숲 만들어줘",
        "성긴 참나무 숲을 500평방미터에 생성해줘",
        "큰 나무들로 빽빽한 숲 만들어줘",
        "작은 자작나무 숲 만들어줘"
    ]

    for cmd in test_commands:
        print(f"\n입력: {cmd}")
        params = handler.parse_forest_command(cmd)
        print(f"파라미터: {json.dumps(params, indent=2, ensure_ascii=False)}")
        response = handler.generate_response(params)
        print(f"응답: {response}")


if __name__ == "__main__":
    test_nlp_handler()
