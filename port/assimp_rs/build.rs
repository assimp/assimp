extern crate bindgen;
use std::env;
use std::path::PathBuf;

fn main() {
    // Rerun if wrapper header changes
    println!("cargo:rerun-if-changed=./src/assimp_wrapper.h");

    // Tell cargo to look for shared libraries in the specified directory
    println!("cargo:rustc-link-search=../../lib/");

    // Tell cargo to tell rustc to link the assimp shared library.
    println!("cargo:rustc-link-lib=assimp");

    // The bindgen::Builder is the main entry point
    // to bindgen, and lets you build up options for
    // the resulting bindings.
    let bindings = bindgen::Builder::default()
        // The input header we would like to generate
        // bindings for.
        .header("./src/assimp_wrapper.h").clang_arg("-I../../include/")
        // Tell cargo to invalidate the built crate whenever any of the
        // included header files changed.
        .parse_callbacks(Box::new(bindgen::CargoCallbacks::new()))
        .blocklist_item("FP_ZERO")
        .blocklist_item("FP_SUBNORMAL")
        .blocklist_item("FP_NORMAL")
        .blocklist_item("FP_INFINITE")
        .blocklist_item("FP_NAN")
        // Finish the builder and generate the bindings.
        .generate()
        // Unwrap the Result and panic on failure.
        .expect("Unable to generate bindings");

    // Write the bindings to the $OUT_DIR/bindings.rs file.
    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings!");
}
