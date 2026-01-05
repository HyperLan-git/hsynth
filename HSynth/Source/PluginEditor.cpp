#include "PluginEditor.hpp"

#include <chrono>

HSynthAudioProcessorEditor::HSynthAudioProcessorEditor(HSynthAudioProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p),
      aKnob(p.getAParam()),
      bKnob(p.getBParam()),
      attackKnob(p.getAttackParam(), 1),
      decayKnob(p.getDecayParam(), 1),
      sustainKnob(p.getSustainParam()),
      releaseKnob(p.getReleaseParam(), 1),
      voicesKnob(p.getVoicesParam()),
      detuneKnob(p.getDetuneParam()),
      phaseKnob(p.getPhaseParam()),
      phaseRandKnob(p.getPhaseRandomnessParam(), 1),
      stShiftKnob(p.getStShiftParam()),
      hzShiftKnob(p.getHzShiftParam(), 10),
      timer(*this),
      limiterAttachment(*p.getLimiterParam(), limiterEnabled),
      volumeAttachment(*p.getVolumeParam(), volumeSlider) {
    setSize(950, 750);
    this->setResizable(true, true);
    this->error.setColour(juce::Label::textColourId, juce::Colours::red);
    this->formula.setTitle("Formula");
    this->formula.setClicksOutsideDismissVirtualKeyboard(true);
    this->formula.setMultiLine(true);
    this->formula.setColour(juce::TextEditor::outlineColourId,
                            juce::Colours::darkgoldenrod);
    this->formula.setColour(juce::TextEditor::focusedOutlineColourId,
                            juce::Colours::gold);
    this->formula.setText(p.getFormula());
    this->title.setText("Formula : ",
                        juce::NotificationType::sendNotificationAsync);
    this->title.setLookAndFeel(&lf);
    this->formula.setTextToShowWhenEmpty("sin(p)", juce::Colours::lightgrey);
    this->formula.setLookAndFeel(&lf);
    this->formula.setColour(juce::TextEditor::backgroundColourId,
                            juce::Colours::cyan.darker(0.95f).withAlpha(0.9f));

    this->formula.onReturnKey = this->formula.onFocusLost = [=] {
        std::string formula(this->formula.getText().toStdString());
        this->audioProcessor.getContext().executeOnGLThread(
            [=](juce::OpenGLContext& ctx) {
                (void)ctx;
                juce::Logger::writeToLog("Compute buffer");
                this->audioProcessor.computeBuffer(formula);
                juce::Logger::writeToLog("Redraw graph");
                redrawGraph();
            },
            true);
        this->setErrorTextFromAudioProcessor();
    };
    this->setWantsKeyboardFocus(true);
    this->formula.onEscapeKey = [=] { this->grabKeyboardFocus(); };

    this->limiterLabel.setText("Norm.",
                               juce::NotificationType::sendNotificationAsync);
    this->limiterLabel.setLookAndFeel(&lf);
    this->limiterLabel.setColour(juce::Label::backgroundColourId,
                                 juce::Colours::black.withAlpha(0.4f));

    this->aKnob.setSliderColor(juce::Slider::thumbColourId,
                               juce::Colours::red.darker());
    this->bKnob.setSliderColor(juce::Slider::thumbColourId,
                               juce::Colours::blue.darker());

    this->attackKnob.setSliderColor(juce::Slider::thumbColourId,
                                    juce::Colours::orangered.darker());
    this->decayKnob.setSliderColor(
        juce::Slider::thumbColourId,
        juce::Colours::orangered.withRotatedHue(0.25f).darker());
    this->sustainKnob.setSliderColor(
        juce::Slider::thumbColourId,
        juce::Colours::orangered.withRotatedHue(0.5f).darker());
    this->releaseKnob.setSliderColor(
        juce::Slider::thumbColourId,
        juce::Colours::orangered.withRotatedHue(0.75f).darker());

    this->voicesKnob.setSliderColor(juce::Slider::thumbColourId,
                                    juce::Colours::cyan.darker());
    this->detuneKnob.setSliderColor(juce::Slider::thumbColourId,
                                    juce::Colours::mediumpurple.darker());

    this->phaseKnob.setSliderColor(juce::Slider::thumbColourId,
                                   juce::Colours::gold.darker());
    this->phaseRandKnob.setSliderColor(juce::Slider::thumbColourId,
                                       juce::Colours::goldenrod.darker());

    this->stShiftKnob.setSliderColor(juce::Slider::thumbColourId,
                                     juce::Colours::burlywood.darker(.6)
                                         .withRotatedHue(.2)
                                         .withMultipliedSaturation(.8));
    this->hzShiftKnob.setSliderColor(juce::Slider::thumbColourId,
                                     juce::Colours::burlywood.darker(.6)
                                         .withRotatedHue(.5)
                                         .withMultipliedSaturation(.8));

    this->limiterEnabled.setToggleState(
        p.getLimiterParam(), juce::NotificationType::dontSendNotification);

    auto volParamName = p.getVolumeParam()->getName(256);
    this->volumeLabel.setText(
        volParamName.substring(0, 1).toUpperCase() + volParamName.substring(1),
        juce::sendNotificationAsync);
    this->volumeLabel.setJustificationType(juce::Justification::centred);
    this->volumeLabel.setLookAndFeel(&lf);
    this->volumeLabel.attachToComponent(&this->volumeSlider, false);
    this->volumeSlider.setTextBoxStyle(juce::Slider::TextBoxAbove, false, 100,
                                       20);
    this->volumeSlider.setSliderStyle(juce::Slider::LinearVertical);
    auto& volRange = p.getVolumeParam()->getNormalisableRange();
    this->volumeSlider.setRange(volRange.start, volRange.end, 0.01);
    auto volDefValue =
        ((juce::RangedAudioParameter*)p.getVolumeParam())->getDefaultValue();
    this->volumeSlider.setDoubleClickReturnValue(
        true, volRange.convertFrom0to1(volDefValue),
        juce::ModifierKeys::Flags::noModifiers);
    this->volumeSlider.setColour(juce::Slider::trackColourId,
                                 juce::Colours::silver);
    this->volumeSlider.setColour(juce::Slider::thumbColourId,
                                 juce::Colours::silver);
    this->volumeSlider.setColour(juce::Slider::textBoxOutlineColourId,
                                 juce::Colours::transparentWhite);
    this->volumeSlider.setLookAndFeel(&lf);

    this->addAndMakeVisible(this->title);
    this->addAndMakeVisible(this->formula);
    this->addAndMakeVisible(this->error);
    this->addAndMakeVisible(this->aKnob);
    this->addAndMakeVisible(this->bKnob);
    this->addAndMakeVisible(this->attackKnob);
    this->addAndMakeVisible(this->decayKnob);
    this->addAndMakeVisible(this->sustainKnob);
    this->addAndMakeVisible(this->releaseKnob);
    this->addAndMakeVisible(this->voicesKnob);
    this->addAndMakeVisible(this->detuneKnob);
    this->addAndMakeVisible(this->phaseKnob);
    this->addAndMakeVisible(this->phaseRandKnob);
    this->addAndMakeVisible(this->stShiftKnob);
    this->addAndMakeVisible(this->hzShiftKnob);
    this->addAndMakeVisible(this->limiterLabel);
    this->addAndMakeVisible(this->limiterEnabled);
    this->addAndMakeVisible(this->volumeSlider);
    this->addAndMakeVisible(this->volumeLabel);

    const char* shaderCode = BinaryData::bg_frag;
    for (int i = 0, lines = 0;
         i < BinaryData::bg_fragSize && lines < BGSHADER_HEADER_LINES;
         i++, shaderCode++) {
        if (*shaderCode == '\n') lines++;
    }

    shader =
        std::make_unique<juce::OpenGLGraphicsContextCustomShader>(shaderCode);
    timer.startTimer(1000 / 30);
}

HSynthAudioProcessorEditor::~HSynthAudioProcessorEditor() {
    this->formula.onReturnKey = this->formula.onFocusLost =
        std::function<void()>();
    this->title.setLookAndFeel(nullptr);
    this->limiterLabel.setLookAndFeel(nullptr);
    this->volumeLabel.setLookAndFeel(nullptr);
    this->volumeSlider.setLookAndFeel(nullptr);
    shader.reset();
    this->audioProcessor.detachContext();
    juce::Logger::outputDebugString("detach");
}

void HSynthAudioProcessorEditor::redrawGraph() {
    this->drawGraph = true;
    juce::MessageManager::callAsync(
        [=]() { this->repaint({45, 95, 660, 710}); });
}

void HSynthAudioProcessorEditor::setErrorText(std::string text) {
    this->error.setText(text, juce::NotificationType::sendNotificationAsync);
}

int frame = 0;
void HSynthAudioProcessorEditor::paint(juce::Graphics& g) {
    std::chrono::time_point<std::chrono::system_clock> time =
        std::chrono::system_clock::now();
    std::chrono::duration<float> elapsed_seconds = time - start;
    // auto& ctx = this->audioProcessor.getContext();
    juce::Result result = shader->checkCompilation(g.getInternalContext());
    if (!result.failed()) {
        timeUniform.emplace(*(shader->getProgram(g.getInternalContext())),
                            "time");
        sizeUniform.emplace(*(shader->getProgram(g.getInternalContext())),
                            "sz");

        g.setColour(juce::Colours::black);
        shader->getProgram(g.getInternalContext())->use();
        timeUniform->set(std::fmod(elapsed_seconds.count(), 100.0f));
        sizeUniform->set(this->getWidth(), this->getHeight());
        shader->fillRect(g.getInternalContext(), getLocalBounds());
    }
    g.fillAll(juce::Colours::black.withAlpha(0.5f));

    constexpr float startX = 100, endX = 700, w = endX - startX;
    constexpr float startY = 100, endY = 690, h = endY - startY;
    g.setColour(juce::Colours::cyan.darker(0.90f).withAlpha(0.7f));
    g.fillRect(startX, startY, w, h);

    g.setColour(juce::Colours::yellow);
    g.setFont(juce::FontOptions(15.0f));
    if (drawGraph) {
        drawGraph = false;
        graph = juce::Path();
        constexpr int maxI = 2048;
        auto frame = audioProcessor.getCurrentFrame();

        graph.preallocateSpace(3 * maxI);
        const float start = std::isfinite(frame[0]) ? frame[0] : 0;
        float prevY = (1 - start) * (h - 4) / 2 + startY + 2;
        graph.startNewSubPath(startX, prevY);
        float len = 0;
        for (int i = 1; i < maxI; i++) {
            len +=
                std::abs(frame[i * 2048 / maxI] - frame[(i - 1) * 2048 / maxI]);
            if (len > 200) break;
        }
        for (int i = 1; i < maxI; i++) {
            float x = startX + i * w / maxI,
                  y = (1 - frame[i * 2048 / maxI]) * (h - 4) / 2 + startY + 2;
            if (!std::isfinite(y)) y = (h - 4) / 2 + startY + 2;

            if (len > 100) i += i % 2;
            if (len > 200) i += 2 * (i % 4);
            prevY = y;
            graph.lineTo(x, y);
        }
    }
    // Ignore the fact that it might get slow in debug mode
    g.strokePath(graph, juce::PathStrokeType(2));

    // I should have just drawn manually a big box instead of all this...
    g.setColour(juce::Colours::black.withAlpha(0.4f));
    g.fillRect(this->title.getBounds()
                   .getUnion(this->formula.getBounds())
                   .expanded(10)
                   .withBottom(100));
    g.fillRect(this->volumeLabel.getBounds()
                   .getUnion(this->volumeSlider.getBounds())
                   .withBottom(625));
    g.fillRect(juce::Rectangle<float>{endX, 625.f, 200.f, 65.f});

    // contour graph
    const float t = elapsed_seconds.count();
    juce::Colour col = juce::Colours::silver.withLightness(0.3);
    g.setGradientFill(juce::ColourGradient(
        col, w * std::cos(t) + w / 2, h * std::sin(t) + h / 2,
        col.withLightness(0.9), -std::cos(t) * w + w / 2,
        -std::sin(t) * h + h / 2, false));
    g.drawRect(juce::Rectangle{startX, startY, w, h}, 2);
}

void HSynthAudioProcessorEditor::setErrorTextFromAudioProcessor() {
    setErrorText(audioProcessor.getError());
}

void HSynthAudioProcessorEditor::resized() {
    this->title.setBounds({110, 0, 300, 25});
    this->formula.setBounds({110, 25, 380, 50});
    this->error.setBounds({100, 75, 400, 25});
    this->aKnob.setBounds({500, 0, 100, 100});
    this->bKnob.setBounds({600, 0, 100, 100});
    this->attackKnob.setBounds({700, 0, 100, 100});
    this->decayKnob.setBounds({700, 100, 100, 100});
    this->sustainKnob.setBounds({700, 200, 100, 100});
    this->releaseKnob.setBounds({700, 300, 100, 100});
    this->voicesKnob.setBounds({700, 400, 100, 100});
    this->detuneKnob.setBounds({700, 500, 100, 100});
    this->phaseKnob.setBounds({800, 0, 100, 100});
    this->phaseRandKnob.setBounds({800, 100, 100, 100});
    this->hzShiftKnob.setBounds({800, 200, 100, 100});
    this->stShiftKnob.setBounds({800, 300, 100, 100});
    this->volumeSlider.setBounds({800, 424, 100, 200});
    this->limiterLabel.setBounds({700, 600, 100, 25});
    this->limiterEnabled.setBounds({760, 600, 25, 25});
}

RepaintTimer::RepaintTimer(juce::Component& comp) : toRepaint(comp) {}

RepaintTimer::~RepaintTimer() {}

void RepaintTimer::timerCallback() {
    juce::Rectangle bounds = this->toRepaint.getBounds();
    if (bounds.getWidth() == 0 || bounds.getHeight() == 0) return;
    this->toRepaint.repaint(bounds);
}
