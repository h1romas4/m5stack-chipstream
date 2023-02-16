use esp_idf_sys as _; // If using the `libstart` feature of `esp-idf-sys`, always keep this module imported

#[no_mangle]
extern "C" fn rust_chipstream_test() -> i32 {
    println!("Hello world from Rust!");

    unsafe {
        ymfm_add_chip(0, 0);
        ymfm_remove_chip(0);
    }

    42
}

#[link(name = "ymfm")]
extern "C" {
    fn ymfm_add_chip(chip_num: u16, clock: u32) -> u32;
    fn ymfm_write(chip_num: u16, index: u16, reg: u32, data: u8);
    fn ymfm_generate(chip_num: u16, index: u16, buffer: *const i32);
    fn ymfm_remove_chip(chip_num: u16);
    // void ymfm_add_rom_data(uint16_t chip_num, uint16_t access_type, uint8_t *buffer, uint32_t length, uint32_t start_address)
    fn ymfm_add_rom_data(
        chip_num: u16,
        access_type: u16,
        buffer: *const u8,
        length: u32,
        start_address: u32,
    );
}
