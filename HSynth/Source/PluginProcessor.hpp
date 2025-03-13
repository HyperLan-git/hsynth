#pragma once

#include <array>
#include <memory>

#include "JuceHeader.h"
#include "Parsing.hpp"
#include "ComputeShader.hpp"

#define MAX_VOICES 32

struct Voice {
    uint8_t note = 0, velocity = 0;
    int64_t timeStart = 0, timeRelease = 0;
    float releaseAmp = 0;
    // Between 0 and 1
    float phase = 0;
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

    inline const std::string& getError() const { return error; }

    inline juce::AudioParameterFloat* getAParam() { return a; }
    inline juce::AudioParameterFloat* getBParam() { return b; }

    inline const WTFrame& getCurrentFrame() const {
        return data[(int)(b->get() * 255)][(int)(a->get() * 255)];
    }

    inline juce::OpenGLContext& getContext() { return context; }

   private:
    std::string formula;
    std::unique_ptr<ComputeShader> shader;

    std::string error;

    struct HyperToken* formulaTree = nullptr;

    float data[256][256][2048] = {0};

    juce::OpenGLContext context;

    juce::AudioParameterFloat *a, *b;
    juce::AudioParameterFloat *hz_shift, *st_shift, *phase;
    juce::AudioParameterFloat *attack, *decay, *sustain, *release;

    // TODO lfo system/other envelopes maybe?

    std::array<struct Voice, MAX_VOICES> voices;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HSynthAudioProcessor)
};
