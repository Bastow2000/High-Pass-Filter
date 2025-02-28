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
#define VOLUME_INTERPOLATION 0.5
#define MAIN_VOLUME 10
#define CUTOFF_FREQUENCY 1000

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

    // Actual data
    int32_t *data;
    //uint8_t* data;
} WavHeader;

//------- FilterState Struct -------
typedef struct {
    // Filter States
    double inputX1;
    double inputX2;
    double inputX3;
    double outputY1;
    double outputY2;

    // Filter Coefficients
    double b1;
    double b2;
    double a0;
    double a1;
    double a2;
} FilterState;

// ------- Function Setup  -------
// Combines all the functions below here
void setup(WavHeader *header, float mainVol, float cutoffFreq);

// Sets up wav file
void setupWavFormat(WavHeader *header);

// Sets up audio data - (Interleaving, Filtering, Audio Packet, Volume)
void setupAudioStream(WavHeader *header, FilterState *state, float mainVol, float cutoffFreq);

// ------- Control Functions  -------
// Sets the coefficients based on frequency
void setFilter(FilterState *state, float frequency);

// Sets gain (Converts db to linear)
double setVolume(double gain);

// ------- Helper Functions  -------
// Lookup table for efficiency
void lookupSine(double *sineWave, float mainVol);

// Linear Interpolation for lookup table
double linearInterpolation(double phaseFrac, u_int16_t phaseInt,const double *sineTable);

// Resets state after use
void filterReset(FilterState *state);

// Deals with the filter states
double processFilter(FilterState *state, float input);

// Writes audio to Wav file
void writeWavFile(WavHeader *header);

#endif
