#include "ps2pcpad.h"
#include "padhelper.h"
#include "networkhelper.h"
#include <unistd.h>

s32 serverthread_id = 0;

extern unsigned char PS2IPS_irx[];
extern unsigned int size_PS2IPS_irx;

void server();

int main(int argc, char *argv[])
{   
    init_scr();
    scr_printf("Starting ps2pad\n");
    printf("Starting ps2pad\n");

    /* For whatever reason, ps2ip_init() never returns when
        ps2pad is executed with uelflauncher*/
    SifExecModuleBuffer(PS2IPS_irx, size_PS2IPS_irx, 0,0,NULL);

    if (ps2ip_init() < 0)
    {
        scr_printf("Could not initialize ps2ip\n");
        printf("ERROR: ps2ip_init falied!\n");
        sleep(5);
        SleepThread();
    }

    scr_printf("Initializing the pad interface\n");
    printf("Initializing the pad interface\n");

trypad:
    if (pad_init() < 0)
    {
        scr_printf("Trying to init pad again\n");
        printf("Trying to init pad again\n");
        sleep(3);

        scr_clear();
        scr_setXY(0, 0);
        goto trypad;
    }

    if (InitializeServer() < 0)
    {
        printf("Failed initializing the server.");
        scr_printf("Failed initializing the server.");
        // If the cause was the thread not being created, why not tell the user
        if (serverthread_id == -1)
        {
            printf(" We couldn't create the thread.");
            scr_printf(" We couldn't create the thread.");
        }
        printf("\n");
        scr_printf("\n");

        SleepThread();
    }

    printf("The server is done initializing\n");
    scr_printf("The server is done initializing\n");
    SleepThread();

    return 0;
}
