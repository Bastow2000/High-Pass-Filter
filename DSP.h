#ifndef DSP_H
#define DSP_H


#include <math.h>
#include <stdint.h>
#include <stddef.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Constexpr | Const: Expressions

// ------- Wave Header Constexpr Variables -------
// 4+(8+SUB_CHUNK_SIZE_1) + (8 + SUB_CHUNK_SIZE_2)
#define CHUNK_SIZE 8228
#define SUB_CHUNK_SIZE_1 16
#define AUDIO_FORMAT 1
#define NUM_CHANNELS 2
#define SAMPLE_RATE 48000

// Sample Rate * BlockAlign
#define BYTE_RATE 384000

// NumChannels * (BitsPerSample /8)
#define BLOCK_ALIGN 8
#define BITS_PER_SAMPLE 32

// NUM_SAMPLES * blockAlign
#define SUB_CHUNK_SIZE_2 8192

// ------- Misc Variables  -------
// Character size
#define CHAR_SIZE 4
#define PACKET_SIZE 64
#define VOLUME_INTERPOLATION 0.5f
#define MAIN_VOLUME 1
#define CUTOFF_FREQUENCY 100

// ------- AudioStream Setup Variables -------
#define NUM_SAMPLES 1024
#define FREQUENCY1 100
#define FREQUENCY2 1000
#define VOLUME1 -6
#define VOLUME2 -10

//------- Filter Coefficients -------
// Originally calculated a fixed filter at 14000Hz cutoff
#define A0 0.88f
#define A1 -1.75f
#define A2 0.88f
#define B0 1.0f
#define B1 -1.74f
#define B2 0.78f

//------- Wav File Struct -------
typedef struct {
    // ------- Chunk Descriptor -------
    // Riff: is the file format used for .wav
    char chunkID[CHAR_SIZE];

    // ChunkSize: size of entire file
    uint32_t chunkSize;

    // File Type header name
    char format[CHAR_SIZE];

    // ------- Format Section  -------
    // Format chunk marker
    char subChunk1ID[CHAR_SIZE];

    // Length of format data
    uint32_t subChunk1Size;

    // Type of format
    uint16_t audioFormat;

    // Number of Channels
    uint16_t numChannels;

    // SampleRate
    uint32_t sampleRate;

    // Number of bytes processed per second of audio
    uint32_t byteRate;

    // Block Align indicates the size in bytes of a unit of sample data
    uint16_t blockAlign;

    // Number of bits used to represent each audio sample
    uint16_t bitsPerSample;

    // ------- Data Section  -------
    // Data chunk marker
    char subChunk2ID[CHAR_SIZE];

    // Size of audio data
    uint32_t subChunk2Size;
} WavHeader;

//------- FilterState Struct -------
typedef struct {
    // Filter States
    float inputX1;
    float inputX2;
    float inputX3;
    float outputY1;
    float outputY2;

    // Filter Coefficients
    float b1;
    float b2;
    float a0;
    float a1;
    float a2;
} FilterState;

//------- AudioState Struct -------
typedef struct {
    FilterState filterState[NUM_CHANNELS];
    float phase[NUM_CHANNELS];
    float gainState[NUM_CHANNELS];
    float cutoffFreq;
    float sineWave[NUM_SAMPLES];
    float phaseIncrement[NUM_CHANNELS];
} AudioState;

// ------- Function Setup  -------
// Combines all the functions below here
void setup(WavHeader *header, const int32_t *audioData);

// Sets up wav file
void setupWavFormat(WavHeader *header);

// Sets up audio data - (Interleaving, Filtering, Audio Packet, Volume)
void setupAudioStream(WavHeader *header, int32_t *audioData, AudioState *audioState, uint16_t packetCount);

// ------- Control Functions  -------
// Sets the coefficients based on frequency
void setFilter(FilterState *filterState, float frequency);

// Sets gain (Converts db to linear)
float setVolume(float gain, float *gainState);

// ------- Helper Functions  -------
// Lookup table for efficiency
void lookupSine(float *sineWave, float mainVol);

// Linear Interpolation for lookup table
float linearInterpolation(float phaseFrac, u_int16_t phaseInt, const float *sineTable);

// Resets state after use
void filterReset(FilterState *filterState);

// Deals with the filter states
float processFilter(FilterState *filterState, float input);

// Writes audio to Wav file
void writeWavFile(const WavHeader *header, const int32_t *audioData);

// Reset audio state and setup Filter Coeff
void audioStateReset(AudioState *audioState);

#endif
