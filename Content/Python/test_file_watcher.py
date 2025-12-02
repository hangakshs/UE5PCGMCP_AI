"""
File Watcher Service 테스트 스크립트
UE5 Output Log에서 실행하여 File Watcher가 작동하는지 확인
"""
import unreal
from pathlib import Path
import json
import time


def test_file_watcher():
    """File Watcher Service가 작동하는지 테스트"""

    unreal.log("=" * 70)
    unreal.log("🔍 File Watcher Service Test")
    unreal.log("=" * 70)

    # 프로젝트 경로
    project_root = Path(unreal.Paths.project_dir())
    comm_dir = project_root / "Intermediate" / "MCP_Commands"
    log_file = comm_dir / "file_watcher.log"
    command_file = comm_dir / "ue5_command.json"
    response_file = comm_dir / "mcp_response.json"

    unreal.log(f"📂 Project Root: {project_root}")
    unreal.log(f"📁 Command Dir: {comm_dir}")
    unreal.log(f"   Exists: {comm_dir.exists()}")

    # 1. 로그 파일 확인
    unreal.log("")
    unreal.log("1️⃣ Checking File Watcher log file...")
    if log_file.exists():
        unreal.log(f"   ✅ Log file found: {log_file}")
        unreal.log(f"   📄 Log file size: {log_file.stat().st_size} bytes")

        # 마지막 10줄 출력
        with open(log_file, 'r', encoding='utf-8') as f:
            lines = f.readlines()
            last_lines = lines[-10:] if len(lines) > 10 else lines

            unreal.log(f"   📖 Last {len(last_lines)} lines:")
            for line in last_lines:
                unreal.log(f"      {line.rstrip()}")
    else:
        unreal.log(f"   ❌ Log file NOT found: {log_file}")
        unreal.log("   File Watcher may not be running!")

    # 2. 기존 파일 정리
    unreal.log("")
    unreal.log("2️⃣ Cleaning up old files...")
    if command_file.exists():
        command_file.unlink()
        unreal.log(f"   🗑️  Removed old command file")
    if response_file.exists():
        response_file.unlink()
        unreal.log(f"   🗑️  Removed old response file")

    # 3. 테스트 명령 작성
    unreal.log("")
    unreal.log("3️⃣ Writing test command...")
    test_command = {
        "command": "밀집된 소나무 숲",
        "timestamp": time.time()
    }

    with open(command_file, 'w', encoding='utf-8') as f:
        json.dump(test_command, f, ensure_ascii=False, indent=2)

    unreal.log(f"   ✅ Test command written: {command_file}")
    unreal.log(f"   📝 Command: {test_command['command']}")

    # 4. Response 대기
    unreal.log("")
    unreal.log("4️⃣ Waiting for File Watcher response...")
    unreal.log("   (waiting up to 5 seconds...)")

    for i in range(10):  # 5초 대기 (0.5초씩)
        time.sleep(0.5)

        if response_file.exists():
            unreal.log(f"   ✅ Response file detected after {(i+1)*0.5:.1f}s!")

            # Response 내용 읽기
            with open(response_file, 'r', encoding='utf-8') as f:
                response = json.load(f)

            unreal.log("   📨 Response content:")
            unreal.log(f"      Action: {response.get('action', 'N/A')}")

            if 'parameters' in response:
                params = response['parameters']
                unreal.log("      Parameters:")
                unreal.log(f"         Tree Type: {params.get('tree_type', 'N/A')}")
                unreal.log(f"         Density: {params.get('density', 'N/A')}")
                unreal.log(f"         Area Size: {params.get('area_size', 'N/A')} cm²")
                unreal.log(f"         Min Distance: {params.get('min_distance', 'N/A')} cm")

            break
    else:
        unreal.log("   ❌ Response file NOT detected after 5 seconds!")
        unreal.log("   File Watcher may not be processing commands!")

    # 5. 결과
    unreal.log("")
    unreal.log("=" * 70)
    if response_file.exists():
        unreal.log("✅ File Watcher Test: SUCCESS")
        unreal.log("   File Watcher is working correctly!")
    else:
        unreal.log("❌ File Watcher Test: FAILED")
        unreal.log("   File Watcher is NOT responding!")
        unreal.log("")
        unreal.log("   Troubleshooting:")
        unreal.log("   1. Check if File Watcher Service is running")
        unreal.log("   2. Check Output Log for Python errors during startup")
        unreal.log("   3. Try restarting UE5 Editor")
        unreal.log("   4. Check file_watcher.log for errors")

    unreal.log("=" * 70)


if __name__ == "__main__":
    test_file_watcher()
