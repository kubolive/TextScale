import os
import sys
import json
import time
import subprocess

SCENE_PATH = os.path.expandvars(r'%APPDATA%\obs-studio\basic\scenes\Untitled.json')
BACKUP_PATH = SCENE_PATH + '.matrix_backup'
LOGS_DIR = os.path.expandvars(r'%APPDATA%\obs-studio\logs')
OBS_EXE = r'C:\Program Files\obs-studio\bin\64bit\obs64.exe'
OBS_DIR = r'C:\Program Files\obs-studio\bin\64bit'

def run_functional_matrix():
    print("=== Stage 6: OBS Functional Matrix Tests ===")
    if not os.path.exists(SCENE_PATH):
        print(f"[FAIL] Scene file not found: {SCENE_PATH}")
        return False

    with open(SCENE_PATH, 'r', encoding='utf-8') as f:
        original_content = f.read()
    with open(BACKUP_PATH, 'w', encoding='utf-8') as f:
        f.write(original_content)

    try:
        scene_data = json.loads(original_content)

        # Source 1: Medium text (target 267px, auto condense, left align)
        # "TIN TỨC TRONG NƯỚC" (~350px estimated)
        src_medium = {
            "id": "text_gdiplus_v3",
            "name": "TEST_MEDIUM_CONDENSE",
            "uuid": "aaaa1111-0000-0000-0000-000000000001",
            "settings": {
                "text": "BẢN TIN DỰ BÁO THỜI TIẾT",
                "font": {"face": "Arial", "size": 32, "flags": 0}
            },
            "filters": [
                {
                    "id": "text_scale_filter",
                    "name": "Filter Auto Medium",
                    "settings": {
                        "enable_auto": True,
                        "target_width": 267,
                        "min_scale": 50,
                        "max_scale": 100,
                        "alignment": 0, # Left
                        "debug_info": True
                    }
                }
            ]
        }

        # Source 2: Center Alignment test
        src_center = {
            "id": "text_gdiplus_v3",
            "name": "TEST_CENTER_ALIGN",
            "uuid": "aaaa1111-0000-0000-0000-000000000002",
            "settings": {
                "text": "TRUNG TÂM",
                "font": {"face": "Arial", "size": 32, "flags": 0}
            },
            "filters": [
                {
                    "id": "text_scale_filter",
                    "name": "Filter Center Align",
                    "settings": {
                        "enable_auto": True,
                        "target_width": 300,
                        "alignment": 1, # Center
                        "debug_info": True
                    }
                }
            ]
        }

        # Source 3: Right Alignment test
        src_right = {
            "id": "text_gdiplus_v3",
            "name": "TEST_RIGHT_ALIGN",
            "uuid": "aaaa1111-0000-0000-0000-000000000003",
            "settings": {
                "text": "BÊN PHẢI",
                "font": {"face": "Arial", "size": 32, "flags": 0}
            },
            "filters": [
                {
                    "id": "text_scale_filter",
                    "name": "Filter Right Align",
                    "settings": {
                        "enable_auto": True,
                        "target_width": 300,
                        "alignment": 2, # Right
                        "debug_info": True
                    }
                }
            ]
        }

        # Source 4: Manual Scaling (75% X, 120% Y)
        src_manual = {
            "id": "text_gdiplus_v3",
            "name": "TEST_MANUAL_SCALING",
            "uuid": "aaaa1111-0000-0000-0000-000000000004",
            "settings": {
                "text": "MANUAL SCALE 75% X 120% Y",
                "font": {"face": "Arial", "size": 28, "flags": 0}
            },
            "filters": [
                {
                    "id": "text_scale_filter",
                    "name": "Filter Manual Scaling",
                    "settings": {
                        "enable_manual": True,
                        "enable_auto": False,
                        "scale_x": 75,
                        "scale_y": 120,
                        "alignment": 0,
                        "debug_info": True
                    }
                }
            ]
        }

        scene_data['sources'].extend([src_medium, src_center, src_right, src_manual])
        current_scene_name = scene_data.get('current_scene')
        for src in scene_data['sources']:
            if src.get('name') == current_scene_name:
                items = src.get('settings', {}).get('items', [])
                items.append({"name": "TEST_MEDIUM_CONDENSE", "source_uuid": src_medium["uuid"], "visible": True})
                items.append({"name": "TEST_CENTER_ALIGN", "source_uuid": src_center["uuid"], "visible": True})
                items.append({"name": "TEST_RIGHT_ALIGN", "source_uuid": src_right["uuid"], "visible": True})
                items.append({"name": "TEST_MANUAL_SCALING", "source_uuid": src_manual["uuid"], "visible": True})
                break

        with open(SCENE_PATH, 'w', encoding='utf-8') as f:
            json.dump(scene_data, f, ensure_ascii=False, indent=2)

        print("[OK] Test sources configured in scene")

        proc = subprocess.Popen([
            OBS_EXE,
            '--disable-shutdown-check',
            '--minimize-to-tray'
        ], cwd=OBS_DIR)

        print("[INFO] Running OBS Studio for 12 seconds...")
        time.sleep(12)

        proc.terminate()
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            proc.kill()
        print("[OK] OBS execution finished")

        log_files = sorted([os.path.join(LOGS_DIR, f) for f in os.listdir(LOGS_DIR) if f.endswith('.txt')],
                           key=os.path.getmtime, reverse=True)
        latest_log = log_files[0]
        print(f"[INFO] Inspecting log: {latest_log}")

        with open(latest_log, 'r', encoding='utf-8', errors='ignore') as f:
            log_content = f.read()

        debug_lines = [line for line in log_content.splitlines() if "[TextScale Debug]" in line]
        print(f"[INFO] Found {len(debug_lines)} debug log entries:")
        for line in debug_lines[-10:]:
            print("  ", line)

        return len(debug_lines) > 0

    finally:
        with open(SCENE_PATH, 'w', encoding='utf-8') as f:
            f.write(original_content)
        if os.path.exists(BACKUP_PATH):
            os.remove(BACKUP_PATH)
        print("[OK] Cleaned up test scene modifications")

if __name__ == '__main__':
    success = run_functional_matrix()
    sys.exit(0 if success else 1)
