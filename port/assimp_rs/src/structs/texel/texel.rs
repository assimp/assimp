#[derive(Clone, Debug, Copy)]
struct Texel {
    b: u32,
    g: u32,
    r: u32,
    a: u32
}

impl Texel {
    pub fn new(b_u32: u32, g_u32: u32,
               r_u32: u32, a_u32: u32) -> Texel {
        Texel {
            b: b_u32,
            g: g_u32,
            r: r_u32,
            a: a_u32
        }
    }
}
