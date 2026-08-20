from pyassimp import material, postprocess, structs


def test_process_flags_are_ints():
    assert isinstance(postprocess.aiProcess_Triangulate, int)
    assert isinstance(postprocess.aiProcess_DropNormals, int)
    assert isinstance(postprocess.aiProcess_EmbedTextures, int)
    assert (
        postprocess.aiProcess_Triangulate | postprocess.aiProcess_JoinIdenticalVertices
    ) == (
        postprocess.aiProcess_Triangulate + postprocess.aiProcess_JoinIdenticalVertices
    )


def test_texture_type_values():
    assert material.aiTextureType_DIFFUSE == 1
    assert material.aiTextureType_UNKNOWN == 18
    assert material.aiTextureType_BASE_COLOR == 12
    assert material.aiTextureType_GLTF_METALLIC_ROUGHNESS == 27


def test_camera_has_orthographic_width():
    names = [name for name, _ in structs.Camera._fields_]
    assert names[-1] == "mOrthographicWidth"
