#pragma once

#include <JuceHeader.h>
#include "Looknfeel.hpp"

class ParamListener : public juce::Slider::Listener,
                      public juce::AudioProcessorParameter::Listener {
   public:
    ParamListener(juce::RangedAudioParameter* param, juce::Slider& slider);

    ~ParamListener() override;

    void sliderValueChanged(juce::Slider* slider) override;

    void sliderDragStarted(juce::Slider* slider) override;

    void sliderDragEnded(juce::Slider* slider) override;

    void parameterValueChanged(int parameterIndex, float newValue) override;

    void parameterGestureChanged(int parameterIndex,
                                 bool gestureIsStarting) override;

   private:
    juce::RangedAudioParameter* param;
    juce::Slider& slider;
};

class KnobComponent : public juce::Component {
   public:
    KnobComponent(juce::RangedAudioParameter* param, double step = .01);
    ~KnobComponent() override;

    void paint(juce::Graphics& g) override;

    void resized() override;

    void setSliderColor(int colorID, juce::Colour color) {
        knob.setColour(colorID, color);
    }

    double getValue() const;

   private:
    juce::Slider knob;
    juce::Label label;
    Looknfeel lf;

    juce::SliderParameterAttachment attachment;

    //ParamListener paramListener;
};