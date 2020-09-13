#include "ps2pad.h"

#define STOP_ON_NON_DUALSHOCK
//#define scr_printf printf

static char padBuf[256] __attribute__((aligned(64)));

void server();
int pad_init();

int main(int argc, char* argv[]){

    SifInitRpc(0);
    init_scr();
    scr_printf("Starting ps2pad\n");
    
    SifLoadModule("host:ps2ips.irx", 0, NULL);

    scr_printf("Loaded ps2ips\n");

    if(ps2ip_init() < 0) {
		scr_printf("ERROR: ps2ip_init falied!\n");
        sleep(5);
		SleepThread();
	}

        scr_clear();
        scr_setXY(0,0);
        scr_printf("Initializing the pad interface\n");
trypad:
        if(pad_init() < 0){

            scr_printf("Trying to init pad again\n");
            sleep(3);

            scr_clear();
            scr_setXY(0,0);
            goto trypad;
        }

    server();
}

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

int pad_init(){
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

void print_binary(unsigned int number)
{
    if (number >> 1) {
        print_binary(number >> 1);
    }
    putc((number & 1) ? '1' : '0', stdout);
}

int errcnt = 0;
void sock_sendPad(struct sockaddr_in clnt, int shudp,struct padButtonStatus buttonStatus){
    clnt.sin_port = htons(18197);
    print_binary(buttonStatus.btns);
    putc('\n',stdout);
    if(sendto(shudp,&buttonStatus,sizeof(buttonStatus),0,&clnt,sizeof(clnt)) < 0){
        scr_printf("Failed sending button status to client (%s) or (%s)\n",strerror(__errno()),strerror(errno));
        if(scr_getY() >= 27){
            scr_clear();
            scr_setXY(0,0);
        }
        errcnt++;
        printf("sendto() failure: %d or %d\n",errno, __errno());
        sleep(1);
        return;
    }
    errcnt = 0;
}

struct timespec socketdelay;
void client_handoff(int cs, int shudp, struct sockaddr_in clientAddr){
    socketdelay.tv_sec = 0;
    socketdelay.tv_nsec = 50000000L;
    u8 ip_address[4];

    ip_address[0] = ip4_addr1((struct ip4_addr *) &clientAddr.sin_addr.s_addr);
    ip_address[1] = ip4_addr2((struct ip4_addr *) &clientAddr.sin_addr.s_addr);
    ip_address[2] = ip4_addr3((struct ip4_addr *) &clientAddr.sin_addr.s_addr);
    ip_address[3] = ip4_addr4((struct ip4_addr *) &clientAddr.sin_addr.s_addr);

    scr_clear();
    scr_setXY(0,0);
    
    scr_printf("Connected to client %d.%d.%d.%d\n",ip_address[0],ip_address[1],ip_address[2],ip_address[3]);

    errcnt = 0;

    // Start our little 'handshake'
    char in[7];
    in[6] = '\0';
    while(1){
        int recvret = recv(cs,&in,sizeof(in) - 1,NULL);
        if(recvret < 0){
            scr_printf("Failure reading from client\n");
            return;       
        }
        else if(recvret == 0){
            scr_printf("Client shutdown\n");
            return;
        }
        // If 'in' is anything else, it's the amount of bytes we read
        scr_printf("Client wrote %s (%d)\n",in,scr_getY());
        if(scr_getY() == 27){
            scr_clear();
            scr_setXY(0,0);
        }

        if(!strcmp(in,"ps2pad")){
            break;
        }
    }

        scr_printf("Client supports this protocol\n");        

        scr_printf("Getting ready to poll the controller\n");
        sleep(2);
        
        struct padButtonStatus buttons;

        while(1){
            if(errcnt == 5){
                scr_printf("Giving up on this connection\n");
                sleep(3);
                return;
            }
            int pad_state = padGetState(0, 0);
            while((pad_state != PAD_STATE_STABLE) && (pad_state != PAD_STATE_FINDCTP1)) {
                if(pad_state==PAD_STATE_DISCONN) {
                    scr_printf("The controller is disconnected. Retrying in 3 seconds.\n");
                    sleep(3);
                }
            pad_state=padGetState(0, 0);
            }

            if(padRead(0,0,&buttons)){
               
                sock_sendPad(clientAddr,shudp,buttons);
                nanosleep(&socketdelay,NULL);
            }
        }

    return;
}

void server(){
    int sh, shudp;
    int cs;
    struct sockaddr_in srvAddr;
    struct sockaddr_in srvUDPAddr;
    struct sockaddr_in clntAddr;
    int clntLen;
    int rc;
    int rcudp;

    scr_printf("Starting server sockets\n");
    shudp = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    sh = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if(sh < 0 || shudp < 0)
    {
        scr_printf("Failed to create socket\n");
        sleep(5);
        SleepThread();
    }

    scr_printf("Created socket %d and %d(udp)\n",sh,shudp);

    memset(&srvAddr,0,sizeof(srvAddr));
    srvAddr.sin_family = AF_INET;
    srvAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    srvAddr.sin_port = htons(18196);

    memset(&srvUDPAddr,0,sizeof(srvUDPAddr));
    srvUDPAddr.sin_family = AF_INET;
    srvUDPAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    srvUDPAddr.sin_port = htons(18197);

    rcudp = bind(shudp, (struct sockaddr *) &srvUDPAddr, sizeof(srvUDPAddr));
    rc = bind(sh, (struct sockaddr *) &srvAddr, sizeof(srvAddr));
    if(rc < 0 || rcudp < 0){
        scr_printf("Failed to bind to socket(s), is the port available?\n");
        sleep(5);
        SleepThread();
    }

    scr_printf("Successfully bound to socket (%d) and (%d) (udp)\n",rc,rcudp);

    rc = listen( sh, 2 );
    if ( rc < 0 || rcudp < 0)
    {
        scr_printf("Listen failed.\n");
        SleepThread();
    }

    scr_printf("Listen returned %i\n", rc, rcudp);

    t_ip_info ip_info;
    ps2ip_getconfig("sm0",&ip_info);
    u8 ip_address[4];

    ip_address[0] = ip4_addr1((struct ip4_addr *) &ip_info.ipaddr);
    ip_address[1] = ip4_addr2((struct ip4_addr *) &ip_info.ipaddr);
    ip_address[2] = ip4_addr3((struct ip4_addr *) &ip_info.ipaddr);
    ip_address[3] = ip4_addr4((struct ip4_addr *) &ip_info.ipaddr);

    clntLen = sizeof(clntAddr);

    while(1){
        scr_printf("Awaiting client connection at ip %d.%d.%d.%d\n",ip_address[0],ip_address[1],ip_address[2],ip_address[3]);
        cs = accept( sh, (struct sockaddr *)&clntAddr, &clntLen );
        if ( cs < 0 ){
            scr_printf( "Accept failed\n" );
            sleep(5);
            SleepThread();
        }
        
        scr_printf( "Accept returned %i.\n", cs );
        client_handoff(cs,shudp,clntAddr);
        
        disconnect(cs);
        sleep(3);
   }
}
