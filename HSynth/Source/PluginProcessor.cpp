#include "PluginProcessor.hpp"
#include "PluginEditor.hpp"

HSynthAudioProcessor::HSynthAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(
          BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
              )
#endif
      ,
      hz_shift(new juce::AudioParameterFloat({"hz_shift", 1}, "Frequency shift",
                                             -10000, 10000, 0)) {
    addParameter(hz_shift);
}

HSynthAudioProcessor::~HSynthAudioProcessor() {}

const juce::String HSynthAudioProcessor::getName() const {
    return JucePlugin_Name;
}

bool HSynthAudioProcessor::acceptsMidi() const {
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool HSynthAudioProcessor::producesMidi() const {
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool HSynthAudioProcessor::isMidiEffect() const {
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double HSynthAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int HSynthAudioProcessor::getNumPrograms() { return 1; }

int HSynthAudioProcessor::getCurrentProgram() { return 0; }

void HSynthAudioProcessor::setCurrentProgram(int index) {}

const juce::String HSynthAudioProcessor::getProgramName(int index) {
    return {};
}

void HSynthAudioProcessor::changeProgramName(int index,
                                             const juce::String& newName) {}

void HSynthAudioProcessor::prepareToPlay(double sampleRate,
                                         int samplesPerBlock) {}

void HSynthAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool HSynthAudioProcessor::isBusesLayoutSupported(
    const BusesLayout& layouts) const {
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else

    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
        layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

#if !JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}
#endif

void HSynthAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                        juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;
    int inputs = getTotalNumInputChannels();
    int outputs = getTotalNumOutputChannels();

    for (int channel = 0; channel < outputs; ++channel) {
        auto* channelData = buffer.getWritePointer(channel);
    }
}

bool HSynthAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* HSynthAudioProcessor::createEditor() {
    return new HSynthAudioProcessorEditor(*this);
}

void HSynthAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {}

void HSynthAudioProcessor::setStateInformation(const void* data,
                                               int sizeInBytes) {}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new HSynthAudioProcessor();
}
