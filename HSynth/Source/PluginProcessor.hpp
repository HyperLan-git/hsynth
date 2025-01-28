#pragma once

#include <JuceHeader.h>
#include <array>
#include "Parsing.hpp"

#define MAX_VOICES 32

struct Voice {
    int note = 0;
    float timeStart = 0, timeRelease = 0;
    float phase = 0;
};

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

   private:
    juce::AudioParameterFloat *hz_shift, *st_shift, *phase;
    juce::AudioParameterFloat *attack, *decay, *sustain, *release;

    // TODO lfo system/other envelopes maybe?

    juce::AudioParameterChoice* wave;

    std::array<struct Voice, MAX_VOICES> voices;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HSynthAudioProcessor)
};
