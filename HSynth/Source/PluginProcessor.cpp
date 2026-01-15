#include "PluginProcessor.hpp"

#include "PluginEditor.hpp"

HSynthAudioProcessor::HSynthAudioProcessor()
    : AudioProcessor(
          BusesProperties()
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      hzShift(new juce::AudioParameterFloat(
          {"hz_shift", 1}, "Frequency shift",
          juce::NormalisableRange<float>(-300, 300, 1), 0)),
      stShift(new juce::AudioParameterFloat(
          {"st_shift", 1}, "Pitch shift",
          juce::NormalisableRange<float>(-24, 24, .01f), 0)),
      a(new juce::AudioParameterFloat(
          juce::ParameterID("a", 1), "a",
          juce::NormalisableRange<float>(0, 1, 0.005f), 0)),
      b(new juce::AudioParameterFloat(
          juce::ParameterID("b", 1), "b",
          juce::NormalisableRange<float>(0, 1, .005f), 0)),
      attack(new juce::AudioParameterFloat(
          juce::ParameterID("attack", 1), "attack",
          juce::NormalisableRange<float>(0, 2000, 1), 1)),
      decay(new juce::AudioParameterFloat(
          juce::ParameterID("decay", 1), "decay",
          juce::NormalisableRange<float>(0, 2000, 1), 50)),
      sustain(new juce::AudioParameterFloat(
          juce::ParameterID("sustain", 1), "sustain",
          juce::NormalisableRange<float>(0, 1, .05f), .75f)),
      release(new juce::AudioParameterFloat(
          juce::ParameterID("release", 1), "release",
          juce::NormalisableRange<float>(0, 2000, 1), 10)),
      voicesPerNote(new juce::AudioParameterInt(juce::ParameterID("voices", 1),
                                                "voices", 1, 16, 1)),
      detune(new juce::AudioParameterFloat(
          juce::ParameterID("detune", 1), "detune",
          juce::NormalisableRange<float>(0, 2, .01f), .2f)),
      phase(new juce::AudioParameterFloat(juce::ParameterID("phase", 1),
                                          "phase", 0, 1, 0)),
      phaseRandomness(new juce::AudioParameterFloat(
          juce::ParameterID("phaseRandomness", 1), "p. random",
          juce::NormalisableRange<float>(0, 100, 1), 100)),
      volume(new juce::AudioParameterFloat(
          juce::ParameterID("volume", 1), "volume",
          juce::NormalisableRange<float>(-60, 0, .1f), -20)),
      limiter(new juce::AudioParameterBool(juce::ParameterID("limiter", 1),
                                           "limiter", true)),
      editorListener(new PListener(
          std::initializer_list<juce::RangedAudioParameter*>{a, b}, this)) {
    addParameter(a);
    addParameter(b);
    addParameter(attack);
    addParameter(decay);
    addParameter(sustain);
    addParameter(release);
    addParameter(hzShift);
    addParameter(stShift);
    addParameter(voicesPerNote);
    addParameter(detune);
    addParameter(phase);
    addParameter(phaseRandomness);
    addParameter(volume);
    addParameter(limiter);

    std::memset(data, 0, 256 * 256 * sizeof(WTFrame));

    this->context = std::make_unique<juce::OpenGLContext>();
    this->context->setOpenGLVersionRequired(
        juce::OpenGLContext::OpenGLVersion::openGL4_3);
}

HSynthAudioProcessor::~HSynthAudioProcessor() {
    this->context->detach();
    this->context->deactivateCurrentContext();
    delete formulaTree;
    delete editorListener;
    // delete[] data;
}

const juce::String HSynthAudioProcessor::getName() const {
    return JucePlugin_Name;
}

bool HSynthAudioProcessor::acceptsMidi() const { return true; }

bool HSynthAudioProcessor::producesMidi() const { return false; }

bool HSynthAudioProcessor::isMidiEffect() const { return false; }

double HSynthAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int HSynthAudioProcessor::getNumPrograms() { return 1; }

int HSynthAudioProcessor::getCurrentProgram() { return 0; }

// TODO presets
void HSynthAudioProcessor::setCurrentProgram(int index) { (void)index; }

const juce::String HSynthAudioProcessor::getProgramName(int index) {
    (void)index;
    return "None";
}

void HSynthAudioProcessor::changeProgramName(int index,
                                             const juce::String& newName) {
    (void)index;
    (void)newName;
}

void HSynthAudioProcessor::prepareToPlay(double sampleRate,
                                         int samplesPerBlock) {
    (void)samplesPerBlock;
    this->prevValidSampleRate = sampleRate;
    this->prevA = *a;
    this->prevB = *b;
}

void HSynthAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool HSynthAudioProcessor::isBusesLayoutSupported(
    const BusesLayout& layouts) const {
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else

    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
        layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

#if !JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}
#endif

// 2^(1/12)
constexpr double SEMITONE_PITCH = 1.0594630943592953;
// TODO memoize and turn to doubles
double getFreq(int midiPitch) {
    double result = 1;
    while (midiPitch < 69 - 6) {
        midiPitch += 12;
        result /= 2.;
    }
    while (midiPitch > 69 + 6) {
        midiPitch -= 12;
        result *= 2;
    }
    return (double)(result * std::pow(SEMITONE_PITCH, midiPitch - 69) * 440);
}

constexpr float interpolExp(int samplesSinceStart, int sampleDuration,
                            float start, float end, bool negCurve = true) {
    if (samplesSinceStart > sampleDuration) return end;
    constexpr float denNeg = -0.6321205588285577f;  // 1/E - 1
    constexpr float denPos = 1.718281828459045f;    // E - 1
    if (!negCurve)
        return start +
               (end - start) *
                   (std::exp(-(float)samplesSinceStart / sampleDuration) - 1) /
                   (denNeg);
    return start +
           (end - start) *
               (std::exp((float)samplesSinceStart / sampleDuration) - 1) /
               (denPos);
}

float sigmoid(float x) { return 1 / (1.f + std::exp(-x)); }
void HSynthAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                        juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;
    if (!data) return;
    const int outputs = getTotalNumOutputChannels();

    const int samples = buffer.getNumSamples();

    const int voicesPerNote = *this->voicesPerNote;
    const float detune = *this->detune;
    const float phase = *this->phase;
    const float phaseRandomness = *this->phaseRandomness;
    const float pitchShift = *this->stShift;

    for (const auto& e : midiMessages) {
        const juce::MidiMessage& msg = e.getMessage();
        if (msg.isController()) {
            int controller = msg.getControllerNumber();
            if (controller % 2) {
                this->a->setValueNotifyingHost(msg.getControllerValue() /
                                               127.f);
            } else {
                this->b->setValueNotifyingHost(msg.getControllerValue() /
                                               127.f);
            }
        } else if (msg.isNoteOff()) {
            for (Voice& v : voices) {
                if (!v.velocity || v.timeRelease != INT64_MIN ||
                    v.note != msg.getNoteNumber())
                    continue;
                v.timeRelease = e.samplePosition;
            }
        } else if (msg.isNoteOn()) {
            bool alreadyOn = false;
            if (freeVoices < voicesPerNote) continue;
            for (Voice& v : voices) {
                if (!v.velocity || v.timeRelease != INT64_MIN ||
                    v.note != msg.getNoteNumber())
                    continue;
                alreadyOn = true;
                break;
            }
            // Next midi message
            if (alreadyOn) continue;
            for (int i = 0; i < voicesPerNote; i++) {
                const float detuneVal =
                    ((i - voicesPerNote / 2.f) * 2 * detune / voicesPerNote /
                     voicesPerNote);
                const float panVal = (i % 2 == 0) ? -1.f * i / voicesPerNote
                                                  : 1.f * i / voicesPerNote;
                for (Voice& v : voices) {
                    if (v.velocity) continue;
                    v.note = (uint8_t)msg.getNoteNumber();
                    v.velocity = msg.getVelocity();
                    v.timeStart = e.samplePosition;
                    v.detune = voicesPerNote == 1 ? 0 : detuneVal;
                    v.pan = voicesPerNote == 1 ? 0 : panVal;
                    v.voiceDetune = detune;
                    v.voice = (uint8_t)i;
                    v.voices = (uint8_t)voicesPerNote;
                    v.freq =
                        (getFreq(v.note)) * (1. + v.detune * SEMITONE_PITCH);
                    v.amp = 1.f / voicesPerNote;
                    v.phase = std::fmod(
                        phase + (phaseRandomness > 0
                                     ? phaseRandomness * randFloat(randomDev)
                                     : 0),
                        1);
                    v.startingPhase = v.phase;
                    // This value indicates not released yet
                    v.timeRelease = INT64_MIN;
                    freeVoices--;
                    break;
                }
            }
        } else if (msg.isAllNotesOff()) {
            for (Voice& v : voices) {
                if (!v.velocity) continue;
                v.timeRelease = e.samplePosition;
            }
        }
    }

    if (prevPhase != phase || prevDetune != detune) {
        for (Voice& v : voices) {
            if (!v.velocity) continue;

            if (v.voices > 1) {
                const float detuneVal = ((v.voice - v.voices / 2.f) * 2 *
                                         detune / v.voices / v.voices);
                v.freq = (getFreq(v.note)) * (1. + detuneVal * SEMITONE_PITCH);
                v.detune = detuneVal;
            }
            v.phase += phase - prevPhase;
            if (v.phase > 1) v.phase -= 1;
            if (v.phase < 0) v.phase += 1;
        }
    }

    prevPhase = phase;
    prevDetune = detune;
    double sampleRate = this->getSampleRate();
    if (sampleRate == 0) sampleRate = prevValidSampleRate;
    const int sampleAttack = (*attack) / 1000 * sampleRate;
    const int sampleDecay = (*decay) / 1000 * sampleRate;
    const int sampleRel = (*release) / 1000 * sampleRate;
    const float sus = *sustain;

    const float freqShift = *hzShift;

    if (sampleRate == 0 || freeVoices == MAX_VOICES || outputs < 2) {
        buffer.clear(0, samples);
        return;
    }

    const float aVal = a->get(), bVal = b->get();
    const float freqShiftMul = std::pow(SEMITONE_PITCH, pitchShift);
    const bool doLimit = *limiter;
    {
        float* leftData = buffer.getWritePointer(0);
        float* rightData = buffer.getWritePointer(1);
        for (Voice& v : voices) {
            if (!v.velocity) continue;
            if (v.timeRelease != INT64_MIN && v.timeRelease < -sampleRel) {
                v.velocity = 0;
                v.timeRelease = INT64_MIN;
                freeVoices++;
                continue;
            }
            const double freq = v.freq + freqShift;
            const double dt = freqShiftMul * freq / sampleRate;
            const float leftAmp =
                (std::cos(v.pan * M_PI / 2) + std::sin(v.pan * M_PI / 2)) *
                std::sqrt(2) / 2;
            const float rightAmp =
                (std::cos(v.pan * M_PI / 2) - std::sin(v.pan * M_PI / 2)) *
                std::sqrt(2) / 2;
            for (int i = v.timeStart > 0 ? v.timeStart : 0; i < samples; i++) {
                const float idx = v.phase * 2047;
                const unsigned int fl = (unsigned int)std::floor(idx);
                const float dif = idx - fl;
                const float curA = lerp(prevA, aVal, (float)i / samples),
                            curB = lerp(prevB, bVal, (float)i / samples);
                float amplitude = sus;
                if (v.timeRelease != INT64_MIN && i > v.timeRelease)
                    amplitude = interpolExp((int)(i - v.timeRelease), sampleRel,
                                            v.releaseAmp, 0, false);
                else if (v.timeStart > -sampleAttack)
                    amplitude = interpolExp((int)(i - v.timeStart),
                                            sampleAttack, 0, 1, true);
                else if (v.timeStart > -sampleAttack - sampleDecay)
                    amplitude =
                        interpolExp((int)(i - v.timeStart - sampleAttack),
                                    sampleDecay, 1, sus, true);

                if (i == v.timeRelease) v.releaseAmp = amplitude;

                if (fl >= 2047) {
                    const float val =
                        amplitude * v.amp * getFrameSample(curA, curB, 2047);
                    if (std::isfinite(val)) {
                        leftData[i] += leftAmp * val;
                        rightData[i] += rightAmp * val;
                    }
                } else {
                    const float val =
                        amplitude * v.amp *
                        ((1 - dif) * getFrameSample(curA, curB, fl) +
                         dif * getFrameSample(curA, curB, fl + 1));
                    if (std::isfinite(val)) {
                        leftData[i] += leftAmp * val;
                        rightData[i] += rightAmp * val;
                    }
                }
                v.phase += dt;
                if (v.phase > 1)
                    v.phase -= 1;
                else if (v.phase < 0)
                    v.phase += 1;
            }
            v.timeStart -= (int64_t)samples;
            if (v.timeRelease != INT64_MIN) v.timeRelease -= (int64_t)samples;
        }
    }
    const float vol = juce::Decibels::decibelsToGain((float)(*volume));

    buffer.applyGain(0, 0, samples, vol);
    buffer.applyGain(1, 0, samples, vol);

    prevA = aVal;
    prevB = bVal;
}

// XXX Emit directly gpu assembly instructions
std::string buildShader(int workGroupsZ, std::string& formula) {
    std::ostringstream str;
    str << "#version 430\n"
           "layout(local_size_x = 1, local_size_y = 1, local_size_z = "
        << workGroupsZ
        << ") in;\n"
           "layout(std430, binding = 1) buffer layoutName"
           "{"
           "float data[];"
           "};\n"
           "float arctan(float x) {return atan(x, 1);}\n"
           "float sinc(float x) {if(x == 0) return 1; else return sin(x)/x;}\n"
           // Thx Photosounder
           "float erf(float x) {"
           "   float y, x2 = x*x;"
           "   y = sqrt(1.0f - exp(-x2)/(sqrt(x2+3.1220878f)-0.766943f));"
           "   return x < 0.0 ? -y : y;"
           "}\n"
           // https://en.wikipedia.org/wiki/Lanczos_approximation
           "float gamma(float z) {"
           "   float P = 3.14159265359f;"
           "   const float[] p = "
           "float[](0.99999999999980993,676.5203681218851,-1259.1392167224028,"
           "     "
           "771.32342877765313,-176.61502916214059,12.507343278686905,-0."
           "13857109526572012,"
           "     9.9843695780195716e-6,1.5056327351493116e-7);"
           "   if(z < 0.5) {"
           // return pi/(sin(P*z)*gamma(1.0-z));
           "     z = 1.0 - z;"
           "     z -= 1.0;"
           "     float x = p[0];"
           "     for(int i = 1; i < 9; i++) {"
           "        x += p[i] / (z+i);"
           "     }"
           "     float t = z + 7.5;"
           "     float r = sqrt(2 * P) * pow(t, z + 0.5) * exp(-t) * x;"
           "     return P / (sin(P*(2.0-z))*r);"
           "   } else {"
           "     z -= 1.0;"
           "     float x = p[0];"
           "     for(int i = 1; i < 9; i++) {"
           "        x += p[i] / (z+i);"
           "     }"
           "     float t = z + 7.5;"
           "     return sqrt(2 * P) * pow(t, z + 0.5) * exp(-t) * x;"
           "   }"
           "}"

           "float mod(float a, float b) { return a - b * floor(a / b); }\n"

           "void main() {"
           "float a = gl_GlobalInvocationID.y / 255.0f;"
           "float b = gl_GlobalInvocationID.x / 255.0f;"
           "float t = gl_GlobalInvocationID.z / "
           "float(gl_NumWorkGroups.z * "
        << workGroupsZ
        << ");"
           "float P = 3.14159265359f;"
           "float T = 2 * P;"
           "float p = t * T;"
           "float e = 2.71828182846f;"
           "data[gl_GlobalInvocationID.z + gl_GlobalInvocationID.y * "
           "gl_NumWorkGroups.z * "
        << workGroupsZ
        << " + gl_GlobalInvocationID.x * "
           "gl_NumWorkGroups.y * gl_NumWorkGroups.z * "
        << workGroupsZ << "] = float(" << formula << ");}";
    return str.str();
}

void HSynthAudioProcessor::resetShader() {
    if (formula.empty() || this->getActiveEditor() == nullptr ||
        !this->context) {
        shader.reset();
        return;
    }

    int workGroups = 2048 >> (execsZ - 1);
    if (execsZ == 0) {
        workGroups = 2048;
        const int maxWG = getShaderMaxWorkGroups();
        this->execsZ = 1;
        while (workGroups > maxWG) {
            workGroups /= 2;
            execsZ *= 2;
        }
    }

    std::string code = buildShader(workGroups, this->formula);
    juce::Logger::writeToLog("code : " + code + "\n");
    try {
        this->shader = std::make_unique<ComputeShader>(
            code.c_str(), std::initializer_list<GLsizeiptr>{256 * 256 * 2048 *
                                                            sizeof(float)});
    } catch (const OpenGLException& e) {
        this->shader.reset();
        std::cerr << e.what() << std::endl;
        errorStr = e.what();
        return;
    } catch (const std::runtime_error& e) {
        this->shader.reset();
        std::cerr << e.what() << std::endl;
        errorStr = e.what();
        return;
    }
}

void HSynthAudioProcessor::computeBuffer(const std::string& formulaStr) {
    delete formulaTree;
    std::setlocale(LC_NUMERIC, "C");
    errorStr.clear();
    this->formulaTree =
        parse(formulaStr, errorStr);  // optimize(parse(formulaStr, err));

    if (!errorStr.empty() || !formulaTree || !this->context) {
        return;
    }

    std::ostringstream str;
    this->formulaTree->printGLSL(str);
    std::string newFormula = str.str();

    int i = 0;

    juce::Logger::writeToLog("Getting context");
    while (!this->context->makeActive() && this->context->isAttached()) {
        std::this_thread::yield();
        if (++i > 500) {
            shader.reset();
            return;
        }
    }
    juce::Logger::writeToLog("Context got");

    if (newFormula != this->formula) {
        juce::Logger::writeToLog("formula : " + newFormula + "\n");
        this->formula = newFormula;
        resetShader();
    }

    if (!shader) return;

    void* ptr = NULL;
    GLBuffer& buf = shader->getBuffer(0);
    try {
        bool passes = buf.getSize() < 256 * 256 * 2048 * sizeof(float);
        shader->run(passes ? 128 : 256, 256, execsZ);
        juce::Logger::writeToLog("Mapping buffer");
        ptr = buf.mapToPtr(buf.getSize());
        OPENGL_ERROR_HANDLE("Error when mapping compute buffer !");
    } catch (const OpenGLException& e) {
        juce::Logger::outputDebugString(e.what());
        return;
    }
    if (!ptr) return;
    // TODO make that a for loop maybe and maybe make the sizes configurable

    std::memcpy(data, ptr, buf.getSize());
    juce::Logger::writeToLog("Unmapping buffer");
    if (!buf.unMap()) {
        juce::Logger::outputDebugString("Memory corruption!\n");
    }
    juce::Logger::writeToLog("Unmapped buffer");
    // TODO normalize every frame
    if (*limiter) {
        float* dat = (float*)data;
        for (int i = 0; i < 256 * 256 * 2048; i++) {
            if (dat[i] > 1) dat[i] = 1;
            if (dat[i] < -1) dat[i] = -1;
        }
        for (int i = 0; i < 256 * 256; i++) {
            float max = 0;
            for (int sample = 0; sample < 2048; sample++) {
                float abs = std::abs(dat[i * 2048 + sample]);
                if (abs > max) max = abs;
            }

            if (max < 1 && max > 0) {
                float inv = 1 / max;
                for (int sample = 0; sample < 2048; sample++) {
                    dat[i * 2048 + sample] *= inv;
                }
            }
        }
    }
}

bool HSynthAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* HSynthAudioProcessor::createEditor() {
    delete this->editorListener;
    auto* editor = new HSynthAudioProcessorEditor(*this);
    // Starting to hate juce just for that... (deadlock caused by listener added
    // while another is firing, this is absolute bollocks)
    this->editorListener = new PListener(
        std::initializer_list<juce::RangedAudioParameter*>{a, b}, this);
    this->context->detach();
    this->context = std::make_unique<juce::OpenGLContext>();
    this->context->setOpenGLVersionRequired(
        juce::OpenGLContext::OpenGLVersion::openGL4_3);
    this->context->attachTo(*(editor));
    this->context->setContinuousRepainting(true);
    this->context->setSwapInterval(30);
    editor->setFormula(this->formula);
    std::string f(this->formula);
    this->formula = "";
    juce::MessageManager::callAsync([=] {
        this->context->executeOnGLThread(
            [=](juce::OpenGLContext& c) {
                while (!c.isAttached()) std::this_thread::yield();
                this->resetShader();
                this->computeBuffer(f);
                this->formula = f;
            },
            false);
    });
    return editor;
}

#define GET_PARAM_NORMALIZED(param) (param->convertTo0to1(*param))
#define SET_PARAM_NORMALIZED(param, value) \
    param->setValueNotifyingHost(param->convertTo0to1(value))

void HSynthAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    juce::MemoryOutputStream stream(destData, true);
    stream.writeInt(0x6767FACE);  // Magic number
    stream.writeInt(0);           // Version
    stream.writeString(this->formula);
    stream.writeFloat(GET_PARAM_NORMALIZED(this->a));
    stream.writeFloat(GET_PARAM_NORMALIZED(this->b));
    stream.writeFloat(GET_PARAM_NORMALIZED(this->attack));
    stream.writeFloat(GET_PARAM_NORMALIZED(this->decay));
    stream.writeFloat(GET_PARAM_NORMALIZED(this->sustain));
    stream.writeFloat(GET_PARAM_NORMALIZED(this->release));
    stream.writeFloat(GET_PARAM_NORMALIZED(this->volume));
    stream.writeFloat(GET_PARAM_NORMALIZED(this->hzShift));
    stream.writeFloat(GET_PARAM_NORMALIZED(this->stShift));
    stream.writeFloat(GET_PARAM_NORMALIZED(this->phase));
    stream.writeFloat(GET_PARAM_NORMALIZED(this->phaseRandomness));
    stream.writeFloat(GET_PARAM_NORMALIZED(this->volume));
    stream.writeInt(*this->voicesPerNote);
    stream.writeFloat(GET_PARAM_NORMALIZED(this->detune));
    stream.writeBool(*this->limiter);
}

bool formulaValid(const std::string& str) {
    return find_if(str.begin(), str.end(),
                   [](char c) { return !std::isprint(c); }) == str.end();
}

void HSynthAudioProcessor::setStateInformation(const void* state,
                                               int sizeInBytes) {
    juce::MemoryInputStream stream(state, static_cast<size_t>(sizeInBytes),
                                   false);
    int magic = stream.readInt();
    if (magic != 0x6767FACE) return;
    int ver = stream.readInt();
    if (ver != 0) return;
    std::string formula(stream.readString().toStdString());
    if (formulaValid(formula)) {
        if (this->getActiveEditor() != nullptr)
            dynamic_cast<HSynthAudioProcessorEditor*>(this->getActiveEditor())
                ->setFormula(formula);
        this->formula = formula;
    }
    this->a->setValueNotifyingHost(stream.readFloat());
    this->b->setValueNotifyingHost(stream.readFloat());
    this->attack->setValueNotifyingHost(stream.readFloat());
    this->decay->setValueNotifyingHost(stream.readFloat());
    this->sustain->setValueNotifyingHost(stream.readFloat());
    this->release->setValueNotifyingHost(stream.readFloat());
    this->volume->setValueNotifyingHost(stream.readFloat());
    this->hzShift->setValueNotifyingHost(stream.readFloat());
    this->stShift->setValueNotifyingHost(stream.readFloat());
    this->phase->setValueNotifyingHost(stream.readFloat());
    this->phaseRandomness->setValueNotifyingHost(stream.readFloat());
    this->volume->setValueNotifyingHost(stream.readFloat());
    SET_PARAM_NORMALIZED(this->voicesPerNote, stream.readInt());
    this->detune->setValueNotifyingHost(stream.readFloat());
    this->limiter->setValueNotifyingHost(stream.readBool());
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new HSynthAudioProcessor();
}
