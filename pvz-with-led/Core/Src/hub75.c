#include "hub75.h"
#include "main.h"

uint8_t fb[PANEL_HEIGHT][PANEL_WIDTH][3];

#define DATA_PORT     GPIOB
#define R1_PIN        GPIO_PIN_0
#define G1_PIN        GPIO_PIN_1
#define B1_PIN        GPIO_PIN_2
#define R2_PIN        GPIO_PIN_3
#define G2_PIN        GPIO_PIN_4
#define B2_PIN        GPIO_PIN_5
#define DATA_MASK     (0x003F)

#define CLK_PORT      GPIOB
#define CLK_PIN       GPIO_PIN_13
#define LAT_PORT      GPIOC
#define LAT_PIN       GPIO_PIN_6
#define OE_PORT       GPIOC
#define OE_PIN        GPIO_PIN_7

#define ADDR_PORT     GPIOG
#define A_PIN         GPIO_PIN_0
#define B_PIN         GPIO_PIN_1
#define C_PIN         GPIO_PIN_2
#define D_PIN         GPIO_PIN_3
#define ADDR_MASK     (0x000F)

static inline void clk_pulse(void){
    CLK_PORT->BSRR = CLK_PIN;
    CLK_PORT->BRR  = CLK_PIN;
}

static inline void latch_pulse(void){
    LAT_PORT->BSRR = LAT_PIN;
    LAT_PORT->BRR  = LAT_PIN;
}

static inline void oe_on(void){
	OE_PORT->BRR  = OE_PIN;
}

static inline void oe_off(void){
	OE_PORT->BSRR = OE_PIN;
}

static inline void set_row(uint8_t row){
    ADDR_PORT->ODR = (ADDR_PORT->ODR & ~ADDR_MASK) | (row & ADDR_MASK);
}


void HUB75_Init(void){
    //Blank the display immediately
    oe_off();
    HUB75_Clear();
}


void HUB75_ISR_Tick(void){
    static uint8_t current_row = 0;
    static uint8_t current_bit = 0;
    static uint16_t dwell_count = 0;

    uint16_t dwell_target = (1u << current_bit);

    if (dwell_count < dwell_target - 1){
        dwell_count++;
        return;   // keep the row lit
    }
    dwell_count = 0;

    //Blank the panel while we switch rows
    oe_off();

    //Advance bit plane and row
    current_bit++;
    if (current_bit >= BCM_BITS){
        current_bit = 0;
        current_row = (current_row + 1) % NUM_ROWS;
    }

    uint8_t bot_row = current_row + NUM_ROWS;

    for (uint8_t col = 0; col < PANEL_WIDTH; col++){
        uint32_t r1 = (fb[current_row][col][0] >> current_bit) & 1;
        uint32_t g1 = (fb[current_row][col][1] >> current_bit) & 1;
        uint32_t b1 = (fb[current_row][col][2] >> current_bit) & 1;
        uint32_t r2 = (fb[bot_row][col][0] >> current_bit) & 1;
        uint32_t g2 = (fb[bot_row][col][1] >> current_bit) & 1;
        uint32_t b2 = (fb[bot_row][col][2] >> current_bit) & 1;

        // Write all 6 data bits simultaneously to GPIOB[5:0]
        uint32_t data = r1 | (g1 << 1) | (b1 << 2) | (r2 << 3) | (g2 << 4) | (b2 << 5);
        DATA_PORT->ODR = (DATA_PORT->ODR & ~DATA_MASK) | data;

        clk_pulse();
    }

    set_row(current_row);
    latch_pulse();
    oe_on();
}

void HUB75_FillScreen(uint8_t r, uint8_t g, uint8_t b){
    for (int y = 0; y < PANEL_HEIGHT; y++){
        for (int x = 0; x < PANEL_WIDTH; x++){
            fb[y][x][0] = r & 0x0F;
            fb[y][x][1] = g & 0x0F;
            fb[y][x][2] = b & 0x0F;
        }
    }
}

// Set a single pixel (x = col 0-63, y = row 0-31)
void HUB75_SetPixel(uint8_t x, uint8_t y, uint8_t r, uint8_t g, uint8_t b)
{
    if (x >= PANEL_WIDTH || y >= PANEL_HEIGHT) return;
    fb[y][x][0] = r & 0x0F;
    fb[y][x][1] = g & 0x0F;
    fb[y][x][2] = b & 0x0F;
}

// Clear display (all off)
void HUB75_Clear(void)
{
    HUB75_FillScreen(0, 0, 0);
}
