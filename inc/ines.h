
#ifndef BRUTENES_INES_H
#define BRUTENES_INES_H

#include "common.h"

#include <optional>
#include <span>

enum class Mirroring {
    Single,
    Horizontal,
    Vertical,
    FourWay,
};

class INES {
public:

    static std::optional<INES> ParseHeader(std::span<u8>);

    INES() = delete;

    u32 prg_size;
    u32 chr_size;
    u16 mapper;
    u8 submapper;
    Mirroring mirroring;
    bool battery;
    bool trainer;

private:

    INES(u32 prg, u32 chr, u16 mapper, u8 submapper, Mirroring mirroring, bool battery, bool trainer) :
        prg_size(prg), chr_size(chr), mapper(mapper), submapper(submapper),
        mirroring(mirroring), battery(battery), trainer(trainer) {}
};


#endif //BRUTENES_INES_H
