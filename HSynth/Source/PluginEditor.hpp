#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.hpp"
#include "Parsing.hpp"

class HSynthAudioProcessorEditor : public juce::AudioProcessorEditor {
   public:
    HSynthAudioProcessorEditor(HSynthAudioProcessor&);
    ~HSynthAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

   private:
    HSynthAudioProcessor& audioProcessor;

    struct HyperToken* formulaTree = nullptr;
    float data[2048] = {};

    juce::TextEditor formula;
    juce::Label title, error;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HSynthAudioProcessorEditor)
};
