
#ifndef __USER_WIFI_H__
#define __USER_WIFI_H__

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"


/*
 * The event group allows multiple bits for each event,
 * but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries
 * */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1


EventGroupHandle_t wifi_get_event_group( void );
void wifi_init_sta( void );

#endif /* __USER_WIFI_H__ */

