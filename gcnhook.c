typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed char s8;
typedef u8 bool;
typedef int s32;
typedef float f32;

#define true 1
#define false 0
#define NULL 0

typedef struct PADStatus {
    u16 button;
    s8 stickX, stickY, substickX, substickY;
    u8 triggerLeft, triggerRight, analogA, analogB;
    s8 err;
} PADStatus;

typedef struct KPAD {
    u32 hold, trig, release;
    u8 padding0[0x20 - 0x0C];
    f32 posx, posy;
    u8 padding1[0x5C - 0x28];
    u8 dev_type;
    s8 wpad_err;
    s8 dpd_valid_fg;
    u8 data_format;
    u8 ex_status[0x24];
} KPAD;

typedef struct Revo {
    u32 vtable;
    u8 mButton;
    u8 p0[3];
    u32 unk08;
    u32 lastRumbleTime;
    s32 rumbleType;
    u32 chan;
    u32 type;
    u8 unk1C, unk1D, unk1E;
    u8 p1;
    KPAD* kpad;
} Revo;

typedef struct State {
    PADStatus pad[4];
    KPAD kpad;
    Revo revo;
    f32 posX, posY;
    u16 prevBtn;
    bool present;
    bool hijacked;
    bool inited;
} State;

#define PADInit ((void (*)(void))0x815767D4)
#define PADRead ((u32 (*)(PADStatus*))0x81576930)
#define PADReset ((u32 (*)(u32))0x815765C0)
#define WPADDisconnect ((s32 (*)(s32))0x8157A960)
#define KPADEnableDPD ((void (*)(s32))0x815883B0)
#define absclamp ((f32 (*)(const f32*, const f32*))0x813366EC)
#define MgrRead ((void (*)(void*))0x81337500)
#define RevoRead ((void (*)(void*))0x813360CC)

#define manager (*(void**)0x81089070)
#define state (*(State*)0x81359EE0)

#define REVO_VTABLE 0x816347F4

#define PAD_ERR_NONE 0
#define PAD_ERR_NO_CONTROLLER -1
#define PAD_CHAN0_BIT 0x80000000u

#define GC_LIM_X 1.8f
#define GC_LIM_Y 1.2f
#define GC_SPEED 0.05f

static const u16 gcBtnMap[20] = {
    0x0001, 0x0001,
    0x0002, 0x0002,
    0x0004, 0x0004,
    0x0008, 0x0008,
    0x0100, 0x0800,
    0x0200, 0x0400,
    0x0400, 0x0200,
    0x0800, 0x0100,
    0x1000, 0x0010,
    0x0010, 0x1000
};

void gcnhook(void) {
    void** slot0;
    f32 tmp, lim;
    s32 err, i;
    u16 gcRawBtn, btn;
    if (!state.inited) {
        PADInit();
        state.inited = true;
    }
    PADRead(state.pad);
    err = state.pad[0].err;
    if (err == PAD_ERR_NO_CONTROLLER) {
        state.present = false;
        PADReset(PAD_CHAN0_BIT);
    } 
    else if (err == PAD_ERR_NONE) {
        state.present = true;
        tmp = state.posX + ((f32)state.pad[0].stickX / 128.0f) * GC_SPEED;
        lim = GC_LIM_X;
        state.posX = absclamp(&tmp, &lim);
        tmp = state.posY - ((f32)state.pad[0].stickY / 128.0f) * GC_SPEED;
        lim = GC_LIM_Y;
        state.posY = absclamp(&tmp, &lim);
        state.kpad.posx = state.posX;
        state.kpad.posy = state.posY;
        state.kpad.dpd_valid_fg = true;
        state.kpad.wpad_err = 0;
        gcRawBtn = state.pad[0].button;
        btn = 0;
        for (i = 0; i < 20; i += 2) {
            if (w & gcBtnMap[i]) {
                btn |= gcBtnMap[i + 1];
            }
        }
        state.kpad.hold = btn;
        state.kpad.trig = (u32)(btn & ~state.prevBtn);
        state.kpad.release = (u32)(state.prevBtn & ~btn);
        state.prevBtn = btn;
    }
    if (manager) {
        MgrRead(manager);
    }
    slot0 = (void**)manager;
    if (state.present) {
        if (!state.hijacked) {
            WPADDisconnect(0);
            state.hijacked = true;
        }
        if (state.revo.vtable == NULL) {
            state.revo.vtable = REVO_VTABLE;
            state.revo.rumbleType = -1;
            state.revo.chan = 0;
            state.revo.type = 2;
            state.revo.kpad = &state.kpad;
        }
        if (slot0) {
            slot0[0] = (void*)&state.revo;
            RevoRead((void*)&state.revo);
        }
    }
    else if (state.hijacked) {
        if (slot0 && slot0[0] == (void*)&state.revo) {
            slot0[0] = NULL;
        }
        state.kpad.hold = state.kpad.trig = state.kpad.release = 0;
        state.kpad.wpad_err = -1;
        state.kpad.dpd_valid_fg = false;
        state.prevBtn = 0;
        state.hijacked = false;
    }
}
