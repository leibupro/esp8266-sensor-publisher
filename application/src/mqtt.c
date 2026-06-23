
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
/*
 * Newer SDK versions
 * use esp_mac.h for
 * esp_read_mac()
 * instead of esp_system.h
 * */
#include "esp_system.h"
#include "esp_log.h"
#include "mqtt_client.h"

#include "data.h"
#include "wifi.h"
#include "mqtt.h"


#define MAC_ALEN 6U
#define MAC_STRLEN ( ( 2U * MAC_ALEN ) + ( MAC_ALEN - 1U ) + 1U )
#define BROKER_URI "mqtt://192.168.1.21:1883"
#define PUB_TOPIC_BASE "/js8/cellar/hallway/%s/dht"
#define TOPIC_SPACE 64U
#define PUB_QOS 1
#define PUB_DATA_BASE ( \
    "Humidity: %2hd %%, " \
    "Temperature: %3hd degree Celsius." \
)
#define DATA_SPACE 64U


static esp_mqtt_client_handle_t client = NULL;
static EventGroupHandle_t s_wifi_event_group = NULL;
static uint8_t wifi_mac[ MAC_ALEN ] = { 0U };
static char mac_str[ MAC_STRLEN ] = { 0 };
static char pub_topic[ TOPIC_SPACE ] = { 0 };
static char pub_data[ DATA_SPACE ] = { 0 };
static const char* const tag_mqtt_pub = "mqtt_publisher";


static void publish(
    const data_record_t* const record
);
static void mqtt_event_handler(
    void* handler_args,
    esp_event_base_t base,
    int32_t event_id,
    void* event_data
);


void mqtt_publisher_init( void )
{
    s_wifi_event_group = wifi_get_event_group();
    /*
     * Newer SDK versions:
     * .broker.address.uri = BROKER_URI,
     * .network.disable_auto_reconnect = false,
     * .session.keepalive = 60,
     * */
    const esp_mqtt_client_config_t mqtt_cfg = {
        .uri = BROKER_URI,
        .disable_auto_reconnect = false,
        /* Unit of keepalive is seconds ... */
        .keepalive = 60,
        .reconnect_timeout_ms = 5000
    };
    esp_read_mac(
        &( wifi_mac[ 0U ] ),
        ESP_MAC_WIFI_STA
    );
    ( void )snprintf(
        &mac_str[ 0U ],
        MAC_STRLEN,
        "%02x:%02x:%02x:%02x:%02x:%02x",
        wifi_mac[ 0U ],
        wifi_mac[ 1U ],
        wifi_mac[ 2U ],
        wifi_mac[ 3U ],
        wifi_mac[ 4U ],
        wifi_mac[ 5U ]
    );
    ( void )snprintf(
        &pub_topic[ 0U ],
        TOPIC_SPACE,
        PUB_TOPIC_BASE,
        &mac_str[ 0U ]
    );
    client = esp_mqtt_client_init( &mqtt_cfg );
    esp_mqtt_client_register_event(
        client,
        ESP_EVENT_ANY_ID,
        mqtt_event_handler,
        NULL
    );
    esp_mqtt_client_start( client );
}


void mqtt_publish_record(
    const data_record_t* const record
)
{
    if( !record )
    {
        ESP_LOGE(
            tag_mqtt_pub,
            "Data record pointer is zero."
        );
        return;
    }
    if( s_wifi_event_group )
    {
        const EventBits_t ev_bits =
            xEventGroupGetBits( s_wifi_event_group );
        if( ev_bits & WIFI_CONNECTED_BIT )
        {
            publish( record );
        }
    }
    else
    {
        ESP_LOGE(
            tag_mqtt_pub,
            "Wifi event group pointer is zero."
        );
        return;
    }
}


static void publish(
    const data_record_t* const record
)
{
    if( client )
    {
        ( void )snprintf(
            &pub_data[ 0U ],
            DATA_SPACE,
            PUB_DATA_BASE,
            record->humidity,
            record->temperature
        );
        /*
         * QoS 1 is the 5th parameter (0, 1, or 2)
         * msg_id > 0 means the message was successfully
         * queued for delivery. The byte lenght parameter
         * is intentionally set to 0. This means that
         * length calculates automatically via strlen(data).
         * */
        int msg_id = esp_mqtt_client_publish(
            client,
            pub_topic,
            pub_data,
            0,
            PUB_QOS,
            0
        );
        if( msg_id > 0 )
        {
            ESP_LOGI(
                tag_mqtt_pub,
                "Sent QoS1 message, ID: %d",
                msg_id
            );
            ESP_LOGI(
                tag_mqtt_pub,
                "%s",
                &pub_data[ 0U ]
            );
        }
    }
    else
    {
        ESP_LOGE(
            tag_mqtt_pub,
            "MQTT client handle (pointer) is zero."
        );
    }
}


static void mqtt_event_handler(
    void* handler_args,
    esp_event_base_t base,
    int32_t event_id,
    void* event_data
)
{
    esp_mqtt_event_handle_t event =
        ( esp_mqtt_event_handle_t )event_data;
    esp_mqtt_event_id_t evid = ( esp_mqtt_event_id_t )event_id;
    ( void )handler_args;
    ( void )base;
    ( void )event;
    switch( evid )
    {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI( tag_mqtt_pub, "Connected to broker." );
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW( tag_mqtt_pub, "Disconnected from broker." );
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE( tag_mqtt_pub, "MQTT error occurred." );
            break;
        default:
            break;
    }
}

