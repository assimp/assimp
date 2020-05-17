#[derive(Clone, Debug, Copy)]
struct Plane {
    a: f32,
    b: f32,
    c: f32,
    d: f32
}

impl Plane {
    pub fn new(
        a_f32: f32,
        b_f32: f32,
        c_f32: f32,
        d_f32: f32
    ) -> Plane {
        Plane {
            a: a_f32,
            b: b_f32,
            c: b_f32,
            d: d_f32
        }
    }
}
