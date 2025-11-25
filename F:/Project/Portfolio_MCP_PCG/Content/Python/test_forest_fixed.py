"""
Fixed Forest Generation Test for UE5.7

UE5.7에서는 EditorLevelLibrary.get_all_actors_of_class가 제거되었습니다.
대신 EditorActorSubsystem을 사용합니다.
"""

import unreal

def find_forest_pcg_manager():
    """Find ForestPCGManager in the current level"""
    try:
        # UE5.7에서는 EditorActorSubsystem 사용
        editor_actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
        world_subsystem = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)

        # Get world
        world = world_subsystem.get_editor_world()

        if not world:
            print("❌ No editor world found!")
            return None

        print(f"✅ World found: {world.get_name()}")

        # Get all actors
        all_actors = editor_actor_subsystem.get_all_level_actors()
        print(f"📊 Total actors in level: {len(all_actors)}")

        # Find ForestPCGManager
        forest_managers = []
        for actor in all_actors:
            if isinstance(actor, unreal.ForestPCGManager):
                forest_managers.append(actor)

        if not forest_managers:
            print("❌ No ForestPCGManager found in level!")
            print("   Please add a ForestPCGManager actor to the level.")
            return None

        print(f"✅ Found {len(forest_managers)} ForestPCGManager(s):")
        for i, manager in enumerate(forest_managers):
            print(f"   [{i}] {manager.get_name()}")

        return forest_managers[0]

    except Exception as e:
        print(f"❌ Error finding ForestPCGManager: {e}")
        import traceback
        traceback.print_exc()
        return None


def test_forest_generation():
    """Test forest generation with natural language command"""
    print("=" * 70)
    print("🌲 FOREST GENERATION TEST (UE5.7 Compatible)")
    print("=" * 70)

    # Find ForestPCGManager
    manager = find_forest_pcg_manager()

    if not manager:
        return False

    # Check MCPClient
    mcp_client = manager.get_editor_property('mcp_client')
    if not mcp_client:
        print("❌ MCPClient not found on ForestPCGManager!")
        return False

    print(f"✅ MCPClient found: {mcp_client.get_name()}")
    print(f"   Use File Communication: {mcp_client.get_editor_property('use_file_communication')}")
    print(f"   Debug Mode: {mcp_client.get_editor_property('debug_mode')}")

    # Send test command
    test_command = "밀집된 소나무 숲"
    print("=" * 70)
    print(f"📤 Sending command: '{test_command}'")
    print("=" * 70)

    try:
        manager.generate_forest_from_nlp(test_command)
        print("✅ Command sent successfully!")
        print("\n📋 What to check next:")
        print("   1. Watch Output Log for processing messages")
        print("   2. Look for 'File Watcher Service' activity")
        print("   3. PCG should generate trees in the level")
        print("=" * 70)
        return True
    except Exception as e:
        print(f"❌ Error sending command: {e}")
        import traceback
        traceback.print_exc()
        return False


if __name__ == "__main__":
    test_forest_generation()
