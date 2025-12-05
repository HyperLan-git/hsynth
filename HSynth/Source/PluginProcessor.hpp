#pragma once

#include <array>
#include <cstring>
#include <memory>
#include <chrono>
#include <random>

#include "ComputeShader.hpp"
#include "JuceHeader.h"
#include "Parsing.hpp"

#define MAX_VOICES 64

struct Voice {
    uint8_t note = 0, velocity = 0, voices = 1, voice = 0;
    int64_t timeStart = 0, timeRelease = 0;
    float releaseAmp = 0;
    float voiceDetune = 0;
    float detune = 0, amp = 0, pan = 0;
    double freq = 0;
    // Between 0 and 1
    float startingPhase = 0;
    double phase = 0;
};

using WTFrame = float[2048];

class HSynthAudioProcessor : public juce::AudioProcessor {
   public:
    HSynthAudioProcessor();
    ~HSynthAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    void computeBuffer(const std::string& formula);

    inline const std::string& getError() const { return errorStr; }

    inline juce::AudioParameterFloat* getAParam() { return a; }
    inline juce::AudioParameterFloat* getBParam() { return b; }

    inline juce::AudioParameterFloat* getAttackParam() { return attack; }
    inline juce::AudioParameterFloat* getDecayParam() { return decay; }
    inline juce::AudioParameterFloat* getSustainParam() { return sustain; }
    inline juce::AudioParameterFloat* getReleaseParam() { return release; }

    inline juce::AudioParameterFloat* getHzShiftParam() { return hzShift; }
    inline juce::AudioParameterFloat* getStShiftParam() { return stShift; }

    inline juce::AudioParameterInt* getVoicesParam() { return voicesPerNote; }
    inline juce::AudioParameterFloat* getDetuneParam() { return detune; }

    inline juce::AudioParameterFloat* getPhaseParam() { return phase; }
    inline juce::AudioParameterFloat* getPhaseRandomnessParam() {
        return phaseRandomness;
    }

    inline const WTFrame& getCurrentFrame() const {
#ifdef _WIN64
        return data[(int)(b->get() * 255) * 256 + (int)(a->get() * 255)];
#else
        return data[(int)(b->get() * 255)][(int)(a->get() * 255)];
#endif
    }

    inline juce::OpenGLContext& getContext() { return context; }

   private:
    std::string formula;
    std::unique_ptr<ComputeShader> shader;

    std::string errorStr;

    struct HyperToken* formulaTree = nullptr;
    // XXX Dumb bug with msvc which causes it to crap itself when allocating 500MB on the stack (I wonder why xd)
#ifdef _WIN64
    WTFrame* data = new WTFrame[256 * 256];
#else
    float data[256][256][2048] = { 0 };
#endif

    juce::OpenGLContext context;

    juce::AudioParameterFloat *a, *b;
    juce::AudioParameterFloat *hzShift, *stShift, *phase;
    juce::AudioParameterFloat *attack, *decay, *sustain, *release;

    juce::AudioParameterInt* voicesPerNote;
    juce::AudioParameterFloat *detune, *phaseRandomness;

    juce::AudioParameterBool* limiter;

    double prevValidSampleRate = 0;
    float prevPhase = 0, prevDetune = 0;

    // TODO lfo system/other envelopes maybe?

    std::array<struct Voice, MAX_VOICES> voices;
    int freeVoices = MAX_VOICES;

    std::random_device randomDev;
    std::uniform_real_distribution<float> randFloat = std::uniform_real_distribution<float>(0, 10);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HSynthAudioProcessor)
};
