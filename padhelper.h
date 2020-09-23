#ifndef ___PADHELPER_H___
#define ___PADHELPER_H___

#include "globalincludes.h"
#include <libpad.h>

static char padBuf[256] __attribute__((aligned(64)));

static int waitPadReady(int port, int slot)
{
    int state;
    int lastState;
    char stateString[16];
    scr_printf("Getting padState\n");
    state = padGetState(port, slot);
    lastState = -1;
    while((state != PAD_STATE_STABLE) && (state != PAD_STATE_FINDCTP1)) {
        if (state != lastState) {
            padStateInt2String(state, stateString);
            printf("Please wait, pad(%d,%d) is in state %s\n",
                       port, slot, stateString);
        }
        lastState = state;
        state=padGetState(port, slot);
    }
    if (lastState != -1) {
        printf("Pad OK!\n");
    }
    return 0;
}

static int pad_init(){
    scr_printf("Loading controller related modules\n");

    int ret;
    ret = SifLoadModule("rom0:SIO2MAN", 0, NULL);
    if (ret < 0) {
        scr_printf("sifLoadModule sio failed: %d\n", ret);
        return -1;
    }

    ret = SifLoadModule("rom0:PADMAN", 0, NULL);
    if (ret < 0) {
        scr_printf("sifLoadModule pad failed: %d\n", ret);
        return -1;
    }

    padInit(0);

    if((ret = padPortOpen(0, 0, padBuf)) == 0) {
        scr_printf("padOpenPort failed: %d\n", ret);
        SleepThread();
    }

    waitPadReady(0,0);

    int modes = padInfoMode(0, 0, PAD_MODETABLE, -1);
    scr_printf("The controller has %d modes. Current mode is %d\n", modes, padInfoMode(0, 0, PAD_MODECURID, 0));

#ifdef STOP_ON_NON_DUALSHOCK
    if(modes == 0){
        scr_printf("The controller is not a valid dualshock device\nTherefore unsupported for now.\n");
        return -1;
    }

    int i = 0;
    do {
        if (padInfoMode(0, 0, PAD_MODETABLE, i) == PAD_TYPE_DUALSHOCK)
            break;
        i++;
    } while (i < modes);
    if (i >= modes) {
        scr_printf("The controller is not a valid dualshock device\nTherefore unsupported for now.\n");
        return 1;
    }
#endif

    padSetMainMode(0, 0, PAD_MMODE_DUALSHOCK, PAD_MMODE_LOCK);

    waitPadReady(0, 0);
    scr_printf("infoPressMode: %d\n", padInfoPressMode(0, 0));

    waitPadReady(0, 0);
    scr_printf("enterPressMode: %d\n", padEnterPressMode(0, 0));

    waitPadReady(0, 0);

    return 0;
}

#endif
