struct Vector2d {
    x: f32,
    y: f32
}

struct Vector3d {
    x: f32,
    y: f32,
    z: f32
}

struct Vector4d {
    x: f32,
    y: f32,
    z: f32,
    w: f32
}

impl Vector2d {
    pub fn new(x_f32: f32, y_f32: f32) -> Vector2d {
        Vector2d {
            x: x_f32,
            y: y_f32
        }
    }
}

impl Vector3d {
    pub fn new(x_f32: f32, y_f32: f32, z_f32: f32) -> Vector3d {
        Vector3d {
            x: x_f32,
            y: y_f32,
            z: z_f32
        }
    }
}

impl Vector4d {
    pub fn new(x_f32: f32, y_f32: f32, z_f32: f32, w_f32: f32) -> Vector4d {
        Vector4d {
            x: x_f32,
            y: y_f32,
            z: z_f32,
            w: w_f32
        }
    }
}

