#[derive(Clone, Debug, Copy)]
struct Matrix3x3 {
    a1: f32,
    a2: f32,
    a3: f32,
    b1: f32,
    b2: f32,
    b3: f32,
    c1: f32,
    c2: f32,
    c3: f32
}

#[derive(Clone, Debug, Copy)]
struct Matrix4x4 {
    a1: f32,
    a2: f32,
    a3: f32,
    a4: f32,
    b1: f32,
    b2: f32,
    b3: f32,
    b4: f32,
    c1: f32,
    c2: f32,
    c3: f32,
    c4: f32,
    d1: f32,
    d2: f32,
    d3: f32,
    d4: f32
}

impl Matrix3x3 {
    pub fn new(
        a1_f32: f32, a2_f32: f32, a3_f32: f32,
        b1_f32: f32, b2_f32: f32, b3_f32: f32,
        c1_f32: f32, c2_f32: f32, c3_f32: f32
    ) -> Matrix3x3 {
        Matrix3x3 {
            a1: a1_f32, a2: a2_f32, a3: a3_f32,
            b1: b1_f32, b2: b2_f32, b3: b3_f32,
            c1: c1_f32, c2: c2_f32, c3: c3_f32
        }
    }
}

impl Matrix4x4 {
    pub fn new(
        a1_f32: f32, a2_f32: f32, a3_f32: f32, a4_f32: f32,
        b1_f32: f32, b2_f32: f32, b3_f32: f32, b4_f32: f32,
        c1_f32: f32, c2_f32: f32, c3_f32: f32, c4_f32: f32,
        d1_f32: f32, d2_f32: f32, d3_f32: f32, d4_f32: f32
    ) -> Matrix4x4 {
        Matrix4x4 {
            a1: a1_f32, a2: a2_f32, a3: a3_f32, a4: a4_f32,
            b1: b1_f32, b2: b2_f32, b3: b3_f32, b4: b4_f32,
            c1: c1_f32, c2: c2_f32, c3: c3_f32, c4: c4_f32,
            d1: d1_f32, d2: d2_f32, d3: d3_f32, d4: d4_f32 
        }
    }
}


