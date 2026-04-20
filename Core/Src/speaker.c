#include "speaker.h"
#include <string.h>

/* ================= DAC BUFFER ================= */
static uint16_t dac_buffer[AUDIO_BUFFER_SIZE];

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
    // Close previous file if open
    if (track->active)
        f_close(&track->file);

    track->active = 0;
    track->read_pos = 0;
    track->write_pos = 0;

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

static void AUDIO_RefillTrack(AudioTrack *track)
{
    if (!track->active) return;

    // Refill in 512-byte chunks as long as there's space
    while (RingBuffer_FreeSpace(track) >= 512)
    {
        UINT br;
        // How much space before wrap-around?
        uint32_t to_end = FILE_BUFFER_SIZE - track->write_pos;
        uint32_t chunk  = (to_end < 512) ? to_end : 512;

        f_read(&track->file, &track->buffer[track->write_pos], chunk, &br);

        if (br == 0) {
            if (track->looping) {
                f_lseek(&track->file, track->data_start);
                // Don't return — loop around and keep filling
                continue;
            } else {
                // EOF, non-looping: mark inactive when buffer drains
                // (don't close yet — samples still in buffer)
                track->looping = 0; // sentinel: file exhausted
                f_close(&track->file);
                // Zero-pad the rest of this chunk
                memset(&track->buffer[track->write_pos], 0, chunk);
                track->write_pos = (track->write_pos + chunk) % FILE_BUFFER_SIZE;
                break;
            }
        }

        track->write_pos = (track->write_pos + br) % FILE_BUFFER_SIZE;

        // Partial read (wrap boundary) — do second half next call
        if (br < chunk) break;
    }
}

/* ===================================================== */
/* ================= SAMPLE READ ======================== */
/* ===================================================== */

static int16_t AUDIO_ReadSample(AudioTrack *track)
{
    if (!track->active) return 0;

    // Need 2 bytes available
    if (RingBuffer_Available(track) < 2) {
        // Buffer underrun — return silence
        // If file is exhausted (not looping, file closed), deactivate
        if (!track->looping) track->active = 0;
        return 0;
    }

    int16_t sample =
        (int16_t)(track->buffer[track->read_pos] |
                 (track->buffer[(track->read_pos + 1) % FILE_BUFFER_SIZE] << 8));

    track->read_pos = (track->read_pos + 2) % FILE_BUFFER_SIZE;
    return sample;
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
        // int32_t mixed = (bgm * 1 / 10) + (sfx * 4 / 5);
        int32_t mixed = (bgm >> 2) + (sfx >> 1);

        // Convert signed 16-bit → unsigned 12-bit for DAC
        buf[i] = (uint16_t)((mixed + 32768) >> 4);
    }
}

/* ===================================================== */
/* ================= PUBLIC API ========================= */
/* ===================================================== */

void AUDIO_PlayMusic_File(const char *filename)
{
    AUDIO_StartTrack(&bgm_track, filename, 1);
    // Pre-fill the entire ring buffer before playback
    AUDIO_RefillTrack(&bgm_track);
}

void AUDIO_PlaySFX_File(const char *filename)
{
    AUDIO_StartTrack(&sfx_track, filename, 0);
    AUDIO_RefillTrack(&sfx_track);
    playing_sfx = 1;
}

void AUDIO_Play(void)
{
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

    if (bgm_track.active) { f_close(&bgm_track.file); bgm_track.active = 0; }
    if (sfx_track.active) { f_close(&sfx_track.file); sfx_track.active = 0; }
    playing_sfx = 0;
}

// Call this from your main loop — handles SD card reads outside of IRQ
void AUDIO_Task(void)
{
    AUDIO_RefillTrack(&bgm_track);
    if (playing_sfx)
        AUDIO_RefillTrack(&sfx_track);
}

void AUDIO_PlayOnce(const char *filename)
{
    // 1. Stop the Background Music track specifically
    if (bgm_track.active) {
        f_close(&bgm_track.file);
        bgm_track.active = 0;
        bgm_track.read_pos = 0;
        bgm_track.write_pos = 0;
    }

    // 2. Start the new file as an SFX (one-shot)
    // Your existing AUDIO_PlaySFX_File already sets loop = 0
    AUDIO_PlaySFX_File(filename);
}

/* ===================================================== */
/* ================= DMA CALLBACKS ====================== */
/* ===================================================== */

// Runs in IRQ — fill the half that DMA just finished playing
void HAL_DAC_ConvHalfCpltCallbackCh1(DAC_HandleTypeDef *hdac)
{
    AUDIO_FillBuffer(&dac_buffer[0], AUDIO_BUFFER_SIZE / 2);
}

void HAL_DAC_ConvCpltCallbackCh1(DAC_HandleTypeDef *hdac)
{
    AUDIO_FillBuffer(&dac_buffer[AUDIO_BUFFER_SIZE / 2], AUDIO_BUFFER_SIZE / 2);
}
