#pragma once

#define _USE_MATH_DEFINES

#include <cmath>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

// TODO cpp-ify all this

#ifndef M_PI
#define M_PI 3.14159265f
#endif

#define WAVETABLE_PARAM_SAMPLES 256
#define WAVETABLE_TIME_SAMPLES 2048
#define MAX_PARAMS 4

enum Function {
    SINC,
    SIN,
    COS,
    TAN,
    ASIN,
    ACOS,
    ATAN,
    LN,
    EXP,
    SQRT,
    COSH,
    SINH,
    TANH,
    ABS,
    SIGN,
    ERF,
    MAX,
    MIN,
    POW,
    GAMMA,
    ROUND,
    FLOOR,
    CEIL
};

const std::string functions[] = {
    "sinc", "sin", "cos",  "tan",  "asin",  "acos",  "arctan", "log",
    "exp", "sqrt", "cosh", "sinh",  "tanh",  "abs",    "sign",
    "erf", "max",  "min", "pow",  "gamma", "round", "floor",  "ceil"};

struct HyperToken {
    enum Type {
        NONE = 0,
        PARENTHESIS,
        FUNCTION,
        VARIABLE,
        NUMBER,

        LESS,
        GREATER,
        EQUAL,
        LESS_OR_EQUAL,
        GREATER_OR_EQUAL,

        ADD,
        SUB,
        MUL,
        DIV,
        MOD,
        POW
    } type = NONE;
    struct HyperToken* a = nullptr;
    struct HyperToken* b = nullptr;
    union {
        float number;
        char var;
        enum Function func;
    };

    ~HyperToken() {
        delete a;
        delete b;
    }

    void printGLSL(std::ostringstream& str) const;

    static bool isOperator(HyperToken::Type t) { return t >= Type::ADD; }

    static int cmpPriority(HyperToken::Type type1, HyperToken::Type type2) {
        if (!isOperator(type1) && !isOperator(type2)) return 0;
        return type2 - type1;
    }
};

std::ostream& operator<<(std::ostream& stream, const struct HyperToken& tok);

constexpr static std::size_t pow256(std::size_t n) {
    return (std::size_t)0x1 << (n << 1);
}

float computeSample(double t, const struct HyperToken& tok, float* params,
                    std::size_t nParams);

class HyperWaveTable {
   public:
    // parameters should be above 0
    explicit HyperWaveTable(std::size_t parameters);

    virtual ~HyperWaveTable();

    void setFunction(const HyperToken*&& func) {
        this->f = func;
        memset(ready, 0, sizeof(bool) * pow256(this->params));
    }

    /**
     * Fills up the content of the wavetable using the function set in
     * setFunction.
     * @param paramFill determines for how many
     * parameters the entirety of the possible resulting tables will be
     * computed.
     * @param paramValues contains the remaining parameter values for
     * which to compute the tables. It must be of the size of registered
     * parameters and first parameters will be modified according to paramFill.
     * @example compute(2, [0, 0, .25f]) will compute every possible wave with
     * parameter 3 = 0.25f
     * compute(1, [21, .1f, 1.f])
     * will compute every possible wave with
     * parameter 2 = 0.1f and parameter 3 = 1.0f
     */
    void compute(std::size_t paramFill, float* paramValues);

    /**
     * Fills an audio block with data from this wavetable's content without
     * calculating new elements in it
     * @return true if it could fill the block or false if samples to compute
     * are not ready yet
     */
    bool fillBlockConst(float* block, int sz, float* parameters, float& phase,
                        float frequency, float sampleRate) const;

    //void fillBlock(float* block, int sz, float* parameters, float& phase,
    //               float frequency);

    static constexpr std::size_t samples = WAVETABLE_TIME_SAMPLES;

   private:
    const struct HyperToken* f = nullptr;
    bool* ready;
    float* content;
    float** fftContent;
    bool spectral = false;
    std::size_t params;
};