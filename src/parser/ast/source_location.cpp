#include "source_location.hpp"
#include "../../lexer/lexer.hpp"

namespace Lua {
    SourceLocation SourceLocation::fromToken(const struct Token& token, const Str& filename) {
        return SourceLocation(filename, token.line, token.column);
    }
}