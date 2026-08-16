# © 2025-2026 Kevin G. Schlosser <kevin.g.schlosser@gmail.com>

import sys


def activate():
    """Locate and activate the MSVC toolchain for the current process.

    No-op on non-Windows platforms. Must run before anything in this process
    invokes a native compiler or build tool (cmake/ninja for the assimp
    build) — pyMSVC mutates os.environ (PATH/INCLUDE/LIB), so it only
    affects the process it runs in, not sibling CI steps.
    """
    if not sys.platform.startswith('win'):
        return

    import ctypes
    import comtypes._safearray  # NOQA

    # pyMSVC's comtypes usage expects VARIANT_BOOL as an attribute that some
    # comtypes versions don't define — patch it in before pyMSVC imports it.
    setattr(comtypes._safearray, 'VARIANT_BOOL', ctypes.c_short)  # NOQA

    import pyMSVC

    env = pyMSVC.setup_environment()
    print(env)
    print()
