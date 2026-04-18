#ifndef PVZ_UART_PROTOCOL_H
#define PVZ_UART_PROTOCOL_H

#include "pvz_frontend.h"

#include <stdbool.h>
#include <stdint.h>

bool pvz_uart_protocol_decode_packet(const uint8_t *packet, uint16_t size, const GameConfig *config,
									 PvzFrontendSnapshot *out_snapshot);

#endif
