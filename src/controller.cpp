
#include "brutenes.h"
#include "controller.h"

u8 Controller::ReadRegister(u16 addr) {
    u8 ret = 0x01;
    if (addr == 0x4016) {
        ret = (last_strobe_value.raw >> 7) & 1;
        last_strobe_value.raw <<= 1;
    }
    return ret;
}

void Controller::WriteRegister(u16 addr, u8 value) {
    if (!strobe) {
        // Start reading input
    } else {
        std::unique_lock<std::mutex> lock(frontend.controller_mutex);
        last_strobe_value = raw_controls;
    }

    strobe = !strobe;
}

void Controller::SetButton(Controller::Button button) {
    raw_controls.raw |= (u8)button;
}

void Controller::ReleaseButton(Controller::Button button) {
    raw_controls.raw &= ~(u8)button;
}
