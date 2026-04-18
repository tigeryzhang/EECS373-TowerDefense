#ifndef PVZ_UART_RX_H
#define PVZ_UART_RX_H

#include "pvz_frontend.h"
#include "stm32l4xx_hal.h"

#include <stdbool.h>
#include <stdint.h>

bool pvz_uart_rx_init(UART_HandleTypeDef *huart, const GameConfig *config);
bool pvz_uart_rx_read_latest(PvzFrontendSnapshot *out_snapshot, uint32_t *age_ms, uint32_t *snapshot_generation);

#endif
