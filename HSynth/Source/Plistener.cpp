#include "PListener.hpp"

void PListener::parameterValueChanged(int parameterIndex, float newValue) {
    (void)parameterIndex;
    (void)newValue;
    HSynthAudioProcessorEditor* editor = (HSynthAudioProcessorEditor*)proc->getActiveEditor();
    if (editor) editor->redrawGraph();
}