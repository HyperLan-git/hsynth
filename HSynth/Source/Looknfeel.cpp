#include "Looknfeel.hpp"

const juce::Font getFont() {
    juce::FontOptions opt(juce::Typeface::createSystemTypefaceFor(
        BinaryData::VelvelyneRegular_ttf,
        BinaryData::VelvelyneRegular_ttfSize));
    juce::Font ft(opt.withHeight(15.f));
    return ft;
}

Looknfeel::Looknfeel() : ft(getFont()) {
    this->setDefaultSansSerifTypeface(this->ft.getTypefacePtr());
    this->setDefaultSansSerifTypefaceName(this->ft.getTypefaceName());
}

juce::Font Looknfeel::getLabelFont(juce::Label& label) { return ft; }

constexpr int SIDES = 10;

void Looknfeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width,
                                 int height, float sliderPos,
                                 float rotaryStartAngle, float rotaryEndAngle,
                                 juce::Slider& slider) {
    juce::Colour outline =
        slider.findColour(juce::Slider::rotarySliderOutlineColourId);
    juce::Colour fill =
        slider.findColour(juce::Slider::rotarySliderFillColourId);

    juce::Rectangle<float> bounds =
        juce::Rectangle<int>(x, y, width, height).toFloat().reduced(10);

    int radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    float toAngle =
        rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    float lineW = juce::jmin(6.0f, radius * 0.5f);
    float arcRadius = radius - lineW * 0.5f;

    if (slider.isEnabled()) {
        juce::Path valueArc;
        valueArc.addCentredArc(bounds.getCentreX(), bounds.getCentreY(),
                               arcRadius + lineW / 4, arcRadius + lineW / 4,
                               0.0f, rotaryStartAngle, toAngle, true);

        g.setColour(fill.brighter());
        g.strokePath(valueArc,
            juce::PathStrokeType(lineW+4, juce::PathStrokeType::curved,
                juce::PathStrokeType::rounded));
        g.setColour(fill);
        g.strokePath(valueArc,
                     juce::PathStrokeType(lineW, juce::PathStrokeType::curved,
                                          juce::PathStrokeType::rounded));
    }

    juce::Path knob;

    float ang = toAngle - juce::MathConstants<float>::pi / SIDES;
    knob.preallocateSpace(1 + 3 * (SIDES + 1));
    knob.startNewSubPath(bounds.getCentre() +
                         juce::Point<float>(arcRadius * std::cos(ang),
                                            arcRadius * std::sin(ang)));
    for (int i = 0; i < SIDES; i++) {
        float angle = i * juce::MathConstants<float>::twoPi / SIDES + ang;
        juce::Point<float> pos(std::cos(angle) * arcRadius,
                               std::sin(angle) * arcRadius);
        pos += bounds.getCentre();
        knob.lineTo(pos);
    }
    knob.closeSubPath();

    const juce::Colour col = slider.findColour(juce::Slider::thumbColourId);
    g.setGradientFill(juce::ColourGradient(col, width, 0.f, col.brighter(0.7f), 0.f, height, false));
    g.fillPath(knob);
    g.setColour(col.darker());
    g.strokePath(knob, juce::PathStrokeType(1.5f));

    float thumbWidth = lineW / 2;
    arcRadius *= .75;
    juce::Point<float> thumbPoint(
        bounds.getCentreX() +
            arcRadius * std::cos(toAngle - juce::MathConstants<float>::halfPi),
        bounds.getCentreY() +
            arcRadius * std::sin(toAngle - juce::MathConstants<float>::halfPi));

    g.setColour(col.darker().contrasting());
    g.fillEllipse(thumbPoint.x - thumbWidth / 2, thumbPoint.y - thumbWidth / 2,
                  thumbWidth, thumbWidth);
}

void Looknfeel::fillTextEditorBackground(juce::Graphics& g, int width,
                                         int height, juce::TextEditor& editor) {
    juce::Path path;
    path.addRoundedRectangle(0, 0, width, height, cornerSizeX, cornerSizeY);
    if (dynamic_cast<juce::AlertWindow*>(editor.getParentComponent()) !=
        nullptr) {
        g.setColour(editor.findColour(juce::TextEditor::backgroundColourId));

        g.fillPath(path);

        g.setColour(editor.findColour(juce::TextEditor::outlineColourId));
        g.strokePath(path, juce::PathStrokeType(1.0f));
    } else {
        g.setColour(editor.findColour(juce::TextEditor::backgroundColourId));
        g.fillPath(path);
    }
}
void Looknfeel::drawTextEditorOutline(juce::Graphics& g, int width, int height,
                                      juce::TextEditor& editor) {
    if (dynamic_cast<juce::AlertWindow*>(editor.getParentComponent()) !=
            nullptr ||
        !editor.isEnabled())
        return;
    juce::Path path;
    path.addRoundedRectangle(0, 0, width, height, cornerSizeX, cornerSizeY);

    std::chrono::time_point<std::chrono::system_clock> time =
        std::chrono::system_clock::now();
    std::chrono::duration<float> elapsed_seconds = time - start;
    float t = elapsed_seconds.count() / 2;
    if (editor.hasKeyboardFocus(true) && !editor.isReadOnly()) {
        juce::Colour col = editor.findColour(juce::TextEditor::focusedOutlineColourId);
        g.setGradientFill(juce::ColourGradient(col, std::cos(t) / width + width / 2, std::sin(t) / height + height / 2,
            col.withLightness(col.getLightness()*1.5), -std::cos(t) / width + width / 2, -std::sin(t) / height + height / 2, false));
        g.strokePath(path, juce::PathStrokeType(4));
    } else {
        juce::Colour col = editor.findColour(juce::TextEditor::outlineColourId);
        g.setGradientFill(juce::ColourGradient(col, std::cos(t) / width + width / 2, std::sin(t) / height + height / 2,
            col.withLightness(col.getLightness() * 1.2), -std::cos(t) / width + width / 2, -std::sin(t) / height + height / 2, false));
        g.strokePath(path, juce::PathStrokeType(1.5));
    }
}