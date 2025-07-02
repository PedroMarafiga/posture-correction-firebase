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
static mpu6050_handle_t mpu0 = NULL;
static mpu6050_handle_t mpu1 = NULL;

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
    mpu0 = mpu6050_create(I2C_MASTER_NUM_0, 0x68);
    if (mpu0 == NULL) {
        ESP_LOGE(TAG, "Erro ao criar MPU6050 0");
        return false;
    }

    mpu1 = mpu6050_create(I2C_MASTER_NUM_1, 0x68);
    if (mpu1 == NULL) {
        ESP_LOGE(TAG, "Erro ao criar MPU6050 1");
        return false;
    }

    // Configuração dos sensores
    mpu6050_wake_up(mpu0);
    mpu6050_wake_up(mpu1);
    mpu6050_config(mpu0, ACCE_FS_8G, GYRO_FS_500DPS);
    mpu6050_config(mpu1, ACCE_FS_8G, GYRO_FS_500DPS);

    ESP_LOGI(TAG, "MPU6050 inicializados com sucesso");
    return true;
}

bool mpu6050_get_orientation(int sensor_id, float *roll, float *pitch) {
    if (roll == NULL || pitch == NULL) {
        return false;
    }

    mpu6050_handle_t mpu;
    if (sensor_id == 0) {
        mpu = mpu0;
    } else if (sensor_id == 1) {
        mpu = mpu1;
    } else {
        ESP_LOGE(TAG, "ID de sensor inválido: %d", sensor_id);
        return false;
    }

    if (mpu == NULL) {
        ESP_LOGE(TAG, "Sensor %d não inicializado", sensor_id);
        return false;
    }

    mpu6050_acce_value_t accel;
    esp_err_t ret = mpu6050_get_acce(mpu, &accel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao ler dados do sensor %d", sensor_id);
        return false;
    }

    *roll = calculate_roll(accel.acce_x, accel.acce_y, accel.acce_z);
    *pitch = calculate_pitch(accel.acce_x, accel.acce_y, accel.acce_z);

    return true;
}
