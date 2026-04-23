
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_spi_flash.h"
#include "driver/gpio.h"
#include "dht.h"


static const char* const tag_main = "app_main";
//static const char* const tag_senrd = "dht11_reader";
//static const char* const tag_mqtt_pub = "mqtt_publisher";


void app_main( void )
{
    esp_err_t sen_read_result = ESP_FAIL;
    int16_t humidity = 0;
    int16_t temperature = 0;

    ESP_LOGI( tag_main, " " );
    ESP_LOGI( tag_main, " " );
    ESP_LOGI( tag_main, "Hello world!" );

    /* Print chip information */
    esp_chip_info_t chip_info;
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

    /*
     * Wait for the sensor because the DH11 is
     * notoriously slow. And also enable the
     * DH11 internal pullup resistor.
     *
     * Caution: D4 label on the board means GPIO_NUM_2 !!!
     * Caution: Just in case one wants to printf floats,
     *          this has to be enabled. Either by adding
     *          -u _printf_float to the linker flags or
     *          through make menuconfig in an example
     *          project (requires re-compilation and copy
     *          of the sdk libraries).
     *
     * expected behavior for the DHT11 is integer-only resolution.
     * While its data protocol technically includes a byte for
     * fractional data, the hardware inside the DHT11 is limited
     * to integer-only resolution.
     *
     * Therefore, the integers are divided by 10 for
     * displaying (printf). The last place is 0 anyways.
     * */
    vTaskDelay( 2500 / portTICK_PERIOD_MS );
    gpio_set_pull_mode( GPIO_NUM_2, GPIO_PULLUP_ONLY );

    for( ;; )
    {
        sen_read_result = dht_read_data(
            DHT_TYPE_DHT11,
            GPIO_NUM_2,
            &humidity,
            &temperature
        );
        humidity /= 10;
        temperature /= 10;
        if( sen_read_result == ESP_OK )
        {
            ESP_LOGD(
                tag_main,
                "Humidity: %hd %%, "
                "Temperature: %hd Degree Celsius.",
                humidity,
                temperature
            );
            humidity = 0;
            temperature = 0;
            sen_read_result = ESP_FAIL;
        }
        else
        {
            ESP_LOGE(
                tag_main,
                "Sensor read error ..."
            );
        }
        ( void )fflush( stdout );
        vTaskDelay( 2500 / portTICK_PERIOD_MS );
    }
}
