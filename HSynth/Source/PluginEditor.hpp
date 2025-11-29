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

constexpr char animShader[] =
    "uniform mat4 padding;\n"
    "uniform float time;\n"
    "void main()\n"
    "{\n"
    " float pi = 3.141596;"
    " float t = pi*time/4.0;"
    " vec2 uv = vec2(pixelPos.x/900.0, pixelPos.y/700);"
    " float r = 0.8*(cos(5.0*(time+uv.x*uv.y))/2.0+0.5)"
    "       + 0.3*(sin(4.0*"
    "         (t-((uv.y+1.0)/(1.01+cos(t/5.0))-uv.y/3.0-0.5)"
    "        -uv.y*0.3))/2.0+0.5),"
    "       g = (cos(5.0*"
    "          (t + uv.y*atan(4.0*uv.x*cos(t/5.0))/2.0 + "
    "           uv.x-uv.y*1.3))/2.0+0.5),"
    "       b = 0.5*(cos(5.0*(t - uv.x))/2.0+0.5)"
    "         + 0.5*(sin(4.0*("
    "           3.0+t-atan("
    "            (uv.y+20.0)/(1.01+cos(3.0+t/5.0))-uv.x/4.0-0.5)-uv.x*0.3"
    "            ))/2.0+0.5);"
    " vec3 col = vec3(abs(r),"
    "            abs(g*0.9+0.1*g/(b+0.9)), abs(0.9*b+0.2*b/(r+0.9)));"
    " gl_FragColor = pixelAlpha * (vec4(col*0.5, 0.99));\n"
    "}\n";
