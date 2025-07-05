# Sistema de Monitoramento de Postura - MPU6050 + Firebase

Este projeto monitora a postura usando 4 sensores MPU6050 e envia alertas para Firebase quando detecta má postura.

## 🔧 Configuração de Wi-Fi

O sistema suporta dois tipos de conexão Wi-Fi:

### 🏠 Wi-Fi Normal (Casa/Trabalho)
Para redes Wi-Fi comuns com senha WPA/WPA2.

### 🏫 WPA2 Enterprise (Universidade)
Para redes corporativas/universitárias com autenticação EAP.

## 📝 Como Alternar Entre os Modos

### 1. Edite o arquivo `main/wifi_config.h`

```c
// Para Wi-Fi de casa
#define USE_ENTERPRISE_WIFI 0
#define WIFI_SSID_HOME "NomeDaSuaRede"
#define WIFI_PASSWORD_HOME "SenhaDaSuaRede"

// Para Wi-Fi da universidade
#define USE_ENTERPRISE_WIFI 1
// (configurações da UTFPR já estão prontas)
```

### 2. Recompile e faça upload

```bash
idf.py build
idf.py flash monitor
```

## 🔌 Conexões dos Sensores MPU6050

### Barramento I2C 0 (GPIO26/25)
- **Sensor 0:** AD0 = GND (endereço 0x68)
- **Sensor 1:** AD0 = VCC (endereço 0x69)

### Barramento I2C 1 (GPIO22/21)
- **Sensor 2:** AD0 = GND (endereço 0x68)
- **Sensor 3:** AD0 = VCC (endereço 0x69)

## 🎯 Lógica de Detecção de Má Postura

- **Posição Base:** Pitch = 0°, Roll = 90°
- **Tolerância:** ±20°
- **Timer:** 20 segundos de má postura consecutiva
- **Ação:** Envia dados para Firebase em `/ma_postura/{timestamp}`

## 📊 Estrutura dos Dados no Firebase

```json
{
  "sensor1": {"roll": 85.2, "pitch": 5.1},
  "sensor2": {"roll": 88.7, "pitch": 2.3},
  "sensor3": {"roll": 91.1, "pitch": -1.2},
  "sensor4": {"roll": 89.9, "pitch": 3.4},
  "status": "má postura",
  "timestamp": 1672531200
}
```

## 🚀 Como Usar

1. **Configure o Wi-Fi** editando `wifi_config.h`
2. **Configure o Firebase** editando `firebase_config.h`
3. **Conecte os sensores** conforme o diagrama
4. **Compile e faça upload**
5. **Monitore os logs** para verificar conexões

## 📋 Logs Importantes

- `Wi-Fi conectado` - Conexão estabelecida
- `Firebase conectado` - Autenticação realizada
- `4 sensores MPU6050 inicializados` - Sensores funcionando
- `Má postura detectada!` - Início da contagem
- `ALERTA: Má postura por 20 segundos!` - Dados enviados

## ⚡ Troubleshooting

### Wi-Fi não conecta
- Verifique SSID e senha no `wifi_config.h`
- Para Enterprise, confirme credenciais EAP

### Sensores não funcionam
- Verifique conexões físicas e resistores pull-up
- Confirme logs para erros específicos

### Firebase não funciona
- Confirme credenciais no `firebase_config.h`
- Verifique regras de segurança do Firebase

## 📁 Estrutura do Projeto

```
main/
├── wpa2_enterprise_main.cpp  # Código principal
├── mpu_wrapper.cpp/.h        # Interface dos sensores
├── wifi_config.h             # Configurações Wi-Fi
├── firebase_config.h         # Configurações Firebase
└── CMakeLists.txt           # Build system
```

## 🔑 Configuração Rápida

### Para usar em casa:
1. Edite `main/wifi_config.h`:
   ```c
   #define USE_ENTERPRISE_WIFI 0
   #define WIFI_SSID_HOME "SeuWiFi"
   #define WIFI_PASSWORD_HOME "SuaSenha"
   ```
2. `idf.py build flash monitor`

### Para usar na universidade:
1. Edite `main/wifi_config.h`:
   ```c
   #define USE_ENTERPRISE_WIFI 1
   ```
2. `idf.py build flash monitor`
