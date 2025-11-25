"""
Quick one-liner test for forest generation (UE5.7 compatible)

Usage in UE5 Editor Python Console:
    py "F:/Project/Portfolio_MCP_PCG/Content/Python/quick_test.py"
"""

import unreal

# UE5.7 compatible way to get ForestPCGManager
editor_actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = editor_actor_subsystem.get_all_level_actors()
forest_managers = [actor for actor in all_actors if isinstance(actor, unreal.ForestPCGManager)]

if forest_managers:
    manager = forest_managers[0]
    print(f"✅ Found: {manager.get_name()}")
    manager.generate_forest_from_nlp("밀집된 소나무 숲")
    print("✅ Command sent!")
else:
    print("❌ No ForestPCGManager found!")
