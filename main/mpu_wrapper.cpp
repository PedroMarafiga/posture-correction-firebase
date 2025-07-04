#include "mpu_wrapper.h"
#include "mpu6050.h"
#include "math.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define I2C_MASTER_SCL_IO    26
#define I2C_MASTER_SDA_IO    25
#define I2C_MASTER_NUM_0     I2C_NUM_0
#define I2C_MASTER_SCL_IO_1  22
#define I2C_MASTER_SDA_IO_1  21
#define I2C_MASTER_NUM_1     I2C_NUM_1
#define I2C_FREQ_HZ          400000

static const char *TAG = "MPU_WRAPPER";
static mpu6050_handle_t mpu0 = NULL;  // I2C0, endereço 0x68
static mpu6050_handle_t mpu1 = NULL;  // I2C0, endereço 0x69
static mpu6050_handle_t mpu2 = NULL;  // I2C1, endereço 0x68
static mpu6050_handle_t mpu3 = NULL;  // I2C1, endereço 0x69

static float calculate_roll(float ax, float ay, float az) {
    return atan(ay / sqrt(ax * ax + az * az)) * (180.0 / M_PI);
}

static float calculate_pitch(float ax, float ay, float az) {
    return atan(-ax / sqrt(ay * ay + az * az)) * (180.0 / M_PI);
}

bool mpu6050_init_all(void) {
    esp_err_t ret;

    // Configuração do barramento I2C 0
    i2c_config_t conf0 = {};
    conf0.mode = I2C_MODE_MASTER;
    conf0.sda_io_num = I2C_MASTER_SDA_IO;
    conf0.scl_io_num = I2C_MASTER_SCL_IO;
    conf0.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf0.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf0.master.clk_speed = I2C_FREQ_HZ;

    ret = i2c_param_config(I2C_MASTER_NUM_0, &conf0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Erro na configuração do I2C 0");
        return false;
    }

    ret = i2c_driver_install(I2C_MASTER_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Erro na instalação do driver I2C 0");
        return false;
    }

    // Configuração do barramento I2C 1
    i2c_config_t conf1 = {};
    conf1.mode = I2C_MODE_MASTER;
    conf1.sda_io_num = I2C_MASTER_SDA_IO_1;
    conf1.scl_io_num = I2C_MASTER_SCL_IO_1;
    conf1.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf1.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf1.master.clk_speed = I2C_FREQ_HZ;

    ret = i2c_param_config(I2C_MASTER_NUM_1, &conf1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Erro na configuração do I2C 1");
        return false;
    }

    ret = i2c_driver_install(I2C_MASTER_NUM_1, I2C_MODE_MASTER, 0, 0, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Erro na instalação do driver I2C 1");
        return false;
    }

    // Inicialização dos sensores MPU6050
    mpu0 = mpu6050_create(I2C_MASTER_NUM_0, 0x68);  // I2C0, endereço 0x68
    if (mpu0 == NULL) {
        ESP_LOGE(TAG, "Erro ao criar MPU6050 0 (I2C0, 0x68)");
        return false;
    }

    mpu1 = mpu6050_create(I2C_MASTER_NUM_0, 0x69);  // I2C0, endereço 0x69
    if (mpu1 == NULL) {
        ESP_LOGE(TAG, "Erro ao criar MPU6050 1 (I2C0, 0x69)");
        return false;
    }

    mpu2 = mpu6050_create(I2C_MASTER_NUM_1, 0x68);  // I2C1, endereço 0x68
    if (mpu2 == NULL) {
        ESP_LOGE(TAG, "Erro ao criar MPU6050 2 (I2C1, 0x68)");
        return false;
    }

    mpu3 = mpu6050_create(I2C_MASTER_NUM_1, 0x69);  // I2C1, endereço 0x69
    if (mpu3 == NULL) {
        ESP_LOGE(TAG, "Erro ao criar MPU6050 3 (I2C1, 0x69)");
        return false;
    }

    // Configuração dos sensores
    mpu6050_wake_up(mpu0);
    mpu6050_wake_up(mpu1);
    mpu6050_wake_up(mpu2);
    mpu6050_wake_up(mpu3);
    
    mpu6050_config(mpu0, ACCE_FS_8G, GYRO_FS_500DPS);
    mpu6050_config(mpu1, ACCE_FS_8G, GYRO_FS_500DPS);
    mpu6050_config(mpu2, ACCE_FS_8G, GYRO_FS_500DPS);
    mpu6050_config(mpu3, ACCE_FS_8G, GYRO_FS_500DPS);

    ESP_LOGI(TAG, "4 sensores MPU6050 inicializados com sucesso");
    return true;
}

bool mpu6050_get_orientation(int sensor_id, float *roll, float *pitch) {
    if (roll == NULL || pitch == NULL) {
        return false;
    }

    mpu6050_handle_t mpu;
    const char* sensor_name;
    
    switch (sensor_id) {
        case 0:
            mpu = mpu0;
            sensor_name = "MPU0 (I2C0, 0x68)";
            break;
        case 1:
            mpu = mpu1;
            sensor_name = "MPU1 (I2C0, 0x69)";
            break;
        case 2:
            mpu = mpu2;
            sensor_name = "MPU2 (I2C1, 0x68)";
            break;
        case 3:
            mpu = mpu3;
            sensor_name = "MPU3 (I2C1, 0x69)";
            break;
        default:
            ESP_LOGE(TAG, "ID de sensor inválido: %d (deve ser 0-3)", sensor_id);
            return false;
    }

    if (mpu == NULL) {
        ESP_LOGE(TAG, "Sensor %d (%s) não inicializado", sensor_id, sensor_name);
        return false;
    }

    mpu6050_acce_value_t accel;
    esp_err_t ret = mpu6050_get_acce(mpu, &accel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao ler dados do sensor %d (%s)", sensor_id, sensor_name);
        return false;
    }

    *roll = calculate_roll(accel.acce_x, accel.acce_y, accel.acce_z);
    *pitch = calculate_pitch(accel.acce_x, accel.acce_y, accel.acce_z);

    return true;
}
