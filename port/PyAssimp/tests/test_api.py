import pytest

from pyassimp import material, postprocess, structs
from pyassimp.errors import AssimpError


def test_available_formats(pyassimp):
    assert "OBJ" in pyassimp.available_formats()


def test_load_path(pyassimp, box_obj):
    with pyassimp.load(str(box_obj)) as scene:
        assert len(scene.meshes) == 1
        assert len(scene.meshes[0].vertices) == 24
        assert len(scene.meshes[0].faces) == 12
        assert len(scene.meshes[0].material.properties["diffuse"]) == 3


def test_load_file_object(pyassimp, box_obj):
    with open(box_obj, "rb") as fp:
        with pyassimp.load(fp, file_type="obj") as scene:
            assert len(scene.meshes) == 1


def test_load_missing_file_raises(pyassimp):
    with pytest.raises(AssimpError):
        with pyassimp.load("does-not-exist.obj"):
            pass


def test_process_flags_are_ints():
    assert isinstance(postprocess.aiProcess_DropNormals, int)
    assert isinstance(postprocess.aiProcess_EmbedTextures, int)


def test_texture_type_unknown():
    assert material.aiTextureType_UNKNOWN == 18


def test_camera_layout():
    assert structs.Camera._fields_[-1][0] == "mOrthographicWidth"
