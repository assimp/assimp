def test_available_formats(pyassimp):
    formats = pyassimp.available_formats()
    assert "OBJ" in formats


def test_load_box_obj(pyassimp, box_obj):
    with pyassimp.load(str(box_obj), processing=pyassimp.postprocess.aiProcess_Triangulate) as scene:
        assert len(scene.meshes) == 1
        mesh = scene.meshes[0]
        assert len(mesh.vertices) == 24
        assert len(mesh.faces) == 12
