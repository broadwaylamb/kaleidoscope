//
// Created by Sergej Jaskiewicz on 2019-05-28.
//

#ifndef KALEIDOSCOPE_LEXER_H
#define KALEIDOSCOPE_LEXER_H

#include "kaleidoscope/Token.h"
#include "llvm/ADT/StringRef.h"
#include "kaleidoscope/SourceManager.h"
#include <vector>

namespace kaleidoscope {

class Lexer {
  const SourceManager &SourceMgr;
  const unsigned BufferID;

  const char *BufferStart;
  const char *BufferEnd;
  const char *CurPtr;
  Token NextToken;

public:

  /// Create a normal lexer that scans the whole source buffer.
  Lexer(const SourceManager &SourceMgr, unsigned BufferID);

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
  void tryLexEditorPlaceholder();
  void skipPoundComment(bool EatNewline);
  void skipToEndOfLine(bool EatNewline);
  void formToken(tok Kind, const char *TokStart);

  /// Determine the token kind of the string, given that it is a valid
  /// identifier. Return tok::identifier if the string is not a reserved word.
  static tok kindOfIdentifier(llvm::StringRef Str);
  void lexOperator();

  void diagnose(const char *Loc, llvm::SourceMgr::DiagKind Kind,
                const llvm::Twine &Msg,
                llvm::ArrayRef<llvm::SMRange> Ranges = llvm::None,
                llvm::ArrayRef<llvm::SMFixIt> FixIts = llvm::None,
                bool ShowColors = true) const {
    SourceMgr.getLLVMSourceMgr().PrintMessage(llvm::SMLoc::getFromPointer(Loc),
                                              Kind, Msg, Ranges, FixIts,
                                              ShowColors);
  }
};

} // namespace kaleidoscope

#endif /* KALEIDOSCOPE_LEXER_H */
