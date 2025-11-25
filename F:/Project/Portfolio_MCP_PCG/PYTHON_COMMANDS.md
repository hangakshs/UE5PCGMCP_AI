# Python Commands for UE5.7

## ⚠️ Important: UE5.7 API Changes

UE5.7에서 Python API가 변경되었습니다:

### ❌ Old (Deprecated)
```python
# UE5.6 이하에서 사용하던 방법
world = unreal.EditorLevelLibrary.get_editor_world()
actors = unreal.EditorLevelLibrary.get_all_actors_of_class(world, unreal.ForestPCGManager)
```

### ✅ New (UE5.7+)
```python
# UE5.7+ 에서 사용하는 방법
editor_actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = editor_actor_subsystem.get_all_level_actors()
forest_managers = [actor for actor in all_actors if isinstance(actor, unreal.ForestPCGManager)]
```

---

## 🚀 Quick Test Commands

### 1. 간단한 테스트 (One-liner)

```python
py "F:/Project/Portfolio_MCP_PCG/Content/Python/quick_test.py"
```

### 2. 상세한 테스트 (Full diagnostics)

```python
py "F:/Project/Portfolio_MCP_PCG/Content/Python/test_forest_fixed.py"
```

---

## 📋 Manual Python Commands

### Find ForestPCGManager

```python
import unreal

# Get subsystem
editor_actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# Get all actors
all_actors = editor_actor_subsystem.get_all_level_actors()

# Find ForestPCGManager
forest_managers = [actor for actor in all_actors if isinstance(actor, unreal.ForestPCGManager)]

if forest_managers:
    print(f"Found: {forest_managers[0].get_name()}")
else:
    print("No ForestPCGManager found!")
```

### Generate Forest

```python
import unreal

# Get ForestPCGManager
editor_actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = editor_actor_subsystem.get_all_level_actors()
forest_managers = [actor for actor in all_actors if isinstance(actor, unreal.ForestPCGManager)]

if forest_managers:
    manager = forest_managers[0]
    manager.generate_forest_from_nlp("밀집된 소나무 숲")
    print("Command sent!")
```

---

## 🔍 Debugging Commands

### Check System Status

```python
import unreal
import os

# Check command directory
cmd_dir = "F:/Project/Portfolio_MCP_PCG/Intermediate/MCP_Commands"
print(f"Command Dir Exists: {os.path.exists(cmd_dir)}")
if os.path.exists(cmd_dir):
    files = os.listdir(cmd_dir)
    print(f"Files in command dir: {len(files)}")
    for f in files:
        print(f"  - {f}")

# Check MCPClient
editor_actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = editor_actor_subsystem.get_all_level_actors()
mcp_clients = [actor for actor in all_actors if isinstance(actor, unreal.MCPClient)]

print(f"\nMCPClient Count: {len(mcp_clients)}")
for client in mcp_clients:
    print(f"  - {client.get_name()}")
    print(f"    Use File: {client.get_editor_property('use_file_communication')}")
    print(f"    Debug: {client.get_editor_property('debug_mode')}")
```

---

## 🛠️ Troubleshooting

### Problem: `AttributeError: type object 'EditorLevelLibrary' has no attribute 'get_all_actors_of_class'`

**Solution:** UE5.7에서는 이 메서드가 제거되었습니다. 위의 새로운 방법을 사용하세요.

### Problem: No ForestPCGManager found

**Solution:**
1. 레벨에 ForestPCGManager 액터가 있는지 확인
2. `Window > Place Actors > All Classes > ForestPCGManager` 로 추가

### Problem: Command not processing

**Solution:**
1. Output Log에서 File Watcher Service 시작 메시지 확인
2. `F:/Project/Portfolio_MCP_PCG/Intermediate/MCP_Commands` 폴더 존재 확인
3. MCPClient의 `Use File Communication` 옵션 확인

---

## 📚 Additional Resources

- [UE5.7 Python API Documentation](https://docs.unrealengine.com/5.7/en-US/PythonAPI/)
- [Editor Scripting Utilities Migration Guide](https://docs.unrealengine.com/5.7/en-US/editor-scripting-utilities-migration/)
