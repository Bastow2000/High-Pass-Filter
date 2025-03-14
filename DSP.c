#include "dsp.h"

// ------- Function Setup  -------
void setup(WavHeader *header, const int32_t *audioData) {
    // Setup wav format for a wav file
    setupWavFormat(header);

    // Write the header file
    writeWavFile(header, audioData);
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

void setupAudioStream(WavHeader *header, int32_t *audioData, AudioState *audioState, const uint16_t packetCount) {
    // Calculated in an attempt to be efficient
    const float sampleDuration = (float) NUM_SAMPLES / SAMPLE_RATE;

    for (u_int16_t sample = 0; sample < PACKET_SIZE; sample++) {
        const uint16_t samples = packetCount + sample;

        for (u_int16_t channel = 0; channel < NUM_CHANNELS; channel++) {
            // Choice for setting channel for audio
            const float vol = setVolume(channel == 0 ? VOLUME1 : VOLUME2, &audioState->gainState[channel]);
            const float freq = (channel == 0) ? FREQUENCY1 : FREQUENCY2;

            audioState->phaseIncrement[channel] = sampleDuration * freq;

            // Interleave index
            const u_int16_t interIndex = (samples * NUM_CHANNELS) + channel;

            // Linear interpolation
            const u_int16_t phaseInt = (u_int16_t) audioState->phase[channel];

            const float phaseFrac = audioState->phase[channel] - (float) phaseInt;

            const float value = linearInterpolation(phaseFrac, phaseInt, audioState->sineWave);

            const float filteredValue = processFilter(&audioState->filterState[channel], (float) value);

            // Scale and store the data
            audioData[interIndex] = (int32_t) ((vol * filteredValue) * INT32_MAX);

            // Increment phase
            audioState->phase[channel] += audioState->phaseIncrement[channel];
            if (audioState->phase[channel] >= NUM_SAMPLES)
                audioState->phase[channel] -= NUM_SAMPLES;
        }
    }
}

// ------- Function Controls  -------
float setVolume(const float gain, float *gainState) {
    // dBFS Conversion

    //5.9 for Volume 1 : 9.7 for volume 2
    // Linear Gain Formula: 10^(dB/20)
    const float linearGain = powf(10, gain / 20.0f);

    // is always 0 dBFS or lower
    // Needs normalisation
    //const double dbfGain = (double)(20 * log10 (linearGain / 1.0 ));

    // Logarithmic Interpolation formula: start *(end/start)^x
    const float interpGain = *gainState > 0
                                 ? linearGain * powf(*gainState / linearGain, VOLUME_INTERPOLATION)
                                 : linearGain;
    *gainState = linearGain;

    return interpGain;
}


void setFilter(FilterState *filterState, const float frequency) {
    // Normalise frequency
    const float normFreq = 2.0f * (float) M_PI * frequency / SAMPLE_RATE;

    // Pre Warp
    const float omegaC = 2 * SAMPLE_RATE * (tanf(normFreq / 2));

    // First z coefficient for the denominator
    const float z0 = powf(omegaC, 2);

    // Second z^{1} coefficient for the denominator
    const float z1 = 135744 * omegaC;

    // Third z^{2} coefficient for the denominator
    const float z2 = powf(2 * SAMPLE_RATE, 2); //  9.216e10^{9} note for self

    // the first z coefficient
    const float norm = z0 + z1 + z2;

    const float a0 = z2 / norm;

    filterState->a0 = a0;
    filterState->a1 = (z2 * -2) / norm;
    filterState->a2 = a0;

    filterState->b1 = (2.0f * z0 - 2.0f * z2) / norm;
    filterState->b2 = (z0 - z1 + z2) / norm;
}

// ------- Function Helpers  -------
void lookupSine(float *sineWave, const float mainVol) {
    // Automatic Storage Duration - no memory clearing needed
    for (u_int16_t sample = 0; sample < NUM_SAMPLES; sample++) {
        sineWave[sample] = mainVol * (sinf(2.0f * (float) M_PI * (float) sample / (float) NUM_SAMPLES));
    }
}

float linearInterpolation(const float phaseFrac, const u_int16_t phaseInt, const float *sineTable) {
    const u_int16_t nextIndex = (phaseInt + 1) & (NUM_SAMPLES - 1);
    const float currentSample = sineTable[phaseInt];
    const float nextSample = sineTable[nextIndex];
    const float sampleValue = currentSample + phaseFrac * (nextSample - currentSample);

    return sampleValue;
}

void filterReset(FilterState *filterState) {
    filterState->inputX2 = 0;
    filterState->inputX1 = 0;
    filterState->outputY2 = 0;
    filterState->outputY1 = 0;
}

float processFilter(FilterState *filterState, const float input) {
    const float output = filterState->a0 * input + filterState->a1 * filterState->inputX1 + filterState->a2 *
                         filterState->inputX2
                         - filterState->b1 * filterState->outputY1 - filterState->b2 * filterState->outputY2;


    filterState->inputX2 = filterState->inputX1;
    filterState->inputX1 = input;
    filterState->outputY2 = filterState->outputY1;
    filterState->outputY1 = output;
    return output;
}

void writeWavFile(const WavHeader *header, const int32_t *audioData) {
    FILE *file = fopen("output.wav", "wb");

    fwrite(header, sizeof(WavHeader), 1, file);

    fwrite(audioData, sizeof(int32_t), NUM_SAMPLES * NUM_CHANNELS, file);

    fclose(file);

    system("afplay output.wav");
}

void audioStateReset(AudioState *audioState) {
    // Reset audio state before use
    audioState->cutoffFreq = CUTOFF_FREQUENCY;
    for (u_int16_t channel = 0; channel < NUM_CHANNELS; channel++) {
        filterReset(&audioState->filterState[channel]);
        audioState->phase[channel] = 0.0f;
        audioState->gainState[channel] = 0.0f;
    }
    // Set state for each filter channel
    setFilter(&audioState->filterState[0], audioState->cutoffFreq);
    setFilter(&audioState->filterState[1], audioState->cutoffFreq);
}
