#include "PluginEditor.hpp"

#include <chrono>

HSynthAudioProcessorEditor::HSynthAudioProcessorEditor(HSynthAudioProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p),
      aKnob(p.getAParam()),
      aListener(p.getAParam(), this),
      bKnob(p.getBParam()),
      bListener(p.getBParam(), this),
      attackKnob(p.getAttackParam()),
      decayKnob(p.getDecayParam()),
      sustainKnob(p.getSustainParam()),
      releaseKnob(p.getReleaseParam()) {
    setSize(800, 700);
    this->error.setColour(juce::Label::textColourId, juce::Colours::red);
    this->formula.setFont(juce::FontOptions(20.0f));
    this->formula.setTitle("Formula");
    this->formula.setClicksOutsideDismissVirtualKeyboard(true);
    this->formula.setMultiLine(true);
    this->title.setText("Formula : ",
                        juce::NotificationType::sendNotificationAsync);
    this->formula.setTextToShowWhenEmpty("Formula example : sin(p)",
                                         juce::Colours::grey);

    this->formula.onReturnKey = this->formula.onFocusLost = [=] {
        std::string formula(this->formula.getText().toStdString());
        this->audioProcessor.getContext().executeOnGLThread(
            [=](juce::OpenGLContext& ctx) {
                (void)ctx;
                juce::Logger::writeToLog("Compute buffer");
                this->audioProcessor.computeBuffer(formula);
                this->error.setText(
                    this->audioProcessor.getError(),
                    juce::NotificationType::sendNotificationAsync);
                juce::Logger::writeToLog("Redraw graph");
                redrawGraph();
            },
            false);
    };
    this->setWantsKeyboardFocus(true);
    this->formula.onEscapeKey = [=] { this->grabKeyboardFocus(); };

    // Must be first one
    this->addAndMakeVisible(this->dummy);
    this->addAndMakeVisible(this->title);
    this->addAndMakeVisible(this->formula);
    this->addAndMakeVisible(this->error);
    this->addAndMakeVisible(this->aKnob);
    this->addAndMakeVisible(this->bKnob);
    this->addAndMakeVisible(this->attackKnob);
    this->addAndMakeVisible(this->decayKnob);
    this->addAndMakeVisible(this->sustainKnob);
    this->addAndMakeVisible(this->releaseKnob);
}

HSynthAudioProcessorEditor::~HSynthAudioProcessorEditor() {
    auto context = juce::OpenGLContext::getContextAttachedTo(*this);
    if (context) context->detach();
}

void HSynthAudioProcessorEditor::redrawGraph() {
    juce::MessageManager::callAsync(
        [=]() { this->repaint({45, 95, 660, 710}); });
}

void HSynthAudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colours::black);

    g.setColour(juce::Colours::yellow);
    g.setFont(juce::FontOptions(15.0f));
    constexpr float startX = 50, endX = 650;
    constexpr float startY = 100, endY = 700;
    constexpr int maxI = 1024;
    const auto& frame = audioProcessor.getCurrentFrame();
    juce::Path p;
    p.preallocateSpace(3 * maxI);
    p.startNewSubPath(50, -frame[0] * 300 + 400);
    for (int i = 1; i < maxI; i++) {
        float x = 50 + i * 600.f / maxI,
              y = -frame[i * 2048 / maxI] * 300 + 400;
        p.lineTo(x, y);
    }
    g.strokePath(p, juce::PathStrokeType(2));
}

void HSynthAudioProcessorEditor::resized() {
    this->title.setBounds({100, 0, 400, 25});
    this->formula.setBounds({100, 25, 400, 50});
    this->error.setBounds({100, 75, 400, 25});
    this->aKnob.setBounds({500, 0, 100, 100});
    this->bKnob.setBounds({600, 0, 100, 100});
    this->attackKnob.setBounds({700, 0, 100, 100});
    this->decayKnob.setBounds({700, 100, 100, 100});
    this->sustainKnob.setBounds({700, 200, 100, 100});
    this->releaseKnob.setBounds({700, 300, 100, 100});
}

PListener::PListener(juce::RangedAudioParameter* param,
                     HSynthAudioProcessorEditor* editor)
    : param(param), editor(editor) {
    this->param->addListener(this);
}

PListener::~PListener() { this->param->removeListener(this); }

void PListener::parameterValueChanged(int parameterIndex, float newValue) {
    (void)parameterIndex;
    (void)newValue;
    this->editor->redrawGraph();
}

void PListener::parameterGestureChanged(int parameterIndex,
    bool gestureIsStarting) {
    (void)parameterIndex;
    (void)gestureIsStarting;
}