#include "pvz_uart_rx.h"

#include "pvz_uart_protocol.h"

#include <string.h>

enum {
	PVZ_UART_RX_BUFFER_SIZE = 1 + (3 * PVZ_MAX_ROWS * PVZ_MAX_COLS),
};

typedef struct {
	UART_HandleTypeDef *uart_handle;
	const GameConfig *config;
	uint8_t rx_buffer[PVZ_UART_RX_BUFFER_SIZE];
	PvzFrontendSnapshot snapshots[2];
	volatile uint8_t published_index;
	volatile uint32_t published_generation;
	volatile uint32_t last_snapshot_ms;
	volatile bool has_snapshot;
} PvzUartRxState;

static PvzUartRxState g_pvz_uart_rx;

static void restore_irq_state(uint32_t primask) {
	if (primask == 0u) {
		__enable_irq();
	}
}

static bool start_receive(void) {
	if (g_pvz_uart_rx.uart_handle == NULL) {
		return false;
	}
	if (HAL_UARTEx_ReceiveToIdle_DMA(g_pvz_uart_rx.uart_handle, g_pvz_uart_rx.rx_buffer, sizeof(g_pvz_uart_rx.rx_buffer)) !=
		HAL_OK) {
		return false;
	}

	if (g_pvz_uart_rx.uart_handle->hdmarx != NULL) {
		__HAL_DMA_DISABLE_IT(g_pvz_uart_rx.uart_handle->hdmarx, DMA_IT_HT);
	}

	return true;
}

static void publish_snapshot(const PvzFrontendSnapshot *snapshot) {
	const uint8_t next_index = (uint8_t)(g_pvz_uart_rx.published_index ^ 1u);

	g_pvz_uart_rx.snapshots[next_index] = *snapshot;
	__DMB();
	g_pvz_uart_rx.last_snapshot_ms = HAL_GetTick();
	g_pvz_uart_rx.published_generation++;
	g_pvz_uart_rx.published_index = next_index;
	g_pvz_uart_rx.has_snapshot = true;
}

static void handle_rx_event(UART_HandleTypeDef *huart, uint16_t size) {
	PvzFrontendSnapshot snapshot;

	if (huart != g_pvz_uart_rx.uart_handle || g_pvz_uart_rx.config == NULL) {
		return;
	}

	if (size > 0u &&
		pvz_uart_protocol_decode_packet(g_pvz_uart_rx.rx_buffer, size, g_pvz_uart_rx.config, &snapshot)) {
		publish_snapshot(&snapshot);
	}

	(void)start_receive();
}

bool pvz_uart_rx_init(UART_HandleTypeDef *huart, const GameConfig *config) {
	memset(&g_pvz_uart_rx, 0, sizeof(g_pvz_uart_rx));
	g_pvz_uart_rx.uart_handle = huart;
	g_pvz_uart_rx.config = config;

	return start_receive();
}

bool pvz_uart_rx_read_latest(PvzFrontendSnapshot *out_snapshot, uint32_t *age_ms, uint32_t *snapshot_generation) {
	uint32_t primask;

	if (out_snapshot == NULL) {
		return false;
	}

	primask = __get_PRIMASK();
	__disable_irq();

	if (!g_pvz_uart_rx.has_snapshot) {
		restore_irq_state(primask);
		return false;
	}

	*out_snapshot = g_pvz_uart_rx.snapshots[g_pvz_uart_rx.published_index];
	if (age_ms != NULL) {
		*age_ms = HAL_GetTick() - g_pvz_uart_rx.last_snapshot_ms;
	}
	if (snapshot_generation != NULL) {
		*snapshot_generation = g_pvz_uart_rx.published_generation;
	}

	restore_irq_state(primask);
	return true;
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) { handle_rx_event(huart, Size); }

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
	if (huart != g_pvz_uart_rx.uart_handle) {
		return;
	}

	(void)start_receive();
}
