"""
Forest Generation Test Script
UE5 에디터의 Python 콘솔에서 실행하여 숲 생성을 테스트합니다.

사용 방법:
1. UE5 에디터에서 Tools > Execute Python Script 선택
2. 이 파일 선택하거나 아래 명령을 Python 콘솔에 복사

또는 Output Log에서 Cmd 입력창에:
    py "<프로젝트경로>/UnrealProject/Content/Python/test_forest_generation.py"
"""
import unreal


def test_forest_generation():
    """숲 생성 테스트"""

    unreal.log("=" * 70)
    unreal.log("🌲 Testing Forest Generation System")
    unreal.log("=" * 70)

    # 1. ForestPCGManager 찾기
    unreal.log("\n1️⃣ Finding ForestPCGManager in level...")

    world = unreal.EditorLevelLibrary.get_editor_world()
    if not world:
        unreal.log_error("❌ Failed to get editor world")
        return False

    unreal.log(f"   World: {world.get_name()}")

    # 모든 ForestPCGManager 액터 찾기
    forest_managers = unreal.GameplayStatics.get_all_actors_of_class(
        world,
        unreal.ForestPCGManager
    )

    if not forest_managers or len(forest_managers) == 0:
        unreal.log_error("=" * 70)
        unreal.log_error("❌ No ForestPCGManager found in level!")
        unreal.log_error("")
        unreal.log_error("   SOLUTION:")
        unreal.log_error("   1. Open your level in UE5 Editor")
        unreal.log_error("   2. Place Modes > All Classes > ForestPCGManager")
        unreal.log_error("   3. Drag and drop into the level")
        unreal.log_error("   4. Run this script again")
        unreal.log_error("=" * 70)
        return False

    manager = forest_managers[0]
    unreal.log(f"   ✅ Found ForestPCGManager: {manager.get_name()}")

    # 2. MCPClient 확인
    unreal.log("\n2️⃣ Checking MCPClient...")

    mcp_client = manager.get_editor_property('mcp_client')
    if not mcp_client:
        unreal.log_warning("   ⚠️  MCPClient not found in ForestPCGManager")
        unreal.log("   ForestPCGManager should auto-create it...")
    else:
        unreal.log(f"   ✅ MCPClient found: {mcp_client.get_name()}")

    # 3. 테스트 명령 전송
    unreal.log("\n3️⃣ Sending test command...")

    test_commands = [
        "밀집된 소나무 숲",
        "성긴 참나무 숲",
        "보통 밀도 자작나무 숲"
    ]

    command = test_commands[0]  # 첫 번째 테스트 명령 사용

    unreal.log(f"   Command: '{command}'")
    unreal.log(f"   Calling GenerateForestFromNLP()...")

    try:
        manager.generate_forest_from_nlp(command)
        unreal.log("   ✅ Command sent successfully!")

    except Exception as e:
        unreal.log_error(f"   ❌ Failed to send command: {e}")
        import traceback
        unreal.log_error(traceback.format_exc())
        return False

    # 4. 결과 확인 안내
    unreal.log("\n4️⃣ What happens next:")
    unreal.log("   📝 Check Output Log for:")
    unreal.log("      - MCPClient: '🚀 Sending Command via File'")
    unreal.log("      - File Watcher: '📨 Command file detected!'")
    unreal.log("      - MCPClient: '📥 Response File Detected!'")
    unreal.log("      - ForestPCGManager: '🌲 Forest Parameters Received!'")
    unreal.log("      - ForestPCGManager: '✅ PCG Forest Generation Complete!'")

    unreal.log("\n" + "=" * 70)
    unreal.log("✅ Test command sent!")
    unreal.log("=" * 70)
    unreal.log("📊 Monitoring:")
    unreal.log("   1. Check Output Log for processing messages")
    unreal.log("   2. Watch for File Watcher Service activity")
    unreal.log("   3. PCG should generate trees in the level")
    unreal.log("=" * 70)

    return True


def test_direct_file_creation():
    """명령 파일을 직접 생성하여 테스트 (File Watcher Service 테스트)"""
    import json
    import time
    from pathlib import Path

    unreal.log("=" * 70)
    unreal.log("🧪 Testing Direct File Creation (File Watcher Test)")
    unreal.log("=" * 70)

    # 프로젝트 경로
    project_dir = Path(unreal.Paths.project_dir())
    command_dir = project_dir / "Intermediate" / "MCP_Commands"
    command_file = command_dir / "ue5_command.json"

    unreal.log(f"   Command Dir: {command_dir}")
    unreal.log(f"   Command File: {command_file}")

    # 디렉토리 생성
    command_dir.mkdir(parents=True, exist_ok=True)
    unreal.log(f"   ✅ Directory ready")

    # 명령 JSON 생성
    command_data = {
        "command": "밀집된 소나무 숲",
        "timestamp": time.time()
    }

    # 파일 작성
    with open(command_file, 'w', encoding='utf-8') as f:
        json.dump(command_data, f, indent=2, ensure_ascii=False)

    unreal.log(f"   ✅ Command file created!")
    unreal.log(f"   Content: {json.dumps(command_data, ensure_ascii=False)}")

    unreal.log("\n📊 Now monitoring:")
    unreal.log("   1. File Watcher Service should detect the file")
    unreal.log("   2. Process the command with NLP Handler")
    unreal.log("   3. Create mcp_response.json")
    unreal.log("   4. MCPClient should read the response")
    unreal.log("   5. Forest should be generated")

    unreal.log("\n⏳ Waiting 5 seconds for File Watcher to process...")
    unreal.log("   Check Output Log for File Watcher activity")
    unreal.log("=" * 70)


def check_system_status():
    """시스템 상태 확인"""
    from pathlib import Path

    unreal.log("=" * 70)
    unreal.log("🔍 System Status Check")
    unreal.log("=" * 70)

    project_dir = Path(unreal.Paths.project_dir())
    command_dir = project_dir / "Intermediate" / "MCP_Commands"

    unreal.log(f"\n📁 Directory Status:")
    unreal.log(f"   Project: {project_dir}")
    unreal.log(f"   Command Dir: {command_dir}")
    unreal.log(f"   Exists: {command_dir.exists()}")

    if command_dir.exists():
        files = list(command_dir.glob("*"))
        unreal.log(f"   Files: {len(files)}")
        for f in files:
            unreal.log(f"      - {f.name} ({f.stat().st_size} bytes)")

    # ForestPCGManager 확인
    world = unreal.EditorLevelLibrary.get_editor_world()
    managers = unreal.GameplayStatics.get_all_actors_of_class(
        world,
        unreal.ForestPCGManager
    )

    unreal.log(f"\n🌲 ForestPCGManager:")
    unreal.log(f"   Count: {len(managers) if managers else 0}")
    if managers:
        for i, mgr in enumerate(managers):
            unreal.log(f"   [{i}] {mgr.get_name()}")
            mcp = mgr.get_editor_property('mcp_client')
            unreal.log(f"       MCPClient: {mcp.get_name() if mcp else 'None'}")

    # MCPClient 확인
    mcp_clients = unreal.GameplayStatics.get_all_actors_of_class(
        world,
        unreal.MCPClient
    )

    unreal.log(f"\n🔌 MCPClient:")
    unreal.log(f"   Count: {len(mcp_clients) if mcp_clients else 0}")
    if mcp_clients:
        for i, client in enumerate(mcp_clients):
            unreal.log(f"   [{i}] {client.get_name()}")
            use_file = client.get_editor_property('use_file_communication')
            debug = client.get_editor_property('debug_mode')
            unreal.log(f"       Use File Communication: {use_file}")
            unreal.log(f"       Debug Mode: {debug}")

    unreal.log("=" * 70)


# 메인 실행
if __name__ == "__main__":
    unreal.log("\n" * 2)
    unreal.log("╔" + "=" * 68 + "╗")
    unreal.log("║" + " " * 20 + "FOREST GENERATION TEST" + " " * 26 + "║")
    unreal.log("╚" + "=" * 68 + "╝")
    unreal.log("")

    # 1. 시스템 상태 확인
    check_system_status()

    unreal.log("\n" + "─" * 70 + "\n")

    # 2. 일반 테스트 (ForestPCGManager 사용)
    test_forest_generation()

    unreal.log("\n" + "─" * 70 + "\n")

    # 3. 직접 파일 생성 테스트 (File Watcher 테스트)
    # test_direct_file_creation()  # 필요시 주석 해제
