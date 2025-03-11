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
      b(new juce::AudioParameterFloat(juce::ParameterID("b", 1), "b", 0, 1,
                                      0)) {
    addParameter(a);
    addParameter(b);
    addParameter(hz_shift);

    this->context.setOpenGLVersionRequired(
        juce::OpenGLContext::OpenGLVersion::openGL4_3);
}

HSynthAudioProcessor::~HSynthAudioProcessor() {
    this->context.detach();
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

void HSynthAudioProcessor::setCurrentProgram(int index) {}

const juce::String HSynthAudioProcessor::getProgramName(int index) {
    return {};
}

void HSynthAudioProcessor::changeProgramName(int index,
                                             const juce::String& newName) {}

void HSynthAudioProcessor::prepareToPlay(double sampleRate,
                                         int samplesPerBlock) {}

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

void HSynthAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                        juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;
    int inputs = getTotalNumInputChannels();
    int outputs = getTotalNumOutputChannels();

    for (int channel = 0; channel < outputs; ++channel) {
        auto* channelData = buffer.getWritePointer(channel);
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

void HSynthAudioProcessor::computeBuffer(const std::string& formula) {
    delete formulaTree;
    std::setlocale(LC_NUMERIC, "C");
    this->formulaTree =
        parse(formula, error);  // optimize(parse(formula, err));

    if (!error.empty() || !formulaTree) {
        return;
    }

    while (!this->context.makeActive());

    if (formula != this->formula) {
        this->formula = formula;
        // This is horrendous but it must be fast
        std::size_t sz = sizeof(SHADERCODE) + 3 + formula.size();
        char* code = new char[sz];
        std::memcpy(code, SHADERCODE, sizeof(SHADERCODE));
        std::memcpy(code + sizeof(SHADERCODE) - 1, formula.c_str(),
                    formula.size());
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
    return editor;
}

void HSynthAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {}

void HSynthAudioProcessor::setStateInformation(const void* data,
                                               int sizeInBytes) {}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new HSynthAudioProcessor();
}
