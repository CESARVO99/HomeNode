"""
PlatformIO extra_script: Adds WinLibs GCC to PATH for native builds.
This script is executed before the build starts and ensures that the
GCC compiler from WinLibs (installed via winget) is available.
"""
import os
from pathlib import Path

# WinLibs common install path via winget
winget_pkg = Path.home() / "AppData" / "Local" / "Microsoft" / "WinGet" / "Packages"
if winget_pkg.exists():
    for pkg_dir in winget_pkg.iterdir():
        if "WinLibs" in pkg_dir.name:
            gcc_bin = pkg_dir / "mingw64" / "bin"
            if (gcc_bin / "gcc.exe").exists():
                current_path = os.environ.get("PATH", "")
                gcc_bin_str = str(gcc_bin)
                if gcc_bin_str not in current_path:
                    os.environ["PATH"] = gcc_bin_str + os.pathsep + current_path
                    print(f"  [add_gcc_path] Added to PATH: {gcc_bin_str}")
                break
