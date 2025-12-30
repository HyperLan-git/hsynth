#pragma once

#include <JuceHeader.h>

#include "Looknfeel.hpp"

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
};