#include <stdio.h>
#include <math.h>

#include <freertos/FreeRTOS.h>
#include <driver/gpio.h>
#include <esp_adc/adc_oneshot.h>
#include <esp_adc/adc_cali.h>
#include <esp_check.h>

#define APP_ADC_PIN GPIO_NUM_8
#define APP_LED_PIN GPIO_NUM_47

#define APP_ADC_ATTEN ADC_ATTEN_DB_6
#define APP_ADC_BITWIDTH ADC_BITWIDTH_12

#define ADC_RAW_HI 800
#define ADC_RAW_LO 300

#define SMA_WINDOW_SZ 10

#define LOOP_DELAY pdMS_TO_TICKS(200)

#define FATAL_TAG "FATAL"

static adc_unit_t s_app_adc_unit;
static adc_channel_t s_app_adc_channel;

esp_err_t setup_pins() {
    esp_err_t r;

    const gpio_config_t gpio_adc = {
        .pin_bit_mask = (uint64_t) 1 << APP_ADC_PIN,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    r =  gpio_config(&gpio_adc);
    ESP_RETURN_ON_ERROR(r, __func__, "gpio_config({.pin: %d}) failed: %s", APP_ADC_PIN, esp_err_to_name(r));

    gpio_reset_pin(APP_LED_PIN);
    const gpio_config_t gpio_led = {
        .pin_bit_mask = (uint64_t) 1 << APP_LED_PIN,
        .mode         = GPIO_MODE_OUTPUT
    };
    r =  gpio_config(&gpio_led);
    ESP_RETURN_ON_ERROR(r, __func__, "gpio_config({.pin: %d}) failed: %s", APP_LED_PIN, esp_err_to_name(r));

    return ESP_OK;
}

esp_err_t setup_unit_and_channel(adc_oneshot_unit_handle_t *p_adc_unit_h) {
    esp_err_t r;

    const adc_oneshot_unit_init_cfg_t unit_conf = {
        .unit_id = s_app_adc_unit,
        .clk_src = ADC_RTC_CLK_SRC_DEFAULT,
    };
    r = adc_oneshot_new_unit(&unit_conf, p_adc_unit_h);
    ESP_RETURN_ON_ERROR(r, __func__, "adc_oneshot_new_unit failed: %s", esp_err_to_name(r));

    adc_oneshot_chan_cfg_t chan_conf = {
        .atten = APP_ADC_ATTEN,
        .bitwidth = APP_ADC_BITWIDTH
    };
    r = adc_oneshot_config_channel(*p_adc_unit_h, s_app_adc_channel, &chan_conf);
    ESP_RETURN_ON_ERROR(r, __func__, "adc_oneshot_config_channel failed: %s", esp_err_to_name(r));

    return ESP_OK;
}

int avg_of(int values[], size_t len) {
    int sum = 0;

    for(int i = 0; i < len; i++) {
        sum += values[i];
    }

    double res = (double) sum/len;

    return lround(res);
}

void app_main(void)
{
    esp_err_t r;

    r = setup_pins();
    ESP_RETURN_VOID_ON_ERROR(r, FATAL_TAG, "Setup pin %d failed: %s", APP_ADC_PIN, esp_err_to_name(r));

    r = adc_oneshot_io_to_channel(APP_ADC_PIN, &s_app_adc_unit, &s_app_adc_channel);
    ESP_RETURN_VOID_ON_ERROR(r, FATAL_TAG, "Wrong input pin %d: %s", APP_ADC_PIN, esp_err_to_name(r));
    ESP_LOGI(__func__, "unit: %d, chan: %d", s_app_adc_unit, s_app_adc_channel);

    adc_oneshot_unit_handle_t adc_unit_h;
    r = setup_unit_and_channel(&adc_unit_h);
    ESP_RETURN_VOID_ON_ERROR(r, FATAL_TAG, "Channel setup failed");

    int fst = -1;
    r = adc_oneshot_read(adc_unit_h, s_app_adc_channel, &fst);
    ESP_RETURN_VOID_ON_ERROR(r, FATAL_TAG, "adc_oneshot_read failed: %s", esp_err_to_name(r));

    int samples[SMA_WINDOW_SZ];
    for(int i = 0; i < SMA_WINDOW_SZ; i++) {
        samples[i] = fst;
    }

    for (int i = 0;; i++) {
        if (i >= SMA_WINDOW_SZ) {
            i %= SMA_WINDOW_SZ;
        }

        adc_oneshot_read(adc_unit_h, s_app_adc_channel, &samples[i]);

        int avg = avg_of(samples, SMA_WINDOW_SZ);

        ESP_LOGI(__func__, "raw: %d, sma: %d", samples[i], avg);

        if (avg <= ADC_RAW_LO) {
            gpio_set_level(APP_LED_PIN, 1);
        } else if (avg >= ADC_RAW_HI) {
            gpio_set_level(APP_LED_PIN, 0);
        }

        vTaskDelay(LOOP_DELAY);
    }
}
