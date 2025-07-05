#include <stdio.h>
#include "driver/i2c.h"
#include "mpu6050.h"
#include "mpu_wrapper.h"
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
#include "wifi_config.h"
#include "mpu_wrapper.h"
#include "esp_timer.h"
#include <time.h>
#include "esp_sntp.h"

#include <iostream>

using namespace ESPFirebase;

static EventGroupHandle_t wifi_event_group;
static esp_netif_t *sta_netif = NULL;
static const char *TAG = "INTEGRADO";

#define CONNECTED_BIT BIT0

static void initialize_sntp(void) {
    ESP_LOGI(TAG, "Initializing SNTP");
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_set_time_sync_notification_cb([](struct timeval *tv) {
        ESP_LOGI(TAG, "Time synchronized");
    });
    esp_sntp_init();

    // Wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;

    while (timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }
    
    if (retry >= retry_count) {
        ESP_LOGW(TAG, "Failed to sync time, using default timestamps");
    }
}

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
    
#if USE_ENTERPRISE_WIFI
    // Configuração para WPA2 Enterprise (universidade)
    ESP_LOGI(TAG, "Configurando Wi-Fi WPA2 Enterprise: %s", WIFI_SSID_ENTERPRISE);
    strcpy((char *)wifi_config.sta.ssid, WIFI_SSID_ENTERPRISE);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    esp_eap_client_set_identity((uint8_t *)EAP_ID, strlen(EAP_ID));
    esp_eap_client_set_username((uint8_t *)EAP_USERNAME, strlen(EAP_USERNAME));
    esp_eap_client_set_password((uint8_t *)EAP_PASSWORD, strlen(EAP_PASSWORD));

    ESP_ERROR_CHECK(esp_wifi_sta_enterprise_enable());
#else
    // Configuração para Wi-Fi normal (casa)
    ESP_LOGI(TAG, "Configurando Wi-Fi normal: %s", WIFI_SSID_HOME);
    strcpy((char *)wifi_config.sta.ssid, WIFI_SSID_HOME);
    strcpy((char *)wifi_config.sta.password, WIFI_PASSWORD_HOME);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
#endif

    ESP_ERROR_CHECK(esp_wifi_start());
}

void mpu_task(void *pvParam) {
    // Aguarda Wi-Fi
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);

    // Inicializa sincronização de tempo
    initialize_sntp();

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
    ESP_LOGI(TAG, "4 sensores MPU6050 inicializados");

    // Variáveis para detecção de má postura
    const float PITCH_BASE = 0.0;  // Valor base para pitch (posição perfeita)
    const float ROLL_BASE = 90.0;  // Valor base para roll (posição perfeita)
    const float TOLERANCE = 20.0;   // Tolerância em graus
    const int BAD_POSTURE_THRESHOLD = 20; // 20 segundos
    
    TickType_t bad_posture_start_time = 0;
    bool is_bad_posture = false;
    bool bad_posture_sent = false;
    float last_roll[4] = {0}, last_pitch[4] = {0};  // Arrays para 4 sensores

    Json::Value data;

    while (1) {
        float roll[4], pitch[4];

        // Lê dados de todos os 4 sensores MPU6050
        for (int i = 0; i < 4; i++) {
            if (mpu6050_get_orientation(i, &roll[i], &pitch[i])) {
                ESP_LOGI(TAG, "MPU%d - Roll: %.2f°, Pitch: %.2f°", i, roll[i], pitch[i]);
                last_roll[i] = roll[i];
                last_pitch[i] = pitch[i];
            } else {
                ESP_LOGE(TAG, "Erro ao ler MPU%d", i);
                roll[i] = pitch[i] = 0.0;
            }
        }

        // Verifica se está em má postura usando MPU0 (sensor principal)
        bool current_bad_posture = false;
        if (roll[0] != 0.0 || pitch[0] != 0.0) {  // Só verifica se temos dados válidos
            float pitch_deviation = fabs(pitch[0] - PITCH_BASE);
            float roll_deviation = fabs(roll[0] - ROLL_BASE);
            
            if (pitch_deviation > TOLERANCE || roll_deviation > TOLERANCE) {
                current_bad_posture = true;
                ESP_LOGW(TAG, "Má postura detectada! Pitch: %.2f° (base: %.2f°), Roll: %.2f° (base: %.2f°)", 
                         pitch[0], PITCH_BASE, roll[0], ROLL_BASE);
            }
        }

        // Controle do timer de má postura
        if (current_bad_posture) {
            if (!is_bad_posture) {
                // Primeira detecção de má postura
                is_bad_posture = true;
                bad_posture_start_time = xTaskGetTickCount();
                bad_posture_sent = false;
                ESP_LOGW(TAG, "Iniciando timer de má postura...");
            } else {
                // Verifica se já passou o tempo limite
                TickType_t current_time = xTaskGetTickCount();
                TickType_t elapsed_time = (current_time - bad_posture_start_time) / configTICK_RATE_HZ;
                
                if (elapsed_time >= BAD_POSTURE_THRESHOLD && !bad_posture_sent) {
                    // Envia dados de má postura para o Firebase com caminho único
                    Json::Value bad_posture_data;
                    bad_posture_data["sensor1"]["roll"] = last_roll[0];
                    bad_posture_data["sensor1"]["pitch"] = last_pitch[0];
                    bad_posture_data["sensor2"]["roll"] = last_roll[1];
                    bad_posture_data["sensor2"]["pitch"] = last_pitch[1];
                    bad_posture_data["sensor3"]["roll"] = last_roll[2];
                    bad_posture_data["sensor3"]["pitch"] = last_pitch[2];
                    bad_posture_data["sensor4"]["roll"] = last_roll[3];
                    bad_posture_data["sensor4"]["pitch"] = last_pitch[3];
                    bad_posture_data["status"] = "má postura";
                    bad_posture_data["timestamp"] = (int64_t)time(NULL);
                    
                    // Cria um caminho único usando timestamp para não sobrescrever
                    char unique_path[100];
                    snprintf(unique_path, sizeof(unique_path), "/ma_postura/%lld", (long long)time(NULL));
                    
                    bool result = db.putData(unique_path, bad_posture_data);
                    ESP_LOGW(TAG, "ALERTA: Má postura por %d segundos! Dados enviados para %s (resultado: %s)", 
                             BAD_POSTURE_THRESHOLD, unique_path, result ? "sucesso" : "erro");
                    
                    bad_posture_sent = true;
                }
            }
        } else {
            // Postura boa - reseta o timer
            if (is_bad_posture) {
                ESP_LOGI(TAG, "Postura corrigida! Timer resetado.");
                is_bad_posture = false;
                bad_posture_sent = false;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

extern "C" void app_main(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    init_wifi();
    xTaskCreate(mpu_task, "mpu_task", 12288, NULL, 5, NULL);
}
