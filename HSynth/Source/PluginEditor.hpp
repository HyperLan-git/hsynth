#pragma once

#include <optional>

#include "ComputeShader.hpp"
#include "JuceHeader.h"
#include "KnobComponent.hpp"
#include "Parsing.hpp"
#include "PluginProcessor.hpp"

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

    juce::ToggleButton limiterEnabled;

    KnobComponent aKnob, bKnob, attackKnob, decayKnob, sustainKnob, releaseKnob,
        voicesKnob, detuneKnob, phaseKnob, phaseRandKnob, stShiftKnob,
        hzShiftKnob;
    PListener aListener, bListener;

    juce::TextEditor formula;
    juce::Label title, error;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HSynthAudioProcessorEditor)
};
