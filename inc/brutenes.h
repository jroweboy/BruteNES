
#ifndef BRUTENES_BRUTENES_H
#define BRUTENES_BRUTENES_H

#include <vector>

#include "common.h"
#include "cpu.h"

class BruteNES {
public:
    BruteNES(std::vector<u8>);
private:
    CPU cpu;

};


#endif //BRUTENES_BRUTENES_H
