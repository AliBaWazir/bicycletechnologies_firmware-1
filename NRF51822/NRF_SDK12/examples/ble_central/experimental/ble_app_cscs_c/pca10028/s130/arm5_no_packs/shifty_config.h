/*These file contains contains PIN definitions customized for shifty hardware*/
#ifndef SHIFTY_CONFIG_H
#define SHIFTY_CONFIG_H

/*nRF custom PCP version number. Set the specified version to 1*/
#define NRF_BCP_V1   1         


#if NRF_BCP_V1
/*********************************************************************
*      definitions for nRF PCP version 1
*********************************************************************/
// <h> SPIS_CONFIGURATION - Spi slave configuration
//==========================================================
#define APP_SPIS_IRQ_PIN      0
#define APP_SPIS_CS_PIN       1
#define APP_SPIS_MOSI_PIN     2
#define APP_SPIS_MISO_PIN     3
#define APP_SPIS_SCK_PIN      4
#define APP_SPIS_IRQ_PRIORITY 3

// <e> NRF_LOG_BACKEND_SERIAL_USES_UART - If enabled data is printed over UART
//==========================================================
#define NRF_LOG_BACKEND_SERIAL_UART_TX_PIN       25
#define NRF_LOG_BACKEND_SERIAL_UART_RX_PIN       24
#define NRF_LOG_BACKEND_SERIAL_UART_RTS_PIN      20 //unused pin
#define NRF_LOG_BACKEND_SERIAL_UART_CTS_PIN      20 //unused pin
#define NRF_LOG_BACKEND_SERIAL_UART_FLOW_CONTROL 0

// I2C
//==========================================================
#define NRF_SCL_PIN         10
#define NRF_SDA_PIN         11

// Other PIN assignments
//==========================================================
#define BNO_INT_PIN     5
#define BNO_B_IND_PIN   6

#define NRF_LEC_PIN     8
#define IRQ1_PIN        9

#define ISET_RESISTOR_ARRAY_PIN  26
#define LOW_BAT_PIN              27




#else 
/*********************************************************************
*      definitions for nRF PCP version 2
*********************************************************************/  
// <h> SPIS_CONFIGURATION - Spi slave configuration
//==========================================================
#define APP_SPIS_IRQ_PIN      2
#define APP_SPIS_CS_PIN       0
#define APP_SPIS_MOSI_PIN     28
#define APP_SPIS_MISO_PIN     23
#define APP_SPIS_SCK_PIN      22
#define APP_SPIS_IRQ_PRIORITY 3

// <e> NRF_LOG_BACKEND_SERIAL_USES_UART - If enabled data is printed over UART
//==========================================================
#define NRF_LOG_BACKEND_SERIAL_UART_TX_PIN       20
#define NRF_LOG_BACKEND_SERIAL_UART_RX_PIN       17
#define NRF_LOG_BACKEND_SERIAL_UART_RTS_PIN      37//unused pin
#define NRF_LOG_BACKEND_SERIAL_UART_CTS_PIN      39//unused pin
#define NRF_LOG_BACKEND_SERIAL_UART_FLOW_CONTROL 0

// I2C
//==========================================================
#define NRF_SCL_PIN       1  
#define NRF_SDA_PIN       3

// Other PIN assignments
//==========================================================
#define NRF_POGO_IRQ_PIN    9
#define NRF_DISPLAY_EN_PIN  8
#define NRF_EN_REG_PIN      7
#define NRF_LOW_BAT_PIN     19

//#define BNO_INT_PIN       5
//#define BNO_B_IND_PIN     6
//#define NRF_LEC_PIN       8
//#define IRQ1_PIN          9
//#define ISET_RESISTOR_ARRAY_PIN  26


#endif //NRF_BCP_V1




#endif //SDK_CONFIG_H
