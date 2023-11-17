#include <iostream>

#include <boost/regex.hpp>

#include <boost/lexical_cast.hpp>

#include <FileComparer/H/FileComparer.h>

namespace utils {

static bool CheckIsClose(double value1, double value2, double tolerance)
{
    double diff   = std::abs(value1 - value2);
    double maxVal = std::max(std::abs(value1), std::abs(value2));

    return diff <= maxVal * tolerance;
}

// for numbers like -1,595,226,615
static double SpecialNumberToDouble(const std::string& number) {
    std::string cleaned_number = number;
    cleaned_number.erase(std::remove(cleaned_number.begin(), cleaned_number.end(), ','), cleaned_number.end());
    return boost::lexical_cast<double>(cleaned_number);
}

FileComparer::FileComparer(double relative_tolerance, Options options)
    : relative_tolerance_(relative_tolerance)
    , options_(options)
{
}

bool FileComparer::Compare(const boost::filesystem::path& p1, const boost::filesystem::path& p2)
{
    std::ifstream f1(p1.string(), std::ifstream::binary);
    std::ifstream f2(p2.string(), std::ifstream::binary);
    if (!f1.is_open())
        throw std::runtime_error("Could not open file " + p1.string());
    if (!f2.is_open())
        throw std::runtime_error("Could not open file " + p2.string());
    return Compare(f1, f2);
}

bool FileComparer::Compare(std::ifstream& f1, std::ifstream& f2)
{
    std::string line1;
    std::string line2;

    while (true) {
        std::getline(f1, line1);
        std::getline(f2, line2);

        if (f1.eof() || f2.eof()) break;

        std::list<Token> tokens_f1 = Tokenize(line1);
        std::list<Token> tokens_f2 = Tokenize(line2);

        if (tokens_f1.size() != tokens_f2.size()) {
            std::cerr << "Size: " << tokens_f1.size() << " != " << tokens_f2.size() << std::endl;
            return false;
        }

        auto it_tokens_1 = tokens_f1.begin();
        auto it_tokens_2 = tokens_f2.begin();
        while (it_tokens_1 != tokens_f1.end()) {
            if (it_tokens_1->type != it_tokens_2->type) {
                std::cerr << "Type: " << it_tokens_1->type << " != " << it_tokens_2->type << std::endl;
                return false;
            }

            if (it_tokens_1->type == Token::Type::NUMBER) {
                double value_1 = boost::lexical_cast<double>(it_tokens_1->value);
                double value_2 = boost::lexical_cast<double>(it_tokens_2->value);

                if (CheckIsClose(value_1, value_2, relative_tolerance_) == false) {
                    std::cerr << it_tokens_1->value << " != " << it_tokens_2->value << std::endl;
                    return false;
                }
            } else if (it_tokens_1->type == Token::Type::NUMBER_SPECIAL) {
                double value_1 = SpecialNumberToDouble(it_tokens_1->value);
                double value_2 = SpecialNumberToDouble(it_tokens_2->value);
   
                if (CheckIsClose(value_1, value_2, relative_tolerance_) == false) {
                    std::cerr << it_tokens_1->value << " != " << it_tokens_2->value << std::endl;
                    return false;
                }
            }else if (it_tokens_1->value != it_tokens_2->value) {
                std::cerr << "Value: " << it_tokens_1->value << " != " << it_tokens_2->value << std::endl;
                return false;
            }

            ++it_tokens_1;
            ++it_tokens_2;
        }
    }

    return f1.eof() && f2.eof();
}

FileComparer::Token FileComparer::Lexer(std::string& text)
{
    static const boost::regex WHITE_X("\\s+");
    static const boost::regex NUMBER_X("(\\+|-)?(\\d+(\\.\\d*)?|\\.\\d+)([eE](\\+|-)?\\d+)?");
    static const boost::regex NUMBER_SPECIAL_X("(-?\\d{1,3}(,\\d{3})*(\\.\\d+)?)");
    static const boost::regex STRING_X("\\w+");
    static const boost::regex SPECIAL_X("[^\\w\\s\\d]+");

    boost::smatch matches;
    Token       token;

    if (boost::regex_search(text, matches, STRING_X) && matches.position() == 0)
        return { Token::Type::STRING, std::move(matches[0]) };
    if (boost::regex_search(text, matches, NUMBER_SPECIAL_X) && matches.position() == 0)
        return { Token::Type::NUMBER_SPECIAL, std::move(matches[0]) };
    if (boost::regex_search(text, matches, NUMBER_X) && matches.position() == 0)
        return { Token::Type::NUMBER, std::move(matches[0]) };
    if (boost::regex_search(text, matches, SPECIAL_X) && matches.position() == 0)
        return { Token::Type::SPECIAL, std::move(matches[0]) };
    if (boost::regex_search(text, matches, WHITE_X) && matches.position() == 0)
        return { Token::Type::WHITE, std::move(matches[0]) };

    return token;
}

std::list<FileComparer::Token> FileComparer::Tokenize(std::string& text)
{
    std::list<Token> tokens;
    while (!text.empty()) {
        Token token = Lexer(text);
        text.erase(0, token.value.size());
        if (options_.ignore_whitespaces && token.type == Token::Type::WHITE)
            continue;
        tokens.emplace_back(std::move(token));
    }

    return tokens;
}

}
