#!/usr/bin/env python3
"""
Generate CMakePresets.json for different compilers and platforms.

Usage:
    python generate_cmake_presets.py --compiler {gcc,clang,msvc,clang-cl} [options]

Options:
    --compiler          Compiler type: gcc, clang, msvc, clang-cl (required)
    --platform          Target platform: windows, linux, macos (default: auto-detect)
    --msvc-toolset      MSVC toolset version: v145 (VS2026) v143 (VS2022) or v142 (VS2019) (default: v145)
    --generator         CMake generator (default: Ninja)
    --output            Output file path (default: CMakePresets.json)
    --arch              Target architecture (default: x64)
"""

import argparse
import json
import sys
import platform as sys_platform

# ---------- Helper functions ----------
def detect_platform():
    system = sys_platform.system().lower()
    if system == "windows":
        return "windows"
    elif system == "linux":
        return "linux"
    elif system == "darwin":
        return "macos"
    else:
        return "linux"  # fallback

def get_compiler_exe(compiler, platform):
    """Return (C compiler, CXX compiler) executable names."""
    if compiler == "gcc":
        if platform == "windows":
            # Assume MinGW gcc/g++ are in PATH (or use explicit names)
            return ("gcc.exe", "g++.exe")
        else:
            return ("gcc", "g++")
    elif compiler == "clang":
        if platform == "windows":
            # Clang for Windows (not clang-cl) – typically clang.exe, clang++.exe
            return ("clang.exe", "clang++.exe")
        else:
            return ("clang", "clang++")
    elif compiler == "msvc":
        # MSVC cl.exe – no suffix needed, CMake finds it via VS env
        return ("cl.exe", "cl.exe")
    elif compiler == "clang-cl":
        # clang-cl.exe
        return ("clang-cl.exe", "clang-cl.exe")
    else:
        raise ValueError(f"Unknown compiler: {compiler}")

def get_toolset(compiler, msvc_toolset):
    """Return toolset dict or None."""
    if compiler == "msvc":
        return {"value": msvc_toolset, "strategy": "external"}
    elif compiler == "clang-cl":
        return {"value": "ClangCL", "strategy": "external"}
    else:
        # For gcc/clang, toolset is usually not needed
        return None

def get_condition(platform):
    """Return condition dict for hostSystemName."""
    if platform == "windows":
        rhs = "Windows"
    elif platform == "linux":
        rhs = "Linux"
    elif platform == "macos":
        rhs = "Darwin"
    else:
        rhs = "Linux"
    return {
        "type": "equals",
        "lhs": "${hostSystemName}",
        "rhs": rhs
    }

def get_vendor(compiler, platform, arch):
    """Return vendor dict (only for Windows MSVC-style compilers)."""
    if platform != "windows":
        return None
    if compiler in ("msvc", "clang-cl"):
        if compiler == "msvc":
            intelliSenseMode = f"windows-msvc-{arch}"
        else:  # clang-cl
            intelliSenseMode = f"windows-clang-{arch}"
        return {
            "microsoft.com/VisualStudioSettings/CMake/1.0": {
                "intelliSenseMode": intelliSenseMode,
                "someCustomFlag": True
            }
        }
    return None

def get_runtime_library(compiler, platform):
    """Return CMAKE_MSVC_RUNTIME_LIBRARY value or None."""
    if platform == "windows" and compiler in ("msvc", "clang-cl"):
        return "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
    return None

def get_flags(compiler, platform, config):
    """Return compiler flags for given configuration (Debug/RelWithDebInfo/Release)."""
    # Common flag sets per compiler family
    if compiler in ("msvc", "clang-cl"):
        # MSVC style flags
        flags = {
            "Debug": "/Od /Zi /MDd",
            "RelWithDebInfo": "/O2 /Zi /MD",
            "Release": "/O2 /MD"
        }
    else:
        # GCC / Clang style flags
        flags = {
            "Debug": "-O0 -g",
            "RelWithDebInfo": "-O2 -g",
            "Release": "-O2"
        }
    return flags.get(config, "")

def get_cache_variables(compiler, platform, msvc_toolset, arch, generator):
    """Build the cacheVariables dict for a base preset."""
    c_compiler, cxx_compiler = get_compiler_exe(compiler, platform)
    cache_vars = {
        "CMAKE_C_COMPILER": c_compiler,
        "CMAKE_CXX_COMPILER": cxx_compiler,
        "CMAKE_CONFIGURATION_TYPES": "Debug;RelWithDebInfo;Release",
    }
    # Runtime library setting (MSVC style)
    runtime_lib = get_runtime_library(compiler, platform)
    if runtime_lib:
        cache_vars["CMAKE_MSVC_RUNTIME_LIBRARY"] = runtime_lib

    # For single-config generators like Ninja, we need to set CMAKE_BUILD_TYPE in each preset,
    # but base preset can omit it. We'll set it in derived presets.
    # Optionally set architecture hint for MSVC with Ninja
    return cache_vars

def generate_presets(compiler, platform, msvc_toolset, generator, output_file, arch):
    """Generate and write CMakePresets.json."""
    # Validate compiler
    if compiler not in ("gcc", "clang", "msvc", "clang-cl"):
        raise ValueError("Compiler must be one of: gcc, clang, msvc, clang-cl")
    if compiler in ("msvc", "clang-cl") and platform != "windows":
        print(f"Warning: {compiler} is typically used on Windows, but platform is {platform}. Continuing anyway.", file=sys.stderr)

    # Base preset name
    base_name = f"{compiler}-base"
    # Unique preset names for configurations
    configs = ["debug", "relwithdebinfo", "release"]
    build_type_map = {
        "debug": "Debug",
        "relwithdebinfo": "RelWithDebInfo",
        "release": "Release"
    }
    env_map = {
        "debug": "debug",
        "relwithdebinfo": "relwithdebinfo",
        "release": "release"
    }

    # Common base data
    base_toolset = get_toolset(compiler, msvc_toolset)
    base_condition = get_condition(platform)
    base_vendor = get_vendor(compiler, platform, arch)
    base_cache_vars = get_cache_variables(compiler, platform, msvc_toolset, arch, generator)

    # Build base preset
    base_preset = {
        "name": base_name,
        "hidden": True,
        "displayName": f"Base Configuration for {compiler}",
        "description": f"Base preset for {compiler} on {platform}",
        "generator": generator,
        "binaryDir": "${sourceDir}/out/build/${presetName}",
        "installDir": "${sourceDir}/out/install/${presetName}",
        "cacheVariables": base_cache_vars,
        "condition": base_condition
    }
    if base_toolset:
        base_preset["toolset"] = base_toolset
    if base_vendor:
        base_preset["vendor"] = base_vendor
    # Architecture (only needed for MSVC-style with Ninja; for other platforms it's optional)
    if platform == "windows" and generator == "Ninja" and compiler in ("msvc", "clang-cl"):
        base_preset["architecture"] = {"value": arch, "strategy": "external"}

    # Build derived presets
    derived_presets = []
    for cfg in configs:
        build_type = build_type_map[cfg]
        preset_name = f"{compiler}_{arch}_{cfg}"
        flags_var = f"CMAKE_CXX_FLAGS_{build_type.upper()}"
        flags = get_flags(compiler, platform, build_type)
        cache_vars = {
            "CMAKE_BUILD_TYPE": build_type,
            flags_var: flags
        }
        derived = {
            "name": preset_name,
            "inherits": base_name,
            "displayName": f"{arch} {build_type} {compiler}",
            "description": f"{build_type} build for {compiler} on {platform}",
            "cacheVariables": cache_vars,
            "environment": {
                "BUILD_MODE": env_map[cfg]
            }
        }
        derived_presets.append(derived)

    # Final JSON structure
    cmake_presets = {
        "version": 9,
        "cmakeMinimumRequired": {
            "major": 3,
            "minor": 28,
            "patch": 0
        },
        "configurePresets": [base_preset] + derived_presets
    }

    # Write to file
    with open(output_file, "w") as f:
        json.dump(cmake_presets, f, indent=2)
    print(f"Generated {output_file} for compiler={compiler}, platform={platform}, generator={generator}")

def main():
    parser = argparse.ArgumentParser(description="Generate CMakePresets.json for various compilers.")
    parser.add_argument("--compiler", required=True, choices=["gcc", "clang", "msvc", "clang-cl"],
                        help="Compiler type")
    parser.add_argument("--platform", choices=["windows", "linux", "macos"],
                        default=detect_platform(), help="Target platform (auto-detected if omitted)")
    parser.add_argument("--msvc-toolset", default="v145", choices=["v142", "v143", "v145"],
                        help="MSVC toolset version (only used for msvc compiler)")
    parser.add_argument("--generator", default="Ninja", help="CMake generator (default: Ninja)")
    parser.add_argument("--output", default="CMakePresets.json", help="Output file path")
    parser.add_argument("--arch", default="x64", help="Target architecture (e.g., x64, x86, arm64)")
    args = parser.parse_args()

    try:
        generate_presets(
            compiler=args.compiler,
            platform=args.platform,
            msvc_toolset=args.msvc_toolset,
            generator=args.generator,
            output_file=args.output,
            arch=args.arch
        )
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()