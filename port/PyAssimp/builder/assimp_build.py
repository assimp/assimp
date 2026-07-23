# © 2025-2026 Kevin G. Schlosser <kevin.g.schlosser@gmail.com>
#
# Builds assimp itself from source (CMake + Ninja) so the pyassimp wheel can
# bundle the resulting shared library directly — there is no prebuilt assimp
# distributed on PyPI, so pyassimp's ctypes bindings (pyassimp.helper's
# search_library()) have nothing to load against unless this package builds
# and ships one itself. Same recipe harness_designer uses to build assimp
# for its own bundled pyassimp, just pointed at this repo's own root instead
# of a git submodule checkout.

import os
import sys

from . import msvc_env, spawn

# builder/ -> PyAssimp -> port -> repo root.
_REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..'))


def _binary_extension():
    if sys.platform.startswith('win'):
        return '.dll'
    if sys.platform.startswith('darwin'):
        return '.dylib'
    return '.so'


def build(debug=False):
    """Configure and build assimp, returning the paths to the compiled
    shared library and any of its platform-specific dependents.
    """
    # cmake/ninja need the MSVC toolchain on PATH to find cl.exe; no-op on
    # non-Windows platforms.
    msvc_env.activate()

    # Without an explicit build type, assimp's CMakeLists defaults to a
    # Debug build — hence the "d" suffix on assimp-vc*-mtd.dll and the
    # multi-hundred-MB .pdb/.ilk files that come with it. debug=True opts
    # back into that for local debugging; everything else gets a proper
    # Release build.
    cmake_build_type = '' if debug else ' -DCMAKE_BUILD_TYPE=Release'
    build_dir = os.path.join(_REPO_ROOT, 'build')
    binary_dir = os.path.join(build_dir, 'bin')

    cmd = (
        f'cmake -G Ninja -DASSIMP_BUILD_TESTS=off -DASSIMP_INSTALL=off{cmake_build_type} '
        f'-S "{_REPO_ROOT}" -B "{build_dir}"&&'
        f'cd "{build_dir}"&&'
        'ninja'
    )

    if sys.platform.startswith('linux'):
        os.environ['CPPFLAGS'] = '-Wno-error=maybe-uninitialized'
        os.environ['CFLAGS'] = '-Wno-error=maybe-uninitialized'
        os.environ['CXXFLAGS'] = '-Wno-error=maybe-uninitialized'

    _, error_text = spawn.spawn(cmd)

    extension = _binary_extension()
    binaries = []
    if os.path.isdir(binary_dir):
        binaries = [
            os.path.join(binary_dir, name)
            for name in os.listdir(binary_dir)
            if name.endswith(extension)
        ]

    # spawn()'s returncode reflects the relay shell's own exit, not
    # necessarily cmake's/ninja's (see builder/spawn.py) — failure is
    # decided from what actually got built instead.
    if not binaries or (error_text and 'warning' not in error_text.lower()):
        raise RuntimeError(f'assimp build failed:\n{error_text}')

    return binaries
