#ifndef __SX1276_H__
#define __SX1276_H__

#include "main.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SX1276_MAX_PACKET_SIZE            255U
#define SX1276_SPI_TIMEOUT_DEFAULT        1000U
#define SX1276_COMPATIBLE_VERSION         0x12U

#define SX1276_MHZ                        1000000ULL
#define SX1276_FREQ_433MHZ                (433ULL * SX1276_MHZ)
#define SX1276_FREQ_447MHZ                (447ULL * SX1276_MHZ)
#define SX1276_FREQ_868MHZ                (868ULL * SX1276_MHZ)
#define SX1276_FREQ_915MHZ                (915ULL * SX1276_MHZ)

#define SX1276_OK                         0U
#define SX1276_ERROR                      1U
#define SX1276_BUSY                       2U
#define SX1276_INVALID_PARAM              3U
#define SX1276_TIMEOUT                    4U
#define SX1276_EMPTY                      5U
#define SX1276_CRC_ERROR                  6U
#define SX1276_INVALID_HEADER             7U

#define SX1276_PA_OUTPUT_RFO              0U
#define SX1276_PA_OUTPUT_PA_BOOST         1U

#define SX1276_CODING_RATE_4_5            0x01U
#define SX1276_CODING_RATE_4_6            0x02U
#define SX1276_CODING_RATE_4_7            0x03U
#define SX1276_CODING_RATE_4_8            0x04U

#define SX1276_IRQ_RX_TIMEOUT             0x80U
#define SX1276_IRQ_RX_DONE                0x40U
#define SX1276_IRQ_PAYLOAD_CRC_ERROR      0x20U
#define SX1276_IRQ_VALID_HEADER           0x10U
#define SX1276_IRQ_TX_DONE                0x08U
#define SX1276_IRQ_CAD_DONE               0x04U
#define SX1276_IRQ_FHSS_CHANGE_CHANNEL    0x02U
#define SX1276_IRQ_CAD_DETECTED           0x01U
#define SX1276_IRQ_RX_ALL                 0xF0U
#define SX1276_IRQ_ALL                    0xFFU

typedef enum {
    SX1276_BW_7_8_KHZ = 0,
    SX1276_BW_10_4_KHZ,
    SX1276_BW_15_6_KHZ,
    SX1276_BW_20_8_KHZ,
    SX1276_BW_31_25_KHZ,
    SX1276_BW_41_7_KHZ,
    SX1276_BW_62_5_KHZ,
    SX1276_BW_125_KHZ,
    SX1276_BW_250_KHZ,
    SX1276_BW_500_KHZ,
    SX1276_BW_LAST
} sx1276_bandwidth_t;

typedef enum {
    SX1276_MODE_SLEEP = 0x00,
    SX1276_MODE_STDBY = 0x01,
    SX1276_MODE_TX = 0x03,
    SX1276_MODE_RX_CONTINUOUS = 0x05,
    SX1276_MODE_RX_SINGLE = 0x06
} sx1276_mode_t;

typedef struct {
    SPI_HandleTypeDef *spi;
    GPIO_TypeDef *nss_port;
    uint16_t nss_pin;
    uint32_t spi_timeout;

    uint32_t frequency;
    uint8_t pa_mode;
    uint8_t tx_base_addr;
    uint8_t rx_base_addr;
} sx1276_t;

uint8_t sx1276_init(sx1276_t *dev,
                    SPI_HandleTypeDef *spi,
                    GPIO_TypeDef *nss_port,
                    uint16_t nss_pin,
                    uint32_t frequency_hz);

void sx1276_reset(GPIO_TypeDef *rst_port, uint16_t rst_pin);
uint8_t sx1276_get_version(sx1276_t *dev);

uint8_t sx1276_read_reg(sx1276_t *dev, uint8_t address);
void sx1276_write_reg(sx1276_t *dev, uint8_t address, uint8_t value);
void sx1276_write_burst(sx1276_t *dev, uint8_t address, const uint8_t *buffer, uint8_t len);
void sx1276_read_burst(sx1276_t *dev, uint8_t address, uint8_t *buffer, uint8_t len);

void sx1276_set_mode(sx1276_t *dev, sx1276_mode_t mode);
void sx1276_set_sleep(sx1276_t *dev);
void sx1276_set_standby(sx1276_t *dev);
void sx1276_set_rx_continuous(sx1276_t *dev);
void sx1276_set_rx_single(sx1276_t *dev);

void sx1276_set_frequency(sx1276_t *dev, uint32_t frequency_hz);
void sx1276_set_tx_power(sx1276_t *dev, uint8_t level_dbm);
void sx1276_set_signal_bandwidth(sx1276_t *dev, sx1276_bandwidth_t bw);
void sx1276_set_spreading_factor(sx1276_t *dev, uint8_t sf);
void sx1276_set_coding_rate(sx1276_t *dev, uint8_t coding_rate);
void sx1276_set_crc(sx1276_t *dev, uint8_t enable);
void sx1276_set_preamble_length(sx1276_t *dev, uint16_t length);
void sx1276_set_rx_symbol_timeout(sx1276_t *dev, uint16_t symbols);
void sx1276_set_explicit_header_mode(sx1276_t *dev);
void sx1276_set_implicit_header_mode(sx1276_t *dev);

void sx1276_set_fifo_tx_base_addr(sx1276_t *dev, uint8_t address);
void sx1276_set_fifo_rx_base_addr(sx1276_t *dev, uint8_t address);
void sx1276_write_fifo(sx1276_t *dev, const uint8_t *data, uint8_t length);
uint8_t sx1276_read_fifo(sx1276_t *dev, uint8_t *data, uint8_t max_length);
uint8_t sx1276_get_packet_length(sx1276_t *dev);

void sx1276_start_tx(sx1276_t *dev, const uint8_t *data, uint8_t length);
uint8_t sx1276_is_transmitting(sx1276_t *dev);

uint8_t sx1276_get_irq_flags(sx1276_t *dev);
void sx1276_clear_irq_flags(sx1276_t *dev, uint8_t mask);
void sx1276_clear_all_irq_flags(sx1276_t *dev);
void sx1276_map_dio0_to_rx_done(sx1276_t *dev);
void sx1276_map_dio0_to_tx_done(sx1276_t *dev);

uint8_t sx1276_is_packet_available(sx1276_t *dev);
uint8_t sx1276_fetch_received(sx1276_t *dev, uint8_t *buffer, uint8_t max_length, uint8_t *error);

int16_t sx1276_get_packet_rssi(sx1276_t *dev);
int8_t sx1276_get_packet_snr(sx1276_t *dev);

#ifdef __cplusplus
}
#endif

#endif
