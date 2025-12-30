#pragma once

class PListener;

#include "PluginEditor.hpp"

class PListener : public juce::AudioProcessorParameter::Listener {
public:
    PListener(std::initializer_list<juce::RangedAudioParameter*> params, juce::AudioProcessor* p) : params(params), proc(p) {
        for (auto param : params)
            param->addListener(this);
    }

    ~PListener() override { for (auto param : params) param->removeListener(this); }

    void parameterValueChanged(int parameterIndex, float newValue) override;

    void parameterGestureChanged(int parameterIndex,
        bool gestureIsStarting) override {
        (void)parameterIndex;
        (void)gestureIsStarting;
    }

private:
    std::vector<juce::RangedAudioParameter*> params;
    juce::AudioProcessor* proc;
};