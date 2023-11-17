#pragma once

#include <fstream>
#include <string>

#include <boost/filesystem.hpp>

#include <FileComparerAPI.h>

namespace utils {
class FILECOMPARER_EXPORT FileComparer {
public:
    struct Options {
        bool ignore_whitespaces = false;
    };

    FileComparer(double relative_tolerance, Options options = {});

    bool Compare(const boost::filesystem::path&, const boost::filesystem::path&);
    bool Compare(std::ifstream&, std::ifstream&);

private:
    struct Token {
        enum Type {
            UNDEFINED,
            STRING,
            WHITE,
            NUMBER,
            NUMBER_SPECIAL,
            SPECIAL
        };

        Type        type;
        std::string value;
    };

    double  relative_tolerance_;
    Options options_;

    Token            Lexer(std::string& text);
    std::list<Token> Tokenize(std::string& text);
};
}