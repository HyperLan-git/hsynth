#include "PluginProcessor.hpp"

#include "PluginEditor.hpp"

HSynthAudioProcessor::HSynthAudioProcessor()
    : AudioProcessor(
          BusesProperties()
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      hzShift(new juce::AudioParameterFloat({"hz_shift", 1}, "Frequency shift",
                                            -10000, 10000, 0)),
      stShift(new juce::AudioParameterFloat({"st_shift", 1}, "Pitch shift", -48,
                                            48, 0)),
      a(new juce::AudioParameterFloat(juce::ParameterID("a", 1), "a", 0, 1, 0)),
      b(new juce::AudioParameterFloat(juce::ParameterID("b", 1), "b", 0, 1, 0)),
      attack(new juce::AudioParameterFloat(juce::ParameterID("attack", 1),
                                           "attack", 0, 2, .01f)),
      decay(new juce::AudioParameterFloat(juce::ParameterID("decay", 1),
                                          "decay", 0, 2, .5f)),
      sustain(new juce::AudioParameterFloat(juce::ParameterID("sustain", 1),
                                            "sustain", 0, 1, .75f)),
      release(new juce::AudioParameterFloat(juce::ParameterID("release", 1),
                                            "release", 0, 2, .1f)),
      voicesPerNote(new juce::AudioParameterInt(juce::ParameterID("voices", 1),
                                                "voices", 1, 16, 1)),
      detune(new juce::AudioParameterFloat(juce::ParameterID("detune", 1),
                                           "detune", 0, 2, .2f)),
      phase(new juce::AudioParameterFloat(juce::ParameterID("phase", 1),
                                          "phase", 0, 1, 0)),
      phaseRandomness(
          new juce::AudioParameterFloat(juce::ParameterID("phaseRandomness", 1),
                                        "phase randomness", 0, 100, 100)),
      limiter(new juce::AudioParameterBool(juce::ParameterID("limiter", 1),
                                           "limiter", true)) {
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
    addParameter(limiter);

    std::memset(data, 0, 256*256*sizeof(WTFrame));

    this->context = std::make_unique<juce::OpenGLContext>();
    this->context->setOpenGLVersionRequired(
        juce::OpenGLContext::OpenGLVersion::openGL4_3);
}

HSynthAudioProcessor::~HSynthAudioProcessor() {
    this->context->deactivateCurrentContext();
    delete formulaTree;
}

const juce::String HSynthAudioProcessor::getName() const {
    return JucePlugin_Name;
}

bool HSynthAudioProcessor::acceptsMidi() const { return true; }

bool HSynthAudioProcessor::producesMidi() const { return false; }

bool HSynthAudioProcessor::isMidiEffect() const {
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double HSynthAudioProcessor::getTailLengthSeconds() const { return 2.0; }

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
    this->prevValidSampleRate = sampleRate;
    (void)samplesPerBlock;
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
    constexpr float denNeg = -0.6321205588285577f; // 1/E - 1
    constexpr float denPos = 1.718281828459045f; // E - 1
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
    const int outputs = getTotalNumOutputChannels();

    const int samples = buffer.getNumSamples();

    const int voicesPerNote = *this->voicesPerNote;
    const float detune = *this->detune;
    const float phase = *this->phase;
    const float phaseRandomness = *this->phaseRandomness;
    const float pitchShift = *this->stShift;

    for (const auto& e : midiMessages) {
        const juce::MidiMessage& msg = e.getMessage();
        if (msg.isNoteOff()) {
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
                    v.note = msg.getNoteNumber();
                    v.velocity = msg.getVelocity();
                    v.timeStart = e.samplePosition;
                    v.detune = voicesPerNote == 1 ? 0 : detuneVal;
                    v.pan = voicesPerNote == 1 ? 0 : panVal;
                    v.voiceDetune = detune;
                    v.voice = i;
                    v.voices = voicesPerNote;
                    v.freq =
                        (getFreq(v.note)) * (1. + v.detune * SEMITONE_PITCH);
                    v.amp = 1. / voicesPerNote;
                    v.phase = std::fmod(
                        phase + (phaseRandomness > 0
                                     ? phaseRandomness * randFloat(randomDev)
                                     : 0),
                        1);
                    v.startingPhase = v.phase;
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
    const int sampleAttack = (*attack) * sampleRate;
    const int sampleDecay = (*decay) * sampleRate;
    const int sampleRel = (*release) * sampleRate;
    const float sus = *sustain;

    const float freqShift = *hzShift;

    if (sampleRate == 0 || freeVoices == MAX_VOICES) {
        buffer.clear(0, samples);
        return;
    }

    auto& frame = getCurrentFrame();
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
                    const float val = amplitude * v.amp * frame[2047];
                    if (std::isfinite(val)) {
                        leftData[i] += leftAmp * val;
                        rightData[i] += rightAmp * val;
                    }
                } else {
                    const float val =
                        amplitude * v.amp *
                        ((1 - dif) * frame[fl] + dif * frame[fl + 1]);
                    if (std::isfinite(val)) {
                        leftData[i] += leftAmp * val;
                        rightData[i] += rightAmp * val;
                    }
                }
                v.phase += dt;
                if (v.phase > 1) v.phase -= 1;
                if (v.phase < 0) v.phase += 1;
            }
            v.timeStart -= (int64_t)samples;
            if (v.timeRelease != INT64_MIN) v.timeRelease -= (int64_t)samples;
        }
    }

    constexpr float GLOBAL_VOLUME = .1f;

    buffer.applyGain(GLOBAL_VOLUME);
}

// XXX Emit directly gpu assembly instructions
constexpr char SHADERCODE[] =
    "#version 430\n"
    "layout(local_size_x = 1, local_size_y = 1, local_size_z = "
    "512) in;\n"
    "layout(std430, binding = 1) buffer layoutName"
    "{"
    "float data[];"
    "};\n"
    "float arctan(float x) {return atan(x, 1);}\n"
    // Thx Photosounder
    "float erf(float x) {"
    "   float y, x2 = x*x;"
    "   y = sqrt(1.0f - exp(-x2)/(sqrt(x2+3.1220878f)-0.766943f));"
    "   return x < 0.0 ? -y : y;"
    "}\n"

    "float mod(float a, float b) { return a - b * floor(a / b); }"

    "void main() {"
    "float a = gl_GlobalInvocationID.y / 255.0f;"
    "float b = gl_GlobalInvocationID.x / 255.0f;"
    "float t = gl_GlobalInvocationID.z / "
    "float(gl_NumWorkGroups.z * 512);"
    "float P = 3.14159265359f;"
    "float T = 2 * P;"
    "float p = t * T;"
    "float e = 2.71828182846f;"
    "data[gl_GlobalInvocationID.z + gl_GlobalInvocationID.y * "
    "gl_NumWorkGroups.z * 512 + gl_GlobalInvocationID.x * "
    "gl_NumWorkGroups.y * gl_NumWorkGroups.z * 512] = float(";

void HSynthAudioProcessor::resetShader() {
    if (formula.empty()) {
        shader.reset();
        return;
    }

    // This is horrendous but it must be fast
    std::size_t sz = sizeof(SHADERCODE) + 3 + this->formula.size();
    char* code = new char[sz];
#pragma warning(disable : 6386)
    std::memcpy(code, SHADERCODE, sizeof(SHADERCODE));
    std::memcpy(code + sizeof(SHADERCODE) - 1, this->formula.c_str(),
        this->formula.size());
    std::memcpy(code + sz - 4, ");}", 4);
    std::cout << code << "\n";
    try {
        this->shader = std::make_unique<ComputeShader>(
            code, std::initializer_list<GLsizeiptr>{256 * 256 * 2048 *
            sizeof(float)});
    }
    catch (const OpenGLException& e) {
        if (e.getErrorCode() == juce::gl::GL_OUT_OF_MEMORY) try {
            this->shader = std::make_unique<ComputeShader>(
                code, std::initializer_list<GLsizeiptr>{
                128 * 256 * 2048 * sizeof(float)});
        }
        catch (const std::runtime_error& e) {
            this->shader.reset();
            delete[] code;
            std::cerr << e.what() << std::endl;
            errorStr = e.what();
            return;
        }
        else {
            this->shader.reset();
            delete[] code;
            std::cerr << e.what() << std::endl;
            errorStr = e.what();
            return;
        }
    }
    catch (const std::runtime_error& e) {
        this->shader.reset();
        delete[] code;
        std::cerr << e.what() << std::endl;
        errorStr = e.what();
        return;
    }
    delete[] code;
}

void HSynthAudioProcessor::computeBuffer(const std::string& formulaStr) {
    delete formulaTree;
    std::setlocale(LC_NUMERIC, "C");
    errorStr.clear();
    this->formulaTree =
        parse(formulaStr, errorStr);  // optimize(parse(formulaStr, err));

    if (!errorStr.empty() || !formulaTree) {
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
        std::cout << newFormula << "\n";
        this->formula = newFormula;
        resetShader();
    }

    if (!shader) return;

    void* ptr = NULL;
    GLBuffer &buf = shader->getBuffer(0);
    try {
        bool passes = buf.getSize() < 256 * 256 * 2048 * sizeof(float);
        shader->run(passes ? 128 : 256, 256, 4);
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
    }
}

bool HSynthAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* HSynthAudioProcessor::createEditor() {
    auto* editor = new HSynthAudioProcessorEditor(*this);
    this->context->detach();
    this->context = std::make_unique<juce::OpenGLContext>();
    this->context->setOpenGLVersionRequired(
        juce::OpenGLContext::OpenGLVersion::openGL4_3);
    this->context->attachTo(*(editor));
    this->context->setContinuousRepainting(true);
    this->context->setSwapInterval(30);
    this->resetShader();
    return editor;
}

void HSynthAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    juce::MemoryOutputStream stream(destData, true);
    stream.writeInt64(this->formula.length());
    stream.write(this->formula.c_str(), this->formula.length() + 1);
}

bool formulaValid(const std::string& str)
{
    return find_if(str.begin(), str.end(),
        [](char c) { return !std::isprint(c); }) == str.end();
}

//TODO add checksum
void HSynthAudioProcessor::setStateInformation(const void* state,
                                               int sizeInBytes) {
    juce::MemoryInputStream stream(state, static_cast<size_t>(sizeInBytes),
                                   false);
    std::size_t len = stream.readInt64();
    if (len <= 0) return;
    char* str = new char[len];
    stream.read(str, len);
    str[len - 1] = 0;
    std::string formula(str);
    if(formulaValid(formula))
        this->computeBuffer(formula);
    delete[] str;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new HSynthAudioProcessor();
}
