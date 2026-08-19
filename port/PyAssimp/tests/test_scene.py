def test_node_mesh_resolution(pyassimp, box_obj):
    with pyassimp.load(str(box_obj)) as scene:
        assert scene.rootnode
        mesh = scene.meshes[0]
        assert any(m is mesh for m in (n for node in scene.rootnode.children for n in node.meshes)) or any(
            m is mesh for m in scene.rootnode.meshes
        )


def test_material_resolution(pyassimp, box_obj):
    with pyassimp.load(str(box_obj)) as scene:
        material = scene.meshes[0].material
        assert len(material.properties["diffuse"]) == 3
