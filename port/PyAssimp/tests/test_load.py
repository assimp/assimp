import pytest

from pyassimp.errors import AssimpError


def test_available_formats(pyassimp):
    assert "OBJ" in pyassimp.available_formats()


def test_load_box_obj(pyassimp, box_obj):
    with pyassimp.load(str(box_obj)) as scene:
        assert len(scene.meshes) == 1
        assert len(scene.meshes[0].vertices) == 24
        assert len(scene.meshes[0].faces) == 12


def test_load_from_file_object(pyassimp, box_obj):
    with open(box_obj, "rb") as fp:
        with pyassimp.load(fp, file_type="obj") as scene:
            assert len(scene.meshes) == 1


def test_load_file_object_requires_file_type(pyassimp, box_obj):
    with open(box_obj, "rb") as fp:
        with pytest.raises(AssimpError):
            with pyassimp.load(fp):
                pass


def test_load_missing_file_raises(pyassimp):
    with pytest.raises(AssimpError):
        with pyassimp.load("does-not-exist.obj"):
            pass


def test_load_without_triangulation(pyassimp, box_obj):
    with pyassimp.load(str(box_obj), processing=0) as scene:
        assert len(scene.meshes[0].faces) == 6
