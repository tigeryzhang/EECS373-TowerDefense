#include "speaker.h"
#include "fatfs.h"
#include "main.h"
#include "stm32l4xx_hal.h"
#include <string.h>

extern DAC_HandleTypeDef hdac1;
extern TIM_HandleTypeDef htim6;

#define AUDIO_REFILL_CHUNK_BYTES       512u
#define AUDIO_START_PRIME_BYTES        AUDIO_BUFFER_SIZE
#define AUDIO_MAX_REFILL_CHUNKS_PER_TASK 4u
#define AUDIO_DMA_FILL_FIRST_HALF      0x01u
#define AUDIO_DMA_FILL_SECOND_HALF     0x02u

/* ================= DAC BUFFER ================= */
static uint16_t dac_buffer[AUDIO_BUFFER_SIZE];
static volatile uint8_t audio_dma_fill_pending = 0u;

/* ================= TRACK ================= */
typedef struct {
    FIL      file;
    uint8_t  buffer[FILE_BUFFER_SIZE];
    volatile uint32_t read_pos;
    volatile uint32_t write_pos;
    uint32_t data_start;
    uint8_t  looping;
    uint8_t  active;
} AudioTrack;

static AudioTrack bgm_track;
static AudioTrack sfx_track;
static uint8_t playing_sfx = 0;

static void AUDIO_ResetTrack(AudioTrack *track)
{
    track->active = 0;
    track->looping = 0;
    track->read_pos = 0;
    track->write_pos = 0;
}

static void AUDIO_CloseTrack(AudioTrack *track)
{
    if (track->active) {
        f_close(&track->file);
    }
    AUDIO_ResetTrack(track);
}

/* ===================================================== */
/* ================= WAV PARSER ========================= */
/* ===================================================== */

static uint32_t WAV_FindDataOffset(FIL *file)
{
    uint8_t buf[8];
    UINT br;
    FRESULT res;

    // Start after RIFF/WAVE header
    res = f_lseek(file, 12);
    if (res != FR_OK) return 0;

    while (1) {
        // Read 8-byte chunk header (ID + Size)
        if (f_read(file, buf, 8, &br) != FR_OK || br < 8) {
            return 0; // EOF or Read Error
        }

        // Little-endian reconstruction
        uint32_t size = (uint32_t)buf[4] | ((uint32_t)buf[5] << 8) |
                        ((uint32_t)buf[6] << 16) | ((uint32_t)buf[7] << 24);

        // Check for "data" (0x61746164)
        if (buf[0]=='d' && buf[1]=='a' && buf[2]=='t' && buf[3]=='a') {
            return (uint32_t)f_tell(file);
        }

        // Calculate next chunk position: Current + Size + Padding
        // RIFF requires 2-byte alignment. If size is odd, add 1.
        FSIZE_t next_chunk = f_tell(file) + size + (size & 1);

        // Move to the next chunk.
        // If f_lseek fails (e.g., offset beyond EOF), the file is malformed.
        if (f_lseek(file, next_chunk) != FR_OK) {
            return 0;
        }
    }
}

/* ===================================================== */
/* ================= RING BUFFER HELPERS ================ */
/* ===================================================== */

static uint32_t RingBuffer_Available(AudioTrack *track)
{
    return (track->write_pos - track->read_pos + FILE_BUFFER_SIZE) % FILE_BUFFER_SIZE;
}

static uint32_t RingBuffer_FreeSpace(AudioTrack *track)
{
    // -1 to distinguish full from empty
    return (track->read_pos - track->write_pos - 1 + FILE_BUFFER_SIZE) % FILE_BUFFER_SIZE;
}

/* ===================================================== */
/* ================= TRACK CONTROL ====================== */
/* ===================================================== */

static void AUDIO_StartTrack(AudioTrack *track, const char *filename, uint8_t loop)
{
    AUDIO_CloseTrack(track);

    if (f_open(&track->file, filename, FA_READ) != FR_OK)
        return;

    track->data_start = WAV_FindDataOffset(&track->file);
    f_lseek(&track->file, track->data_start);
    track->looping = loop;
    track->active = 1;
}

/* ===================================================== */
/* ================= BUFFER REFILL ====================== */
/* ===================================================== */

static uint8_t AUDIO_RefillTrackChunk(AudioTrack *track)
{
    if (!track->active || RingBuffer_FreeSpace(track) < AUDIO_REFILL_CHUNK_BYTES) {
        return 0;
    }

    while (1) {
        UINT br;
        // How much space before wrap-around?
        uint32_t to_end = FILE_BUFFER_SIZE - track->write_pos;
        uint32_t chunk  = (to_end < AUDIO_REFILL_CHUNK_BYTES) ? to_end : AUDIO_REFILL_CHUNK_BYTES;

        f_read(&track->file, &track->buffer[track->write_pos], chunk, &br);

        if (br == 0) {
            if (track->looping) {
                f_lseek(&track->file, track->data_start);
                continue;
            } else {
                // EOF, non-looping: mark inactive when buffer drains
                // (don't close yet — samples still in buffer)
                track->looping = 0; // sentinel: file exhausted
                f_close(&track->file);
                // Zero-pad the rest of this chunk
                memset(&track->buffer[track->write_pos], 0, chunk);
                track->write_pos = (track->write_pos + chunk) % FILE_BUFFER_SIZE;
                return 1;
            }
        }

        track->write_pos = (track->write_pos + br) % FILE_BUFFER_SIZE;

        return 1;
    }
}

static void AUDIO_PrimeTrack(AudioTrack *track, uint32_t min_available_bytes)
{
    while (track->active && RingBuffer_Available(track) < min_available_bytes) {
        if (!AUDIO_RefillTrackChunk(track)) {
            break;
        }
    }
}

static void AUDIO_RefillTrack(AudioTrack *track, uint32_t max_chunks)
{
    for (uint32_t chunk_index = 0; chunk_index < max_chunks; ++chunk_index) {
        if (!AUDIO_RefillTrackChunk(track)) {
            break;
        }
    }
}

/* ===================================================== */
/* ================= SAMPLE READ ======================== */
/* ===================================================== */

static int16_t AUDIO_ReadSample(AudioTrack *track)
{
    if (!track->active) return 0;

    // Need 1 byte now
    if (RingBuffer_Available(track) < 1) {
        if (!track->looping) track->active = 0;
        return 0;
    }

    uint8_t byte = track->buffer[track->read_pos];

    track->read_pos = (track->read_pos + 1) % FILE_BUFFER_SIZE;

    // Convert unsigned 8-bit → signed 16-bit
    return ((int16_t)byte - 128) << 8;
}

/* ===================================================== */
/* ================= MIXER ============================== */
/* ===================================================== */

static void AUDIO_FillBuffer(uint16_t *buf, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++)
    {
        int32_t bgm = AUDIO_ReadSample(&bgm_track);
        int32_t sfx = 0;

        if (playing_sfx) {
            if (sfx_track.active) {
                sfx = AUDIO_ReadSample(&sfx_track);
            } else {
                // SFX finished — clear flag AFTER last sample consumed
                playing_sfx = 0;
            }
        }


        // OR a slightly louder "Headroom" mix (e.g., 80% each)
        int32_t mixed = (bgm >> 1) + (sfx >> 1);

        // Convert signed 16-bit → unsigned 12-bit for DAC
        buf[i] = (uint16_t)((mixed + 32768) >> 4);
    }
}

static void AUDIO_ServicePendingDmaFills(void)
{
    uint8_t pending;
    const uint32_t primask = __get_PRIMASK();

    __disable_irq();
    pending = audio_dma_fill_pending;
    audio_dma_fill_pending = 0u;
    if (primask == 0u) {
        __enable_irq();
    }

    if ((pending & AUDIO_DMA_FILL_FIRST_HALF) != 0u) {
        AUDIO_FillBuffer(&dac_buffer[0], AUDIO_BUFFER_SIZE / 2);
    }
    if ((pending & AUDIO_DMA_FILL_SECOND_HALF) != 0u) {
        AUDIO_FillBuffer(&dac_buffer[AUDIO_BUFFER_SIZE / 2], AUDIO_BUFFER_SIZE / 2);
    }
}

/* ===================================================== */
/* ================= PUBLIC API ========================= */
/* ===================================================== */

void AUDIO_PlayMusic_File(const char *filename)
{
    AUDIO_StartTrack(&bgm_track, filename, 1);
    AUDIO_PrimeTrack(&bgm_track, AUDIO_START_PRIME_BYTES);
}

void AUDIO_PlaySFX_File(const char *filename)
{
    AUDIO_StartTrack(&sfx_track, filename, 0);
    AUDIO_PrimeTrack(&sfx_track, AUDIO_START_PRIME_BYTES);
    playing_sfx = 1;
}

void AUDIO_Play(void)
{
    audio_dma_fill_pending = 0u;

    // Fill first half and second half explicitly so DMA
    // double-buffer starts in a known state
    AUDIO_FillBuffer(&dac_buffer[0],                   AUDIO_BUFFER_SIZE / 2);
    AUDIO_FillBuffer(&dac_buffer[AUDIO_BUFFER_SIZE / 2], AUDIO_BUFFER_SIZE / 2);

    HAL_DAC_Start_DMA(
        &hdac1,
        DAC_CHANNEL_1,
        (uint32_t*)dac_buffer,
        AUDIO_BUFFER_SIZE,
        DAC_ALIGN_12B_R
    );

    HAL_TIM_Base_Start(&htim6);
}

void AUDIO_Stop(void)
{
    HAL_DAC_Stop_DMA(&hdac1, DAC_CHANNEL_1);
    HAL_TIM_Base_Stop(&htim6);

    AUDIO_StopAllTracks();
    audio_dma_fill_pending = 0u;
}

void AUDIO_StopAllTracks(void)
{
    AUDIO_CloseTrack(&bgm_track);
    AUDIO_CloseTrack(&sfx_track);
    playing_sfx = 0;
}

// Call this from your main loop — handles SD card reads outside of IRQ
void AUDIO_Task(void)
{
    AUDIO_RefillTrack(&bgm_track, AUDIO_MAX_REFILL_CHUNKS_PER_TASK);
    if (playing_sfx) {
        AUDIO_RefillTrack(&sfx_track, AUDIO_MAX_REFILL_CHUNKS_PER_TASK);
    }
    AUDIO_ServicePendingDmaFills();
}

void AUDIO_PlayOnce_File(const char *filename)
{
    AUDIO_CloseTrack(&bgm_track);
    AUDIO_PlaySFX_File(filename);
}

void AUDIO_PlayOnce(const char *filename)
{
    AUDIO_PlayOnce_File(filename);
}

uint8_t AUDIO_IsBGMFinished(void)
{
    return (uint8_t)(bgm_track.active == 0u);
}

/* ===================================================== */
/* ================= DMA CALLBACKS ====================== */
/* ===================================================== */

// Runs in IRQ — fill the half that DMA just finished playing
void HAL_DAC_ConvHalfCpltCallbackCh1(DAC_HandleTypeDef *hdac)
{
    (void)hdac;
    audio_dma_fill_pending |= AUDIO_DMA_FILL_FIRST_HALF;
}

void HAL_DAC_ConvCpltCallbackCh1(DAC_HandleTypeDef *hdac)
{
    (void)hdac;
    audio_dma_fill_pending |= AUDIO_DMA_FILL_SECOND_HALF;
}
