#!/usr/bin/env python3
# cubeMX_dualcore_patch.py
#
# Usage (from project root):
#   python cubeMX_dualcore_patch.py
#
# This script patches a CubeMX dual-core project to your C++ app_main workflow.

from __future__ import annotations
import json
import os
import re
import shutil
from pathlib import Path
from typing import Optional, Tuple

PROJECT_ROOT = Path.cwd()
ENABLE_BACKUPS = False
# -----------------------------
# Helpers
# -----------------------------


def patch_main_c(main_c: Path) -> bool:
    """
    Patch CubeMX main.c to support C++ app_main() entry.

    Actions:
    1. Verify file exists
    2. Comment out DUAL_CORE_BOOT_SYNC_SEQUENCE if present
    3. Ensure app_main() declaration exists
    4. Avoid duplicate insertion
    """

    if not main_c.exists():
        print(f"[ERROR] main.c not found: {main_c}")
        return False

    backup_file(main_c)

    src = read_text(main_c)
    original = src
    changed = False

    # ------------------------------------------------
    # Check that this looks like a CubeMX main.c
    # ------------------------------------------------

    if "HAL_Init()" not in src or "SystemClock_Config" not in src:
        print("[WARN] File does not look like CubeMX main.c")

    # ------------------------------------------------
    # Disable DUAL_CORE_BOOT_SYNC_SEQUENCE
    # ------------------------------------------------

    pattern = r"^[ \t]*#define\s+DUAL_CORE_BOOT_SYNC_SEQUENCE"
    match = re.search(pattern, src, re.MULTILINE)

    if match:
        src = re.sub(
            pattern,
            "// #define DUAL_CORE_BOOT_SYNC_SEQUENCE",
            src,
            count=1,
            flags=re.MULTILINE,
        )
        print("[OK] Disabled DUAL_CORE_BOOT_SYNC_SEQUENCE")
        changed = True

    # ------------------------------------------------
    # Ensure app_main declaration exists
    # ------------------------------------------------

    if "void app_main(void);" not in src:

        extern_block = (
            "\n#ifdef __cplusplus\n"
            'extern "C" {\n'
            "#endif\n"
            "void app_main(void);\n"
            "#ifdef __cplusplus\n"
            "}\n"
            "#endif\n"
        )

        pattern = r'#include\s+"main\.h"'

        if re.search(pattern, src):

            src = re.sub(pattern, '#include "main.h"' + extern_block, src, count=1)

            print("[OK] app_main declaration inserted")
            changed = True

        else:
            print('[WARN] Could not locate #include "main.h"')

    else:
        print("[OK] app_main already declared")

    # ------------------------------------------------
    # Write file only if modified
    # ------------------------------------------------

    if src != original:
        write_text(main_c, src)
        print("[OK] main.c patched")
        return True

    print("[OK] main.c already correct")
    return False


def copy_default_app(cm7_core_dir: Path, force: bool = False) -> bool:
    """
    Copy template App folder into CM7/Core/App.

    Source:
        ../App_default/App

    Destination:
        CM7/Core/App

    Args:
        cm7_core_dir : Path to CM7/Core
        force        : If True, overwrite existing App folder

    Returns:
        True if copy occurred
        False otherwise
    """

    parent = PROJECT_ROOT.parent
    source_app = parent / "App_default" / "App"
    dest_app = cm7_core_dir / "App"

    # ------------------------------------------------
    # Validate source
    # ------------------------------------------------

    if not source_app.exists():
        print(f"[ERROR] Template App folder not found: {source_app}")
        return False

    if not source_app.is_dir():
        print(f"[ERROR] Template App path is not a directory: {source_app}")
        return False

    # ------------------------------------------------
    # Validate destination root
    # ------------------------------------------------

    if not cm7_core_dir.exists():
        print(f"[ERROR] CM7/Core directory not found: {cm7_core_dir}")
        return False

    if not cm7_core_dir.is_dir():
        print(f"[ERROR] Invalid CM7/Core path: {cm7_core_dir}")
        return False

    # ------------------------------------------------
    # Handle existing destination
    # ------------------------------------------------

    if dest_app.exists():

        if not dest_app.is_dir():
            print(f"[ERROR] Destination exists but is not a directory: {dest_app}")
            return False

        if not force:
            print(f"[SKIP] App folder already exists: {dest_app}")
            print("       Use force=True to overwrite.")
            return False

        try:
            shutil.rmtree(dest_app)
            print(f"[INFO] Removed existing App folder")
        except Exception as e:
            print(f"[ERROR] Failed to remove existing App folder: {e}")
            return False

    # ------------------------------------------------
    # Perform copy
    # ------------------------------------------------

    try:
        shutil.copytree(source_app, dest_app)
    except Exception as e:
        print(f"[ERROR] Failed to copy App folder: {e}")
        return False

    print(f"[OK] Template App copied to: {dest_app}")

    return True


def replace_linker_script(force: bool = True) -> bool:
    """
    Replace CM7 linker script with template from App_default.

    Source:
        ../App_default/stm32h755xx_flash_CM7.ld

    Destination:
        CM7/stm32h755xx_flash_CM7.ld
    """

    parent = PROJECT_ROOT.parent

    source_linker = parent / "App_default" / "stm32h755xx_flash_CM7.ld"
    dest_linker = PROJECT_ROOT / "CM7" / "stm32h755xx_flash_CM7.ld"

    # ------------------------------------------------
    # Validate source
    # ------------------------------------------------

    if not source_linker.exists():
        print(f"[ERROR] Linker template not found: {source_linker}")
        return False

    if not source_linker.is_file():
        print(f"[ERROR] Template is not a file: {source_linker}")
        return False

    # ------------------------------------------------
    # Validate destination directory
    # ------------------------------------------------

    if not dest_linker.parent.exists():
        print(f"[ERROR] CM7 directory not found: {dest_linker.parent}")
        return False

    # ------------------------------------------------
    # Handle existing linker
    # ------------------------------------------------

    if dest_linker.exists():

        if not force:
            print(f"[SKIP] Linker already exists: {dest_linker}")
            return False

        try:
            dest_linker.unlink()
            print("[INFO] Existing linker removed")
        except Exception as e:
            print(f"[ERROR] Failed to remove existing linker: {e}")
            return False

    # ------------------------------------------------
    # Copy linker
    # ------------------------------------------------

    try:
        shutil.copy2(source_linker, dest_linker)
    except Exception as e:
        print(f"[ERROR] Failed to copy linker script: {e}")
        return False

    print(f"[OK] Linker script installed -> {dest_linker}")

    return True


def patch_root_cmakelists(root_cmake: Path) -> bool:
    """
    Ensure the root CMakeLists.txt supports C++.

    Adds:
        project(${CMAKE_PROJECT_NAME} C CXX ASM)
        set(CMAKE_CXX_STANDARD 17)
        set(CMAKE_CXX_STANDARD_REQUIRED ON)
    """

    # ----------------------------------------
    # Validate file
    # ----------------------------------------

    if not root_cmake.exists():
        print(f"[ERROR] Root CMakeLists.txt not found: {root_cmake}")
        return False

    backup_file(root_cmake)

    src = read_text(root_cmake)
    original = src
    changed = False

    # ----------------------------------------
    # Check if patch already applied
    # ----------------------------------------

    has_cpp_project = "project(${CMAKE_PROJECT_NAME} C CXX ASM)" in src
    has_cpp_std = "CMAKE_CXX_STANDARD" in src

    if has_cpp_project and has_cpp_std:
        print("[OK] Root CMake already configured for C++")
        return False

    # ----------------------------------------
    # Locate insertion point
    # ----------------------------------------

    pattern = r"set\s*\(\s*CMAKE_PROJECT_NAME\s+[^\)]+\)"

    match = re.search(pattern, src)

    if not match:
        print("[WARN] Could not locate CMAKE_PROJECT_NAME in CMakeLists")
        return False

    insert_block = (
        "\n\n# Enable C, C++, and ASM\n"
        "project(${CMAKE_PROJECT_NAME} C CXX ASM)\n\n"
        "# Set C++ standard\n"
        "set(CMAKE_CXX_STANDARD 17)\n"
        "set(CMAKE_CXX_STANDARD_REQUIRED ON)\n"
    )

    src = src.replace(match.group(0), match.group(0) + insert_block)

    changed = True

    # ----------------------------------------
    # Write file if modified
    # ----------------------------------------

    if src != original:
        write_text(root_cmake, src)
        print("[OK] Root CMakeLists patched for C++ support")
        return True

    return False


def patch_cm7_cmake(cm7_cmake: Path) -> bool:
    """
    Patch CM7 CMakeLists.txt to support:
    - C++ compilation
    - app.cpp source
    - Core/App include path
    - printf float support
    """

    if not cm7_cmake.exists():
        print(f"[ERROR] CM7 CMakeLists not found: {cm7_cmake}")
        return False

    backup_file(cm7_cmake)

    src = read_text(cm7_cmake)
    original = src
    changed = False

    # ------------------------------------------------
    # Enable C++ in project()
    # ------------------------------------------------

    if "project(${CMAKE_PROJECT_NAME} C CXX ASM)" not in src:
        src = re.sub(
            r"project\s*\(\s*\$\{CMAKE_PROJECT_NAME\}\s*\)",
            "project(${CMAKE_PROJECT_NAME} C CXX ASM)",
            src,
            count=1,
        )
        print("[OK] Enabled C++ in project()")
        changed = True

    # ------------------------------------------------
    # Enable C++ language
    # ------------------------------------------------

    if "enable_language(C CXX ASM)" not in src:
        src = src.replace(
            "enable_language(C ASM)",
            "enable_language(C CXX ASM)",
        )
        print("[OK] Enabled CXX language")
        changed = True

    # ------------------------------------------------
    # Add C++ standard
    # ------------------------------------------------

    if "CMAKE_CXX_STANDARD" not in src:
        insert = (
            "\nset(CMAKE_CXX_STANDARD 17)\n" "set(CMAKE_CXX_STANDARD_REQUIRED ON)\n"
        )
        src += insert
        print("[OK] Added C++17 standard")
        changed = True

    # ------------------------------------------------
    # Ensure Core/App include path
    # ------------------------------------------------

    if "Core/App" not in src:
        src = src.replace(
            "target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE",
            "target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE\n"
            "    ${CMAKE_CURRENT_SOURCE_DIR}/Core/App",
        )
        print("[OK] Added Core/App include path")
        changed = True

    # ------------------------------------------------
    # Ensure app.cpp is compiled
    # ------------------------------------------------

    if "Core/App/app.cpp" not in src:
        src = src.replace(
            "target_sources(${CMAKE_PROJECT_NAME} PRIVATE",
            "target_sources(${CMAKE_PROJECT_NAME} PRIVATE\n"
            "    ${CMAKE_CURRENT_SOURCE_DIR}/Core/App/app.cpp",
        )
        print("[OK] Added app.cpp source")
        changed = True

    # ------------------------------------------------
    # Enable printf float support
    # ------------------------------------------------

    if "_printf_float" not in src:
        src += (
            "\n# Enable float support in printf\n"
            "target_link_options(${CMAKE_PROJECT_NAME} PRIVATE -u _printf_float)\n"
        )
        print("[OK] Enabled printf float support")
        changed = True

    # ------------------------------------------------
    # Write file
    # ------------------------------------------------

    if src != original:
        write_text(cm7_cmake, src)
        return True

    print("[OK] CM7 CMake already patched")
    return False


def copy_and_patch_vscode(vscode_dir: Path, build_dir: Path) -> bool:
    """
    Install VSCode debug configuration and patch ELF path.
    """

    parent = PROJECT_ROOT.parent
    template_dir = parent / "App_default"

    source_tasks = template_dir / "tasks.json"
    source_launch = template_dir / "launch.json"

    dest_tasks = vscode_dir / "tasks.json"
    dest_launch = vscode_dir / "launch.json"

    # ------------------------------------------------
    # Validate templates
    # ------------------------------------------------

    if not source_tasks.exists():
        print(f"[ERROR] Missing template: {source_tasks}")
        return False

    if not source_launch.exists():
        print(f"[ERROR] Missing template: {source_launch}")
        return False

    # ------------------------------------------------
    # Detect ELF
    # ------------------------------------------------

    elf_name = detect_first_cm7_elf(build_dir)

    if elf_name is None:
        print("[WARN] No *_CM7.elf found in CM7/build/")
        print("       Build the project once then run the script again.")
        return False

    elf_path = f"${{workspaceFolder}}/CM7/build/{elf_name}"

    # ------------------------------------------------
    # Create .vscode folder
    # ------------------------------------------------

    vscode_dir.mkdir(parents=True, exist_ok=True)

    # ------------------------------------------------
    # Copy templates
    # ------------------------------------------------

    shutil.copy2(source_tasks, dest_tasks)
    shutil.copy2(source_launch, dest_launch)

    # ------------------------------------------------
    # Patch tasks.json
    # ------------------------------------------------

    try:
        tasks_data = json.loads(dest_tasks.read_text(encoding="utf-8"))

        for task in tasks_data.get("tasks", []):
            if task.get("label") == "Flash CM7":
                args = task.get("args", [])
                for i, arg in enumerate(args):
                    if isinstance(arg, str) and arg.endswith(".elf"):
                        args[i] = elf_path

        dest_tasks.write_text(json.dumps(tasks_data, indent=2), encoding="utf-8")

    except Exception as e:
        print(f"[ERROR] Failed to patch tasks.json: {e}")
        return False

    # ------------------------------------------------
    # Patch launch.json
    # ------------------------------------------------

    try:
        launch_data = json.loads(dest_launch.read_text(encoding="utf-8"))

        for cfg in launch_data.get("configurations", []):
            for img in cfg.get("imagesAndSymbols", []):
                if "imageFileName" in img:
                    img["imageFileName"] = elf_path

        dest_launch.write_text(json.dumps(launch_data, indent=2), encoding="utf-8")

    except Exception as e:
        print(f"[ERROR] Failed to patch launch.json: {e}")
        return False

    print(f"[OK] VSCode configured with ELF: {elf_name}")

    return True


def insert_app_main_after_leds(main_c: Path) -> bool:
    """
    Insert app_main() after BSP LED initialization block.
    """

    if not main_c.exists():
        print(f"[ERROR] main.c not found: {main_c}")
        return False

    src = read_text(main_c)

    if "app_main();" in src:
        print("[OK] app_main() already present")
        return False

    pattern = (
        r"BSP_LED_On\(LED_GREEN\);\s*\n"
        r"\s*BSP_LED_On\(LED_YELLOW\);\s*\n"
        r"\s*BSP_LED_On\(LED_RED\);"
    )

    match = re.search(pattern, src)

    if not match:
        print("[WARN] LED initialization block not found")
        return False

    insert_pos = match.end()

    src = src[:insert_pos] + "\n  app_main();" + src[insert_pos:]

    write_text(main_c, src)

    print("[OK] app_main() inserted after LED initialization")

    return True


def disable_button_led_demo(main_c: Path) -> bool:
    """
    Comment CubeMX button demo using // and insert HAL_Delay(500).
    """

    if not main_c.exists():
        print(f"[ERROR] main.c not found: {main_c}")
        return False

    src = read_text(main_c)

    if "HAL_Delay(500);" in src:
        print("[OK] Button demo already patched")
        return False

    pattern = r"""
if\s*\(BspButtonState\s*==\s*BUTTON_PRESSED\)\s*
\{
.*?
BSP_LED_Toggle\(LED_RED\);\s*
.*?
\}
"""

    match = re.search(pattern, src, re.DOTALL | re.VERBOSE)

    if not match:
        print("[WARN] Button demo block not found")
        return False

    block = match.group(0)

    # Comment every line
    commented_lines = "\n".join("// " + line for line in block.splitlines())

    insert = commented_lines + "\n  HAL_Delay(500);"

    src = src.replace(block, insert, 1)

    write_text(main_c, src)

    print("[OK] Button demo disabled and delay inserted")

    return True


def backup_file(p: Path) -> None:
    if not ENABLE_BACKUPS:
        return
    if not p.exists():
        return
    bak = p.with_suffix(p.suffix + ".bak")
    if not bak.exists():
        shutil.copy2(p, bak)


def read_text(p: Path) -> str:
    return p.read_text(encoding="utf-8", errors="ignore")


def write_text(p: Path, s: str) -> None:
    p.parent.mkdir(parents=True, exist_ok=True)
    p.write_text(s, encoding="utf-8", newline="\n")


def replace_once(src: str, pattern: str, repl: str, flags=0) -> Tuple[str, bool]:
    m = re.search(pattern, src, flags)
    if not m:
        return src, False
    out = (
        src[: m.start()]
        + re.sub(pattern, repl, src[m.start() : m.end()], flags=flags)
        + src[m.end() :]
    )
    # The above re.sub on the match slice is safe but a bit odd; do simpler:
    out = re.sub(pattern, repl, src, count=1, flags=flags)
    return out, True


def ensure_line_commented_define(src: str, define_name: str) -> Tuple[str, bool]:
    # Match line with optional spaces then #define DUAL_CORE_BOOT_SYNC_SEQUENCE
    pat = rf"(?m)^[ \t]*#define[ \t]+{re.escape(define_name)}[ \t]*$"
    if re.search(pat, src):
        # Replace with commented version (single //)
        out = re.sub(pat, rf"// #define {define_name}", src, count=1)
        return out, True
    # If already commented, do nothing
    pat2 = rf"(?m)^[ \t]*//[ \t]*#define[ \t]+{re.escape(define_name)}[ \t]*$"
    if re.search(pat2, src):
        return src, False
    return src, False


def detect_first_cm7_elf(build_dir: Path) -> Optional[str]:
    if not build_dir.exists():
        return None
    # First file that ends with _CM7.elf
    for p in sorted(build_dir.glob("*_CM7.elf")):
        return str(p.name)
    return None


# -----------------------------
# Step 2: Create app.cpp/hpp
# -----------------------------

APP_HPP_TEMPLATE = r"""#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void app_main(void);

#ifdef __cplusplus
}
#endif
"""

APP_CPP_TEMPLATE = r"""#include "app.hpp"
#include <cstdio>

extern "C" void app_main(void)
{
    // TODO: replace with your real app
    std::printf("Hello from app_main() (CM7)\r\n");
}
"""


# -----------------------------
# Step 5: CM7/CMakeLists patch
# -----------------------------


# -----------------------------
# .vscode launch/tasks
# -----------------------------


# def write_launch_json(vscode_dir: Path, elf_rel_path: str) -> None:
#     launch = {
#         "version": "0.2.0",
#         "configurations": [
#             {
#                 "type": "stlinkgdbtarget",
#                 "request": "launch",
#                 "name": "STM32 CM7 Debug",
#                 "cwd": "${workspaceFolder}",
#                 "deviceCore": "Cortex-M7",
#                 "runEntry": "main",
#                 "imagesAndSymbols": [{"imageFileName": elf_rel_path}],
#             }
#         ],
#     }
#     write_text(vscode_dir / "launch.json", json.dumps(launch, indent=2))


# def write_tasks_json(vscode_dir: Path, elf_rel_path: str) -> None:
#     # NOTE: command assumes STM32_Programmer_CLI.exe is in PATH, same as your setup.
#     tasks = {
#         "version": "2.0.0",
#         "tasks": [
#             {
#                 "label": "Flash CM7",
#                 "type": "shell",
#                 "command": "STM32_Programmer_CLI.exe",
#                 "args": ["-c", "port=SWD", "-w", elf_rel_path, "-rst"],
#                 "group": {"kind": "build", "isDefault": True},
#                 "problemMatcher": [],
#             },
#             {
#                 "label": "Build CM7",
#                 "type": "shell",
#                 "command": "echo Building...",
#                 "dependsOn": "CMake: build",
#                 "problemMatcher": [],
#             },
#             {
#                 "label": "Build + Flash CM7",
#                 "dependsOrder": "sequence",
#                 "dependsOn": ["Build CM7", "Flash CM7"],
#             },
#         ],
#     }
#     write_text(vscode_dir / "tasks.json", json.dumps(tasks, indent=2))


# -----------------------------
# Main
# -----------------------------


def main() -> None:
    cm7_main = PROJECT_ROOT / "CM7" / "Core" / "Src" / "main.c"
    cm4_main = PROJECT_ROOT / "CM4" / "Core" / "Src" / "main.c"
    root_cmake = PROJECT_ROOT / "CMakeLists.txt"
    cm7_cmake = PROJECT_ROOT / "CM7" / "CMakeLists.txt"
    cm7_mxgen = PROJECT_ROOT / "CM7" / "mx-generated.cmake"
    cm7_core_dir = PROJECT_ROOT / "CM7" / "Core"
    vscode_dir = PROJECT_ROOT / ".vscode"
    cm7_build_dir = PROJECT_ROOT / "CM7" / "build"

    print("== CubeMX Dual-Core Patch ==")

    # 1) comment DUAL_CORE_BOOT_SYNC_SEQUENCE + 3) add app_main call
    print("[1/8] Patching main.c (CM7/CM4)...")
    ch1 = patch_main_c(cm7_main)
    ch2 = patch_main_c(cm4_main)

    # 2) create app files
    print("[2/8] Copying default App folder ...")
    copy_default_app(cm7_core_dir, force=True)

    print("[3/8] Replacing linker script ...")
    replace_linker_script(force=True)

    # 4) root CMakeLists
    print("[4/8] Patching root CMakeLists.txt ...")
    ch3 = patch_root_cmakelists(root_cmake)

    # 5) CM7 CMakeLists
    print("[5/8] Patching CM7/CMakeLists.txt ...")
    ch4 = patch_cm7_cmake(cm7_cmake)

    # print("[7/8] Copying and patching VSCode config ...")
    copy_and_patch_vscode(vscode_dir, cm7_build_dir)

    # print("[8/8] Inserting app_main() after BSP block ...")
    insert_app_main_after_leds(cm7_main)

    # print("[9/9] Disabling CubeMX button LED demo ...")
    disable_button_led_demo(cm7_main)

    print("\nDone.")


if __name__ == "__main__":
    main()
