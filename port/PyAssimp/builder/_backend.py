# © 2025-2026 Kevin G. Schlosser <kevin.g.schlosser@gmail.com>
#
# PEP 517 build backend for pyassimp — no setuptools anywhere. There is no
# prebuilt assimp on PyPI, so this backend builds assimp itself from source
# (see builder/assimp_build.py) and bundles the resulting shared library
# into the wheel. See builder/wheel_build.py for the actual wheel/sdist
# assembly.
#
# Toggle: pip install . --config-settings debug=true builds assimp in Debug
# mode instead of Release (for local debugging).

from . import wheel_build


def get_requires_for_build_wheel(config_settings=None):
    return []


def get_requires_for_build_sdist(config_settings=None):
    return []


def build_wheel(wheel_directory, config_settings=None, metadata_directory=None):
    return wheel_build.build_wheel(wheel_directory, config_settings)


def build_sdist(sdist_directory, config_settings=None):
    return wheel_build.build_sdist(sdist_directory, config_settings)
