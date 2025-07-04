#ifndef MPU_WRAPPER_H
#define MPU_WRAPPER_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicializa os quatro sensores MPU6050
 * @return true se a inicialização foi bem-sucedida, false caso contrário
 */
bool mpu6050_init_all(void);

/**
 * @brief Obtém os valores de roll e pitch de um sensor específico
 * @param sensor_id ID do sensor (0, 1, 2 ou 3)
 *                  0: I2C0 endereço 0x68
 *                  1: I2C0 endereço 0x69
 *                  2: I2C1 endereço 0x68  
 *                  3: I2C1 endereço 0x69
 * @param roll Ponteiro para armazenar o valor do roll
 * @param pitch Ponteiro para armazenar o valor do pitch
 * @return true se a leitura foi bem-sucedida, false caso contrário
 */
bool mpu6050_get_orientation(int sensor_id, float *roll, float *pitch);

#ifdef __cplusplus
}
#endif

#endif // MPU_WRAPPER_H