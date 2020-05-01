#[derive(Clone, Debug, Copy)]
struct MemoryInfo {
    textures: u32,
    materials: u32,
    meshes: u32,
    nodes: u32,
    animations: u32,
    cameras: u32,
    lights: u32,
    total: u32
}

impl MemoryInfo {
    pub fn new(
            textures_uint: u32,
            materials_uint: u32,
            meshes_uint: u32,
            nodes_uint: u32,
            animations_uint: u32,
            cameras_uint: u32,
            lights_uint: u32,
            total_uint: u32) -> MemoryInfo {
        
        MemoryInfo {
            textures: textures_uint,
            materials: materials_uint,
            meshes: meshes_uint,
            nodes: nodes_uint,
            animations: animations_uint,
            cameras: cameras_uint,
            lights: lights_uint,
            total: total_uint
        }
    }
}
