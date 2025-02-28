#include <stdio.h>
#include <stdlib.h>
#include "dsp.h"


int main(void) {
    WavHeader* header = malloc(sizeof(WavHeader));
    for (u_int16_t packet = 0; packet < NUM_SAMPLES; packet += PACKET_SIZE) {
        setup(header, MAIN_VOLUME, CUTOFF_FREQUENCY);
    }
    free(header);
    return 0;
}