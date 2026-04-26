
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "wifi.h"


/*
 * Max 1 minute between attempts
 * */
#define MAX_RETRY_DELAY_MS 60000U
/*
 * Start with 2 seconds
 * */
#define MIN_RETRY_DELAY_MS 2000U


static const char* const tag_wifi = "wifi_station";
static const char* const tag_wifi_reconn = "wifi_reconnect_task";
/*
 * FreeRTOS event group to signal when we are connected.
 * */
static EventGroupHandle_t s_wifi_event_group = NULL;
static TaskHandle_t reconn_task_handle = NULL;
static int s_retry_num = 0;
static const char* const esp_wifi_ssid = "vqe-media";
static const char* const esp_wifi_pwd =
#include "passwd.h"
;


static void event_handler(
    void* arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void* event_data
);
static void wifi_reconnect_task( void* params );


EventGroupHandle_t wifi_get_event_group( void )
{
    return s_wifi_event_group;
}


void wifi_init_sta( void )
{
    /*
     * Following line used to initialize the
     * Non-Volatile Storage (NVS) partition in
     * the flash memory. The ESP32 WiFi library
     * requires NVS to be initialized because
     * it uses this storage for several critical
     * internal functions.
     * */
    ESP_ERROR_CHECK( nvs_flash_init() );
    s_wifi_event_group = xEventGroupCreate();
    tcpip_adapter_init();
    ESP_ERROR_CHECK( esp_event_loop_create_default() );

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init( &cfg ) );

    ESP_ERROR_CHECK(
        esp_event_handler_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &event_handler,
            NULL
        )
    );
    ESP_ERROR_CHECK(
        esp_event_handler_register(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            &event_handler,
            NULL
        )
    );

    wifi_config_t wifi_config = { 0 };
    ( void )strncpy(
        ( void* )&( wifi_config.sta.ssid[ 0U ] ),
        ( void* )esp_wifi_ssid,
        sizeof( wifi_config.sta.ssid )
    );
    ( void )strncpy(
        ( void* )&( wifi_config.sta.password[ 0U ] ),
        ( void* )esp_wifi_pwd,
        sizeof( wifi_config.sta.password )
    );
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK( esp_wifi_set_mode( WIFI_MODE_STA ) );
    ESP_ERROR_CHECK(
        esp_wifi_set_config(
            ESP_IF_WIFI_STA,
            &wifi_config
        )
    );

    BaseType_t err_chk = xTaskCreate(
        wifi_reconnect_task,
        "wifi_reconnect_task",
        2048,
        NULL,
        5,
        &reconn_task_handle
    );
    if( err_chk != pdPASS )
    {
        ESP_LOGE(
            tag_wifi,
            "Failed to create wifi reconnection task!"
        );
        esp_restart();
    }

    ESP_ERROR_CHECK( esp_wifi_start() );
    ESP_LOGI( tag_wifi, "wifi_init_sta finished." );

    /*
     * Waiting until the connection is established
     * (WIFI_CONNECTED_BIT). Since we priorly started
     * the reconnect task, the WIFI_FAIL_BIT is not
     * handled here. The bits are set by
     * the event_handler().
     * */
    EventBits_t bits = xEventGroupWaitBits(
        s_wifi_event_group,
        WIFI_CONNECTED_BIT,
        pdFALSE,
        pdFALSE,
        portMAX_DELAY
    );

    /*
     * xEventGroupWaitBits() returns the bits before the call
     * returned, hence we can test which event actually
     * happened.
     * */
    if( bits & WIFI_CONNECTED_BIT )
    {
        ESP_LOGI(
            tag_wifi,
            "connected to ap SSID: %s.",
            esp_wifi_ssid
        );
    }
    else
    {
        ESP_LOGE( tag_wifi, "UNEXPECTED EVENT" );
    }
}


static void event_handler(
    void* arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void* event_data
)
{
    ( void )arg;
    if( ( event_base == WIFI_EVENT ) &&
        ( event_id == WIFI_EVENT_STA_START ) )
    {
        esp_wifi_connect();
    }
    else if( ( event_base == WIFI_EVENT ) &&
             ( event_id == WIFI_EVENT_STA_DISCONNECTED ) )
    {
        wifi_event_sta_disconnected_t* event =
            ( wifi_event_sta_disconnected_t* )event_data;
        ESP_LOGI( tag_wifi, "connect to the AP fail" );
        ESP_LOGW( tag_wifi, "Disconnect reason: %u", event->reason );
        if( ( event->reason == WIFI_REASON_AUTH_FAIL ) ||
            ( event->reason == WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT ) )
        {
            ESP_LOGE(
                tag_wifi,
                "Critical: Wrong wifi password. "
                "Reconnect aborted."
            );
        }
        else
        {
            xEventGroupSetBits(
                s_wifi_event_group,
                WIFI_FAIL_BIT
            );
        }
    }
    else if( ( event_base == IP_EVENT ) &&
             ( event_id == IP_EVENT_STA_GOT_IP ) )
    {
        ip_event_got_ip_t* event = ( ip_event_got_ip_t* )event_data;
        ESP_LOGI(
            tag_wifi,
            "got ip: %s",
            ip4addr_ntoa( &event->ip_info.ip )
        );
        s_retry_num = 0;
        xEventGroupSetBits(
            s_wifi_event_group,
            WIFI_CONNECTED_BIT
        );
        /*
         * RESET: Telling the reconnect task
         * that the connection worked!
         * */
        if( reconn_task_handle != NULL )
        {
            xTaskNotifyGive( reconn_task_handle );
        }
    }
}


static void wifi_reconnect_task( void* params )
{
    uint32_t delay_ms = MIN_RETRY_DELAY_MS;
    ( void )params;

    for( ;; )
    {
        /*
         * Wait for the event handler to set the FAIL_BIT.
         * */
        ( void )xEventGroupWaitBits(
            s_wifi_event_group,
            WIFI_FAIL_BIT,
            pdTRUE,
            pdFALSE,
            portMAX_DELAY
        );
        /*
         * Check if we should reset the timer because we
         * successfully connected recently. See xTaskNotifyGive()
         * in the event handler.
         * ulTaskNotifyTake() is non-blocking here because
         * the timeout value is 0, so the call returns
         * immediately.
         * */
        if( ulTaskNotifyTake( pdTRUE, 0 ) > 0 )
        {
            delay_ms = MIN_RETRY_DELAY_MS;
        }
        ESP_LOGI(
            tag_wifi_reconn,
            "Retrying in %u ms ...",
            delay_ms
        );
        vTaskDelay( pdMS_TO_TICKS( delay_ms ) );
        esp_wifi_connect();
        /*
         * Increase delay for the NEXT failure,
         * exponential backoff algorithm.
         * */
        delay_ms = ( ( delay_ms * 2U ) > MAX_RETRY_DELAY_MS ) ?
            MAX_RETRY_DELAY_MS :
            ( delay_ms * 2U )
        ;
    }
}

