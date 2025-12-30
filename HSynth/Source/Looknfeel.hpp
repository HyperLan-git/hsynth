#pragma once

#include <chrono>

#include "JuceHeader.h"

class Looknfeel : public juce::LookAndFeel_V4 {
   public:
    Looknfeel();

    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider&) override;

    void fillTextEditorBackground(juce::Graphics&, int width, int height,
                                  juce::TextEditor&) override;
    void drawTextEditorOutline(juce::Graphics&, int width, int height,
                               juce::TextEditor&) override;
    juce::Font getLabelFont(juce::Label& label) override;
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width,
                          int height, float sliderPos, float minSliderPos,
                          float maxSliderPos,
                          const juce::Slider::SliderStyle style,
                          juce::Slider& slider) override;

   private:
    float cornerSizeX = 10, cornerSizeY = 20;
    juce::Font ft;
    std::chrono::time_point<std::chrono::system_clock> start =
        std::chrono::system_clock::now();
};