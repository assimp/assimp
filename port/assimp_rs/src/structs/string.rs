pub const MAXLEN: u32 = 1024;

#[derive(Clone, Debug)]
struct Str {
    length: u32,
    data: Vec<char>
}

impl Str {
    pub fn new(len_u32: u32, data_string: String) -> Str {
        Str {
            length: len_u32,
            data: data_string.chars().collect()
        }
    }
}

