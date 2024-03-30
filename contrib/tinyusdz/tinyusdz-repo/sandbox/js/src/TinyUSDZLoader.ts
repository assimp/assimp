import * as THREE from 'three';

export class TinyUSDZLoader {

  //private tusdzModule: 

  //constructor(
}

class TextureResolver {
  filename: string;
  loader: THREE.TextureLoader;

  constructor(filename: string) {
    this.filename = filename;
    this.loader = THREE.TextureLoader();
  }

}

class TydraMesh {

  _geometry: THREE.BufferGeometry;
  _mesh: THREE.Mesh;
  _id: number;
  _interface: object;
  _points: Float32Array | null;
  _normals: Float32Array | null;
  _colors: Float32Array | null;
  _uvs: Float32Array | null;
  _indices: Uint32Array | null;

  constructor(id: number, tydraInterface: object, root: object) {
    this._geometry = new THREE.BufferGeometry();
    this._id = id;
    this._interface = tydraInterface;
    this._points = null;
    this._normals = null;

    // Vertex colors
    this._colors = null;
    // TEXCOORD_0 only
    this._uvs = null;
    this._indices = null;

    const material = new THREE.MeshPhysicalMaterial({
      side: THREE.DoubleSize,
      color: new THREE.Color(0x00ff00), // a green color to indicate a missing material
    });

    this._mesh = new THREE.Mesh(this._geometry, material);

    this._mesh.castShadow = true;
    this._mesh.receiveShadow = true;

    root.add(this._mesh);
  }

}

window.bora = 3;
console.log(window);
console.log(window.bora);
