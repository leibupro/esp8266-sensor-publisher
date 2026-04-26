
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
#define BROKER_URI "mqtt://192.168.1.21:1883"
#define PUB_TOPIC_BASE "/js8/cellar/hallway/%s/dht"
#define PUB_QOS 1


static esp_mqtt_client_handle_t client = NULL;
static EventGroupHandle_t s_wifi_event_group = NULL;
static uint8_t wifi_mac[ MAC_ALEN ] = { 0U };
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
    const char* const topic = "/js8/test";
    const char* const data = "Foobarbazqux ...";
    if( client )
    {
        /*
         * QoS 1 is the 5th parameter (0, 1, or 2)
         * msg_id > 0 means the message was successfully
         * queued for delivery.
         * */
        int msg_id = esp_mqtt_client_publish(
            client,
            topic,
            data,
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
                "Humidity: %2hd %%, "
                "Temperature: %3hd degree Celsius.",
                record->humidity,
                record->temperature
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

