#pragma once

#include <JuceHeader.h>

class ProcessingComponent {
   public:
    void setBounds(juce::Rectangle<int> b) { this->bounds = b; }
    juce::Rectangle<int> getBounds() const { return bounds; }

    virtual void processBlock(juce::AudioBuffer<float>& buf,
                              juce::MidiBuffer& midi) = 0;

    virtual void paint(juce::Graphics& g) = 0;

   private:
    juce::Rectangle<int> bounds;
};