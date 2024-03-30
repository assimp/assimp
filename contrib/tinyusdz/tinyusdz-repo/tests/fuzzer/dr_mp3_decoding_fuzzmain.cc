#include <cstdint>

#define DR_MP3_IMPLEMENTATION
#include "external/dr_mp3.h"

#if 0
static void data_callback(ma_device* pDevice, void* pFramesOut, const void* pFramesIn, ma_uint32 frameCount)
{
    drmp3* pMP3;

    pMP3 = (drmp3*)pDevice->pUserData;
    DRMP3_ASSERT(pMP3 != NULL);

    if (pDevice->playback.format == ma_format_f32) {
        drmp3_read_pcm_frames_f32(pMP3, frameCount, pFramesOut);
    } else if (pDevice->playback.format == ma_format_s16) {
        drmp3_read_pcm_frames_s16(pMP3, frameCount, pFramesOut);
    } else {
        DRMP3_ASSERT(DRMP3_FALSE);  /* Should never get here. */
    }

    (void)pFramesIn;
}
#endif

static int parse_wav(const uint8_t* data, size_t size) {
  if (size > 1024 * 1024 * 128 * 4) {
    return -1;
  }

  drmp3 mp3Mem;

  if (!drmp3_init_memory(&mp3Mem, data, size, nullptr)) {
    return -1;  // do not add to copus
  }

  uint32_t max_frames = 1024 * 1024 * 128;

  for (size_t i = 0; i < max_frames; i++) {
    // drmp3_uint64 iSample;
    drmp3_uint64 pcmFrameCountMemory;
    drmp3_int16 pcmFramesMemory[4096];

    pcmFrameCountMemory = drmp3_read_pcm_frames_s16(
        &mp3Mem, DRMP3_COUNTOF(pcmFramesMemory) / mp3Mem.channels,
        pcmFramesMemory);

#if 0
        /* Check individual frames. */
        for (iSample = 0; iSample < pcmFrameCountMemory * mp3Memory.channels; iSample += 1) {
        }
#endif

    /* We've reached the end if we didn't return any PCM frames. */
    if (pcmFrameCountMemory == 0) {
      break;
    }
  }

  drmp3_uninit(&mp3Mem);

  return 0;
}

extern "C" int LLVMFuzzerTestOneInput(std::uint8_t const* data,
                                      std::size_t size) {
  return parse_wav(data, size);
}
