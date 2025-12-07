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

class RepaintTimer : public juce::Timer {
   public:
    RepaintTimer(juce::Component&);
    virtual ~RepaintTimer();

    void timerCallback() override;

   private:
    juce::Component& toRepaint;
};

// Lines of uniforms and definitions that get added by juce in a dirty way...
constexpr int BGSHADER_HEADER_LINES = 5;

class HSynthAudioProcessorEditor : public juce::AudioProcessorEditor {
   public:
    HSynthAudioProcessorEditor(HSynthAudioProcessor&);
    virtual ~HSynthAudioProcessorEditor();

    void paint(juce::Graphics&) override;
    void resized() override;
    void redrawGraph();

   private:
    HSynthAudioProcessor& audioProcessor;

    juce::ToggleButton limiterEnabled;

    std::unique_ptr<juce::OpenGLGraphicsContextCustomShader> shader;
    std::optional<juce::OpenGLShaderProgram::Uniform> timeUniform;

    std::chrono::time_point<std::chrono::system_clock> start =
        std::chrono::system_clock::now();
    RepaintTimer timer;

    bool drawGraph = true;
    juce::Path graph;

    KnobComponent aKnob, bKnob, attackKnob, decayKnob, sustainKnob, releaseKnob,
        voicesKnob, detuneKnob, phaseKnob, phaseRandKnob, stShiftKnob,
        hzShiftKnob;
    PListener aListener, bListener;

    juce::TextEditor formula;
    juce::Label title, error;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HSynthAudioProcessorEditor)
};
