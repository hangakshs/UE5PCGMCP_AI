"""
NLP Handler for Forest PCG Generation
자연어를 PCG 파라미터로 변환하는 모듈
"""
import re
import json
from typing import Dict, Any, List


class ForestNLPHandler:
    """자연어 명령을 숲 생성 파라미터로 변환"""

    def __init__(self):
        self.tree_types = {
            '단풍나무': 'maple',  # 더 긴 키워드를 먼저
            '소나무': 'pine',
            '참나무': 'oak',
            '자작나무': 'birch',
            '떡갈나무': 'oak',
            '나무': 'generic_tree'
        }

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
            'density': 'medium',
            'size': 'medium',
            'area_size': 5000.0,  # 기본 50m x 100m
            'randomness': 0.5,
            'min_distance': 200.0,  # cm
            'max_distance': 500.0   # cm
        }

        text_lower = text.lower()

        # 나무 종류 파싱
        for korean, english in self.tree_types.items():
            if korean in text:
                params['tree_type'] = english
                break

        # 밀도 파싱
        for keyword, density in self.density_keywords.items():
            if keyword in text:
                params['density'] = density
                break

        # 크기 파싱
        for keyword, size in self.size_keywords.items():
            if keyword in text:
                params['size'] = size
                break

        # 면적 파싱 (숫자 추출)
        area_match = re.search(r'(\d+(?:\.\d+)?)\s*(?:평방미터|제곱미터|m2|㎡)', text)
        if area_match:
            area = float(area_match.group(1))
            params['area_size'] = area * 10000.0  # m² to cm²

        # 밀도에 따른 거리 조정
        if params['density'] == 'dense':
            params['min_distance'] = 150.0
            params['max_distance'] = 300.0
            params['randomness'] = 0.3
        elif params['density'] == 'sparse':
            params['min_distance'] = 400.0
            params['max_distance'] = 800.0
            params['randomness'] = 0.7

        # 크기에 따른 스케일 조정
        scale_multipliers = {
            'tiny': 0.5,
            'small': 0.75,
            'medium': 1.0,
            'large': 1.5,
            'huge': 2.0
        }
        params['scale_multiplier'] = scale_multipliers.get(params['size'], 1.0)

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
        tree = tree_kr.get(params['tree_type'], '나무')
        area_m2 = params['area_size'] / 10000.0

        response = f"{density} {size} {tree} 숲을 약 {area_m2:.0f}㎡ 영역에 생성합니다.\n"
        response += f"나무 간 최소 거리: {params['min_distance']:.0f}cm, "
        response += f"최대 거리: {params['max_distance']:.0f}cm\n"
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
