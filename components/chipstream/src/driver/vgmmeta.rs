// license:BSD-3-Clause
// copyright-holders:Hiromasa Tanaka
use nom::bytes::complete::{tag, take};
use nom::number::complete::{le_u16, le_u32, le_u8};
use nom::IResult;

use crate::driver::gd3meta::{parse_gd3, Gd3};
use crate::driver::meta::Jsonlize;

///
/// https://vgmrips.net/wiki/VGM_Specification
///
#[derive(Deserialize, Serialize, Default, Debug)]
pub struct VgmHeader {
    pub eof: u32,
    pub version: u32,
    pub clock_sn76489: u32,
    pub clock_ym2413: u32,
    pub offset_gd3: u32,
    pub total_samples: u32,
    pub offset_loop: u32,
    pub loop_samples: u32,
    pub rate: u32,
    pub sn76489_fb: u16,
    pub sn76489_w: u8,
    pub sn76489_f: u8,
    pub clock_ym2612: u32,
    pub clock_ym2151: u32,
    pub vgm_data_offset: u32,
    pub clock_sega_pcm: u32,
    pub spcm_interface: u32,
    pub clock_rf5c68: u32,
    pub clock_ym2203: u32,
    pub clock_ym2608: u32,
    pub clock_ym2610_b: u32,
    pub clock_ym3812: u32,
    pub clock_ym3526: u32,
    pub clock_y8950: u32,
    pub clock_ymf262: u32,
    pub clock_ymf278_b: u32,
    pub clock_ym271: u32,
    pub clock_ymz280b: u32,
    pub clock_rf5c164: u32,
    pub clock_pwm: u32,
    pub clock_ay8910: u32,
    pub ay8910_flag: u32,
    pub volume_modifier: u8,
    pub reserved01: u8,
    pub loop_base: u8,
    pub loop_modifier: u8,
    pub clock_gb_dmg: u32,
    pub clock_nes_apu: u32,
    pub clock_multi_pcm: u32,
    pub clock_upd7759: u32,
    pub clock_okim6258: u32,
    pub okmi6258_flag: u8,
    pub k054539_flag: u8,
    pub c140_chip_type: u8,
    pub reserved02: u8,
    pub clock_okim6295: u32,
    pub clock_k051649: u32,
    pub clock_k054539: u32,
    pub clock_huc6280: u32,
    pub clock_c140: u32,
    pub clock_k053260: u32,
    pub clock_pokey: u32,
    pub clock_qsound: u32,
    pub clock_scsp: u32,
    pub extra_hdr_ofs: u32,
    pub clock_wonder_swan: u32,
    pub clock_vsu: u32,
    pub clock_saa1099: u32,
    pub clock_es5503: u32,
    pub clock_es5506: u32,
    pub es5503_amount_channel: u8,
    pub es5506_amount_channel: u8,
    pub c352_clock_divider: u8,
    pub reserved03: u8,
    pub clock_x1_010: u32,
    pub clock_c352: u32,
    pub clock_ga20: u32,
    pub reserved04: u32,
    pub reserved05: u32,
    pub reserved06: u32,
    pub reserved07: u32,
    pub reserved08: u32,
    pub reserved09: u32,
    pub reserved10: u32,
    pub extra_hdr: ExtraHeader,
}

#[derive(Deserialize, Serialize, Default, Debug)]
pub struct ExtraHeader {
    pub header_size: u32,
    pub chip_clock_ofs: u32,
    pub chip_volume_ofs: u32,
    pub chip_clock: Vec<ChipClock>,
    pub chip_volume: Vec<ChipVolume>,
}

#[derive(Deserialize, Serialize, Default, Debug)]
pub struct ChipClock {
    pub chip_id: u8,
    pub clock_second_chip: u32,
}

#[derive(Deserialize, Serialize, Default, Debug)]
pub struct ChipVolume {
    pub chip_id: u8,
    pub flags: u8,
    pub volume: u16,
}

///
/// Parse VGM header
///
fn parse_vgm_header(i: &[u8]) -> IResult<&[u8], VgmHeader> {
    let (i, _) = tag("Vgm ")(i)?;
    let (i, eof) = le_u32(i)?;
    let (i, version) = take(4usize)(i)?;
    let version = version
        .iter()
        .rev()
        .map(|n| format!("{n:02X}"))
        .collect::<String>();
    let version = version.parse().unwrap_or(0);
    let (i, clock_sn76489) = le_u32(i)?;
    let (i, clock_ym2413) = le_u32(i)?;
    let (i, offset_gd3) = le_u32(i)?;
    let (i, total_samples) = le_u32(i)?;
    let (i, offset_loop) = le_u32(i)?;
    let (i, loop_samples) = le_u32(i)?;
    let (i, rate) = le_u32(i)?;
    let (i, sn76489_fb) = le_u16(i)?;
    let (i, sn76489_w) = le_u8(i)?;
    let (i, sn76489_f) = le_u8(i)?;
    let (i, clock_ym2612) = le_u32(i)?;
    let (i, clock_ym2151) = le_u32(i)?;
    let (i, vgm_data_offset) = le_u32(i)?;
    let (i, clock_sega_pcm) = le_u32(i)?;
    let (i, spcm_interface) = le_u32(i)?;
    let (i, clock_rf5c68) = le_u32(i)?;
    let (i, clock_ym2203) = le_u32(i)?;
    let (i, clock_ym2608) = le_u32(i)?;
    let (i, clock_ym2610_b) = le_u32(i)?;
    let (i, clock_ym3812) = le_u32(i)?;
    let (i, clock_ym3526) = le_u32(i)?;
    let (i, clock_y8950) = le_u32(i)?;
    let (i, clock_ymf262) = le_u32(i)?;
    let (i, clock_ymf278_b) = le_u32(i)?;
    let (i, clock_ym271) = le_u32(i)?;
    let (i, clock_ymz280b) = le_u32(i)?;
    let (i, clock_rf5c164) = le_u32(i)?;
    let (i, clock_pwm) = le_u32(i)?;
    let (i, clock_ay8910) = le_u32(i)?;
    let (i, ay8910_flag) = le_u32(i)?;
    let (i, volume_modifier) = le_u8(i)?;
    let (i, reserved01) = le_u8(i)?;
    let (i, loop_base) = le_u8(i)?;
    let (i, loop_modifier) = le_u8(i)?;
    let (i, clock_gb_dmg) = le_u32(i)?;
    let (i, clock_nes_apu) = le_u32(i)?;
    let (i, clock_multi_pcm) = le_u32(i)?;
    let (i, clock_upd7759) = le_u32(i)?;
    let (i, clock_okim6258) = le_u32(i)?;
    let (i, okmi6258_flag) = le_u8(i)?;
    let (i, k054539_flag) = le_u8(i)?;
    let (i, c140_chip_type) = le_u8(i)?;
    let (i, reserved02) = le_u8(i)?;
    let (i, clock_okim6295) = le_u32(i)?;
    let (i, clock_k051649) = le_u32(i)?;
    let (i, clock_k054539) = le_u32(i)?;
    let (i, clock_huc6280) = le_u32(i)?;
    let (i, clock_c140) = le_u32(i)?;
    let (i, clock_k053260) = le_u32(i)?;
    let (i, clock_pokey) = le_u32(i)?;
    let (i, clock_qsound) = le_u32(i)?;
    let (i, clock_scsp) = le_u32(i)?;
    let (i, extra_hdr_ofs) = le_u32(i)?;
    let (i, clock_wonder_swan) = le_u32(i)?;
    let (i, clock_vsu) = le_u32(i)?;
    let (i, clock_saa1099) = le_u32(i)?;
    let (i, clock_es5503) = le_u32(i)?;
    let (i, clock_es5506) = le_u32(i)?;
    let (i, es5503_amount_channel) = le_u8(i)?;
    let (i, es5506_amount_channel) = le_u8(i)?;
    let (i, c352_clock_divider) = le_u8(i)?;
    let (i, reserved03) = le_u8(i)?;
    let (i, clock_x1_010) = le_u32(i)?;
    let (i, clock_c352) = le_u32(i)?;
    let (i, clock_ga20) = le_u32(i)?;
    let (i, reserved04) = le_u32(i)?;
    let (i, reserved05) = le_u32(i)?;
    let (i, reserved06) = le_u32(i)?;
    let (i, reserved07) = le_u32(i)?;
    let (i, reserved08) = le_u32(i)?;
    let (i, reserved09) = le_u32(i)?;
    let (i, reserved10) = le_u32(i)?;

    let mut header = VgmHeader::default();

    if version >= 100 {
        header = VgmHeader {
            eof,
            version,
            clock_sn76489,
            clock_ym2413,
            offset_gd3,
            total_samples,
            offset_loop,
            loop_samples,
            ..header
        };
    }
    if version >= 101 {
        header = VgmHeader { rate, ..header };
    }
    if version >= 110 {
        header = VgmHeader {
            sn76489_fb,
            sn76489_w,
            clock_ym2612,
            clock_ym2151,
            ..header
        };
    }
    if version >= 150 {
        header = VgmHeader {
            vgm_data_offset,
            ..header
        };
    }
    if version >= 151 {
        header = VgmHeader {
            sn76489_f,
            clock_sega_pcm,
            spcm_interface,
            clock_rf5c68,
            clock_ym2203,
            clock_ym2608,
            clock_ym2610_b,
            clock_ym3812,
            clock_ym3526,
            clock_y8950,
            clock_ymf262,
            clock_ymf278_b,
            clock_ym271,
            clock_ymz280b,
            clock_rf5c164,
            clock_pwm,
            clock_ay8910,
            ay8910_flag,
            loop_modifier,
            ..header
        };
    }
    if version >= 160 {
        header = VgmHeader {
            volume_modifier,
            reserved01,
            loop_base,
            ..header
        };
    }
    if version >= 161 {
        header = VgmHeader {
            clock_gb_dmg,
            clock_nes_apu,
            clock_multi_pcm,
            clock_upd7759,
            clock_okim6258,
            okmi6258_flag,
            k054539_flag,
            c140_chip_type,
            reserved02,
            clock_okim6295,
            clock_k051649,
            clock_k054539,
            clock_huc6280,
            clock_c140,
            clock_k053260,
            clock_pokey,
            clock_qsound,
            ..header
        };
    }
    if version >= 170 {
        header = VgmHeader {
            extra_hdr_ofs,
            ..header
        };
    }
    if version >= 171 {
        header = VgmHeader {
            clock_scsp,
            clock_wonder_swan,
            clock_vsu,
            clock_saa1099,
            clock_es5503,
            clock_es5506,
            es5503_amount_channel,
            es5506_amount_channel,
            c352_clock_divider,
            reserved03,
            clock_x1_010,
            clock_c352,
            clock_ga20,
            reserved04,
            reserved05,
            reserved06,
            reserved07,
            reserved08,
            reserved09,
            reserved10,
            ..header
        };
    }

    Ok((i, header))
}

fn parse_extra_header(i: &[u8], mut header: VgmHeader) -> IResult<&[u8], VgmHeader> {
    let mut extra_hdr = ExtraHeader::default();

    // Header Size
    let (j, header_size) = le_u32(i)?;
    let (j, chip_clock_ofs) = if header_size >= 0x08 {
        le_u32(j)?
    } else {
        (j, 0)
    };
    let (_, chip_volume_ofs) = if header_size >= 0x0c {
        le_u32(j)?
    } else {
        (j, 0)
    };

    // Chip Clock Header
    if chip_clock_ofs != 0 {
        extra_hdr.chip_clock =
            match parse_extra_header_clock(&i[(0x04 + chip_clock_ofs as usize)..]) {
                Ok((_, extra_header_clock)) => extra_header_clock,
                Err(error) => return Err(error),
            };
        extra_hdr.chip_clock_ofs = chip_clock_ofs;
    }
    // Chip Volume Header
    if chip_volume_ofs != 0 {
        extra_hdr.chip_volume =
            match parse_extra_header_volume(&i[(0x08 + chip_volume_ofs as usize)..]) {
                Ok((_, extra_header_volume)) => extra_header_volume,
                Err(error) => return Err(error),
            };
        extra_hdr.chip_volume_ofs = chip_volume_ofs;
    }

    extra_hdr.header_size = header_size;
    header.extra_hdr = extra_hdr;

    Ok((i, header))
}

fn parse_extra_header_clock(mut i: &[u8]) -> IResult<&[u8], Vec<ChipClock>> {
    let mut clock: Vec<ChipClock> = Vec::new();
    let entry_count;

    (i, entry_count) = le_u8(i)?;
    for _ in 0..entry_count {
        let chip_id;
        let clock_second_chip;
        (i, chip_id) = le_u8(i)?;
        (i, clock_second_chip) = le_u32(i)?;
        clock.push(ChipClock {
            chip_id,
            clock_second_chip,
        });
    }

    Ok((i, clock))
}

fn parse_extra_header_volume(mut i: &[u8]) -> IResult<&[u8], Vec<ChipVolume>> {
    let mut vol: Vec<ChipVolume> = Vec::new();
    let entry_count;

    (i, entry_count) = le_u8(i)?;
    for _ in 0..entry_count {
        let chip_id;
        let flags;
        let volume;
        (i, chip_id) = le_u8(i)?;
        (i, flags) = le_u8(i)?;
        (i, volume) = le_u16(i)?;
        vol.push(ChipVolume {
            chip_id,
            flags,
            volume,
        });
    }

    Ok((i, vol))
}

///
/// Parse VGM meta
///
pub(crate) fn parse_vgm_meta(vgmdata: &[u8]) -> Result<(VgmHeader, Gd3), &'static str> {
    // clean header
    let mut vgm_data_offset: usize =
        (u32::from_le_bytes(vgmdata[0x34..=0x37].try_into().unwrap()) + 0x34) as usize;
    if vgm_data_offset >= 0xff {
        vgm_data_offset = 0xff;
    }
    // The length of vgm_data_offset takes precedence over the length of the header.
    let mut clean_header = [0_u8; 0x100];
    clean_header[..vgm_data_offset].copy_from_slice(&vgmdata[..vgm_data_offset]);

    let mut header = match parse_vgm_header(&clean_header) {
        Ok((_, header)) => header,
        Err(_) => return Err("vgm header parse error."),
    };
    // Parse extra header
    if header.version >= 170 && header.extra_hdr_ofs != 0 {
        let extra_header = &vgmdata[(0xbc + header.extra_hdr_ofs as usize)..];
        header = match parse_extra_header(extra_header, header) {
            Ok((_, header)) => header,
            Err(_) => return Err("vgm extra header parse error."),
        }
    }
    // Parse GD3
    let gd3 = match parse_gd3(&vgmdata[(0x14 + header.offset_gd3 as usize)..]) {
        Ok((_, gd3)) => gd3,
        Err(_) => Gd3::default(), // blank values
    };

    Ok((header, gd3))
}

impl Jsonlize for VgmHeader {}

#[cfg(test)]
mod tests {
    use super::parse_vgm_meta;
    use super::Jsonlize;
    use std::fs::File;
    use std::io::Read;

    #[test]
    fn test_1() {
        parse("./docs/vgm/ym2612.vgm")
    }

    #[test]
    fn test_2() {
        parse("./docs/vgm/segapcm-2.vgm")
    }

    #[test]
    fn test_3() {
        parse("./docs/vgm/ym2203-v170.vgm")
    }

    #[test]
    fn test_4() {
        parse("./docs/vgm/c140-v170.vgm")
    }

    fn parse(filepath: &str) {
        // load sn76489 vgm file
        let mut file = File::open(filepath).unwrap();
        let mut buffer = Vec::new();
        let _ = file.read_to_end(&mut buffer).unwrap();
        let (header, gd3) = parse_vgm_meta(&buffer).unwrap();

        println!("{header:#?}");
        println!("{gd3:#?}");
        println!("{:#?}", header.get_json());
        println!("{:#?}", gd3.get_json());
    }
}
