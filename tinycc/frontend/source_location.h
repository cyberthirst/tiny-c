#pragma once
#include <string>
#include <unordered_map>
#include <vector>

namespace tiny {

    class SourceLocation {
    public:
        SourceLocation(std::string const & filename, size_t line, size_t col):
            line_(line),
            col_(col) {
            Filenames & f = Filenames_();
            auto i = f.lookup.find(filename);
            if (i == f.lookup.end()) {
                i = f.lookup.insert(std::make_pair(filename, f.names.size())).first;
                f.names.push_back(filename);
            }
            file_ = i->second;
        }

        std::string const & file() const {
            return Filenames_().names[file_];
        }

        size_t line() const {
            return line_;
        }

        size_t col() const {
            return col_;
        }

    private:

        friend class Lexer;

        size_t file_;
        size_t line_;
        size_t col_;

        struct Filenames {
            std::vector<std::string> names;
            std::unordered_map<std::string, size_t> lookup;
        };

        static Filenames & Filenames_() {
            static Filenames singleton;
            return singleton;
        }

        friend std::ostream & operator << (std::ostream & s, SourceLocation const & l) {
            s << l.file() << " [" << l.line() << ", " << l.col() << "]";
            return s;
        }

    }; // tiny::SourceLocation

}