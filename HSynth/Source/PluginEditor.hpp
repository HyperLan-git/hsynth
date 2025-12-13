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

class BoolParamListener : public juce::Button::Listener, public juce::AudioProcessorParameter::Listener {
   public:
       BoolParamListener(juce::AudioParameterBool* param, juce::ToggleButton* button) : param(param), button(button) {
           param->addListener(this);
           button->addListener(this);
       }

       void buttonClicked(juce::Button* b) override {
           param->setValueNotifyingHost(b->getToggleState() ? 1.f : 0.f);
       }

       void parameterValueChanged(int parameterIndex, float newValue) override {
           button->setToggleState(newValue > 0.f, juce::NotificationType::sendNotificationAsync);
       }

       void parameterGestureChanged(int parameterIndex,
           bool gestureIsStarting) override {}

       ~BoolParamListener() {
           param->removeListener(this);
           button->removeListener(this);
       }
   private:
    juce::AudioParameterBool* param;
    juce::ToggleButton* button;
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

    void setErrorText(std::string text);

    void setErrorTextFromAudioProcessor() {
        setErrorText(audioProcessor.getError());
    }

   private:
    HSynthAudioProcessor& audioProcessor;

    juce::ToggleButton limiterEnabled;
    juce::Label limiterLabel;
    BoolParamListener limiterListener;

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
