pub const MAXLEN: usize = 1024;

/// Want to consider replacing `Vec<char>`
/// with a comparable definition at 
/// https://doc.rust-lang.org/src/alloc/string.rs.html#415-417
#[derive(Clone, Debug)]
struct Str {
    length: usize,
    data: Vec<char>
}

impl Str {
    pub fn new(len_u32: usize, data_string: String) -> Str {
        Str {
            length: len_u32,
            data: data_string.chars().collect()
        }
    }
}

/// MaterialPropertyStr
/// The size of length is truncated to 4 bytes on a 64-bit platform when used as a 
/// material property (see MaterialSystem.cpp, as aiMaterial::AddProperty() ).
#[derive(Clone, Debug)]
struct MaterialPropertyStr {
    length: usize,
    data: Vec<char>
}


impl MaterialPropertyStr {
    pub fn new(len_u32: usize, data_string: String) -> MaterialPropertyStr {
        MaterialPropertyStr {
            length: len_u32,
            data: data_string.chars().collect()
        }
    }
}

   

