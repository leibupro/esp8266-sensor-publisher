
#ifndef __USER_MQTT_H__
#define __USER_MQTT_H__

#include "esp_event.h"

#include "data.h"


void mqtt_publisher_init( void );
void mqtt_publish_record(
    const data_record_t* const record
);

#endif /* __USER_MQTT_H__ */

