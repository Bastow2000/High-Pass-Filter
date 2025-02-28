#include "dsp.h"


// ------- Function Setup  -------
void setup(WavHeader *header, const float mainVol, const float cutoffFreq) {
    // Filter both channels
    FilterState state[NUM_CHANNELS] = {0};

    // Setup wav format for a wav file
    setupWavFormat(header);

    // Apply audio to memory, filter and set amplitude
    setupAudioStream(header, state, mainVol, cutoffFreq);

    // Write to the wave file
    writeWavFile(header);
}

void setupWavFormat(WavHeader *header) {
    // Initialise Wav Header Struct
    memcpy(header->chunkID, "RIFF", CHAR_SIZE);
    memcpy(header->format, "WAVE", CHAR_SIZE);
    memcpy(header->subChunk1ID, "fmt ", CHAR_SIZE);
    memcpy(header->subChunk2ID, "data", CHAR_SIZE);

    // Variables are pre-calculated - Thought process:
    // Attempt at being more efficient
    header->subChunk1Size = SUB_CHUNK_SIZE_1;
    header->audioFormat = AUDIO_FORMAT;
    header->numChannels = NUM_CHANNELS;
    header->sampleRate = SAMPLE_RATE;
    header->bitsPerSample = BITS_PER_SAMPLE;
    header->blockAlign = BLOCK_ALIGN;
    header->byteRate = BYTE_RATE;
    header->subChunk2Size = SUB_CHUNK_SIZE_2;
    header->chunkSize = CHUNK_SIZE;
}

void setupAudioStream(WavHeader *header, FilterState *state, const float mainVol, const float cutoffFreq) {

    // Reset filter state
    filterReset(&state[0]);
    filterReset(&state[1]);

    header->data = (int32_t *) malloc(NUM_SAMPLES * NUM_CHANNELS * sizeof(int32_t));

    // Calculated in an attempt to be efficient
    const double sampleDuration = (double) NUM_SAMPLES / SAMPLE_RATE;

    double sineWave[NUM_SAMPLES];
    double phaseIncrement[NUM_CHANNELS];
    double phase[NUM_CHANNELS] = {0.0, 0.0};

    // Gets the lookup table for a sine wave
    lookupSine(sineWave, mainVol);

        for (u_int16_t sample = 0; sample < NUM_SAMPLES; sample++) {

            for (u_int16_t channel = 0; channel < NUM_CHANNELS; channel++) {
                // Set state for each filter channel
                setFilter(&state[channel], cutoffFreq);

                // Choice for setting channel for audio
                const double vol = setVolume(channel == 0 ? VOLUME1 : VOLUME2);
                const double freq = (channel == 0) ? FREQUENCY1 : FREQUENCY2;

                phaseIncrement[channel] = sampleDuration * freq;

                // Interleave index
                const u_int16_t interIndex = (sample * NUM_CHANNELS) + channel;

                // Linear interpolation
                const u_int16_t phaseInt = (u_int16_t) phase[channel];

                const double phaseFrac = phase[channel] - phaseInt;

                const double value = linearInterpolation(phaseFrac, phaseInt, sineWave );

                const double filteredValue = processFilter(&state[channel], (float) value);


                // Scale and store the data
                header->data[interIndex] = (int32_t) ((vol * filteredValue) * INT32_MAX);

                // Increment phase
                phase[channel] += phaseIncrement[channel];
                if (phase[channel] >= NUM_SAMPLES)
                    phase[channel] -= NUM_SAMPLES;

        }
    }
}

// ------- Function Controls  -------
double setVolume(const double gain) {
    // dBFS Conversion
    static double gainState = 0;

    //5.9 for Volume 1 : 9.7 for volume 2
    // Linear Gain Formula: 10^(dB/20)
    const double linearGain = (double) pow(10, gain / 20.0f);

    // is always 0 dBFS or lower
    // Needs normalisation
    //const double dbfGain = (double)(20 * log10 (linearGain / 1.0 ));

    // Logarithmic Interpolation formula: start *(end/start)^x
    const double interpGain = gainState > 0
                                  ? linearGain * pow(gainState / linearGain, VOLUME_INTERPOLATION)
                                  : linearGain;
    gainState = gain;

    return interpGain;
}


void setFilter(FilterState *state, const float frequency) {
    // Normalise frequency
    const double normFreq = 2 * M_PI * frequency / SAMPLE_RATE;

    // Pre Warp
    const double omegaC = 2 * SAMPLE_RATE * (tan(normFreq / 2));

    // First z coefficient for the denominator
    const double z0 = pow(omegaC, 2);

    // Second z^{1} coefficient for the denominator
    const double z1 = 135744 * omegaC;

    // Third z^{2} coefficient for the denominator
    const double z2 = pow(2 * SAMPLE_RATE, 2); //  9.216e10^{9} note for self

    // the first z coefficient
    const double norm = z0 + z1 + z2;

    const double a0 = z2 / norm;

    state->a0 = a0;
    state->a1 = (z2 * -2) / norm;
    state->a2 = a0;

    state->b1 = (2.0 * z0 - 2.0 * z2) / norm;
    state->b2 = (z0 - z1 + z2) / norm;
}

// ------- Function Helpers  -------
void lookupSine(double *sineWave, const float mainVol) {
    // Automatic Storage Duration - no memory clearing needed
    for (u_int16_t sample = 0; sample < NUM_SAMPLES; sample++) {
        sineWave[sample] = mainVol * (sin(2.0f * M_PI * (float) sample / (float) NUM_SAMPLES));
    }
}

double linearInterpolation(const double phaseFrac, const u_int16_t phaseInt,const double *sineTable) {
    const u_int16_t nextIndex = (phaseInt + 1) & (NUM_SAMPLES - 1);
    const double currentSample = sineTable[phaseInt];
    const double nextSample = sineTable[nextIndex];
    const double sampleValue = currentSample + phaseFrac * (nextSample - currentSample);

    return sampleValue;
}

void filterReset(FilterState *state) {
    state->inputX2 = 0;
    state->inputX1 = 0;
    state->outputY2 = 0;
    state->outputY1 = 0;
}

double processFilter(FilterState *state, const float input){
    const double output = state->a0 * input + state->a1 * state->inputX1 + state->a2 * state->inputX2
                          - state->b1 * state->outputY1 - state->b2 * state->outputY2;


    state->inputX2 = state->inputX1;
    state->inputX1 = input;
    state->outputY2 = state->outputY1;
    state->outputY1 = output;
    return output;
}

void writeWavFile(WavHeader *header) {

    FILE *file = fopen("output.wav", "wb");


    fwrite(header, sizeof(WavHeader), 1, file);

    fwrite(header->data, sizeof(int32_t), NUM_SAMPLES * NUM_CHANNELS, file);

    fclose(file);

    system("afplay output.wav");

    // Free memory
    free(header->data);
}
