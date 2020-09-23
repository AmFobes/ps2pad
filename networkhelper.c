#include "networkhelper.h"

static s32 sock_tcpServer;
static s32 sock_udpServer;

static ee_thread_t thread_server;
static u8 thread_server_stack[0x1800] __attribute__((aligned(16)));
static s32 serverthread_id = 0;

static struct sockaddr_in srvAddr;

static struct timespec socket_delay;

static u8 localIP[4];

static s32 sock_tcpClient;
static struct sockaddr_in addr_client;
static s32 ip_client[4];
static s32 sz_addr_client;

static s32 basethread_id = 0;
struct padButtonStatus padStatus;
void FetchLocalIP()
{
    t_ip_info ip_info;
    /* No error handling :S*/
    ps2ip_getconfig("sm0", &ip_info);
    localIP[0] = ip4_addr1((struct ip4_addr *)&ip_info.ipaddr);
    localIP[1] = ip4_addr2((struct ip4_addr *)&ip_info.ipaddr);
    localIP[2] = ip4_addr3((struct ip4_addr *)&ip_info.ipaddr);
    localIP[3] = ip4_addr4((struct ip4_addr *)&ip_info.ipaddr);
}

static void thr_serverListener(u8 localIP[4])
{

StartListen:
    scr_printf("Waiting for a TCP client at %d.%d.%d.%d:%d\n",
               localIP[0], localIP[1], localIP[2], localIP[3], SERVER_PORT_TCP);

    printf("Waiting for a TCP client at %d.%d.%d.%d:%d\n",
           localIP[0], localIP[1], localIP[2], localIP[3], SERVER_PORT_TCP);

        
    /*

        Wait for a tcp client to connect

    */
    while (1)
    {
        sock_tcpClient = accept(sock_tcpServer, (struct sockaddr *)&addr_client, &sz_addr_client);
        if (sock_tcpClient < 0)
        {
            printf("Client tried to connect but failed. errno(%d)\n", *__errno());
            continue;
        }
        printf("New client connected. Socket handle(%d)\n   sz_addr_client(%d)\n", sock_tcpClient, sz_addr_client);
        scr_printf("New client connected\n");
        break;
    }

    ip_client[0] = ip4_addr1((struct ip4_addr *)&addr_client.sin_addr.s_addr);
    ip_client[1] = ip4_addr2((struct ip4_addr *)&addr_client.sin_addr.s_addr);
    ip_client[2] = ip4_addr3((struct ip4_addr *)&addr_client.sin_addr.s_addr);
    ip_client[3] = ip4_addr4((struct ip4_addr *)&addr_client.sin_addr.s_addr);

    scr_printf("Client ip address is %d.%d.%d.%d\n", ip_client[0], ip_client[1], ip_client[2], ip_client[3]);
    printf("Client ip address is %d.%d.%d.%d\n", ip_client[0], ip_client[1], ip_client[2], ip_client[3]);

    scr_printf("Waiting for a client handshake\n");
    printf("Waiting for client to do the handshake\n");
    char hsBuf[9];
   
    while (1)
    {
        int recvRet = recv(sock_tcpClient, &hsBuf, sizeof(hsBuf) - 1, UNULL);
        if (recvRet < 0)
        {
            printf("Failure reading from client, dropping it.\n");
            scr_printf("Couldn't read from the client\n");

            scr_printf("With this current version, a client _cannot_ reconnect reliably.\nPlease restart your PS2\n");
            printf("With this current version, a client _cannot_ reconnect reliably.\n lease restart your PS2\n");

            TerminateThread(basethread_id);
            while(1)
                SleepThread();

            disconnect(recvRet);
            goto StartListen;
        }
        else if (recvRet == 0)
        {
            printf("Client shutdown\n");
            scr_printf("Couldn't read from the client (shutdown)\n");
            scr_printf("With this current version, a client _cannot_ reconnect reliably.\nPlease restart your PS2\n");
            printf("With this current version, a client _cannot_ reconnect reliably.\nPlease restart your PS2\n");

            TerminateThread(basethread_id);
            while(1)
                SleepThread();
            disconnect(recvRet);
            goto StartListen;
        }

        printf("Read %d bytes, checking if it's a handshake\n", recvRet);

        if (strcmp(hsBuf, "ps2pcpad") == 0)
        {
            scr_printf("Client supports ps2pcpad :)\n");
            printf("Client supports ps2pcpad\n");
        }
        break;
    }

    printf("Let's start sending UDP data to the client ip\n");
    scr_printf("Sending pad data to the client\n");
    addr_client.sin_port = htons(SERVER_PORT_UDP); // Overwrite the tcp port with the udp port

    u8 fails = 0;
    s32 sendRet;
    while (1)
    {
        /* Resume our base (pad fetching) thread in case it fell asleep*/
        ResumeThread(basethread_id);
        padRead(0, 0, &padStatus);
        sendRet = sendto(sock_udpServer, &padStatus, sizeof(padStatus), 0, (struct sockaddr *)&addr_client, sizeof(addr_client));
        if (sendRet < 0)
        {
            fails++;
            scr_printf("Failed sending info to client. Failure %u\n", fails);
            printf("Failed sending info to client. Failure %u\n", fails);
        }
        else
        {
            fails = 0;
        }
        if (fails == 10)
        {
            scr_printf("Giving up after 10 tries\n");
            printf("Giving up after 10 tries\n");
            disconnect(sock_tcpClient);
            goto StartListen;
        }
        nanosleep(&socket_delay, NULL);
    }
}

s32 InitializeServer()
{
    //pad_ptr = padButtonStatus_ptr;
    sz_addr_client = sizeof(addr_client);
    
    FetchLocalIP();

    scr_printf("Our local ip is %d.%d.%d.%d\n",
               localIP[0], localIP[1], localIP[2], localIP[3]);

    scr_printf("Initializing the TCP & UDP servers\n");

    socket_delay.tv_sec = 0;
    socket_delay.tv_nsec = SERVER_UDP_SEND_DELAY_NS;
    /*

        Creating the sockets

    */
    printf("Creating tcp socket...\n");

    sock_tcpServer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (sock_tcpServer < 0)
    {
        printf("Failed, errno(%d)\n", errno);
        return -1;
    }

    printf("Successful. Handle(%d)\n", sock_tcpServer);

    printf("Creating udp socket...");

    sock_udpServer = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (sock_udpServer < 0)
    {
        printf("Failed, errno(%d)\n", *__errno());
        return -1;
    }

    printf("Successful. Handle(%d)\n", sock_udpServer);

    /*

        Binding the sockets

    */

    printf("Binding the TCP server on port %d...", SERVER_PORT_TCP);

    srvAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    srvAddr.sin_family = AF_INET;
    srvAddr.sin_port = htons(SERVER_PORT_TCP);

    if (bind(sock_tcpServer, (struct sockaddr *)&srvAddr, sizeof(srvAddr)) < 0)
    {
        printf("Failed, errno(%d)\n", *__errno());
        return -1;
    }

    printf("Successful\n");

    printf("Binding the UDP server on port %d...", SERVER_PORT_UDP);

    srvAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    srvAddr.sin_family = AF_INET;
    srvAddr.sin_port = htons(SERVER_PORT_UDP);

    if (bind(sock_udpServer, (struct sockaddr *)&srvAddr, sizeof(srvAddr)) < 0)
    {
        printf("Failed, errno(%d)\n", *__errno());
        return -1;
    }

    printf("Successful\n");

    /* At this point, the udp socket is ready to blast anyone who asks for it */

    printf("Listening to the TCP server...");

    // Can't get a great answer as to what our backlog value should be
    if (listen(sock_tcpServer, 2) < 0)
    {
        printf("Failed, errno(%d)\n", *__errno());
        return -1;
    }

    printf("Successful\n");

    thread_server.func = &thr_serverListener;
    thread_server.attr = 0;
    thread_server.option = 0;
    thread_server.stack = thread_server_stack;
    thread_server.stack_size = sizeof(thread_server_stack);
    thread_server.gp_reg = &_gp;
    thread_server.initial_priority = 0x16;

    basethread_id = GetThreadId();
    serverthread_id = CreateThread(&thread_server);
    if (serverthread_id < 0)
    {
        printf("Failed creating the thread\n");
        serverthread_id = -1;
        return -1;
    }

    StartThread(serverthread_id, localIP);
    return 0;
}
