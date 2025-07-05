#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

// ========================================
// CONFIGURAÇÃO DE WI-FI
// ========================================

// Escolha o tipo de Wi-Fi:
// 0 = Wi-Fi normal (casa/trabalho)
// 1 = WPA2 Enterprise (universidade)
#define USE_ENTERPRISE_WIFI 0

// ========================================
// Wi-Fi NORMAL (casa/trabalho)
// ========================================
#define WIFI_SSID_HOME "ITECPB-SPRINT"
#define WIFI_PASSWORD_HOME "SMCTI_2024@"

// ========================================
// WPA2 ENTERPRISE (universidade)
// ========================================
#define WIFI_SSID_ENTERPRISE "UTFPR-ALUNO"
#define EAP_ID "a2456621"
#define EAP_USERNAME "a2456621"
#define EAP_PASSWORD "qatezc10"

// ========================================
// INSTRUÇÕES DE USO:
// ========================================
/*
1. Para Wi-Fi de casa:
   - Mude USE_ENTERPRISE_WIFI para 0
   - Configure WIFI_SSID_HOME com o nome da sua rede
   - Configure WIFI_PASSWORD_HOME com a senha da sua rede
   - Recompile o projeto

2. Para Wi-Fi da universidade:
   - Mude USE_ENTERPRISE_WIFI para 1
   - As configurações da UTFPR já estão prontas
   - Recompile o projeto

3. Para recompilar:
   - No terminal: idf.py build
   - Para fazer upload: idf.py flash monitor
*/

#endif // WIFI_CONFIG_H
