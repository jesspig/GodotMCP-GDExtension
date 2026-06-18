import os
import platform
import shutil
from pathlib import Path

def detect_environment(compiler_hint=None, generator_hint=None, cache_hint=None) -> dict:
    """Detect the best available toolchain and map to CMakePreset name.
    
    Args:
        compiler_hint: None (auto) or one of 'msvc', 'clang-cl', 'gcc', 'clang', 'appleclang', 'mingw'
        generator_hint: None (auto) or one of 'ninja', 'make', 'vs'
        cache_hint: None (auto) or one of 'sccache', 'ccache', 'off'
    
    Returns:
        dict with keys: selected_preset, vs_env, compiler_id, generator, cache_name
    
    Hints can also be set via environment variables:
        GODOTMCP_COMPILER_HINT, GODOTMCP_GENERATOR_HINT, GODOTMCP_CACHE_HINT
    """
    # Read env var fallbacks if hints not passed directly
    if compiler_hint is None:
        compiler_hint = os.environ.get("GODOTMCP_COMPILER_HINT")
    if generator_hint is None:
        generator_hint = os.environ.get("GODOTMCP_GENERATOR_HINT")
    if cache_hint is None:
        env_cache = os.environ.get("GODOTMCP_CACHE_HINT")
        if env_cache in ("auto", "sccache", "ccache", "off"):
            cache_hint = env_cache

    system = platform.system()
    selected_preset = "default"
    vs_env = None
    compiler_id = "unknown"
    generator = "ninja"
    cache_name = None

    # --- Generator detection ---
    ninja_available = shutil.which("ninja") is not None
    make_available = shutil.which("make") is not None or shutil.which("gmake") is not None
    
    if generator_hint == "ninja" and not ninja_available:
        generator_hint = None
    if generator_hint == "make" and not make_available:
        generator_hint = None
    
    if generator_hint:
        generator = generator_hint
    elif ninja_available:
        generator = "ninja"
    elif make_available:
        generator = "make"
    else:
        generator = "default"

    compiler_path = None

    # --- Compiler detection ---
    if system == "Windows":
        from scripts._msvc import find_available_compilers, capture_vs_dev_env
        
        if compiler_hint:
            if compiler_hint == "clang-cl":
                from scripts._msvc import find_clang_cl
                compiler_path = find_clang_cl()
                if compiler_path:
                    compiler_id = "clang-cl"
                    vs_env = capture_vs_dev_env()
                else:
                    compiler_id = "unknown"
            elif compiler_hint == "msvc":
                from scripts._msvc import find_msvc_cl
                compiler_path = find_msvc_cl()
                if compiler_path:
                    compiler_id = "msvc"
                    vs_env = capture_vs_dev_env()
                else:
                    compiler_id = "unknown"
            elif compiler_hint == "mingw":
                from scripts._mingw import find_mingw_gcc
                mingw = find_mingw_gcc()
                if mingw:
                    compiler_id = "mingw"
                    compiler_path = mingw[0]["path"]
                else:
                    compiler_id = "unknown"
            else:
                compiler_id = compiler_hint
        else:
            compilers = find_available_compilers()
            if compilers:
                best = compilers[0]
                compiler_path = best["path"]
                if best["type"] == "clang":
                    compiler_id = "clang-cl"
                    vs_env = capture_vs_dev_env()
                elif best["type"] == "msvc":
                    compiler_id = "msvc"
                    vs_env = capture_vs_dev_env()
            else:
                from scripts._mingw import find_mingw_gcc
                mingw = find_mingw_gcc()
                if mingw:
                    compiler_id = "mingw"
                    compiler_path = mingw[0]["path"]
                else:
                    compiler_id = "unknown"
    elif system == "Linux":
        # Linux: prefer GCC, fallback Clang
        if compiler_hint == "clang":
            from scripts._linux import find_clang_versions
            clang_list = find_clang_versions()
            if clang_list:
                compiler_id = "clang"
            else:
                compiler_id = "unknown"
        else:
            from scripts._linux import find_gcc_versions, find_clang_versions
            gcc_list = find_gcc_versions()
            if gcc_list:
                compiler_id = "gcc"
            else:
                clang_list = find_clang_versions()
                if clang_list:
                    compiler_id = "clang"
                else:
                    compiler_id = "unknown"
    elif system == "Darwin":
        # macOS: prefer AppleClang, fallback Homebrew GCC/Clang
        if compiler_hint == "gcc":
            from scripts._macos import find_homebrew_gcc
            gcc_list = find_homebrew_gcc()
            compiler_id = "gcc" if gcc_list else "unknown"
        elif compiler_hint == "clang":
            from scripts._macos import find_homebrew_clang
            clang_list = find_homebrew_clang()
            compiler_id = "clang" if clang_list else "unknown"
        else:
            from scripts._macos import find_apple_clang
            apple = find_apple_clang()
            compiler_id = "appleclang" if apple else "unknown"

    # Inject compiler bin dir into PATH so CMake/Ninja can find it
    if compiler_path and vs_env:
        compiler_bin = str(Path(compiler_path).parent)
        path_entries = [compiler_bin]
        existing_path = vs_env.get("PATH", "")
        if compiler_bin.lower() not in existing_path.lower():
            vs_env["PATH"] = ";".join(path_entries + [existing_path])

    # --- Map to preset ---
    if system == "Windows":
        if compiler_id == "clang-cl":
            selected_preset = "win-clang-cl"
        elif compiler_id == "msvc":
            if generator == "ninja":
                selected_preset = "win-msvc"
            else:
                selected_preset = "win-vs"
        elif compiler_id == "mingw":
            selected_preset = "win-mingw"
            generator = "ninja"
        else:
            selected_preset = "win-vs"
    elif system == "Linux":
        if compiler_id == "clang":
            selected_preset = "linux-clang"
        else:
            selected_preset = "linux-gcc"
    elif system == "Darwin":
        selected_preset = "macos-clang"
    else:
        selected_preset = "default"
    
    # selected_preset is the prefix without build type suffix
    # Caller appends -debug or -release based on CMAKE_BUILD_TYPE

    # --- Cache detection ---
    if cache_hint == "off":
        cache_name = None
    elif cache_hint == "sccache":
        cache_name = "sccache" if shutil.which("sccache") else None
    elif cache_hint == "ccache":
        if compiler_id == "msvc":
            cache_name = None
        else:
            cache_name = "ccache" if shutil.which("ccache") else None
    else:
        sccache = shutil.which("sccache")
        if sccache:
            cache_name = "sccache"
        elif compiler_id != "msvc":
            ccache = shutil.which("ccache")
            cache_name = "ccache" if ccache else None

    return {
        "selected_preset": selected_preset,
        "vs_env": vs_env,
        "compiler_id": compiler_id,
        "generator": generator,
        "cache_name": cache_name,
    }
