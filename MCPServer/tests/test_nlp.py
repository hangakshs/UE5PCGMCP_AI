"""
NLP Handler 테스트 스크립트
자연어 명령이 올바르게 파싱되는지 확인
"""
from nlp_handler import ForestNLPHandler
import json


def test_commands():
    """다양한 자연어 명령 테스트"""

    handler = ForestNLPHandler()

    test_cases = [
        {
            "command": "밀집된 소나무 숲 만들어줘",
            "expected": {
                "tree_type": "pine",
                "density": "dense"
            }
        },
        {
            "command": "성긴 참나무 숲을 500평방미터에 생성해줘",
            "expected": {
                "tree_type": "oak",
                "density": "sparse",
                "area_size": 5000000.0  # 500㎡ = 5000000cm²
            }
        },
        {
            "command": "큰 나무들로 빽빽한 숲 만들어줘",
            "expected": {
                "density": "dense",
                "size": "large"
            }
        },
        {
            "command": "작은 자작나무 숲 만들어줘",
            "expected": {
                "tree_type": "birch",
                "size": "small"
            }
        },
        {
            "command": "거대한 단풍나무 숲 1000평방미터",
            "expected": {
                "tree_type": "maple",
                "size": "huge",
                "area_size": 10000000.0
            }
        },
        {
            "command": "보통 숲",
            "expected": {
                "density": "medium",
                "size": "medium"
            }
        }
    ]

    print("=" * 80)
    print("NLP Handler 테스트")
    print("=" * 80)

    passed = 0
    failed = 0

    for idx, test in enumerate(test_cases, 1):
        command = test["command"]
        expected = test["expected"]

        print(f"\n[테스트 {idx}] 입력: {command}")
        print("-" * 80)

        # 파싱
        params = handler.parse_forest_command(command)

        # 응답 생성
        response = handler.generate_response(params)

        print(f"파라미터:")
        print(json.dumps(params, indent=2, ensure_ascii=False))
        print(f"\n응답:")
        print(response)

        # 검증
        test_passed = True
        for key, value in expected.items():
            if params.get(key) != value:
                print(f"\n❌ 실패: {key} - 예상 {value}, 실제 {params.get(key)}")
                test_passed = False
                failed += 1
                break

        if test_passed:
            print("\n✅ 통과")
            passed += 1

    # 결과 요약
    print("\n" + "=" * 80)
    print(f"테스트 결과: {passed}/{len(test_cases)} 통과")
    print("=" * 80)

    if failed == 0:
        print("🎉 모든 테스트 통과!")
    else:
        print(f"⚠️  {failed}개 테스트 실패")

    return failed == 0


def test_edge_cases():
    """엣지 케이스 테스트"""

    handler = ForestNLPHandler()

    print("\n" + "=" * 80)
    print("엣지 케이스 테스트")
    print("=" * 80)

    edge_cases = [
        "숲",
        "나무 많이",
        "소나무",
        "10000평방미터",
        "빽빽한 큰 작은 나무",  # 모순된 명령
        ""
    ]

    for command in edge_cases:
        print(f"\n입력: '{command}'")
        params = handler.parse_forest_command(command)
        response = handler.generate_response(params)
        print(f"결과: {params['tree_type']}, {params['density']}, {params['size']}")
        print(f"응답: {response}")


def benchmark_performance():
    """성능 벤치마크"""
    import time

    handler = ForestNLPHandler()
    command = "밀집된 큰 소나무 숲을 1000평방미터에 생성해줘"

    iterations = 10000
    start_time = time.time()

    for _ in range(iterations):
        handler.parse_forest_command(command)

    elapsed = time.time() - start_time
    avg_time = (elapsed / iterations) * 1000  # ms

    print("\n" + "=" * 80)
    print("성능 벤치마크")
    print("=" * 80)
    print(f"반복 횟수: {iterations:,}")
    print(f"총 시간: {elapsed:.3f}초")
    print(f"평균 파싱 시간: {avg_time:.3f}ms")
    print(f"초당 처리량: {iterations/elapsed:.0f} req/s")


if __name__ == "__main__":
    # 기본 테스트
    success = test_commands()

    # 엣지 케이스
    test_edge_cases()

    # 성능 테스트
    benchmark_performance()

    # 종료 코드
    exit(0 if success else 1)
