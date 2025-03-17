#include "PluginProcessor.hpp"
#include "PluginEditor.hpp"

HSynthAudioProcessor::HSynthAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(
          BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
              )
#endif
      ,
      hz_shift(new juce::AudioParameterFloat({"hz_shift", 1}, "Frequency shift",
                                             -10000, 10000, 0)),
      a(new juce::AudioParameterFloat(juce::ParameterID("a", 1), "a", 0, 1, 0)),
      b(new juce::AudioParameterFloat(juce::ParameterID("b", 1), "b", 0, 1, 0)),
      attack(new juce::AudioParameterFloat(juce::ParameterID("attack", 1),
                                           "attack", 0, 5, .01f)),
      decay(new juce::AudioParameterFloat(juce::ParameterID("decay", 1),
                                          "decay", 0, 5, .5f)),
      sustain(new juce::AudioParameterFloat(juce::ParameterID("sustain", 1),
                                            "sustain", 0, 1, .75f)),
      release(new juce::AudioParameterFloat(juce::ParameterID("release", 1),
                                            "release", 0, 5, .1f)) {
    addParameter(a);
    addParameter(b);
    addParameter(attack);
    addParameter(decay);
    addParameter(sustain);
    addParameter(release);
    addParameter(hz_shift);

    this->context.setOpenGLVersionRequired(
        juce::OpenGLContext::OpenGLVersion::openGL4_3);
}

HSynthAudioProcessor::~HSynthAudioProcessor() {
    this->context.deactivateCurrentContext();
    delete formulaTree;
}

const juce::String HSynthAudioProcessor::getName() const {
    return JucePlugin_Name;
}

bool HSynthAudioProcessor::acceptsMidi() const {
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool HSynthAudioProcessor::producesMidi() const {
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool HSynthAudioProcessor::isMidiEffect() const {
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double HSynthAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int HSynthAudioProcessor::getNumPrograms() { return 1; }

int HSynthAudioProcessor::getCurrentProgram() { return 0; }

//TODO presets
void HSynthAudioProcessor::setCurrentProgram(int index) { (void)index; }

const juce::String HSynthAudioProcessor::getProgramName(int index) {
    (void)index;
    return {};
}

void HSynthAudioProcessor::changeProgramName(int index,
    const juce::String& newName) {
    (void)index;
    (void)newName;
}

void HSynthAudioProcessor::prepareToPlay(double sampleRate,
    int samplesPerBlock) {
    (void)sampleRate;
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
constexpr float SEMITONE_PITCH = 1.05946309436f;
float getFreq(int midiPitch) {
    float result = 1;
    while (midiPitch < 69 - 6) {
        midiPitch += 12;
        result /= 2.;
    }
    while (midiPitch > 69 + 6) {
        midiPitch -= 12;
        result *= 2;
    }
    return (float)(result * std::pow(SEMITONE_PITCH, midiPitch - 69) * 440);
}

float interpolExp(int samplesSinceStart, int sampleDuration, float start,
                  float end, bool negCurve = true) {
    if (samplesSinceStart > sampleDuration) return end;
    if (!negCurve)
        return start +
               (end - start) *
                   (std::exp(-(float)samplesSinceStart / sampleDuration) - 1) /
                   (std::exp(-1.f) - 1);
    return start +
           (end - start) *
               (std::exp((float)samplesSinceStart / sampleDuration) - 1) /
               (std::exp(1.f) - 1);
}

void HSynthAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                        juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;
    const int outputs = getTotalNumOutputChannels();

    const double samples = buffer.getNumSamples();

    for (const auto& e : midiMessages) {
        if (e.getMessage().isNoteOff()) {
            for (Voice& v : voices) {
                if (!v.velocity || v.timeRelease != INT64_MIN ||
                    v.note != e.getMessage().getNoteNumber())
                    continue;
                v.timeRelease = e.samplePosition;
                break;
            }
        } else if (e.getMessage().isNoteOn()) {
            bool alreadyOn = false;
            for (Voice& v : voices) {
                if (!v.velocity || v.timeRelease != INT64_MIN ||
                    v.note != e.getMessage().getNoteNumber())
                    continue;
                alreadyOn = true;
                break;
            }
            // Next midi message
            if (alreadyOn) continue;
            for (Voice& v : voices) {
                if (v.velocity) continue;
                v.note = (uint8_t)e.getMessage().getNoteNumber();
                v.velocity = e.getMessage().getVelocity();
                v.timeStart = e.samplePosition;
                v.phase = 0;
                v.timeRelease = INT64_MIN;
                break;
            }
        } else if (e.getMessage().isAllNotesOff()) {
            for (Voice& v : voices) {
                if (!v.velocity) continue;
                v.timeRelease = e.samplePosition;
            }
        }
    }

    float* channelData = buffer.getWritePointer(0);
    const int sampleAttack = (int)((*attack) * getSampleRate());
    const int sampleDecay = (int)((*decay) * getSampleRate());
    const int sampleRel = (int)((*release) * getSampleRate());
    const float sus = *sustain;

    const float hzShift = *hz_shift;

    auto& frame = getCurrentFrame();
    for (Voice& v : voices) {
        if (!v.velocity) continue;
        if (v.timeRelease != INT64_MIN && v.timeRelease < -sampleRel) {
            v.velocity = 0;
            v.timeRelease = INT64_MIN;
            continue;
        }
        const float freq = getFreq(v.note) + hzShift;
        const float dt = (float)(freq / this->getSampleRate());
        for (int i = v.timeStart > 0 ? (int)v.timeStart : 0; i < samples; i++) {
            float idx = v.phase * 2047;
            int fl = (int)std::floor(idx);
            float dif = idx - fl;
            float amplitude = sus;
            if (v.timeRelease != INT64_MIN && i > v.timeRelease)
                amplitude = interpolExp((int)(i - v.timeRelease), sampleRel,
                                        v.releaseAmp, 0, false);
            else if (v.timeStart > -sampleAttack)
                amplitude =
                    interpolExp((int)(i - v.timeStart), sampleAttack, 0, 1, true);
            else if (v.timeStart > -sampleAttack - sampleDecay)
                amplitude = interpolExp((int)(i - v.timeStart - sampleAttack),
                                        sampleDecay, 1, sus, true);

            if (i == v.timeRelease) v.releaseAmp = amplitude;

            // TODO implement Decay
            channelData[i] +=
                amplitude * ((1 - dif) * frame[fl] + dif * frame[fl + 1]);
            v.phase += dt;
            if (v.phase > 1) v.phase -= 1;
        }
        v.timeStart -= (int64_t)samples;
        if (v.timeRelease != INT64_MIN) v.timeRelease -= (int64_t)samples;
    }

    constexpr float GLOBAL_VOLUME = .1f;

    buffer.applyGain(GLOBAL_VOLUME);

    for (int channel = 1; channel < outputs; ++channel) {
        auto* d = buffer.getWritePointer(channel);
        std::memcpy(d, channelData, (std::size_t)(sizeof(float) * samples));
    }
}

constexpr char SHADERCODE[] =
    "#version 430\n"
    "layout(local_size_x = 1, local_size_y = 1, local_size_z = "
    "1024) in;\n"
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

    "void main() {"
    "float a = gl_GlobalInvocationID.y / 256.0f;"
    "float b = gl_GlobalInvocationID.x / 256.0f;"
    "float t = gl_GlobalInvocationID.z / "
    "float(gl_NumWorkGroups.z * 1024);"
    "float P = 3.14159265359f;"
    "float T = 2 * P;"
    "float p = t * T;"
    "float e = 2.71828182846f;"
    "data[gl_GlobalInvocationID.z + gl_GlobalInvocationID.y * "
    "gl_NumWorkGroups.z * 1024 + gl_GlobalInvocationID.x * "
    "gl_NumWorkGroups.y * gl_NumWorkGroups.z * 1024] = float(";

void HSynthAudioProcessor::computeBuffer(const std::string& formulaStr) {
    delete formulaTree;
    std::setlocale(LC_NUMERIC, "C");
    errorStr.clear();
    this->formulaTree =
        parse(formulaStr, errorStr);  // optimize(parse(formulaStr, err));

    if (!errorStr.empty() || !formulaTree) {
        return;
    }

    while (!this->context.makeActive() && this->context.isAttached());

    if (formulaStr != this->formula) {
        this->formula = formulaStr;
        // This is horrendous but it must be fast
        std::size_t sz = sizeof(SHADERCODE) + 3 + formulaStr.size();
        char* code = new char[sz];
#pragma warning(disable: 6386)
        std::memcpy(code, SHADERCODE, sizeof(SHADERCODE));
        std::memcpy(code + sizeof(SHADERCODE) - 1, formulaStr.c_str(),
            formulaStr.size());
        std::memcpy(code + sz - 4, ");}", 4);
        std::cout << code << std::endl;
        try {
            this->shader = std::make_unique<ComputeShader>(
                code, std::initializer_list<GLsizeiptr>{256 * 256 * 2048 *
                                                        sizeof(float)});
        } catch (const OpenGLException& e) {
            if (e.getErrorCode() == juce::gl::GL_OUT_OF_MEMORY)
                this->shader = std::make_unique<ComputeShader>(
                    code, std::initializer_list<GLsizeiptr>{128 * 256 * 2048 *
                                                            sizeof(float)});
            else {
                delete[] code;
                std::cerr << e.what() << std::endl;
                return;
            }
        }
        delete[] code;
    }

    if (!shader) return;

    auto& buf = shader->getBuffer(0);
    bool passes = buf.getSize() < 256 * 256 * 2048 * sizeof(float);
    shader->run(passes ? 128 : 256, 256, 2);
    void* ptr = buf.mapToPtr(buf.getSize());
    try {
        OPENGL_ERROR_HANDLE("Error when mapping compute buffer !");
    } catch (const OpenGLException& e) {
        std::cerr << e.what() << std::endl;
        return;
    }
    if (!ptr) return;
    // TODO make that a for loop maybe and maybe make the sizes configurable

    std::memcpy(data, ptr, buf.getSize());
    if (!buf.unMap()) {
        std::cerr << "Memory corruption!\n";
    }
    // this->context.deactivateCurrentContext();
}

bool HSynthAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* HSynthAudioProcessor::createEditor() {
    auto* editor = new HSynthAudioProcessorEditor(*this);
    this->context.attachTo(*(editor));
    this->context.setContinuousRepainting(true);
    this->context.setSwapInterval(60);
    return editor;
}

void HSynthAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    (void)destData;
}

void HSynthAudioProcessor::setStateInformation(const void* state,
    int sizeInBytes) {
    (void)state;
    (void)sizeInBytes;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new HSynthAudioProcessor();
}
