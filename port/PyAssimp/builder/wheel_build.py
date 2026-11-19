# © 2025-2026 Kevin G. Schlosser <kevin.g.schlosser@gmail.com>
#
# Assembles the pyassimp wheel without setuptools: build assimp itself (see
# builder/assimp_build.py), stage pyassimp's pure-Python package plus the
# freshly compiled shared library into a build directory mirroring the final
# layout, then hand the tree to wheel.wheelfile.WheelFile (which computes
# RECORD hashes and writes the archive for us).

import os
import shutil

from . import assimp_build

_PKG_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
# PyAssimp -> port -> repo root — needed for the sdist, which has to carry
# the whole assimp source tree since that's what building this wheel from
# source actually compiles.
_REPO_ROOT = os.path.abspath(os.path.join(_PKG_ROOT, '..', '..'))
_PKG_NAME = 'pyassimp'


def _load_project_metadata():
    try:
        import tomllib
    except ImportError:
        import tomli as tomllib

    with open(os.path.join(_PKG_ROOT, 'pyproject.toml'), 'rb') as f:
        data = tomllib.load(f)

    return data['project']


def _stage_package(build_root, binaries):
    src_root = os.path.join(_PKG_ROOT, _PKG_NAME)
    dst_root = os.path.join(build_root, _PKG_NAME)
    shutil.copytree(src_root, dst_root, ignore=shutil.ignore_patterns('__pycache__'))

    # Upstream's setup.py never shipped the compiled library alongside the
    # package (no package_data entry for it), leaving pyassimp.helper's
    # search_library() to hope the caller's environment already has one.
    # Bundling it here makes the wheel self-contained.
    for binary in binaries:
        shutil.copyfile(binary, os.path.join(dst_root, os.path.basename(binary)))


def _stage_data(build_root, name, version):
    """Reproduce upstream setup.py's data_files: README.rst under
    share/pyassimp, and every file in scripts/ under share/examples/pyassimp.
    """
    data_root = os.path.join(build_root, f'{name}-{version}.data', 'data')

    readme_dst_dir = os.path.join(data_root, 'share', 'pyassimp')
    os.makedirs(readme_dst_dir, exist_ok=True)
    shutil.copyfile(
        os.path.join(_PKG_ROOT, 'README.rst'),
        os.path.join(readme_dst_dir, 'README.rst'),
    )

    scripts_src = os.path.join(_PKG_ROOT, 'scripts')
    scripts_dst = os.path.join(data_root, 'share', 'examples', 'pyassimp')
    os.makedirs(scripts_dst, exist_ok=True)
    for file_name in os.listdir(scripts_src):
        shutil.copyfile(os.path.join(scripts_src, file_name), os.path.join(scripts_dst, file_name))


def _wheel_tag():
    # pyassimp's own code is pure Python (ctypes bindings, no C extension
    # module), so the wheel isn't tied to a Python ABI — only to the
    # platform the bundled assimp shared library was compiled for.
    import packaging.tags

    platform_tag = next(packaging.tags.platform_tags())
    return f'py3-none-{platform_tag}'


def _write_metadata(dist_info_dir, project):
    license_ = project.get('license')
    # SPDX license-expression metadata (PEP 639) needs Core Metadata 2.4;
    # everything else this writes is valid back to 2.1.
    metadata_version = '2.4' if isinstance(license_, str) else '2.1'

    lines = [
        f'Metadata-Version: {metadata_version}',
        f'Name: {project["name"]}',
        f'Version: {project["version"]}',
    ]
    if isinstance(license_, str):
        lines.append(f'License-Expression: {license_}')
    if project.get('description'):
        lines.append(f'Summary: {project["description"]}')
    if project.get('readme'):
        lines.append('Description-Content-Type: text/markdown')
    if project.get('requires-python'):
        lines.append(f'Requires-Python: {project["requires-python"]}')
    for author in project.get('authors', []):
        if author.get('name'):
            lines.append(f'Author: {author["name"]}')
    for maintainer in project.get('maintainers', []):
        if maintainer.get('name'):
            lines.append(f'Maintainer: {maintainer["name"]}')
    for url_name, url in project.get('urls', {}).items():
        lines.append(f'Project-URL: {url_name}, {url}')
    for dep in project.get('dependencies', []):
        lines.append(f'Requires-Dist: {dep}')

    body = ''
    readme = project.get('readme')
    if readme:
        readme_path = os.path.join(_PKG_ROOT, readme)
        if os.path.exists(readme_path):
            with open(readme_path, encoding='utf-8') as f:
                body = f.read()

    content = '\n'.join(lines) + '\n\n' + body
    with open(os.path.join(dist_info_dir, 'METADATA'), 'w', encoding='utf-8') as f:
        f.write(content)


def _write_wheel_file(dist_info_dir, tag):
    content = (
        'Wheel-Version: 1.0\n'
        'Generator: pyassimp_backend\n'
        'Root-Is-Purelib: false\n'
        f'Tag: {tag}\n'
    )
    with open(os.path.join(dist_info_dir, 'WHEEL'), 'w', encoding='utf-8') as f:
        f.write(content)


def build_wheel(wheel_directory, config_settings=None):
    config_settings = config_settings or {}
    debug = str(config_settings.get('debug', 'false')).lower() == 'true'

    project = _load_project_metadata()
    name = project['name']
    version = project['version']

    binaries = assimp_build.build(debug=debug)

    build_root = os.path.join(_PKG_ROOT, 'build', 'wheel')
    if os.path.exists(build_root):
        shutil.rmtree(build_root)
    os.makedirs(build_root)

    _stage_package(build_root, binaries)
    _stage_data(build_root, name, version)

    dist_info_dir = os.path.join(build_root, f'{name}-{version}.dist-info')
    os.makedirs(dist_info_dir)
    _write_metadata(dist_info_dir, project)
    tag = _wheel_tag()
    _write_wheel_file(dist_info_dir, tag)

    wheel_name = f'{name}-{version}-{tag}.whl'
    wheel_path = os.path.join(wheel_directory, wheel_name)

    from wheel.wheelfile import WheelFile
    with WheelFile(wheel_path, 'w') as wf:
        wf.write_files(build_root)

    return wheel_name


def build_sdist(sdist_directory, config_settings=None):
    import subprocess
    import tarfile

    project = _load_project_metadata()
    prefix = f'{project["name"]}-{project["version"]}'

    try:
        result = subprocess.run(
            ['git', 'ls-files'], cwd=_REPO_ROOT, check=True,
            capture_output=True, text=True,
        )
        tracked = result.stdout.splitlines()
    except (subprocess.CalledProcessError, FileNotFoundError):
        tracked = []
        for root, dirs, files in os.walk(_REPO_ROOT):
            dirs[:] = [d for d in dirs if d not in ('.git', '__pycache__', 'build')]
            for file_name in files:
                tracked.append(os.path.relpath(os.path.join(root, file_name), _REPO_ROOT))

    sdist_name = f'{prefix}.tar.gz'
    sdist_path = os.path.join(sdist_directory, sdist_name)

    with tarfile.open(sdist_path, 'w:gz') as tar:
        for rel_path in tracked:
            tar.add(
                os.path.join(_REPO_ROOT, rel_path),
                arcname=os.path.join(prefix, rel_path),
            )

    return sdist_name
