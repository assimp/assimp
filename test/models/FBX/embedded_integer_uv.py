"""Generate an original minimal FBX 7400 fixture for integer texture UV values.

No data from the issue reporter's model is used. Run from any directory.
The embedded payload is a 1x1 uncompressed 24-bit TGA pixel.
"""

from pathlib import Path
import struct
import copy


def scalar(kind, value):
    return kind.encode() + struct.pack('<' + {'I': 'i', 'L': 'q', 'F': 'f', 'D': 'd'}[kind], value)


def string(value):
    data = value.encode()
    return b'S' + struct.pack('<I', len(data)) + data


def array(kind, values):
    data = struct.pack('<' + {'d': 'd', 'i': 'i'}[kind] * len(values), *values)
    return kind.encode() + struct.pack('<III', len(values), 0, len(data)) + data


def node(name, props=(), children=()):
    return name.encode(), props, children


def encode(entry, start):
    name, props, children = entry
    body = b''.join(props)
    result = name + body
    for child in children:
        result += encode(child, start + 13 + len(result))
    if children:
        result += bytes(13)
    return struct.pack('<IIIB', start + 13 + len(result), len(props), len(body), len(name)) + result


def connection(source, destination, prop=None):
    props = [string('OP' if prop else 'OO'), scalar('L', source), scalar('L', destination)]
    if prop:
        props.append(string(prop))
    return node('C', props)


pixel = bytes([0, 0, 2]) + bytes(9) + bytes([1, 0, 1, 0, 24, 0, 0, 0, 255])
objects = [
    node('Geometry', [scalar('L', 1), string('Triangle\0\1Geometry'), string('Mesh')], [
        node('Vertices', [array('d', [0, 0, 0, 1, 0, 0, 0, 1, 0])]),
        node('PolygonVertexIndex', [array('i', [0, 1, -3])]),
        node('LayerElementMaterial', [scalar('I', 0)], [
            node('MappingInformationType', [string('AllSame')]),
            node('ReferenceInformationType', [string('IndexToDirect')]),
            node('Materials', [array('i', [0])]),
        ]),
        node('Layer', [scalar('I', 0)], [
            node('LayerElement', children=[
                node('Type', [string('LayerElementMaterial')]),
                node('TypedIndex', [scalar('I', 0)]),
            ]),
        ]),
    ]),
    node('Model', [scalar('L', 2), string('Triangle\0\1Model'), string('Mesh')], [
        node('Version', [scalar('I', 232)]),
    ]),
    node('Material', [scalar('L', 3), string('Material\0\1Material'), string('')], [
        node('ShadingModel', [string('phong')]),
    ]),
    node('Texture', [scalar('L', 4), string('Texture\0\1Texture'), string('')], [
        node('Type', [string('TextureVideoClip')]),
        node('FileName', [string('pixel.tga')]),
        node('RelativeFilename', [string('pixel.tga')]),
        node('ModelUVTranslation', [scalar('I', -2), scalar('I', 3)]),
        node('ModelUVScaling', [scalar('I', 4), scalar('I', 5)]),
    ]),
    node('Video', [scalar('L', 5), string('Video\0\1Video'), string('Clip')], [
        node('FileName', [string('pixel.tga')]),
        node('RelativeFilename', [string('pixel.tga')]),
        node('Content', [b'R' + struct.pack('<I', len(pixel)) + pixel]),
    ]),
]
roots = [
    node('FBXHeaderExtension', children=[node('FBXVersion', [scalar('I', 7400)])]),
    node('Objects', children=objects),
    node('Connections', children=[connection(1, 2), connection(2, 0), connection(3, 2),
                                  connection(4, 3, 'DiffuseColor'), connection(5, 4)]),
]
for variant, kind in [('integer', 'I'), ('float', 'F'), ('double', 'D'), ('invalid', 'S'),
                      ('override', 'I')]:
    fixture = copy.deepcopy(roots)
    texture_children = fixture[1][2][3][2]
    texture_children[3] = node('ModelUVTranslation',
                              [string('bad') if kind == 'S' else scalar(kind, value)
                               for value in [-2, 3]])
    texture_children[4] = node('ModelUVScaling', [scalar('I' if kind == 'S' else kind, value)
                                               for value in [4, 5]])
    if variant == 'override':
        texture_children.append(node('Properties70', children=[
            node('P', [string('Translation'), string('Vector3D'), string('Vector'), string(''),
                       scalar('D', 6), scalar('D', 7), scalar('D', 0)]),
            node('P', [string('Scaling'), string('Vector3D'), string('Vector'), string(''),
                       scalar('D', 8), scalar('D', 9), scalar('D', 1)]),
        ]))
    data = b'Kaydara FBX Binary  \0\x1a\0' + struct.pack('<I', 7400)
    for root in fixture:
        data += encode(root, len(data))
    data += bytes(13)
    Path(__file__).with_name(f'embedded_{variant}_uv.fbx').write_bytes(data)
