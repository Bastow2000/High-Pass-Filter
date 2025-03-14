#include <stdio.h>
#include <stdlib.h>
#include "dsp.h"


int main(void) {
    // initialises space for the size of the header
    WavHeader *header = malloc(sizeof(WavHeader));
    AudioState *audioState = malloc(sizeof(AudioState));
    int32_t *audioData = malloc(NUM_SAMPLES * NUM_CHANNELS * sizeof(int32_t));

    // Resets filters, channels, phase and other
    audioStateReset(audioState);

    // Gets the lookup table for a sine wave
    lookupSine(audioState->sineWave, MAIN_VOLUME);

    // Process the data in packets
    for (u_int16_t packet = 0; packet < NUM_SAMPLES; packet += PACKET_SIZE) {
        // Apply audio to memory, filter and set amplitude
        setupAudioStream(header, audioData, audioState, packet);
    }
    setup(header, audioData);

    free(header);
    free(audioData);
    free(audioState);
    return 0;
}
