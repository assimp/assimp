
#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

#[cfg(test)]
mod tests {
    #[test]
    fn import_test() {
        unsafe {
            use crate::aiImportFile;
            let mut file: *mut dyn const i8 = std::ptr::null_mut();
            //let file = String::from("test.obj");
            //let (ptr, len, cap) = file.into_raw_parts();
            //let raw_file = unsafe{String::from_raw_parts}
            let asset = aiImportFile(file, 0);
        }
    }
}
