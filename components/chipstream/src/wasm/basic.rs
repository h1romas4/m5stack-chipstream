// license:BSD-3-Clause
// copyright-holders:Hiromasa Tanaka
use std::cell::RefCell;
use std::rc::Rc;

use crate::{
    driver::{self, VgmPlay, XgmPlay},
    sound::{RomBusType, RomIndex, SoundChipType, SoundSlot},
};

///
/// for WebAssembly Instance on thread-local
///
type VgmPlayBank = Rc<RefCell<Vec<VgmPlay>>>;
std::thread_local!(static VGM_PLAY: VgmPlayBank = {
    Rc::new(RefCell::new(Vec::new()))
});

type XgmPlayBank = Rc<RefCell<Vec<XgmPlay>>>;
std::thread_local!(static XGM_PLAY: XgmPlayBank = {
    Rc::new(RefCell::new(Vec::new()))
});

type SoundSlotBank = Rc<RefCell<Vec<SoundSlot>>>;
std::thread_local!(static SOUND_SLOT: SoundSlotBank = {
    Rc::new(RefCell::new(Vec::new()))
});

type MemoryBank = Rc<RefCell<Vec<Vec<u8>>>>;
std::thread_local!(static MEMORY: MemoryBank = {
    Rc::new(RefCell::new(Vec::new()))
});

///
/// Get thread local value Utility
///
fn get_vgm_bank() -> VgmPlayBank {
    VGM_PLAY.with(|rc| rc.clone())
}

fn get_xgm_bank() -> XgmPlayBank {
    XGM_PLAY.with(|rc| rc.clone())
}

fn get_sound_slot_bank() -> SoundSlotBank {
    SOUND_SLOT.with(|rc| rc.clone())
}

fn get_memory_bank() -> MemoryBank {
    MEMORY.with(|rc| rc.clone())
}

///
/// WebAssembly basic interfaces
///
#[no_mangle]
pub extern "C" fn memory_alloc(memory_index_id: u32, length: u32) {
    get_memory_bank()
        .borrow_mut()
        .insert(memory_index_id as usize, vec![0; length as usize]);
}

#[no_mangle]
pub extern "C" fn memory_get_alloc_len() -> u32 {
    get_memory_bank().borrow_mut().len() as u32
}

#[no_mangle]
pub extern "C" fn memory_get_ref(memory_index_id: u32) -> *mut u8 {
    get_memory_bank()
        .borrow_mut()
        .get_mut(memory_index_id as usize)
        .unwrap()
        .as_mut_ptr()
}

#[no_mangle]
pub extern "C" fn memory_get_len(memory_index_id: u32) -> u32 {
    get_memory_bank()
        .borrow_mut()
        .get_mut(memory_index_id as usize)
        .unwrap()
        .len() as u32
}

#[no_mangle]
pub extern "C" fn memory_drop(memory_index_id: u32) {
    get_memory_bank()
        .borrow_mut()
        .remove(memory_index_id as usize);
}

#[no_mangle]
pub extern "C" fn vgm_create(
    vgm_index_id: u32,
    output_sampling_rate: u32,
    output_sample_chunk_size: u32,
    memory_index_id: u32,
) -> bool {
    let vgmplay = VgmPlay::new(
        SoundSlot::new(
            driver::VGM_TICK_RATE,
            output_sampling_rate,
            output_sample_chunk_size as usize,
        ),
        get_memory_bank()
            .borrow_mut()
            .get(memory_index_id as usize)
            .unwrap(),
    );
    if vgmplay.is_err() {
        return false;
    }
    get_vgm_bank()
        .borrow_mut()
        .insert(vgm_index_id as usize, vgmplay.unwrap());
    true
}

#[no_mangle]
pub extern "C" fn xgm_create(
    xgm_index_id: u32,
    output_sampling_rate: u32,
    output_sample_chunk_size: u32,
    memory_index_id: u32,
) -> bool {
    let xgmplay = XgmPlay::new(
        SoundSlot::new(
            driver::XGM_NTSC_TICK_RATE,
            output_sampling_rate,
            output_sample_chunk_size as usize,
        ),
        get_memory_bank()
            .borrow_mut()
            .get(memory_index_id as usize)
            .unwrap(),
    );
    if xgmplay.is_err() {
        return false;
    }
    get_xgm_bank()
        .borrow_mut()
        .insert(xgm_index_id as usize, xgmplay.unwrap());
    true
}

#[no_mangle]
pub extern "C" fn sound_slot_create(
    sound_slot_index_id: u32,
    external_tick_rate: u32,
    output_sampling_rate: u32,
    output_sample_chunk_size: u32,
) {
    get_sound_slot_bank().borrow_mut().insert(
        sound_slot_index_id as usize,
        SoundSlot::new(
            external_tick_rate,
            output_sampling_rate,
            output_sample_chunk_size as usize,
        ),
    );
}

#[no_mangle]
pub extern "C" fn sound_slot_add_sound_device(
    sounde_slot_index: u32,
    sound_chip_type: u32,
    number_of: u32,
    clock: u32,
) {
    get_sound_slot_bank()
        .borrow_mut()
        .get_mut(sounde_slot_index as usize)
        .unwrap()
        .add_sound_device(
            get_sound_chip_type(sound_chip_type),
            number_of as usize,
            clock,
        )
}

#[no_mangle]
pub extern "C" fn sound_slot_write(
    sounde_slot_index: u32,
    sound_chip_type: u32,
    sound_chip_index: u32,
    port: u32,
    data: u32,
) {
    get_sound_slot_bank()
        .borrow_mut()
        .get_mut(sounde_slot_index as usize)
        .unwrap()
        .write(
            get_sound_chip_type(sound_chip_type),
            sound_chip_index as usize,
            port,
            data,
        )
}

#[no_mangle]
pub extern "C" fn sound_slot_update(sounde_slot_index: u32, tick_count: u32) {
    get_sound_slot_bank()
        .borrow_mut()
        .get_mut(sounde_slot_index as usize)
        .unwrap()
        .update(tick_count as usize)
}

#[no_mangle]
pub extern "C" fn sound_slot_is_stream_filled(sounde_slot_index: u32) -> u32 {
    get_sound_slot_bank()
        .borrow_mut()
        .get_mut(sounde_slot_index as usize)
        .unwrap()
        .is_stream_filled()
        .into()
}

#[no_mangle]
pub extern "C" fn sound_slot_stream(sounde_slot_index: u32) {
    get_sound_slot_bank()
        .borrow_mut()
        .get_mut(sounde_slot_index as usize)
        .unwrap()
        .stream()
}

#[no_mangle]
pub extern "C" fn sound_slot_sampling_l_ref(sounde_slot_index: u32) -> *const f32 {
    get_sound_slot_bank()
        .borrow_mut()
        .get_mut(sounde_slot_index as usize)
        .unwrap()
        .get_output_sampling_l_ref()
}

#[no_mangle]
pub extern "C" fn sound_slot_sampling_r_ref(sounde_slot_index: u32) -> *const f32 {
    get_sound_slot_bank()
        .borrow_mut()
        .get_mut(sounde_slot_index as usize)
        .unwrap()
        .get_output_sampling_r_ref()
}

#[no_mangle]
pub extern "C" fn sound_slot_sampling_s16le_ref(sounde_slot_index: u32) -> *const i16 {
    get_sound_slot_bank()
        .borrow_mut()
        .get_mut(sounde_slot_index as usize)
        .unwrap()
        .get_output_sampling_s16le_ref()
}

#[no_mangle]
pub extern "C" fn sound_slot_set_output_level_rate(
    sounde_slot_index: u32,
    sound_chip_type: u32,
    sound_chip_index: u32,
    channel: u32,
    output_level_rate: f32,
) {
    get_sound_slot_bank()
        .borrow_mut()
        .get_mut(sounde_slot_index as usize)
        .unwrap()
        .set_output_level_rate(
            get_sound_chip_type(sound_chip_type),
            sound_chip_index as usize,
            channel as usize,
            output_level_rate,
        );
}

#[no_mangle]
pub extern "C" fn sound_slot_add_rom(
    sounde_slot_index: u32,
    sound_chip_type: u32,
    sound_chip_index: u32,
    rom_index: u32,
    memory_index_id: u32,
    start_address: u32,
    end_address: u32,
) {
    let rom_index = get_rom_index(rom_index);
    get_sound_slot_bank()
        .borrow_mut()
        .get_mut(sounde_slot_index as usize)
        .unwrap()
        .add_rom(
            get_sound_chip_type(sound_chip_type),
            sound_chip_index as usize,
            rom_index,
            get_memory_bank()
                .borrow_mut()
                .get(memory_index_id as usize)
                .unwrap(),
            start_address as usize,
            end_address as usize,
        );
}

#[no_mangle]
pub extern "C" fn sound_slot_set_rom_bus_type(
    sounde_slot_index: u32,
    sound_chip_type: u32,
    sound_chip_index: u32,
    rom_index: u32,
    rom_bus_type: u32,
) {
    let rom_index = get_rom_index(rom_index);
    let rom_bus_type = get_rom_bus_type(rom_bus_type);
    get_sound_slot_bank()
        .borrow_mut()
        .get_mut(sounde_slot_index as usize)
        .unwrap()
        .set_rom_bus_type(
            get_sound_chip_type(sound_chip_type),
            sound_chip_index as usize,
            rom_index,
            rom_bus_type,
        );
}

#[no_mangle]
pub extern "C" fn sound_slot_add_data_block(
    sounde_slot_index: u32,
    data_block_id: u32,
    memory_index_id: u32,
) {
    get_sound_slot_bank()
        .borrow_mut()
        .get_mut(sounde_slot_index as usize)
        .unwrap()
        .add_data_block(
            data_block_id as usize,
            get_memory_bank()
                .borrow_mut()
                .get(memory_index_id as usize)
                .unwrap(),
        );
}

#[no_mangle]
pub extern "C" fn sound_slot_get_data_block(
    sounde_slot_index: u32,
    data_block_id: u32,
) -> *const u8 {
    get_sound_slot_bank()
        .borrow_mut()
        .get_mut(sounde_slot_index as usize)
        .unwrap()
        .get_data_block(data_block_id as usize)
        .as_ptr()
}

#[no_mangle]
pub extern "C" fn sound_slot_add_data_stream(
    sounde_slot_index: u32,
    sound_chip_type: u32,
    sound_chip_index: u32,
    data_stream_id: u32,
    write_port: u32,
) {
    get_sound_slot_bank()
        .borrow_mut()
        .get_mut(sounde_slot_index as usize)
        .unwrap()
        .add_data_stream(
            get_sound_chip_type(sound_chip_type),
            sound_chip_index as usize,
            sound_chip_index as usize,
            data_stream_id,
            write_port,
        );
}

#[no_mangle]
pub extern "C" fn sound_slot_set_data_stream_frequency(
    sounde_slot_index: u32,
    sound_chip_type: u32,
    sound_chip_index: u32,
    data_stream_id: u32,
    frequency: u32,
) {
    get_sound_slot_bank()
        .borrow_mut()
        .get_mut(sounde_slot_index as usize)
        .unwrap()
        .set_data_stream_frequency(
            get_sound_chip_type(sound_chip_type),
            sound_chip_index as usize,
            data_stream_id as usize,
            frequency,
        );
}

#[no_mangle]
pub extern "C" fn sound_slot_attach_data_block_to_stream(
    sounde_slot_index: u32,
    sound_chip_type: u32,
    sound_chip_index: u32,
    data_stream_id: u32,
    data_block_id: u32,
) {
    get_sound_slot_bank()
        .borrow_mut()
        .get_mut(sounde_slot_index as usize)
        .unwrap()
        .attach_data_block_to_stream(
            get_sound_chip_type(sound_chip_type),
            sound_chip_index as usize,
            data_stream_id as usize,
            data_block_id as usize,
        );
}

#[no_mangle]
pub extern "C" fn sound_slot_start_data_stream(
    sounde_slot_index: u32,
    sound_chip_type: u32,
    sound_chip_index: u32,
    data_stream_id: u32,
    data_block_start_offset: u32,
    data_block_length: u32,
) {
    get_sound_slot_bank()
        .borrow_mut()
        .get_mut(sounde_slot_index as usize)
        .unwrap()
        .start_data_stream(
            get_sound_chip_type(sound_chip_type),
            sound_chip_index as usize,
            data_stream_id as usize,
            data_block_start_offset as usize,
            data_block_length as usize,
        );
}

#[no_mangle]
pub extern "C" fn sound_slot_start_data_stream_fast(
    sounde_slot_index: u32,
    sound_chip_type: u32,
    sound_chip_index: u32,
    data_stream_id: u32,
    data_block_id: u32,
) {
    get_sound_slot_bank()
        .borrow_mut()
        .get_mut(sounde_slot_index as usize)
        .unwrap()
        .start_data_stream_fast(
            get_sound_chip_type(sound_chip_type),
            sound_chip_index as usize,
            data_stream_id as usize,
            data_block_id as usize,
        );
}

#[no_mangle]
pub extern "C" fn sound_slot_stop_data_stream(
    sounde_slot_index: u32,
    sound_chip_type: u32,
    sound_chip_index: u32,
    data_stream_id: u32,
) {
    get_sound_slot_bank()
        .borrow_mut()
        .get_mut(sounde_slot_index as usize)
        .unwrap()
        .stop_data_stream(
            get_sound_chip_type(sound_chip_type),
            sound_chip_index as usize,
            data_stream_id as usize,
        );
}

#[no_mangle]
pub extern "C" fn sound_slot_is_stop_data_stream(
    sounde_slot_index: u32,
    sound_chip_type: u32,
    sound_chip_index: u32,
    data_stream_id: u32,
) -> bool {
    get_sound_slot_bank()
        .borrow_mut()
        .get_mut(sounde_slot_index as usize)
        .unwrap()
        .is_stop_data_stream(
            get_sound_chip_type(sound_chip_type),
            sound_chip_index as usize,
            data_stream_id as usize,
        )
}

#[no_mangle]
pub extern "C" fn vgm_get_sampling_l_ref(vgm_index_id: u32) -> *const f32 {
    get_vgm_bank()
        .borrow_mut()
        .get_mut(vgm_index_id as usize)
        .unwrap()
        .get_sampling_l_ref()
}

#[no_mangle]
pub extern "C" fn vgm_get_sampling_r_ref(vgm_index_id: u32) -> *const f32 {
    get_vgm_bank()
        .borrow_mut()
        .get_mut(vgm_index_id as usize)
        .unwrap()
        .get_sampling_r_ref()
}

#[no_mangle]
pub extern "C" fn vgm_get_sampling_s16le_ref(vgm_index_id: u32) -> *const i16 {
    get_vgm_bank()
        .borrow_mut()
        .get_mut(vgm_index_id as usize)
        .unwrap()
        .get_output_sampling_s16le_ref()
}

#[no_mangle]
pub extern "C" fn vgm_get_sampling_s16le(vgm_index_id: u32, s16le: *mut i16) {
    get_vgm_bank()
        .borrow_mut()
        .get_mut(vgm_index_id as usize)
        .unwrap()
        .get_output_sampling_s16le(s16le);
}

#[no_mangle]
pub extern "C" fn vgm_get_header_json(vgm_index_id: u32) -> u32 {
    let json = get_vgm_bank()
        .borrow_mut()
        .get_mut(vgm_index_id as usize)
        .unwrap()
        .get_vgm_header_json();
    // UTF-8 json into allocate memory
    let memory_index_id = memory_get_alloc_len();
    get_memory_bank()
        .borrow_mut()
        .insert(memory_index_id as usize, json.into_bytes());
    // return memory index id
    memory_index_id
}

#[no_mangle]
pub extern "C" fn vgm_get_gd3_json(vgm_index_id: u32) -> u32 {
    let json = get_vgm_bank()
        .borrow_mut()
        .get_mut(vgm_index_id as usize)
        .unwrap()
        .get_vgm_gd3_json();
    // UTF-8 json into allocate memory
    let memory_index_id = memory_get_alloc_len();
    get_memory_bank()
        .borrow_mut()
        .insert(memory_index_id as usize, json.into_bytes());
    // return memory index id
    memory_index_id
}

#[no_mangle]
pub extern "C" fn xgm_get_sampling_l_ref(xgm_index_id: u32) -> *const f32 {
    get_xgm_bank()
        .borrow_mut()
        .get_mut(xgm_index_id as usize)
        .unwrap()
        .get_sampling_l_ref()
}

#[no_mangle]
pub extern "C" fn xgm_get_sampling_r_ref(xgm_index_id: u32) -> *const f32 {
    get_xgm_bank()
        .borrow_mut()
        .get_mut(xgm_index_id as usize)
        .unwrap()
        .get_sampling_r_ref()
}

#[no_mangle]
pub extern "C" fn xgm_get_sampling_s16le_ref(xgm_index_id: u32) -> *const i16 {
    get_xgm_bank()
        .borrow_mut()
        .get_mut(xgm_index_id as usize)
        .unwrap()
        .get_output_sampling_s16le_ref()
}

#[no_mangle]
pub extern "C" fn xgm_get_header_json(xgm_index_id: u32) -> u32 {
    let json = get_xgm_bank()
        .borrow_mut()
        .get_mut(xgm_index_id as usize)
        .unwrap()
        .get_xgm_header_json();
    // UTF-8 json into allocate memory
    let memory_index_id = memory_get_alloc_len();
    get_memory_bank()
        .borrow_mut()
        .insert(memory_index_id as usize, json.into_bytes());
    // return memory index id
    memory_index_id
}

#[no_mangle]
pub extern "C" fn xgm_get_gd3_json(xgm_index_id: u32) -> u32 {
    let json = get_xgm_bank()
        .borrow_mut()
        .get_mut(xgm_index_id as usize)
        .unwrap()
        .get_xgm_gd3_json();
    // UTF-8 json into allocate memory
    let memory_index_id = memory_get_alloc_len();
    get_memory_bank()
        .borrow_mut()
        .insert(memory_index_id as usize, json.into_bytes());
    // return memory index id
    memory_index_id
}

#[no_mangle]
pub extern "C" fn vgm_play(vgm_index_id: u32) -> usize {
    get_vgm_bank()
        .borrow_mut()
        .get_mut(vgm_index_id as usize)
        .unwrap()
        .play(true)
}

#[no_mangle]
pub extern "C" fn xgm_play(xgm_index_id: u32) -> usize {
    get_xgm_bank()
        .borrow_mut()
        .get_mut(xgm_index_id as usize)
        .unwrap()
        .play(true)
}

#[no_mangle]
pub extern "C" fn vgm_drop(vgm_index_id: u32) {
    get_vgm_bank().borrow_mut().remove(vgm_index_id as usize);
}

#[no_mangle]
pub extern "C" fn xgm_drop(xgm_index_id: u32) {
    get_xgm_bank().borrow_mut().remove(xgm_index_id as usize);
}

#[no_mangle]
pub extern "C" fn sound_slot_drop(sound_slot_index_id: u32) {
    get_sound_slot_bank()
        .borrow_mut()
        .remove(sound_slot_index_id as usize);
}

fn get_sound_chip_type(sound_chip_type: u32) -> SoundChipType {
    match sound_chip_type {
        0 => SoundChipType::YM2149,
        1 => SoundChipType::YM2151,
        2 => SoundChipType::YM2203,
        3 => SoundChipType::YM2413,
        4 => SoundChipType::YM2608,
        5 => SoundChipType::YM2610,
        6 => SoundChipType::YM2612,
        7 => SoundChipType::YM3526,
        8 => SoundChipType::Y8950,
        9 => SoundChipType::YM3812,
        10 => SoundChipType::YMF262,
        11 => SoundChipType::YMF278B,
        12 => SoundChipType::SEGAPSG,
        13 => SoundChipType::SN76489,
        14 => SoundChipType::PWM,
        15 => SoundChipType::SEGAPCM,
        16 => SoundChipType::OKIM6258,
        17 => SoundChipType::C140,
        18 => SoundChipType::C219,
        19 => SoundChipType::OKIM6295,
        _ => panic!("not supported sound chip type"),
    }
}

fn get_rom_index(rom_index: u32) -> RomIndex {
    match rom_index {
        0 => RomIndex::YM2608_DELTA_T,
        1 => RomIndex::YM2610_ADPCM,
        2 => RomIndex::YM2610_DELTA_T,
        3 => RomIndex::YMF278B_ROM,
        4 => RomIndex::YMF278B_RAM,
        5 => RomIndex::Y8950_ROM,
        6 => RomIndex::SEGAPCM_ROM,
        7 => RomIndex::OKIM6295_ROM,
        8 => RomIndex::C140_ROM,
        _ => panic!("not support rom index"),
    }
}

fn get_rom_bus_type(rom_bus_type: u32) -> Option<RomBusType> {
    match rom_bus_type {
        0 => None,
        1 => Some(RomBusType::C140_TYPE_SYSTEM2),
        2 => Some(RomBusType::C140_TYPE_SYSTEM21),
        3 => Some(RomBusType::C219_TYPE_ASIC219),
        4 => Some(RomBusType::OKIM6295),
        _ => panic!("not supported rom bus type type"),
    }
}
