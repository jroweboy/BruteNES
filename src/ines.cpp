
#include <string>

#include "ines.h"

std::optional<INES> INES::ParseHeader(std::span<u8> header) {
    if (header.size() != 16) {
        return std::nullopt;
    }

    // verify the magic bytes are correct
    std::string magic{header.begin(), header.begin()+3};
    if (magic != "NES\x1a") {
        return std::nullopt;
    }

    // check for nes2.0 header
    bool nes20 = ((header[7] & 0b0000'1100) == 0b0000'1000);

    u32 prg_size = header[4];
    u32 chr_size = header[5];

    Mirroring mirroring;
    switch (header[6] & 0b0000'1001) {
        case 0b0000'0000:
            mirroring = Mirroring::Vertical;
            break;
        case 0b0000'0001:
            mirroring = Mirroring::Horizontal;
            break;
        case 0b0000'1000:
            mirroring = Mirroring::Single;
            break;
        case 0b0000'1001:
            mirroring = Mirroring::FourWay;
            break;
        default:
            // Unreachable
            return std::nullopt;
    }

    bool battery = (header[6] & 0b0000'0010) != 0;
    bool trainer = (header[6] & 0b0000'0100) != 0;

    u16 mapper = (header[7] & 0xf0) | ((header[6] & 0xf0) >> 4);
    u8 submapper = 0;
    if (nes20) {
        mapper |= (header[8] & 0x0f) << 8;
        submapper = (header[8] & 0xf0) >> 4;
        prg_size |= (header[9] & 0x0f) << 8;
        chr_size |= (header[9] & 0xf0) << 4;
    }

    prg_size *= 0x4000;
    chr_size *= 0x2000;
    return INES{prg_size, chr_size, mapper, submapper, mirroring, battery, trainer};
}
