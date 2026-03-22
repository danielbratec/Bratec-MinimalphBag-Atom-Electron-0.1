#include <iostream>
#include <string>
#include <unordered_map>
#include <cctype>
#include <stdexcept>
#include <vector>
#include <variant>

// Valor da variável: número ou string
using Value = std::variant<long long, std::string>;

// ------------------------------
//          Tokens
// ------------------------------
enum class TokenType {
    VAR, PIN, DIGITAL_WRITE, DIGITAL_READ, PRINT, PRINT_TABLE,
    IDENTIFIER, NUMBER, STRING,
    HIGH, LOW, AND, OR, XOR, NOT,
    ASSIGN, SEMICOLON, COMMA,
    EOF_TOKEN, COMMENT, UNKNOWN
};

struct Token {
    TokenType type;
    std::string value;
    int line = 1;

    Token(TokenType t, std::string v = "", int l = 1) : type(t), value(std::move(v)), line(l) {}
};

// ------------------------------
//          Lexer
// ------------------------------
class Lexer {
private:
    std::string source;
    size_t pos = 0;
    int line = 1;

    char peek() const { return pos < source.size() ? source[pos] : '\0'; }
    char advance() { return pos < source.size() ? source[pos++] : '\0'; }

    void skipWhitespace() {
        while (std::isspace(peek())) {
            if (peek() == '\n') ++line;
            advance();
        }
    }

    Token number() {
        std::string num;
        while (std::isdigit(peek())) num += advance();
        return Token(TokenType::NUMBER, num, line);
    }

    Token word() {
        std::string id;
        while (std::isalnum(peek()) || peek() == '_') id += advance();

        if (id == "var")           return Token(TokenType::VAR, "", line);
        if (id == "pin")           return Token(TokenType::PIN, "", line);
        if (id == "digitalWrite")  return Token(TokenType::DIGITAL_WRITE, "", line);
        if (id == "digitalRead")   return Token(TokenType::DIGITAL_READ, "", line);
        if (id == "print")         return Token(TokenType::PRINT, "", line);
        if (id == "printTable")    return Token(TokenType::PRINT_TABLE, "", line);
        if (id == "HIGH")          return Token(TokenType::HIGH, "", line);
        if (id == "LOW")           return Token(TokenType::LOW, "", line);
        if (id == "AND")           return Token(TokenType::AND, "", line);
        if (id == "OR")            return Token(TokenType::OR, "", line);
        if (id == "INPUT")         return Token(TokenType::PIN, "", line);
        if (id == "OUTPUT")        return Token(TokenType::PIN, "", line);

        return Token(TokenType::IDENTIFIER, id, line);
    }

    Token str() {
        std::string s;
        advance(); // "
        while (peek() != '"' && peek() != '\0' && peek() != '\n') s += advance();
        if (peek() == '"') advance();
        return Token(TokenType::STRING, s, line);
    }

public:
    explicit Lexer(std::string src) : source(std::move(src)) {}

    std::vector<Token> tokenize() {
        std::vector<Token> tokens;
        while (pos < source.size()) {
            skipWhitespace();
            char c = peek();
            if (c == '\0') break;

            if (std::isdigit(c))              tokens.push_back(number());
            else if (c == '"')                tokens.push_back(str());
            else if (std::isalpha(c))         tokens.push_back(word());
            else if (c == '=') { advance(); tokens.emplace_back(TokenType::ASSIGN, "=", line); }
            else if (c == ';') { advance(); tokens.emplace_back(TokenType::SEMICOLON, ";", line); }
            else if (c == ',') { advance(); tokens.emplace_back(TokenType::COMMA, ",", line); }
            else if (c == '/' && pos+1 < source.size() && source[pos+1] == '/') {
                advance(); advance();
                while (peek() != '\n' && peek() != '\0') advance();
                tokens.emplace_back(TokenType::COMMENT, "", line);
            }
            else {
                advance();
                tokens.emplace_back(TokenType::UNKNOWN, std::string(1,c), line);
            }
        }
        tokens.emplace_back(TokenType::EOF_TOKEN, "", line);
        return tokens;
    }
};

// ------------------------------
//          Interpreter
// ------------------------------
class Interpreter {
private:
    std::vector<Token> tokens;
    size_t current = 0;
    std::unordered_map<std::string, Value> variables;

    const Token& peekT() const { return tokens[current]; }
    Token consume() { return tokens[current++]; }

    bool match(TokenType t) {
        if (peekT().type == t) { consume(); return true; }
        return false;
    }

    void expect(TokenType t, const std::string& msg) {
        if (!match(t)) throw std::runtime_error("Linha " + std::to_string(peekT().line) + ": esperado " + msg);
    }

    Value getExpression();

public:
    explicit Interpreter(std::vector<Token> t) : tokens(std::move(t)) {}

    void run() {
        while (peekT().type != TokenType::EOF_TOKEN) {
            if (match(TokenType::VAR)) {
                expect(TokenType::IDENTIFIER, "nome da var");
                std::string nome = tokens[current-1].value;
                expect(TokenType::ASSIGN, "=");
                Value v = getExpression();
                expect(TokenType::SEMICOLON, ";");
                variables[nome] = std::move(v);
            }
            else if (match(TokenType::PRINT)) {
                Value v = getExpression();
                expect(TokenType::SEMICOLON, ";");

                if (std::holds_alternative<long long>(v)) {
                    std::cout << std::get<long long>(v) << '\n';
                } else {
                    std::cout << std::get<std::string>(v) << '\n';
                }
            }
            else if (match(TokenType::IDENTIFIER)) {
                std::string nome = tokens[current-1].value;
                expect(TokenType::ASSIGN, "=");
                Value v = getExpression();
                expect(TokenType::SEMICOLON, ";");
                variables[nome] = std::move(v);
            }
            else if (match(TokenType::COMMENT)) {}
            else {
                throw std::runtime_error("Comando inválido linha " + std::to_string(peekT().line));
            }
        }
    }
};

Value Interpreter::getExpression() {
    if (match(TokenType::NUMBER)) {
        return std::stoll(tokens[current-1].value);
    }
    if (match(TokenType::STRING)) {
        return tokens[current-1].value;
    }
    if (match(TokenType::HIGH)) {
        return 1LL;
    }
    if (match(TokenType::LOW)) {
        return 0LL;
    }
    if (match(TokenType::IDENTIFIER)) {
        std::string n = tokens[current-1].value;
        auto it = variables.find(n);
        if (it == variables.end()) throw std::runtime_error("Var não existe: " + n);
        return it->second;
    }
    throw std::runtime_error("Expressão inválida linha " + std::to_string(peekT().line));
}

// ------------------------------
//          Main
// ------------------------------
int main() {
    std::cout << "MinimalphBag Atom Electron v0.1  -  Linguagem simples para Eletrônica\n";
    std::cout << "Digite o código. Duas linhas vazias = executar.\n\n";

    std::string line, src;
    while (std::getline(std::cin, line)) {
        if (line.empty() && !src.empty()) break;
        src += line + "\n";
    }

    if (src.empty()) return 0;

    try {
        Lexer lex(src);
        auto toks = lex.tokenize();
        Interpreter interp(std::move(toks));
        interp.run();
        std::cout << "\nPronto.\n";
    } catch (const std::exception& e) {
        std::cerr << "ERRO: " << e.what() << '\n';
        return 1;
    }
    return 0;
}