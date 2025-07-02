#include <stdio.h>
#include "driver/i2c.h"
#include "mpu6050.h"
#include "math.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "esp_netif.h"
#include "esp_eap_client.h"

#include "jsoncpp/value.h"
#include "jsoncpp/json.h"
#include "esp_firebase/app.h"
#include "esp_firebase/rtdb.h"
#include "firebase_config.h"
#include "mpu_wrapper.h"  // Adicione esta linha

#include <iostream>

using namespace ESPFirebase;

#define WIFI_SSID "UTFPR-ALUNO"
#define EAP_ID "a2456621"
#define EAP_USERNAME "a2456621"
#define EAP_PASSWORD "qatezc10"
#define CONNECTED_BIT BIT0

static EventGroupHandle_t wifi_event_group;
static esp_netif_t *sta_netif = NULL;
static const char *TAG = "INTEGRADO";

void wifi_event_handler(void *arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
    }
}

void init_wifi() {
    ESP_ERROR_CHECK(esp_netif_init());
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    sta_netif = esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    wifi_config_t wifi_config = {};
    strcpy((char *)wifi_config.sta.ssid, WIFI_SSID);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    esp_eap_client_set_identity((uint8_t *)EAP_ID, strlen(EAP_ID));
    esp_eap_client_set_username((uint8_t *)EAP_USERNAME, strlen(EAP_USERNAME));
    esp_eap_client_set_password((uint8_t *)EAP_PASSWORD, strlen(EAP_PASSWORD));

    ESP_ERROR_CHECK(esp_wifi_sta_enterprise_enable());
    ESP_ERROR_CHECK(esp_wifi_start());
}

void mpu_task(void *pvParam) {
    // Aguarda Wi-Fi
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);

    // Inicializa Firebase
    FirebaseApp app = FirebaseApp(API_KEY);
    if (!app.loginUserAccount({USER_EMAIL, USER_PASSWORD})) {
        ESP_LOGE(TAG, "Falha no login Firebase");
        vTaskDelete(NULL);
    }
    RTDB db(&app, DATABASE_URL);
    ESP_LOGI(TAG, "Firebase conectado");

    // Inicializa os MPU6050 usando o wrapper
    if (!mpu6050_init_all()) {
        ESP_LOGE(TAG, "Falha na inicialização dos sensores MPU6050");
        vTaskDelete(NULL);
    }
    ESP_LOGI(TAG, "Sensores MPU6050 inicializados");

    Json::Value data;

    while (1) {
        float roll0, pitch0, roll1, pitch1;

        // Lê dados do MPU0
        if (mpu6050_get_orientation(0, &roll0, &pitch0)) {
            ESP_LOGI(TAG, "MPU0 - Roll: %.2f°, Pitch: %.2f°", roll0, pitch0);
        } else {
            ESP_LOGE(TAG, "Erro ao ler MPU0");
            roll0 = pitch0 = 0.0;
        }

        // Lê dados do MPU1
        if (mpu6050_get_orientation(1, &roll1, &pitch1)) {
            ESP_LOGI(TAG, "MPU1 - Roll: %.2f°, Pitch: %.2f°", roll1, pitch1);
        } else {
            ESP_LOGE(TAG, "Erro ao ler MPU1");
            roll1 = pitch1 = 0.0;
        }

        // Envia dados para o Firebase
        data["sensor1"]["roll"] = roll0;
        data["sensor1"]["pitch"] = pitch0;
        data["sensor2"]["roll"] = roll1;
        data["sensor2"]["pitch"] = pitch1;

        db.putData("/accel", data);
        ESP_LOGI(TAG, "Dados enviados ao Firebase");

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

extern "C" void app_main(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    init_wifi();
    xTaskCreate(mpu_task, "mpu_task", 12288, NULL, 5, NULL);
}
