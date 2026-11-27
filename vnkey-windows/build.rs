fn main() {
    println!("cargo:rerun-if-changed=resources.rc");
    println!("cargo:rerun-if-changed=vnkey.manifest");
    println!("cargo:rerun-if-changed=v.ico");
    println!("cargo:rerun-if-changed=e.ico");
    let _ = embed_resource::compile("resources.rc", embed_resource::NONE);
}
