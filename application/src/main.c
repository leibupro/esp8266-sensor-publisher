
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_spi_flash.h"
#include "driver/gpio.h"
#include "dht.h"
#include "wifi.h"

/*
 * Caution: D4 label on the board means GPIO_NUM_2 !!!
 * */
#define DHT_GPIO GPIO_NUM_2
#define SENSOR_TYPE DHT_TYPE_DHT11
/*
 * Unit is milliseconds ...
 * */
#define SEN_RD_INTERVAL 2500
#define MAX_QUE_ITEMS 16

typedef struct
{
    int16_t humidity;
    int16_t temperature;
}
dht11_record_t;

static const char* const tag_main = "app_main";
static const char* const tag_senrd = "dht11_reader";
static const char* const tag_pub = "data_publisher";

static QueueHandle_t dht_rec_que = NULL;

/*
 * Public function prototypes ...
 * */
void app_main( void );
/*
 * Private function prototypes ...
 * */
static void dht_task( void* params );
static void data_pub_task( void* params );


void app_main( void )
{
    BaseType_t err_chk = -1;
    esp_chip_info_t chip_info = { 0 };

    ESP_LOGI( tag_main, " " );
    ESP_LOGI( tag_main, " " );
    ESP_LOGI( tag_main, "Hello world!" );

    /* Print chip information */
    esp_chip_info( &chip_info );
    ESP_LOGI(
        tag_main,
        "This is ESP8266 chip with %d CPU cores, WiFi, "
        "silicon revision %d, "
        "%dMB %s flash.",
        chip_info.cores,
        chip_info.revision,
        spi_flash_get_chip_size() / ( 1024 * 1024 ),
        ( chip_info.features & CHIP_FEATURE_EMB_FLASH ) ?
        "embedded" : "external"
    );
    dht_rec_que = xQueueCreate(
        MAX_QUE_ITEMS,
        sizeof( dht11_record_t )
    );
    if( dht_rec_que == NULL )
    {
        ESP_LOGE(
            tag_main,
            "Couldn't create data record queue. "
            "The journey ends here."
        );
        return;
    }
    wifi_init_sta();
    err_chk = xTaskCreate(
        dht_task,    // Entry
        "DHT_task",  // Name
        4096,        // Task stack size
        ( void* )0U, // Parameters
        5,           // Priority
        ( void* )0U  // Parameters
    );
    if( err_chk != pdPASS )
    {
        ESP_LOGE(
            tag_main,
            "Couldn't create sensor data retrieval task. "
            "The journey ends here."
        );
        return;
    }
    err_chk = -1;
    err_chk = xTaskCreate(
        data_pub_task,
        "Data_pub_task",
        4096,
        ( void* )0U,
        5,
        ( void* )0U
    );
    if( err_chk != pdPASS )
    {
        ESP_LOGW(
            tag_main,
            "Couldn't create sensor data publishing task."
        );
    }
}


static void dht_task( void* params )
{
    esp_err_t sen_read_result = ESP_FAIL;
    BaseType_t err_chk = -1;
    dht11_record_t record = { 0 };
    ( void )params;
    /*
     * Wait for the sensor because the DH11 is
     * notoriously slow.
     *
     * Caution: Just in case one wants to printf floats,
     *          this has to be enabled. Either by adding
     *          -u _printf_float to the linker flags or
     *          through make menuconfig in an example
     *          project (requires re-compilation and copy
     *          of the sdk libraries).
     *
     * Expected behavior for the DHT11 is integer-only resolution.
     * While its data protocol technically includes a byte for
     * fractional data, the hardware inside the DHT11 is limited
     * to integer-only resolution.
     *
     * Therefore, the integers are divided by 10 for
     * displaying (printf). The last place is 0 anyways.
     * */
    vTaskDelay( pdMS_TO_TICKS( 2500 ) );
    ESP_LOGI(
        tag_senrd,
        "About to start the "
        "sensor data retrieval task ..."
    );
    /*
     * Enable the DH11 internal pullup resistor.
     * */
    gpio_set_pull_mode( DHT_GPIO, GPIO_PULLUP_ONLY );

    for( ;; )
    {
        sen_read_result = dht_read_data(
            SENSOR_TYPE,
            DHT_GPIO,
            &( record.humidity ),
            &( record.temperature )
        );
        if( sen_read_result == ESP_OK )
        {
            record.humidity /= 10;
            record.temperature /= 10;
            err_chk = xQueueSend( dht_rec_que, &record, 0 );
            if( err_chk != pdPASS )
            {
                ESP_LOGW(
                    tag_senrd,
                    "Queue full. Skipping this sensor reading."
                );
            }
            record.humidity = 0;
            record.temperature = 0;
            sen_read_result = ESP_FAIL;
            err_chk = -1;
        }
        else
        {
            ESP_LOGE(
                tag_senrd,
                "Sensor read error ..."
            );
        }
        ( void )fflush( stdout );
        vTaskDelay( pdMS_TO_TICKS( SEN_RD_INTERVAL ) );
    }
}


static void data_pub_task( void* params )
{
    BaseType_t err_chk = -1;
    dht11_record_t record = { 0 };
    ( void )params;
    ESP_LOGI(
        tag_pub,
        "About to start the "
        "sensor data publishing task ..."
    );

    for( ;; )
    {
        /*
         * Block indefinitely until
         * data arrives in the queue.
         * */
        err_chk = xQueueReceive(
            dht_rec_que,
            &record,
            portMAX_DELAY
        );
        if( err_chk == pdPASS )
        {
            ESP_LOGD(
                tag_pub,
                "Humidity: %2hd %%, "
                "Temperature: %3hd degree Celsius.",
                record.humidity,
                record.temperature
            );
            record.humidity = 0;
            record.temperature = 0;
            err_chk = -1;
        }
        else
        {
            ESP_LOGW(
                tag_pub,
                "Queue receive timeout - "
                "no data available."
            );
        }
    }
}

