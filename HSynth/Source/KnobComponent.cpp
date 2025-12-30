#include "KnobComponent.hpp"

KnobComponent::KnobComponent(juce::RangedAudioParameter* param, double step)
    : label(param->getName(128), param->getName(128)),
      attachment(*param, knob) {
    addAndMakeVisible(knob);
    addAndMakeVisible(label);
    label.setText(label.getText().substring(0, 1).toUpperCase() +
                      label.getText().substring(1),
                  juce::sendNotificationAsync);

    label.attachToComponent(&knob, false);
    label.setJustificationType(juce::Justification::centred);
    label.setLookAndFeel(&lf);

    auto range = param->getNormalisableRange();

    knob.setLookAndFeel(&lf);
    knob.setColour(juce::Slider::thumbColourId, juce::Colours::grey.brighter());
    knob.setColour(juce::Slider::textBoxOutlineColourId,
                   juce::Colours::transparentWhite);
    knob.setSliderStyle(juce::Slider::SliderStyle::RotaryVerticalDrag);
    knob.setTextBoxStyle(juce::Slider::TextBoxAbove, false, 100, 20);
    knob.setDoubleClickReturnValue(
        true, range.convertFrom0to1(param->getDefaultValue()),
        juce::ModifierKeys::Flags::noModifiers);

    knob.setRange(range.start, range.end, step);
    knob.setValue(param->convertFrom0to1(param->getValue()));

    setSize(100, 100);
}

KnobComponent::~KnobComponent() {
    label.setLookAndFeel(nullptr);
    knob.setLookAndFeel(nullptr);
}

void KnobComponent::paint(juce::Graphics& g) {
    g.fillAll(juce::Colours::black.withAlpha(0.4f));
}

double KnobComponent::getValue() const { return knob.getValue(); }

void KnobComponent::resized() {
    label.setBounds(0, 0, 100, 20);
    knob.setBounds(0, 20, 100, 80);
}
