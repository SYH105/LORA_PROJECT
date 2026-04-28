#include "SX1276.h"

#define SX1276_REG_FIFO                    0x00U
#define SX1276_REG_OP_MODE                 0x01U
#define SX1276_REG_FRF_MSB                 0x06U
#define SX1276_REG_FRF_MID                 0x07U
#define SX1276_REG_FRF_LSB                 0x08U
#define SX1276_REG_PA_CONFIG               0x09U
#define SX1276_REG_OCP                     0x0BU
#define SX1276_REG_LNA                     0x0CU
#define SX1276_REG_FIFO_ADDR_PTR           0x0DU
#define SX1276_REG_FIFO_TX_BASE_ADDR       0x0EU
#define SX1276_REG_FIFO_RX_BASE_ADDR       0x0FU
#define SX1276_REG_FIFO_RX_CURRENT_ADDR    0x10U
#define SX1276_REG_IRQ_FLAGS               0x12U
#define SX1276_REG_RX_NB_BYTES             0x13U
#define SX1276_REG_PKT_SNR_VALUE           0x19U
#define SX1276_REG_PKT_RSSI_VALUE          0x1AU
#define SX1276_REG_MODEM_CONFIG_1          0x1DU
#define SX1276_REG_MODEM_CONFIG_2          0x1EU
#define SX1276_REG_SYMB_TIMEOUT_LSB        0x1FU
#define SX1276_REG_PREAMBLE_MSB            0x20U
#define SX1276_REG_PREAMBLE_LSB            0x21U
#define SX1276_REG_PAYLOAD_LENGTH          0x22U
#define SX1276_REG_MODEM_CONFIG_3          0x26U
#define SX1276_REG_DETECTION_OPTIMIZE      0x31U
#define SX1276_REG_DETECTION_THRESHOLD     0x37U
#define SX1276_REG_DIO_MAPPING_1           0x40U
#define SX1276_REG_VERSION                 0x42U
#define SX1276_REG_PA_DAC                  0x4DU

#define SX1276_LONG_RANGE_MODE             0x80U
#define SX1276_PA_DAC_HIGH_POWER           0x87U
#define SX1276_PA_DAC_DEFAULT              0x84U
#define SX1276_OCP_ON                      0x20U
#define SX1276_MC1_IMPLICIT_HEADER_MODE    0x01U
#define SX1276_MC2_CRC_ON                  0x04U
#define SX1276_MC3_AGC_AUTO                0x04U
#define SX1276_MC3_LOW_DATA_RATE_OPTIMIZE  0x08U
#define SX1276_BIT7                        0x80U

static void sx1276_select(sx1276_t *dev)
{
    HAL_GPIO_WritePin(dev->nss_port, dev->nss_pin, GPIO_PIN_RESET);
}

static void sx1276_unselect(sx1276_t *dev)
{
    HAL_GPIO_WritePin(dev->nss_port, dev->nss_pin, GPIO_PIN_SET);
}

static void sx1276_set_ocp(sx1276_t *dev, uint8_t imax)
{
    uint8_t ocp_trim;

    if (imax < 45U) {
        imax = 45U;
    }
    if (imax > 240U) {
        imax = 240U;
    }

    if (imax <= 120U) {
        ocp_trim = (uint8_t)((imax - 45U) / 5U);
    } else {
        ocp_trim = (uint8_t)((imax + 30U) / 10U);
    }

    sx1276_write_reg(dev, SX1276_REG_OCP, (uint8_t)(SX1276_OCP_ON | ocp_trim));
}

static void sx1276_update_ldro(sx1276_t *dev)
{
    uint8_t mc1 = sx1276_read_reg(dev, SX1276_REG_MODEM_CONFIG_1);
    uint8_t mc2 = sx1276_read_reg(dev, SX1276_REG_MODEM_CONFIG_2);
    uint8_t mc3 = SX1276_MC3_AGC_AUTO;
    uint8_t bw = (uint8_t)((mc1 >> 4) & 0x0FU);
    uint8_t sf = (uint8_t)((mc2 >> 4) & 0x0FU);

    if ((bw == SX1276_BW_125_KHZ && sf >= 11U) ||
        (bw == SX1276_BW_62_5_KHZ && sf >= 10U) ||
        (bw == SX1276_BW_31_25_KHZ && sf >= 9U)) {
        mc3 |= SX1276_MC3_LOW_DATA_RATE_OPTIMIZE;
    }

    sx1276_write_reg(dev, SX1276_REG_MODEM_CONFIG_3, mc3);
}

uint8_t sx1276_read_reg(sx1276_t *dev, uint8_t address)
{
    uint8_t tx = (uint8_t)(address & 0x7FU);
    uint8_t rx = 0U;

    sx1276_select(dev);
    HAL_SPI_Transmit(dev->spi, &tx, 1U, dev->spi_timeout);
    HAL_SPI_Receive(dev->spi, &rx, 1U, dev->spi_timeout);
    sx1276_unselect(dev);

    return rx;
}

void sx1276_write_reg(sx1276_t *dev, uint8_t address, uint8_t value)
{
    uint8_t tx[2];

    tx[0] = (uint8_t)(address | SX1276_BIT7);
    tx[1] = value;

    sx1276_select(dev);
    HAL_SPI_Transmit(dev->spi, tx, 2U, dev->spi_timeout);
    sx1276_unselect(dev);
}

void sx1276_write_burst(sx1276_t *dev, uint8_t address, const uint8_t *buffer, uint8_t len)
{
    uint8_t header = (uint8_t)(address | SX1276_BIT7);

    sx1276_select(dev);
    HAL_SPI_Transmit(dev->spi, &header, 1U, dev->spi_timeout);
    HAL_SPI_Transmit(dev->spi, (uint8_t *)buffer, len, dev->spi_timeout);
    sx1276_unselect(dev);
}

void sx1276_read_burst(sx1276_t *dev, uint8_t address, uint8_t *buffer, uint8_t len)
{
    uint8_t header = (uint8_t)(address & 0x7FU);

    sx1276_select(dev);
    HAL_SPI_Transmit(dev->spi, &header, 1U, dev->spi_timeout);
    HAL_SPI_Receive(dev->spi, buffer, len, dev->spi_timeout);
    sx1276_unselect(dev);
}

void sx1276_reset(GPIO_TypeDef *rst_port, uint16_t rst_pin)
{
    HAL_GPIO_WritePin(rst_port, rst_pin, GPIO_PIN_RESET);
    HAL_Delay(1U);
    HAL_GPIO_WritePin(rst_port, rst_pin, GPIO_PIN_SET);
    HAL_Delay(10U);
}

uint8_t sx1276_get_version(sx1276_t *dev)
{
    return sx1276_read_reg(dev, SX1276_REG_VERSION);
}

void sx1276_set_mode(sx1276_t *dev, sx1276_mode_t mode)
{
    sx1276_write_reg(dev, SX1276_REG_OP_MODE, (uint8_t)(SX1276_LONG_RANGE_MODE | (uint8_t)mode));
}

void sx1276_set_sleep(sx1276_t *dev)
{
    sx1276_set_mode(dev, SX1276_MODE_SLEEP);
}

void sx1276_set_standby(sx1276_t *dev)
{
    sx1276_set_mode(dev, SX1276_MODE_STDBY);
}

void sx1276_set_rx_continuous(sx1276_t *dev)
{
    sx1276_write_reg(dev, SX1276_REG_FIFO_RX_BASE_ADDR, dev->rx_base_addr);
    sx1276_clear_irq_flags(dev, SX1276_IRQ_RX_ALL);
    sx1276_set_mode(dev, SX1276_MODE_RX_CONTINUOUS);
}

void sx1276_set_rx_single(sx1276_t *dev)
{
    sx1276_write_reg(dev, SX1276_REG_FIFO_RX_BASE_ADDR, dev->rx_base_addr);
    sx1276_clear_irq_flags(dev, SX1276_IRQ_RX_ALL);
    sx1276_set_mode(dev, SX1276_MODE_RX_SINGLE);
}

void sx1276_set_frequency(sx1276_t *dev, uint32_t frequency_hz)
{
    uint64_t frf = (((uint64_t)frequency_hz) << 19) / 32000000ULL;

    dev->frequency = frequency_hz;
    sx1276_write_reg(dev, SX1276_REG_FRF_MSB, (uint8_t)(frf >> 16));
    sx1276_write_reg(dev, SX1276_REG_FRF_MID, (uint8_t)(frf >> 8));
    sx1276_write_reg(dev, SX1276_REG_FRF_LSB, (uint8_t)(frf));
}

void sx1276_set_tx_power(sx1276_t *dev, uint8_t level_dbm)
{
    if (dev->pa_mode == SX1276_PA_OUTPUT_RFO) {
        if (level_dbm > 15U) {
            level_dbm = 15U;
        }
        sx1276_write_reg(dev, SX1276_REG_PA_CONFIG, (uint8_t)(0x70U | level_dbm));
        return;
    }

    if (level_dbm < 2U) {
        level_dbm = 2U;
    }
    if (level_dbm > 20U) {
        level_dbm = 20U;
    }

    if (level_dbm > 17U) {
        sx1276_write_reg(dev, SX1276_REG_PA_DAC, SX1276_PA_DAC_HIGH_POWER);
        sx1276_set_ocp(dev, 140U);
        level_dbm = (uint8_t)(level_dbm - 3U);
    } else {
        sx1276_write_reg(dev, SX1276_REG_PA_DAC, SX1276_PA_DAC_DEFAULT);
        sx1276_set_ocp(dev, 100U);
    }

    sx1276_write_reg(dev, SX1276_REG_PA_CONFIG, (uint8_t)(0x80U | (level_dbm - 2U)));
}

void sx1276_set_signal_bandwidth(sx1276_t *dev, sx1276_bandwidth_t bw)
{
    uint8_t mc1;

    if (bw >= SX1276_BW_LAST) {
        return;
    }

    mc1 = sx1276_read_reg(dev, SX1276_REG_MODEM_CONFIG_1);
    mc1 = (uint8_t)((mc1 & 0x0FU) | ((uint8_t)bw << 4));
    sx1276_write_reg(dev, SX1276_REG_MODEM_CONFIG_1, mc1);
    sx1276_update_ldro(dev);
}

void sx1276_set_spreading_factor(sx1276_t *dev, uint8_t sf)
{
    uint8_t mc2;

    if (sf < 6U) {
        sf = 6U;
    }
    if (sf > 12U) {
        sf = 12U;
    }

    if (sf == 6U) {
        sx1276_write_reg(dev, SX1276_REG_DETECTION_OPTIMIZE, 0xC5U);
        sx1276_write_reg(dev, SX1276_REG_DETECTION_THRESHOLD, 0x0CU);
    } else {
        sx1276_write_reg(dev, SX1276_REG_DETECTION_OPTIMIZE, 0xC3U);
        sx1276_write_reg(dev, SX1276_REG_DETECTION_THRESHOLD, 0x0AU);
    }

    mc2 = sx1276_read_reg(dev, SX1276_REG_MODEM_CONFIG_2);
    mc2 = (uint8_t)((mc2 & 0x0FU) | (sf << 4));
    sx1276_write_reg(dev, SX1276_REG_MODEM_CONFIG_2, mc2);
    sx1276_update_ldro(dev);
}

void sx1276_set_coding_rate(sx1276_t *dev, uint8_t coding_rate)
{
    uint8_t mc1;

    if (coding_rate < SX1276_CODING_RATE_4_5) {
        coding_rate = SX1276_CODING_RATE_4_5;
    }
    if (coding_rate > SX1276_CODING_RATE_4_8) {
        coding_rate = SX1276_CODING_RATE_4_8;
    }

    mc1 = sx1276_read_reg(dev, SX1276_REG_MODEM_CONFIG_1);
    mc1 &= (uint8_t)~0x0EU;
    mc1 |= (uint8_t)(coding_rate << 1);
    sx1276_write_reg(dev, SX1276_REG_MODEM_CONFIG_1, mc1);
}

void sx1276_set_crc(sx1276_t *dev, uint8_t enable)
{
    uint8_t mc2 = sx1276_read_reg(dev, SX1276_REG_MODEM_CONFIG_2);

    if (enable != 0U) {
        mc2 |= SX1276_MC2_CRC_ON;
    } else {
        mc2 &= (uint8_t)~SX1276_MC2_CRC_ON;
    }

    sx1276_write_reg(dev, SX1276_REG_MODEM_CONFIG_2, mc2);
}

void sx1276_set_preamble_length(sx1276_t *dev, uint16_t length)
{
    sx1276_write_reg(dev, SX1276_REG_PREAMBLE_MSB, (uint8_t)(length >> 8));
    sx1276_write_reg(dev, SX1276_REG_PREAMBLE_LSB, (uint8_t)(length));
}

void sx1276_set_rx_symbol_timeout(sx1276_t *dev, uint16_t symbols)
{
    uint8_t mc2;

    if (symbols < 4U) {
        symbols = 4U;
    }
    if (symbols > 1023U) {
        symbols = 1023U;
    }

    mc2 = sx1276_read_reg(dev, SX1276_REG_MODEM_CONFIG_2);
    mc2 &= 0xFCU;
    mc2 |= (uint8_t)((symbols >> 8) & 0x03U);
    sx1276_write_reg(dev, SX1276_REG_MODEM_CONFIG_2, mc2);
    sx1276_write_reg(dev, SX1276_REG_SYMB_TIMEOUT_LSB, (uint8_t)(symbols & 0xFFU));
}

void sx1276_set_explicit_header_mode(sx1276_t *dev)
{
    uint8_t mc1 = sx1276_read_reg(dev, SX1276_REG_MODEM_CONFIG_1);
    mc1 &= (uint8_t)~SX1276_MC1_IMPLICIT_HEADER_MODE;
    sx1276_write_reg(dev, SX1276_REG_MODEM_CONFIG_1, mc1);
}

void sx1276_set_implicit_header_mode(sx1276_t *dev)
{
    uint8_t mc1 = sx1276_read_reg(dev, SX1276_REG_MODEM_CONFIG_1);
    mc1 |= SX1276_MC1_IMPLICIT_HEADER_MODE;
    sx1276_write_reg(dev, SX1276_REG_MODEM_CONFIG_1, mc1);
}

void sx1276_set_fifo_tx_base_addr(sx1276_t *dev, uint8_t address)
{
    dev->tx_base_addr = address;
    sx1276_write_reg(dev, SX1276_REG_FIFO_TX_BASE_ADDR, address);
}

void sx1276_set_fifo_rx_base_addr(sx1276_t *dev, uint8_t address)
{
    dev->rx_base_addr = address;
    sx1276_write_reg(dev, SX1276_REG_FIFO_RX_BASE_ADDR, address);
}

void sx1276_write_fifo(sx1276_t *dev, const uint8_t *data, uint8_t length)
{
    sx1276_write_reg(dev, SX1276_REG_FIFO_ADDR_PTR, dev->tx_base_addr);
    sx1276_write_reg(dev, SX1276_REG_PAYLOAD_LENGTH, length);
    sx1276_write_burst(dev, SX1276_REG_FIFO, data, length);
}

uint8_t sx1276_get_packet_length(sx1276_t *dev)
{
    uint8_t implicit_header = (uint8_t)(sx1276_read_reg(dev, SX1276_REG_MODEM_CONFIG_1) & SX1276_MC1_IMPLICIT_HEADER_MODE);

    if (implicit_header != 0U) {
        return sx1276_read_reg(dev, SX1276_REG_PAYLOAD_LENGTH);
    }

    return sx1276_read_reg(dev, SX1276_REG_RX_NB_BYTES);
}

uint8_t sx1276_read_fifo(sx1276_t *dev, uint8_t *data, uint8_t max_length)
{
    uint8_t length = sx1276_get_packet_length(dev);
    uint8_t current_addr;

    if (length > max_length) {
        length = max_length;
    }

    current_addr = sx1276_read_reg(dev, SX1276_REG_FIFO_RX_CURRENT_ADDR);
    sx1276_write_reg(dev, SX1276_REG_FIFO_ADDR_PTR, current_addr);
    sx1276_read_burst(dev, SX1276_REG_FIFO, data, length);

    return length;
}

void sx1276_start_tx(sx1276_t *dev, const uint8_t *data, uint8_t length)
{
    if (length == 0U || length > SX1276_MAX_PACKET_SIZE) {
        return;
    }

    sx1276_set_standby(dev);
    sx1276_clear_irq_flags(dev, SX1276_IRQ_TX_DONE);
    sx1276_write_fifo(dev, data, length);
    sx1276_set_mode(dev, SX1276_MODE_TX);
}

uint8_t sx1276_is_transmitting(sx1276_t *dev)
{
    uint8_t opmode = sx1276_read_reg(dev, SX1276_REG_OP_MODE);
    return (((opmode & 0x07U) == SX1276_MODE_TX) ? 1U : 0U);
}

uint8_t sx1276_get_irq_flags(sx1276_t *dev)
{
    return sx1276_read_reg(dev, SX1276_REG_IRQ_FLAGS);
}

void sx1276_clear_irq_flags(sx1276_t *dev, uint8_t mask)
{
    sx1276_write_reg(dev, SX1276_REG_IRQ_FLAGS, mask);
}

void sx1276_clear_all_irq_flags(sx1276_t *dev)
{
    sx1276_write_reg(dev, SX1276_REG_IRQ_FLAGS, SX1276_IRQ_ALL);
}

void sx1276_map_dio0_to_rx_done(sx1276_t *dev)
{
    sx1276_write_reg(dev, SX1276_REG_DIO_MAPPING_1, 0x00U);
}

void sx1276_map_dio0_to_tx_done(sx1276_t *dev)
{
    sx1276_write_reg(dev, SX1276_REG_DIO_MAPPING_1, 0x40U);
}

uint8_t sx1276_is_packet_available(sx1276_t *dev)
{
    uint8_t irq = sx1276_get_irq_flags(dev);
    return ((irq & SX1276_IRQ_RX_DONE) != 0U) ? 1U : 0U;
}

uint8_t sx1276_fetch_received(sx1276_t *dev, uint8_t *buffer, uint8_t max_length, uint8_t *error)
{
    uint8_t irq = sx1276_get_irq_flags(dev);
    uint8_t result = SX1276_EMPTY;
    uint8_t length = 0U;

    sx1276_clear_irq_flags(dev, SX1276_IRQ_RX_ALL);

    if ((irq & SX1276_IRQ_RX_TIMEOUT) != 0U) {
        result = SX1276_TIMEOUT;
    } else if ((irq & SX1276_IRQ_RX_DONE) == 0U) {
        result = SX1276_EMPTY;
    } else if ((irq & SX1276_IRQ_VALID_HEADER) == 0U) {
        result = SX1276_INVALID_HEADER;
    } else if ((irq & SX1276_IRQ_PAYLOAD_CRC_ERROR) != 0U) {
        result = SX1276_CRC_ERROR;
    } else {
        length = sx1276_read_fifo(dev, buffer, max_length);
        result = SX1276_OK;
    }

    if (error != 0) {
        *error = result;
    }

    return length;
}

int16_t sx1276_get_packet_rssi(sx1276_t *dev)
{
    uint8_t raw = sx1276_read_reg(dev, SX1276_REG_PKT_RSSI_VALUE);

    if (dev->frequency < 868000000UL) {
        return (int16_t)raw - 164;
    }

    return (int16_t)raw - 157;
}

int8_t sx1276_get_packet_snr(sx1276_t *dev)
{
    return (int8_t)((int8_t)sx1276_read_reg(dev, SX1276_REG_PKT_SNR_VALUE) / 4);
}

uint8_t sx1276_init(sx1276_t *dev,
                    SPI_HandleTypeDef *spi,
                    GPIO_TypeDef *nss_port,
                    uint16_t nss_pin,
                    uint32_t frequency_hz)
{
    uint8_t version;
    uint8_t lna;

    if ((dev == 0) || (spi == 0) || (nss_port == 0)) {
        return SX1276_INVALID_PARAM;
    }

    dev->spi = spi;
    dev->nss_port = nss_port;
    dev->nss_pin = nss_pin;
    dev->spi_timeout = SX1276_SPI_TIMEOUT_DEFAULT;
    dev->frequency = frequency_hz;
    dev->pa_mode = SX1276_PA_OUTPUT_PA_BOOST;
    dev->tx_base_addr = 0x00U;
    dev->rx_base_addr = 0x00U;

    version = sx1276_get_version(dev);
    if (version != SX1276_COMPATIBLE_VERSION) {
        return SX1276_ERROR;
    }

    sx1276_set_sleep(dev);
    sx1276_set_frequency(dev, frequency_hz);
    sx1276_set_fifo_tx_base_addr(dev, 0x00U);
    sx1276_set_fifo_rx_base_addr(dev, 0x00U);
    sx1276_set_spreading_factor(dev, 10U);
    sx1276_set_signal_bandwidth(dev, SX1276_BW_125_KHZ);
    sx1276_set_coding_rate(dev, SX1276_CODING_RATE_4_5);
    sx1276_set_preamble_length(dev, 8U);
    sx1276_set_explicit_header_mode(dev);
    sx1276_set_crc(dev, 1U);

    lna = sx1276_read_reg(dev, SX1276_REG_LNA);
    sx1276_write_reg(dev, SX1276_REG_LNA, (uint8_t)(lna | 0x03U));

    sx1276_set_tx_power(dev, 20U);
    sx1276_clear_all_irq_flags(dev);
    sx1276_set_standby(dev);

    return SX1276_OK;
}
