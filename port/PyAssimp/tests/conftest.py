from pathlib import Path

import pytest

REPO_ROOT = Path(__file__).resolve().parents[3]
MODELS = REPO_ROOT / "test" / "models"


@pytest.fixture(scope="session")
def pyassimp():
    try:
        import pyassimp
    except Exception as exc:
        pytest.skip(f"pyassimp unavailable: {exc}")
    return pyassimp


@pytest.fixture
def box_obj():
    path = MODELS / "OBJ" / "box.obj"
    if not path.is_file():
        pytest.skip(f"test model missing: {path}")
    return path
