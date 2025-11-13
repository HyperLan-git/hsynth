#include "Parsing.hpp"

// All this implementation is kinda slow and bad so forgive me

struct HyperToken* parseSymbol(const char*& c, std::string& err) {
    struct HyperToken* res = nullptr;
    if (*c <= '9' && *c >= '0') {
        std::size_t len = 0;
        res = new HyperToken();
        res->type = HyperToken::NUMBER;
        res->number = std::stof(std::string(c), &len);
        if (std::isnan(res->number)) {
            delete res;
            err = std::string("Invalid number near : ") + std::string(c);
            return nullptr;
        }
        c += len;
        return res;
    }

    if (!std::isalpha(*c)) {
        err = std::string("Invalid character near : ") + std::string(c);
        return nullptr;
    }
    for (int i = 0; i < sizeof(functions) / sizeof(std::string); i++) {
        const char* c2 = functions[i].c_str();
        std::size_t l = functions[i].length();
        if (std::strncmp(c, c2, l) == 0 && c[l] == '(') {
            bool binary = i == Function::MAX || i == Function::MIN;
            std::size_t grp = l + 1;
            int cnt = 0;
            bool done = false;
            for (; c[grp] != '\0'; grp++) {
                if (c[grp] == '(')
                    cnt++;
                else if (c[grp] == ')' && cnt == 0) {
                    done = true;
                    break;
                } else if (c[grp] == ')')
                    cnt--;
            }
            if (!done) {
                err = std::string("Missing end of function call for: ") +
                      std::string(c);
                return nullptr;
            }
            std::string content(c + l + 1);
            content = content.substr(0, grp - l - 1);
            if (content.length() == 0) {
                err = std::string("Missing argument near: ") + std::string(c);
                return nullptr;
            }
            if (binary) {
                bool found = false;
                int cnt = 0;
                std::size_t comma = 0;
                for (; comma < content.length(); comma++) {
                    if (content[comma] == ',' && cnt == 0) {
                        found = true;
                        break;
                    } else if (content[comma] == '(')
                        cnt++;
                    else if (content[comma] == ')') {
                        if (cnt <= 0) {
                            err = std::string("Invalid parenthesis near: ") +
                                  std::string(c + l + 1 + comma);
                            return nullptr;
                        }
                        cnt--;
                    }
                }
                if (!found) {
                    err = std::string("Missing argument in: ") + std::string(c);
                    return nullptr;
                }
                struct HyperToken* arg1 = parse(content.substr(0, comma), err);
                if (!arg1) return nullptr;
                struct HyperToken* arg2 = parse(content.substr(comma + 1), err);
                if (!arg2) {
                    delete arg1;
                    return nullptr;
                }
                res = new HyperToken();
                res->type = HyperToken::FUNCTION;
                res->func = (Function)i;
                res->a = arg1;
                res->b = arg2;
                c += grp + 1;
                return res;
            }
            struct HyperToken* arg1 = parse(content, err);
            if (!arg1) return nullptr;
            res = new HyperToken();
            res->type = HyperToken::FUNCTION;
            res->func = (Function)i;
            res->a = arg1;
            c += grp + 1;
            return res;
        }
    }
    if (std::isalpha(*c) && !std::isalpha(c[1])) {
        res = new HyperToken();
        res->type = HyperToken::VARIABLE;
        res->var = *(c++);
        return res;
    }
    err = std::string("Invalid token near: ") + std::string(c);
    return res;
}

#define NEWOP(tp)                                                            \
    if (!op && !leftOp) {                                                    \
        err = std::string("Missing operand before: ") + std::string(c);      \
        return nullptr;                                                      \
    } else if (op) {                                                         \
        delete op;                                                           \
        err = std::string("Two operators in a row near: ") + std::string(c); \
        return nullptr;                                                      \
    }                                                                        \
    op = new HyperToken();                                                   \
    op->type = tp;                                                           \
    op->a = leftOp;

// Maybe one of those days I should actually use exceptions xd
struct HyperToken* parse(std::string input, std::string& err) {
    struct HyperToken *leftOp = nullptr, *op = nullptr;
    for (const char* c = input.c_str(); *c != '\0'; c++) {
        switch (*c) {
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                // Ignored
                break;
            case '(': {
                bool done = false;
                int cnt = 0;
                std::size_t v = 1;
                for (; c[v] != '\0'; v++) {
                    if (c[v] == '(')
                        cnt++;
                    else if (c[v] == ')' && cnt == 0) {
                        done = true;
                        break;
                    } else if (c[v] == ')')
                        cnt--;
                }
                if (!done) {
                    if (!op)
                        delete leftOp;
                    else
                        delete op;
                    err = std::string("Illegal parenthesis near: ") +
                          std::string(c);
                    return nullptr;
                }
                if (!leftOp) {
                    leftOp =
                        parse(input.substr(c - input.c_str() + 1, v - 1), err);
                    if (!leftOp) return nullptr;

                    struct HyperToken* res = new HyperToken();
                    res->type = HyperToken::PARENTHESIS;
                    res->a = leftOp;
                    leftOp = res;
                } else if (!op) {
                    delete leftOp;
                    err = std::string("Illegal paren near: ") + std::string(c);
                    return nullptr;
                } else {
                    op->b =
                        parse(input.substr(c - input.c_str() + 1, v - 1), err);
                    if (!op->b) {
                        delete op;
                        return nullptr;
                    }
                    leftOp = op;
                    op = nullptr;

                    struct HyperToken* res = new HyperToken();
                    res->type = HyperToken::PARENTHESIS;
                    res->a = leftOp;
                    leftOp = res;
                }
                c += v;
            } break;
            case ')':
                if (!op)
                    delete leftOp;
                else
                    delete op;
                err = std::string("Invalid closing parenthesis near: ") +
                      std::string(c);
                return nullptr;

            case '*':
                if (c[1] == '*') {
                    NEWOP(HyperToken::POW);
                    c++;
                    break;
                }
            case '.':
                NEWOP(HyperToken::MUL);
                break;

            case '>':
                if (c[1] == '=') {
                    NEWOP(HyperToken::GREATER_OR_EQUAL);
                    c++;
                    break;
                }
                NEWOP(HyperToken::GREATER);
                break;
            case '<':
                if (c[1] == '=') {
                    NEWOP(HyperToken::LESS_OR_EQUAL);
                    c++;
                    break;
                }
                NEWOP(HyperToken::LESS);
                break;

            case '+':
                NEWOP(HyperToken::ADD);
                break;
            case '-':
                if ((!leftOp || op) && c[1] <= '9' && c[1] >= '0') {
                    std::size_t len = 0;
                    float num = std::stof(std::string(c), &len);
                    if (std::isnan(num)) {
                        if (op) delete op;
                        err = std::string("Invalid number near : ") +
                              std::string(c);
                        return nullptr;
                    }

                    struct HyperToken* res = new HyperToken();
                    res->type = HyperToken::NUMBER;
                    res->number = num;
                    if (op) {
                        op->b = res;
                        leftOp = op;
                        op = nullptr;
                    } else {
                        leftOp = res;
                    }
                    c += len - 1;
                } else {
                    NEWOP(HyperToken::SUB);
                }
                break;
            case '/':
                NEWOP(HyperToken::DIV);
                break;
            case '%':
                NEWOP(HyperToken::MOD);
                break;
            case '=':
                NEWOP(HyperToken::EQUAL);
                break;
            default:
                if (!leftOp) {
                    leftOp = parseSymbol(c, err);
                    c--;
                    if (!leftOp) return nullptr;
                } else if (!op) {
                    delete leftOp;
                    err = std::string("Invalid tokens near: ") + std::string(c);
                    return nullptr;
                } else {
                    op->b = parseSymbol(c, err);
                    c--;
                    if (!op->b) {
                        delete op;
                        return nullptr;
                    }
                    if (HyperToken::isOperator(op->a->type) &&
                        HyperToken::cmpPriority(op->type, op->a->type) < 0) {
                        op->a = leftOp->b;
                        leftOp->b = op;
                        op = nullptr;
                    } else {
                        leftOp = op;
                        op = nullptr;
                    }
                }
                break;
        }
    }
    if (op) {
        delete op;
        err = std::string("Incomplete expression !");
        return nullptr;
    }

    return leftOp;
}