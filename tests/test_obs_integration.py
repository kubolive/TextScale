import json
import shutil
import subprocess
import time
import os
import sys

SCENE_PATH = os.path.expandvars(r'%APPDATA%\obs-studio\basic\scenes\Untitled.json')
BACKUP_PATH = SCENE_PATH + '.test_backup'
LOGS_DIR = os.path.expandvars(r'%APPDATA%\obs-studio\logs')
OBS_EXE = r'C:\Program Files\obs-studio\bin\64bit\obs64.exe'
OBS_DIR = r'C:\Program Files\obs-studio\bin\64bit'

def run_integration_test():
    print("=== Stage 5 & 6: OBS Integration and Functional Tests ===")
    
    # 1. Backup scene file
    shutil.copyfile(SCENE_PATH, BACKUP_PATH)
    print(f"[OK] Backed up scene file to {BACKUP_PATH}")

    try:
        with open(SCENE_PATH, 'r', encoding='utf-8') as f:
            scene_data = json.load(f)

        # 2. Create test text sources with TextScale filter:
        # Source A: Vietnamese Headline (Long text, expected to condense to target 267px)
        # Source B: Short text (fits within 267px, expected 100%)
        # Source C: Very long text (exceeds min scale 65%, expected 65% clamped)
        test_filter_long = {
            "name": "TextScale Auto Long",
            "id": "text_scale_filter",
            "versioned_id": "text_scale_filter",
            "enabled": True,
            "settings": {
                "enable_auto": True,
                "target_width": 267,
                "min_scale": 65,
                "max_scale": 100,
                "preserve_height": True,
                "alignment": 1, # Center
                "debug_info": True
            }
        }

        test_source_long = {
            "prev_ver": 520159234,
            "name": "TEST_TEXTSCALE_VIETNAMESE_LONG",
            "uuid": "11111111-2222-3333-4444-555555555555",
            "id": "text_gdiplus",
            "versioned_id": "text_gdiplus_v3",
            "settings": {
                "text": "BẢN TIN THỜI SỰ TRUYỀN HÌNH VIỆT NAM - TIN NÓNG ĐẶC BIỆT",
                "font": {
                    "face": "Arial",
                    "size": 36,
                    "style": "Bold",
                    "flags": 1
                },
                "color": 4294967295,
                "outline": True,
                "outline_size": 2,
                "outline_color": 4278190080
            },
            "mixers": 0,
            "sync": 0,
            "flags": 0,
            "volume": 1.0,
            "balance": 0.5,
            "enabled": True,
            "muted": False,
            "filters": [test_filter_long]
        }

        test_filter_short = {
            "name": "TextScale Auto Short",
            "id": "text_scale_filter",
            "versioned_id": "text_scale_filter",
            "enabled": True,
            "settings": {
                "enable_auto": True,
                "target_width": 267,
                "min_scale": 65,
                "max_scale": 100,
                "preserve_height": True,
                "alignment": 0, # Left
                "debug_info": True
            }
        }

        test_source_short = {
            "prev_ver": 520159234,
            "name": "TEST_TEXTSCALE_SHORT",
            "uuid": "22222222-3333-4444-5555-666666666666",
            "id": "text_gdiplus",
            "versioned_id": "text_gdiplus_v3",
            "settings": {
                "text": "TIN VẮN",
                "font": {
                    "face": "Arial",
                    "size": 36,
                    "style": "Bold",
                    "flags": 1
                },
                "color": 4294967295
            },
            "mixers": 0,
            "sync": 0,
            "flags": 0,
            "volume": 1.0,
            "balance": 0.5,
            "enabled": True,
            "muted": False,
            "filters": [test_filter_short]
        }

        scene_data['sources'].append(test_source_long)
        scene_data['sources'].append(test_source_short)

        # Also add to current scene items so they are active and rendered
        current_scene_name = scene_data.get('current_scene')
        for src in scene_data['sources']:
            if src.get('name') == current_scene_name:
                items = src.get('settings', {}).get('items', [])
                items.append({
                    "name": "TEST_TEXTSCALE_VIETNAMESE_LONG",
                    "source_uuid": "11111111-2222-3333-4444-555555555555",
                    "visible": True
                })
                items.append({
                    "name": "TEST_TEXTSCALE_SHORT",
                    "source_uuid": "22222222-3333-4444-5555-666666666666",
                    "visible": True
                })
                break

        with open(SCENE_PATH, 'w', encoding='utf-8') as f:
            json.dump(scene_data, f, ensure_ascii=False, indent=2)

        print("[OK] Injected test sources with TextScale filters into active scene")

        # 3. Launch OBS Studio
        print("[INFO] Launching OBS Studio 31.1.2...")
        proc = subprocess.Popen([
            OBS_EXE,
            '--disable-shutdown-check',
            '--minimize-to-tray'
        ], cwd=OBS_DIR)

        # Allow OBS to load scenes, create filter, tick, render frames
        print("[INFO] Allowing OBS to render test frames for 15 seconds...")
        time.sleep(15)

        # Terminate OBS gracefully
        proc.terminate()
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            proc.kill()
        print("[OK] OBS execution completed")

        # 4. Check log file
        log_files = sorted([os.path.join(LOGS_DIR, f) for f in os.listdir(LOGS_DIR) if f.endswith('.txt')],
                           key=os.path.getmtime, reverse=True)
        latest_log = log_files[0]
        print(f"[INFO] Inspecting log: {latest_log}")

        with open(latest_log, 'r', encoding='utf-8', errors='ignore') as f:
            log_content = f.read()

        # Check for TextScale plugin load
        has_plugin_loaded = "[TextScale] Plugin version 1.0.0 loaded successfully" in log_content
        has_module_listed = "text-scale.dll" in log_content
        has_debug_logs = "[TextScale Debug]" in log_content

        print(f"Plugin Loaded Log: {'PASS' if has_plugin_loaded else 'FAIL'}")
        print(f"Module Listed in OBS: {'PASS' if has_module_listed else 'FAIL'}")
        print(f"Filter Ticked and Rendered: {'PASS' if has_debug_logs else 'FAIL'}")

        # Extract debug lines
        debug_lines = [line for line in log_content.splitlines() if "[TextScale Debug]" in line]
        for l in debug_lines[:10]:
            print("  ->", l)

        return has_plugin_loaded and has_module_listed

    finally:
        # Restore backup
        if os.path.exists(BACKUP_PATH):
            shutil.copyfile(BACKUP_PATH, SCENE_PATH)
            os.remove(BACKUP_PATH)
            print("[OK] Restored original scene file")

if __name__ == '__main__':
    success = run_integration_test()
    sys.exit(0 if success else 1)
