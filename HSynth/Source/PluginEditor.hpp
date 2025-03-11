#pragma once

#include <optional>
#include "JuceHeader.h"
#include "PluginProcessor.hpp"
#include "Parsing.hpp"
#include "KnobComponent.hpp"
#include "ComputeShader.hpp"

class HSynthAudioProcessorEditor;

class PListener : public juce::AudioProcessorParameter::Listener {
   public:
    PListener(juce::RangedAudioParameter* param,
              HSynthAudioProcessorEditor* editor);

    ~PListener() override;

    void parameterValueChanged(int parameterIndex, float newValue) override;

    void parameterGestureChanged(int parameterIndex,
                                 bool gestureIsStarting) override;

   private:
    juce::RangedAudioParameter* param;
    HSynthAudioProcessorEditor* editor;
};

class HSynthAudioProcessorEditor : public juce::AudioProcessorEditor {
   public:
    HSynthAudioProcessorEditor(HSynthAudioProcessor&);
    ~HSynthAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void redrawGraph();

   private:
    HSynthAudioProcessor& audioProcessor;

    juce::Component dummy;

    KnobComponent aKnob, bKnob;
    PListener aListener, bListener;

    juce::TextEditor formula;
    juce::Label title, error;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HSynthAudioProcessorEditor)
};
