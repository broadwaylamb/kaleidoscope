//
// Created by Sergej Jaskiewicz on 2019-05-28.
//

#ifndef KALEIDOSCOPE_LEXER_H
#define KALEIDOSCOPE_LEXER_H

#include "kaleidoscope/Token.h"
#include "llvm/ADT/StringRef.h"
#include <vector>

namespace kaleidoscope {

class Lexer {
  const char *BufferStart;
  const char *BufferEnd;
  const char *CurPtr;
  Token NextToken;

public:
  Lexer(const char *BufferStart, const char *BufferEnd)
      : BufferStart(BufferStart), BufferEnd(BufferEnd), CurPtr(BufferStart) {}

  Lexer(const Lexer &) = delete;
  void operator=(const Lexer &) = delete;

  Token lex() {
    auto result = NextToken;
    if (result.isNot(tok::eof)) {
      lexImpl();
    }
    return result;
  }

  const Token &peekNextToken() const { return NextToken; }

private:
  void lexImpl();
  void lexIdentifier();
  void lexNumber();
  void lexTrivia();
  void skipPoundComment(bool EatNewline);
  void skipToEndOfLine(bool EatNewline);
  void formToken(tok Kind, const char *TokStart);

  /// Determine the token kind of the string, given that it is a valid
  /// identifier. Return tok::identifier if the string is not a reserved word.
  static tok kindOfIdentifier(llvm::StringRef Str);
};

} // namespace kaleidoscope

#endif /* KALEIDOSCOPE_LEXER_H */
