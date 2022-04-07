#[derive(Clone, Debug, Copy)]
struct Color3D {
    r: f32,
    g: f32,
    b: f32
}

impl Color3D {
    pub fn new(r_f32: f32, g_f32: f32, b_f32: f32) -> Color3D {
        Color3D {r: r_f32, g: g_f32, b: b_f32 }
    }
}

#[derive(Clone, Debug, Copy)]
struct Color4D {
    r: f32,
    g: f32,
    b: f32,
    a: f32
}

impl Color4D {
    pub fn new(r_f32: f32, g_f32: f32, b_f32: f32, a_f32: f32) -> Color4D {
        Color4D {r: r_f32, g: g_f32, b: b_f32, a: a_f32 }
    }
}

