#include "HyperWaveTable.hpp"

HyperWaveTable::HyperWaveTable(std::size_t parameters)
    : content((float*)new float[pow256(parameters) * WAVETABLE_TIME_SAMPLES]),
      ready(new bool[pow256(parameters)]),
      params(parameters),
      f() {
    memset(ready, false, sizeof(bool) * pow256(parameters));
}

HyperWaveTable::~HyperWaveTable() {
    delete[] content;
    delete[] ready;
}

float computeSample(double t, const struct HyperToken& tok, float* params,
                    std::size_t nParams);

float computeFunction(double t, const struct HyperToken& tok, float* params,
                      std::size_t nParams) {
    float arg1 = computeSample(t, *(tok.a), params, nParams);
    switch (tok.func) {
        case SIN:
            return std::sin(arg1);
        case COS:
            return std::cos(arg1);
        case TAN:
            return std::tan(arg1);
        case ASIN:
            return std::asin(arg1);
        case ACOS:
            return std::acos(arg1);
        case ATAN:
            return std::atan(arg1);
        case LN:
            return std::log(arg1);
        case EXP:
            return std::exp(arg1);
        case SQRT:
            return std::sqrt(arg1);
        case COSH:
            return std::cosh(arg1);
        case SINH:
            return std::sinh(arg1);
        case TANH:
            return std::tanh(arg1);
        case ABS:
            return arg1 < 0 ? -arg1 : arg1;
        case SIGN:
            return arg1 < 0 ? -1.f : (arg1 > 0 ? 1.f : 0.f);
        case ERF:
            return std::erf(arg1);
        case MAX:
            return std::max<float>(arg1,
                                   computeSample(t, *(tok.b), params, nParams));
        case MIN:
            return std::min<float>(arg1,
                                   computeSample(t, *(tok.b), params, nParams));
        case GAMMA:
            return std::tgamma(arg1);
        case ROUND:
            return std::round(arg1);
        case CEIL:
            return std::ceil(arg1);
        case FLOOR:
            return std::floor(arg1);
        default:
            return 0;
    }
}

// exp(1)
constexpr float E = 0.577215664901532860606512f;

float computeVariable(double t, const struct HyperToken& tok, float* params,
                      std::size_t nParams) {
    switch (tok.var) {
        case 'e':
            return E;
        case 'P':
            return M_PI;
        case 'T':
            return M_PI * 2;
        case 't':
            return t;
        case 'p':
            return t * M_PI * 2;
        case 'a':
            return params[0];
        case 'b':
            return params[1];
        default:
            return 0;
    }
}

float computeSample(double t, const struct HyperToken& tok, float* params,
                    std::size_t nParams) {
    switch (tok.type) {
        case HyperToken::Type::FUNCTION:
            return computeFunction(t, tok, params, nParams);
        case HyperToken::Type::VARIABLE:
            return computeVariable(t, tok, params, nParams);
        case HyperToken::Type::NUMBER:
            return tok.number;
        case HyperToken::Type::PARENTHESIS:
            return computeSample(t, *(tok.a), params, nParams);
        case HyperToken::Type::EQUAL:
            return computeSample(t, *(tok.a), params, nParams) ==
                   computeSample(t, *(tok.b), params, nParams);
        case HyperToken::Type::GREATER:
            return computeSample(t, *(tok.a), params, nParams) >
                   computeSample(t, *(tok.b), params, nParams);
        case HyperToken::Type::LESS:
            return computeSample(t, *(tok.a), params, nParams) <
                   computeSample(t, *(tok.b), params, nParams);
        case HyperToken::Type::GREATER_OR_EQUAL:
            return computeSample(t, *(tok.a), params, nParams) >=
                   computeSample(t, *(tok.b), params, nParams);
        case HyperToken::Type::LESS_OR_EQUAL:
            return computeSample(t, *(tok.a), params, nParams) <=
                   computeSample(t, *(tok.b), params, nParams);
        case HyperToken::Type::SUB:
            return computeSample(t, *(tok.a), params, nParams) -
                   computeSample(t, *(tok.b), params, nParams);
        case HyperToken::Type::ADD:
            return computeSample(t, *(tok.a), params, nParams) +
                   computeSample(t, *(tok.b), params, nParams);
        case HyperToken::Type::MUL:
            return computeSample(t, *(tok.a), params, nParams) *
                   computeSample(t, *(tok.b), params, nParams);
        case HyperToken::Type::DIV:
            return computeSample(t, *(tok.a), params, nParams) /
                   computeSample(t, *(tok.b), params, nParams);
        case HyperToken::Type::MOD:
            return std::fmod(computeSample(t, *(tok.a), params, nParams),
                             computeSample(t, *(tok.b), params, nParams));
        case HyperToken::Type::POW:
            return std::pow(computeSample(t, *(tok.a), params, nParams),
                            computeSample(t, *(tok.b), params, nParams));
        case HyperToken::Type::NONE:
            return 0;
        default:
            return 0;
    }
}

std::size_t getBoolIndex(float* paramValues, std::size_t params) {
    std::size_t res = 0;
    for (std::size_t i = 0; i < params; i++) {
        res *= WAVETABLE_PARAM_SAMPLES;
        res += paramValues[i] * WAVETABLE_PARAM_SAMPLES;
    }
    return res;
}

void HyperWaveTable::compute(std::size_t paramFill, float* paramValues) {
    // TODO if == 1
    if (paramFill == 0) {
        struct HyperToken root = *(this->f);
        std::size_t index = getBoolIndex(paramValues, this->params);
        for (std::size_t i = 0; i < samples; i++) {
            double t = i;
            t /= samples;
            this->content[index * samples + i] =
                computeSample(t, root, paramValues, this->params);
        }
        this->ready[index] = true;
    } else {
        for (int p = 0; p < WAVETABLE_PARAM_SAMPLES; p++) {
            paramValues[paramFill - 1] = (float)(p) / WAVETABLE_PARAM_SAMPLES;
            this->compute(paramFill - 1, paramValues);
        }
    }
}

int paramMin[MAX_PARAMS] = {};
int paramVal[MAX_PARAMS] = {};

float lerpArr(float* arr, int idx, float delta) {
    return arr[idx] * delta + arr[idx + 1] * (1 - delta);
}

// TODO this is sample interpolation, we need spectral interpolation too
// (another content table that contains fft data and would get deconstructed in
// real time)
bool HyperWaveTable::fillBlockConst(float* block, int sz, float* parameters,
                                    float& phase, float frequency,
                                    float sampleRate) const {
    if (spectral) return false;
    for (int i = 0; i < this->params; i++) {
        float d = parameters[i] * WAVETABLE_PARAM_SAMPLES;
        float min = std::floor(d);
        if (d == WAVETABLE_PARAM_SAMPLES) min = WAVETABLE_PARAM_SAMPLES - 1;
        paramMin[i] = min;
        paramVal[i] = d;
    }
    if (params == 1) {
        int idxMin = paramMin[0];
        if (!ready[idxMin] || !ready[idxMin + 1]) return false;
        float dt = frequency / sampleRate;
        float f = paramVal[0] - paramMin[0];
        for (int i = 0; i < samples; i++) {
            float sample = phase * WAVETABLE_TIME_SAMPLES;
            float t = std::floor(sample);
            block[i] =
                lerpArr(this->content + idxMin, t, sample - t) * f +
                lerpArr(this->content + idxMin + 1, t, sample - t) * (1 - f);
            phase += dt;
            if (phase > 1) phase -= 1;
        }
    }
    if (params == 2) {
        int idxMin = (paramMin[0] * WAVETABLE_PARAM_SAMPLES) | paramMin[1];
        if (!ready[idxMin] || !ready[idxMin + 1] ||
            !ready[idxMin + WAVETABLE_PARAM_SAMPLES] ||
            !ready[idxMin + WAVETABLE_PARAM_SAMPLES + 1]) {
            return false;
        }
#define SQRDIST(x, y) (x) * (x) + (y) * (y)
#define SQRTDIST(x, y) std::sqrt((x) * (x) + (y) * (y))
        float f0 =
            SQRDIST(paramVal[1] - paramMin[1], paramVal[0] - paramMin[0]);
        float f1 =
            SQRDIST(paramVal[1] - paramMin[1], 1 - paramVal[0] + paramMin[0]);
        float f2 =
            SQRDIST(1 - paramVal[1] + paramMin[1], paramVal[0] - paramMin[0]);
        float f3 = SQRDIST(1 - paramVal[1] + paramMin[1],
                           1 - paramVal[0] + paramMin[0]);
        float sum = f0 + f1 + f2 + f3;
        f0 /= sum;
        f1 /= sum;
        f2 /= sum;
        f3 /= sum;
        float dt = frequency / sampleRate;
        for (int i = 0; i < samples; i++) {
            float sample = phase * WAVETABLE_TIME_SAMPLES;
            float t = std::floor(sample);
            block[i] =
                lerpArr(this->content + idxMin, t, sample - t) * f0 +
                lerpArr(this->content + idxMin + 1, t, sample - t) * f1 +
                lerpArr(this->content + idxMin + WAVETABLE_PARAM_SAMPLES, t,
                        sample - t) *
                    f2 +
                lerpArr(this->content + idxMin + WAVETABLE_PARAM_SAMPLES + 1, t,
                        sample - t) *
                    f3;
            phase += dt;
            if (phase > 1) phase -= 1;
        }
    }
    /* else if (params == 3) {
        int idxMin = (paramMin[0] << 16) | (paramMin[1] << 8) | paramMin[2];
        if (!ready[idxMin] || !ready[idxMin + 1] ||
            !ready[idxMin + WAVETABLE_PARAM_SAMPLES] ||
            !ready[idxMin + WAVETABLE_PARAM_SAMPLES + 1] ||
            !ready[idxMin +
                   WAVETABLE_PARAM_SAMPLES * WAVETABLE_PARAM_SAMPLES] ||
            !ready[idxMin + WAVETABLE_PARAM_SAMPLES * WAVETABLE_PARAM_SAMPLES +
                   1] ||
            !ready[idxMin + WAVETABLE_PARAM_SAMPLES * WAVETABLE_PARAM_SAMPLES +
                   WAVETABLE_PARAM_SAMPLES] ||
            !ready[idxMin + WAVETABLE_PARAM_SAMPLES * WAVETABLE_PARAM_SAMPLES +
                   WAVETABLE_PARAM_SAMPLES + 1]) {
            delete[] paramMax;
            delete[] paramMin;
            return false;
        }
    }*/
    return true;
}

void HyperWaveTable::fillBlock(float* block, int sz, float* parameters,
                               float& phase, const float frequency) {}

std::ostream& operator<<(std::ostream& stream, const struct HyperToken& tok) {
    stream << "Tok type: ";
    switch (tok.type) {
        case HyperToken::NONE:
            stream << "none";
            break;
        case HyperToken::FUNCTION: {
            const int i = tok.func;
            if (i > CEIL)
                stream << "function\nFunction: UNKNOWN";
            else {
                const char* func = functions[i].c_str();
                stream << "function\nFunction: " << func;
            }
            break;
        }
        case HyperToken::VARIABLE:
            stream << "var\nName: " << tok.var;
            break;
        case HyperToken::NUMBER:
            stream << "number\nValue: " << tok.number;
            break;
        case HyperToken::ADD:
            stream << '+';
            break;
        case HyperToken::SUB:
            stream << '-';
            break;
        case HyperToken::DIV:
            stream << '/';
            break;
        case HyperToken::MUL:
            stream << '*';
            break;
        case HyperToken::POW:
            stream << "**";
            break;
        case HyperToken::MOD:
            stream << '%';
            break;
        case HyperToken::LESS:
            stream << '<';
            break;
        case HyperToken::LESS_OR_EQUAL:
            stream << "<=";
            break;
        case HyperToken::GREATER:
            stream << '>';
            break;
        case HyperToken::GREATER_OR_EQUAL:
            stream << ">=";
            break;
    }
    stream << "\n";
    if (tok.a) {
        stream << "\nOperand 1: " << *(tok.a) << '\n';
        if (tok.b) stream << "\nOperand 2: " << *(tok.b) << '\n';
    }
    return stream;
}

void HyperToken::printGLSL(std::ostringstream& str) const {
    switch (this->type) {
        case HyperToken::FUNCTION:
            switch (this->func) {
                case SIN:
                    str << "sin(";
                    a->printGLSL(str);
                    str << ")";
                    break;
                case COS:
                    str << "cos(";
                    a->printGLSL(str);
                    str << ")";
                    break;
                case TAN:
                    str << "tan(";
                    a->printGLSL(str);
                    str << ")";
                    break;
                case ASIN:
                    str << "asin(";
                    a->printGLSL(str);
                    str << ")";
                    break;
                case ACOS:
                    str << "acos(";
                    a->printGLSL(str);
                    str << ")";
                    break;
                case ATAN:
                    str << "atan(";
                    a->printGLSL(str);
                    str << ")";
                    break;
                case LN:
                    str << "log(";
                    a->printGLSL(str);
                    str << ")";
                    break;
                case EXP:
                    str << "exp(";
                    a->printGLSL(str);
                    str << ")";
                    break;
                case SQRT:
                    str << "sqrt(";
                    a->printGLSL(str);
                    str << ")";
                    break;
                case COSH:
                    str << "cosh(";
                    a->printGLSL(str);
                    str << ")";
                    break;
                case SINH:
                    str << "sinh(";
                    a->printGLSL(str);
                    str << ")";
                    break;
                case TANH:
                    str << "tanh(";
                    a->printGLSL(str);
                    str << ")";
                    break;
                case ABS:
                    str << "abs(";
                    a->printGLSL(str);
                    str << ")";
                    break;
                case SIGN:
                    str << "sign(";
                    a->printGLSL(str);
                    str << ")";
                    break;
                case ERF:
                    str << "erf(";
                    a->printGLSL(str);
                    str << ")";
                    break;
                case MAX:
                    str << "max((";
                    a->printGLSL(str);
                    str << "),(";
                    b->printGLSL(str);
                    str << "))";
                    break;
                case MIN:
                    str << "min((";
                    a->printGLSL(str);
                    str << "),(";
                    b->printGLSL(str);
                    str << "))";
                    break;
                case GAMMA:
                    str << "gamma(";
                    a->printGLSL(str);
                    str << ")";
                    break;
                case ROUND:
                    str << "round(";
                    a->printGLSL(str);
                    str << ")";
                    break;
                case CEIL:
                    str << "ceil(";
                    a->printGLSL(str);
                    str << ")";
                    break;
                case FLOOR:
                    str << "floor(";
                    a->printGLSL(str);
                    str << ")";
                    break;
                default:
                    break;
            }
            break;
        case HyperToken::VARIABLE:
            str << this->var;
            break;
        case HyperToken::NUMBER:
            str << this->number;
            break;
        case HyperToken::ADD:
            str << "(";
            a->printGLSL(str);
            str << ")+(";
            b->printGLSL(str);
            str << ")";
            break;
        case HyperToken::SUB:
            str << "(";
            a->printGLSL(str);
            str << ")-(";
            b->printGLSL(str);
            str << ")";
            break;
        case HyperToken::DIV:
            str << "(";
            a->printGLSL(str);
            str << ")/(";
            b->printGLSL(str);
            str << ")";
            break;
        case HyperToken::MUL:
            str << "(";
            a->printGLSL(str);
            str << ")*(";
            b->printGLSL(str);
            str << ")";
            break;
        case HyperToken::POW:
            str << "pow((";
            a->printGLSL(str);
            str << "),(";
            b->printGLSL(str);
            str << "))";
            break;
        case HyperToken::MOD:
            str << "mod((";
            a->printGLSL(str);
            str << "),(";
            b->printGLSL(str);
            str << "))";
            break;
        case HyperToken::LESS:
            str << "((";
            a->printGLSL(str);
            str << ")<(";
            b->printGLSL(str);
            str << "))";
            break;
        case HyperToken::LESS_OR_EQUAL:
            str << "((";
            a->printGLSL(str);
            str << ")<=(";
            b->printGLSL(str);
            str << "))";
            break;
        case HyperToken::GREATER:
            str << "((";
            a->printGLSL(str);
            str << ")>(";
            b->printGLSL(str);
            str << "))";
            break;
        case HyperToken::GREATER_OR_EQUAL:
            str << "((";
            a->printGLSL(str);
            str << ")>=(";
            b->printGLSL(str);
            str << "))";
            break;
        case HyperToken::PARENTHESIS:
            str << "(";
            a->printGLSL(str);
            str << ")";
        default:
            break;
    }
}
