

#ifndef BRUTENES_CONTROLLER_H
#define BRUTENES_CONTROLLER_H

#include "common.h"

class BruteNES;

// TODO this controller code sucks.
class Controller {
public:
    enum class Button : u8 {
        RIGHT  = 1 << 0,
        LEFT  = 1 << 1,
        DOWN  = 1 << 2,
        UP  = 1 << 3,
        START  = 1 << 4,
        SELECT  = 1 << 5,
        B  = 1 << 6,
        A  = 1 << 7,
    };

    explicit Controller(BruteNES& frontend) : frontend(frontend) {}

    virtual u8 ReadRegister(u16 addr);

    virtual void WriteRegister(u16 addr, u8 value);

    // TODO change the interface for the controller to poll through an input mapping
    // instead of having the frontend write through events
    virtual void SetButton(Button button);
    virtual void ReleaseButton(Button button);

private:
    bool strobe{false};
    BruteNES& frontend;

    // TODO: This really sucks to not share the controls struct
    struct Controls {
        u8 raw;
        struct {
            u8 right : 1;
            u8 left : 1;
            u8 down : 1;
            u8 up : 1;
            u8 start : 1;
            u8 select : 1;
            u8 b : 1;
            u8 a : 1;
        };
    };
    Controls raw_controls{};

    Controls last_strobe_value{};
};


#endif //BRUTENES_CONTROLLER_H
