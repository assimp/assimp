import os.path
import unittest

import pyassimp

# Find the root path of the test file so we can find the
# test models above our directory
HERE = os.path.abspath(os.path.dirname(__file__))
MODELS_DIR = os.path.abspath(os.path.join(HERE, '..', '..', 'test', 'models'))
TEST_SKINNED_MODEL = os.path.join(MODELS_DIR, 'glTF2', 'simple_skin', 'simple_skin.gltf')
TEST_COLLADA = os.path.join(MODELS_DIR, 'Collada', 'COLLADA.dae')


class PyAssimpTests(unittest.TestCase):
    def test_skinned(self):
        with pyassimp.load_scoped(TEST_SKINNED_MODEL) as scene:
            self.assertIsNotNone(scene.rootnode)
            self.assertEqual(len(scene.meshes), 1)
            bone_names = [b.name for b in scene.meshes[0].bones]
            self.assertEqual(["nodes_1", "nodes_2"], bone_names)

    def test_collada_parses(self):
        with pyassimp.load_scoped(TEST_COLLADA) as scene:
            self.assertIsNotNone(scene.rootnode)

    def test_regular_load(self):
        scene = None
        try:
            scene = pyassimp.load(TEST_SKINNED_MODEL)
            self.assertIsNotNone(scene.rootnode)
        finally:
            if scene:
                pyassimp.release(scene)


if __name__ == "__main__":
    unittest.main()
