
#ifndef BRUTENES_INES_H
#define BRUTENES_INES_H

#include "common.h"

#include <optional>
#include <span>

class INES {
public:
    enum class Mirroring {
        Single,
        Horizontal,
        Vertical,
        FourWay,
    };

    static std::optional<INES> ParseHeader(std::span<u8>);

    INES() = delete;

    u32 prg_size;
    u32 chr_size;
    u16 mapper;
    u8 submapper;
    Mirroring mirroring;
    bool battery;
    bool trainer;

    u32 chr_ram_size;

private:

    INES(u32 prg, u32 chr, u16 mapper, u8 submapper, Mirroring mirroring, bool battery, bool trainer, u32 chr_ram_size) :
        prg_size(prg), chr_size(chr), mapper(mapper), submapper(submapper),
        mirroring(mirroring), battery(battery), trainer(trainer), chr_ram_size(chr_ram_size) {}
};


#endif //BRUTENES_INES_H
