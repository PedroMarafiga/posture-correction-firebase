# Conexões dos 4 Sensores MPU6050

## Configuração dos Barramentos I2C

### Barramento I2C 0 (I2C_NUM_0)
- **SCL:** GPIO 26
- **SDA:** GPIO 25

### Barramento I2C 1 (I2C_NUM_1)
- **SCL:** GPIO 22
- **SDA:** GPIO 21

## Mapeamento dos Sensores

| Sensor ID | Barramento | Endereço I2C | Pino AD0 | Descrição |
|-----------|------------|--------------|----------|-----------|
| 0 | I2C0 | 0x68 | GND | MPU0 (I2C0, 0x68) |
| 1 | I2C0 | 0x69 | VCC | MPU1 (I2C0, 0x69) |
| 2 | I2C1 | 0x68 | GND | MPU2 (I2C1, 0x68) |
| 3 | I2C1 | 0x69 | VCC | MPU3 (I2C1, 0x69) |

## Conexões Físicas

### Sensor 0 (MPU0)
```
MPU6050 #1  ->  ESP32
VCC         ->  3.3V
GND         ->  GND
SCL         ->  GPIO26
SDA         ->  GPIO25
AD0         ->  GND (endereço 0x68)
```

### Sensor 1 (MPU1)
```
MPU6050 #2  ->  ESP32
VCC         ->  3.3V
GND         ->  GND
SCL         ->  GPIO26 (compartilhado com MPU0)
SDA         ->  GPIO25 (compartilhado com MPU0)
AD0         ->  3.3V (endereço 0x69)
```

### Sensor 2 (MPU2)
```
MPU6050 #3  ->  ESP32
VCC         ->  3.3V
GND         ->  GND
SCL         ->  GPIO22
SDA         ->  GPIO21
AD0         ->  GND (endereço 0x68)
```

### Sensor 3 (MPU3)
```
MPU6050 #4  ->  ESP32
VCC         ->  3.3V
GND         ->  GND
SCL         ->  GPIO22 (compartilhado com MPU2)
SDA         ->  GPIO21 (compartilhado com MPU2)
AD0         ->  3.3V (endereço 0x69)
```

## Importante

1. **Resistores Pull-up:** Certifique-se de que há resistores pull-up de 4.7kΩ nos pinos SCL e SDA de cada barramento
2. **Alimentação:** Todos os sensores devem ser alimentados com 3.3V
3. **Pino AD0:** Este pino determina o endereço I2C:
   - AD0 = GND → endereço 0x68
   - AD0 = VCC → endereço 0x69
4. **Compartilhamento:** Sensores no mesmo barramento compartilham SCL e SDA, mas têm endereços diferentes
5. **Cabo:** Use cabos curtos e de boa qualidade para evitar interferências

## Como Funciona

- **2 barramentos I2C independentes** permitem melhor performance
- **2 endereços por barramento** usando o pino AD0
- **Total de 4 sensores** sem conflitos de endereço
- **Leitura sequencial** de todos os sensores no código
