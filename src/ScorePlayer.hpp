#ifndef SCORE_PLAYER_H
#define SCORE_PLAYER_H

#include "../external/dsp/_dsp.h"

#include <cmath>
#include <list>
#include <atomic>

#include <iostream>

#define MINIAUDIO_IMPLEMENTATION
#define MA_ENABLE_ONLY_SPECIFIC_BACKENDS
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    #define MA_ENABLE_WINMM
#elif __APPLE__
    #define MA_ENABLE_COREAUDIO
#elif __linux__
   #define MA_ENABLE_ALSA
#else
#   error "Unknown compiler"
#endif
#define MA_NO_DECODING
#define MA_NO_ENCODING
#define MA_NO_WAV
#define MA_NO_FLAC
#define MA_NO_MP3
#define MA_NO_RESOURCE_MANAGER
#define MA_NO_NODE_GRAPH
#define MA_NO_GENERATION
#include "C:/terminal_cpp/_STEMS/template_miniaudio/external/miniaudio-master/miniaudio.h"

namespace ScorePlayer {
///////////////// METHODS TO CALL /////////////////////
///////////////////////////////////////////////////////
void initializeAudio();
void closeAudio();
void playFrequencies(const std::vector<double>& freqs);
void stop(bool muteReverb);
////////////////////////////////////////////////////////
///////////////////////////////////////////////////////
    
namespace audio_settings {	
	inline constexpr int NCHANNELS = 2;
	inline constexpr int SAMPLERATE = 1 * 44100;
	inline constexpr int PERIOD_SIZE_MS = 10;
	inline constexpr int NPERIODS = 4;
	typedef short BITRES; // // bit resolution
}

static_assert (audio_settings::NCHANNELS == 2); // // for now, just 2 channels supported
static double frame[audio_settings::NCHANNELS]; // // in stack. NCHANNELS is small

struct NoteData {
    double frequency;
    int durationInTakts;
    friend std::ostream& operator<<(std::ostream& os, const NoteData& x) {
        os << "{ "<<x.frequency<<", "<<x.durationInTakts<<" }";
        return os;
    }
};

std::vector<double> isolatedFreqs;
std::vector<double> isolatedFreqs_waiting;
std::atomic_flag isDataBeingUsed = ATOMIC_FLAG_INIT;

namespace ScoreSynth {
    using namespace dsp;
    const int SR = audio_settings::SAMPLERATE;
    const double DT = 1./(double)SR;
    
    Clock clock;
    
    void Start() {}
    
    struct Oscillator {
        Double2 y;
        void operator()() {
            y.null();
            saw();
            y += lp(saw.y).y;
        }
        void freq(double f) {
            saw.dy(DT * f);
            lp.dy(DT * std::min(14000., f*6));
        }
        Sawbl saw;
        Onepole_lp lp;
    };

    std::vector<Oscillator> units = std::vector<Oscillator>(30); ////////////////////////////////////////////////
    int nUsedUnits = 0;

    enum class State {
        None,
        Idle,
        Playing
    };
    std::atomic<State> state[2]{ State::Idle, State::None }; // // state[0] is current state. state[1] is requested state. //////////////////// atomic ???????
    
    double gainCompensation;
        
    Yafr2 rev{SR, 0.7, 0.5};
    
    double amp = 0;
    double ampDy = 1./(SR*0.03);
    
    void applyNewFreqs() {
        if (isDataBeingUsed.test_and_set(std::memory_order_acquire)) return;
        isolatedFreqs = isolatedFreqs_waiting;
        isDataBeingUsed.clear(std::memory_order_release);
        state[0] = State::Playing;
        state[1] = State::None;
        nUsedUnits = isolatedFreqs.size();
        for (int i = 0; i < nUsedUnits; ++i) units[i].freq(isolatedFreqs[i]);
        gainCompensation = nUsedUnits == 0 ? 1. : 1./nUsedUnits;
    }
    
    bool muteReverb = false;
    
    Double2 y{0.,0.};
    void Update(double* frame) {
        
        if (state[0] == State::Idle && state[1] == State::None) {
            // // be idle
        }
        else if (state[0] == State::Idle && state[1] == State::Playing) {
            applyNewFreqs();
        }
        else if (state[0] == State::Playing && state[1] == State::None) {
            amp += ampDy;
            if (amp > 1) amp = 1;
        }
        else if (state[0] == State::Playing && state[1] == State::Idle) {
            amp -= ampDy;
            if (amp <= 0) {
                amp = 0;
                state[0] = State::Idle;
                state[1] = State::None;
                if (muteReverb) rev.reset();
            }
        }
        else if (state[0] == State::Playing && state[1] == State::Playing) {
            amp -= ampDy;
            if (amp <= 0) {
                amp = 0;
                applyNewFreqs();
            }
        }
        else {
            std::cerr << "synth states wtf\n";
            throw "err";
        }
        
        y.null();
        if (state[0] == State::Playing) {
            for (int u = 0; u < nUsedUnits; ++u) {
                units[u]();
                y += units[u].y;
            }
            y *= gainCompensation;
            y *= amp;
            rev.x += y;
        }
        
        rev();
        double revMix = 0.7;
        y = (1-revMix)*y + revMix*(muteReverb ? amp : 1)*rev.y;
        
        for (int c = 0; c < 2; ++c) {
            if (y[c] < -1) y[c] = -1;
            if (y[c] > 1) y[c] = 1;
        }
        frame[0] = y[0]; frame[1] = y[1];
    }
    
    void requestIsolatedNotes() {
        state[1] = State::Playing;
        muteReverb = false;
        return;
    }
    
    void requestStop(bool _muteReverb) {
        muteReverb = _muteReverb;
        if (state[0] == State::Idle) state[1] = State::None;
        else state[1] = State::Idle;
        return;
    }
} // // namespace ScoreSynth

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
	float *pt = (float *)pOutput;
	for (int n = 0, n2 = 0, n2p1 = 1; n < frameCount; ++n, n2+=2, n2p1+=2) {
        ScoreSynth::Update(frame);
		pt[n2] = (float)frame[0];
		pt[n2p1] = (float)frame[1];
	}
}

ma_device_config config;
ma_device device;
void init() {
    isolatedFreqs.reserve(30);
    isolatedFreqs_waiting.reserve(30);
    
    ScoreSynth::Start();
    
    config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_f32;
    config.playback.channels = audio_settings::NCHANNELS;
    config.sampleRate = audio_settings::SAMPLERATE;
    config.dataCallback = data_callback;
    config.periodSizeInMilliseconds = audio_settings::PERIOD_SIZE_MS;
    config.periods = audio_settings::NPERIODS;
    if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) {
        std::cerr << "ERROR: Failed to initialize the device.\n";
        throw "err";
    }
    ma_device_start(&device);
}

void uninit() {
    ma_device_uninit(&device);
}

void playFrequencies(const std::vector<double>& freqs) {
    if (isDataBeingUsed.test_and_set(std::memory_order_acquire)) return;
    isolatedFreqs_waiting = freqs;
    isDataBeingUsed.clear(std::memory_order_release);
    ScoreSynth::requestIsolatedNotes();
}

void stop(bool muteReverb = false) {
    ScoreSynth::requestStop(muteReverb);
}

} // // namespace

#endif /* end of include guard: SCORE_PLAYER_H */
