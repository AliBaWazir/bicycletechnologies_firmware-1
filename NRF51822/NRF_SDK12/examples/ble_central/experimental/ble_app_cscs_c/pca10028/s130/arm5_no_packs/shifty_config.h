/*These file contains contains PIN definitions customized for shifty hardware*/
#ifndef SHIFTY_CONFIG_H
#define SHIFTY_CONFIG_H

//==========================================================
// <h> SPIS_CONFIGURATION - Spi slave configuration

//==========================================================
#define APP_SPIS_SCK_PIN      4
#define APP_SPIS_MISO_PIN     3
#define APP_SPIS_MOSI_PIN     2
#define APP_SPIS_CS_PIN       1
#define APP_SPIS_IRQ_PRIORITY 3


// <e> NRF_LOG_BACKEND_SERIAL_USES_UART - If enabled data is printed over UART
//==========================================================
#define NRF_LOG_BACKEND_SERIAL_UART_TX_PIN       25
#define NRF_LOG_BACKEND_SERIAL_UART_RX_PIN       24
#define NRF_LOG_BACKEND_SERIAL_UART_RTS_PIN      0
#define NRF_LOG_BACKEND_SERIAL_UART_CTS_PIN      0
#define NRF_LOG_BACKEND_SERIAL_UART_FLOW_CONTROL 0

#endif //SDK_CONFIG_H
