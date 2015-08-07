//Copyright 2015 <>< Charles Lohr, see LICENSE file.

#include "mem.h"
#include "c_types.h"
#include "user_interface.h"
#include "ets_sys.h"
#include "driver/uart.h"
#include "osapi.h"
#include "espconn.h"
#include "mystuff.h"
#include "ws2812_i2s.h"

#define PORT 7777
#define SERVER_TIMEOUT 1500
#define MAX_CONNS 5
#define MAX_FRAME 2000

#define procTaskPrio        0
#define procTaskQueueLen    1

static volatile os_timer_t some_timer;
static struct espconn *pUdpServer;
uint8_t last_leds[512*3];
int last_led_count;
void user_rf_pre_init(void)
{
	//nothing.
}


char * strcat( char * dest, char * src )
{
	return strcat(dest, src );
}



//Tasks that happen all the time.

os_event_t    procTaskQueue[procTaskQueueLen];
static uint8_t printed_ip = 0;
static void ICACHE_FLASH_ATTR
procTask(os_event_t *events)
{
	system_os_post(procTaskPrio, 0, 0 );

	CSTick( 0 );

	if( events->sig == 0 && events->par == 0 )
	{
		//Idle Event.
		struct station_config wcfg;
		char stret[256];
		char *stt = &stret[0];
		struct ip_info ipi;

		int stat = wifi_station_get_connect_status();

//		printf( "STAT: %d\n", stat );

		if( stat == STATION_WRONG_PASSWORD || stat == STATION_NO_AP_FOUND || stat == STATION_CONNECT_FAIL )
		{
			wifi_set_opmode_current( 2 );
			stt += ets_sprintf( stt, "Connection failed: %d\n", stat );
			uart0_sendStr(stret);
		}

		if( stat == STATION_GOT_IP && !printed_ip )
		{
			wifi_station_get_config( &wcfg );
			wifi_get_ip_info(0, &ipi);
			stt += ets_sprintf( stt, "STAT: %d\n", stat );
			stt += ets_sprintf( stt, "IP: %d.%d.%d.%d\n", (ipi.ip.addr>>0)&0xff,(ipi.ip.addr>>8)&0xff,(ipi.ip.addr>>16)&0xff,(ipi.ip.addr>>24)&0xff );
			stt += ets_sprintf( stt, "NM: %d.%d.%d.%d\n", (ipi.netmask.addr>>0)&0xff,(ipi.netmask.addr>>8)&0xff,(ipi.netmask.addr>>16)&0xff,(ipi.netmask.addr>>24)&0xff );
			stt += ets_sprintf( stt, "GW: %d.%d.%d.%d\n", (ipi.gw.addr>>0)&0xff,(ipi.gw.addr>>8)&0xff,(ipi.gw.addr>>16)&0xff,(ipi.gw.addr>>24)&0xff );
			stt += ets_sprintf( stt, "WCFG: /%s/%s/\n", wcfg.ssid, wcfg.password );
			uart0_sendStr(stret);
			printed_ip = 1;
		}
	}

}

//Timer event.
static void ICACHE_FLASH_ATTR
 myTimer(void *arg)
{
	CSTick( 1 );

	uart0_sendStr(".");
//	printf( "%d\n",underrunCnt );
//	uint8_t ledout[] = { 0x00, 0xff, 0xaa, 0x00, 0xff, 0xaa, };
//	ws2812_push( ledout, 6 );
}


//Called when new packet comes in.
static void ICACHE_FLASH_ATTR
udpserver_recv(void *arg, char *pusrdata, unsigned short len)
{
	struct espconn *pespconn = (struct espconn *)arg;
//	uint8_t buffer[MAX_FRAME];

//	uint8_t ledout[] = { 0x00, 0xff, 0xaa, 0x00, 0xff, 0xaa, };
	uart0_sendStr("X");
	ws2812_push( pusrdata+3, len-3 );

	len -= 3;
	if( len > sizeof(last_leds) + 3 )
	{
		len = sizeof(last_leds) + 3;
	}
	ets_memcpy( last_leds, pusrdata+3, len );
	last_led_count = len / 3;
}

void ICACHE_FLASH_ATTR charrx( uint8_t c )
{
	//Called from UART.
}


void user_init(void)
{
	uart_init(BIT_RATE_115200, BIT_RATE_115200);
	int wifiMode = wifi_get_opmode();

	uart0_sendStr("\r\nCustom Server\r\n");


//Uncomment this to force a system restore.
//	system_restore();


	CSPreInit();
/*
	struct station_config stationConf;
	wifi_set_opmode( 1 ); //We broadcast our ESSID, wait for peopel to join.
	os_memcpy(&stationConf.ssid, "xxx", ets_strlen( "xxx" ) + 1);
	os_memcpy(&stationConf.password, "yyy", ets_strlen( "yyy" ) + 1);

	wifi_set_opmode( 1 );
	wifi_station_set_config(&stationConf);
	wifi_station_connect();**/

    pUdpServer = (struct espconn *)os_zalloc(sizeof(struct espconn));
	ets_memset( pUdpServer, 0, sizeof( struct espconn ) );
	espconn_create( pUdpServer );
	pUdpServer->type = ESPCONN_UDP;
	pUdpServer->proto.udp = (esp_udp *)os_zalloc(sizeof(esp_udp));
	pUdpServer->proto.udp->local_port = 7777;
	espconn_regist_recvcb(pUdpServer, udpserver_recv);

/*	wifi_station_dhcpc_start();
*/
	if( espconn_create( pUdpServer ) )
	{
		while(1) { uart0_sendStr( "\r\nFAULT\r\n" ); }
	}

	CSInit();

	//Add a process
	system_os_task(procTask, procTaskPrio, procTaskQueue, procTaskQueueLen);

	//Timer example
	os_timer_disarm(&some_timer);
	os_timer_setfn(&some_timer, (os_timer_func_t *)myTimer, NULL);
	os_timer_arm(&some_timer, 100, 1);

	ws2812_init();

	system_os_post(procTaskPrio, 0, 0 );
}


//There is no code in this project that will cause reboots if interrupts are disabled.
void EnterCritical()
{
}

void ExitCritical()
{
}


