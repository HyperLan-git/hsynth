#include "PluginEditor.hpp"

HSynthAudioProcessorEditor::HSynthAudioProcessorEditor(HSynthAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p) {
    setSize(700, 700);
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
        std::string err;
        delete formulaTree;
        this->formulaTree = parse(this->formula.getText().toStdString(), err);
        this->error.setText(err, juce::NotificationType::sendNotificationAsync);

        if (err.empty() && formulaTree) {
            float a = 0;
            for (int i = 0; i < 2048; i++) {
                this->data[i] = computeSample((float)i / 2048.f,
                                              *(this->formulaTree), &a, 1);
            }
            this->repaint({0, 0, 700, 700});
        }
    };
    this->setWantsKeyboardFocus(true);
    this->formula.onEscapeKey = [=] { this->grabKeyboardFocus(); };

    this->addAndMakeVisible(this->title);
    this->addAndMakeVisible(this->formula);
    this->addAndMakeVisible(this->error);
}

HSynthAudioProcessorEditor::~HSynthAudioProcessorEditor() {
    // TODO ensure this is not being used while it's being deleted
    delete this->formulaTree;
}

void HSynthAudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colours::black);

    g.setColour(juce::Colours::yellow);
    g.setFont(juce::FontOptions(15.0f));
    float startX = 50, endX = 650;
    float startY = 100, endY = 700;
    float prevX = 50, prevY = -this->data[0] * 300 + 400;
    for (int i = 1; i < 2048; i++) {
        float x = 50 + i * 600 / 2048.f, y = -this->data[i] * 300 + 400;
        g.drawLine({prevX, prevY, x, y});
        prevX = x;
        prevY = y;
    }
}

void HSynthAudioProcessorEditor::resized() {
    this->title.setBounds({100, 0, 400, 25});
    this->formula.setBounds({100, 25, 400, 50});
    this->error.setBounds({100, 75, 400, 25});
}
