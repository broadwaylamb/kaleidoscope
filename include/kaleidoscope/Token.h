//
// Created by Sergej Jaskiewicz on 2019-05-28.
//

#ifndef KALEIDOSCOPE_TOKEN_H
#define KALEIDOSCOPE_TOKEN_H

#include "kaleidoscope/TokenKinds.h"
#include "llvm/Support/SMLoc.h"

namespace kaleidoscope {

class Token {
  /// Kind - The actual flavor of token this is.
  ///
  tok Kind;

  /// Text - The actual string covered by the token in the source buffer.
  llvm::StringRef Text;

public:
  Token(tok Kind, llvm::StringRef text) : Kind(Kind), Text(text) {}

  Token() : Token(tok::NUM_TOKENS, {}) {}

  /// is/isNot - Predicates to check if this token is a specific kind, as in
  /// "if (Tok.is(tok::l_brace)) {...}".
  bool is(tok K) const { return Kind == K; }
  bool isNot(tok K) const { return !is(K); }

  // Predicates to check to see if the token is any of a list of tokens.

  bool isAny(tok K) const { return is(K); }

  template <typename... T> bool isAny(tok K1, tok K2, T... Ks) const {
    if (is(K1)) {
      return true;
    }
    return isAny(K2, Ks...);
  }

  template <typename... T> bool isNot(tok K, T... Ks) const {
    return !isAny(K, Ks...);
  }

  bool isKeyword() const {
    switch (Kind) {
#define KEYWORD(X)                                                             \
  case tok::kw_##X:                                                            \
    return true;
#include "kaleidoscope/TokenKinds.def"
    default:
      return false;
    }
  }

  bool isLiteral() const {
    switch (Kind) {
    case tok::floating_literal:
      return true;
    default:
      return false;
    }
  }

  bool isPunctuation() const {
    switch (Kind) {
#define PUNCTUATOR(Name, Str)                                                  \
  case tok::Name:                                                              \
    return true;
#include "kaleidoscope/TokenKinds.def"
    default:
      return false;
    }
  }

  llvm::SMLoc getLoc() const {
    return llvm::SMLoc::getFromPointer(Text.begin());
  }

  unsigned getLength() const { return Text.size(); }

  llvm::SMRange getRange() const {
    return {getLoc(),
            llvm::SMLoc::getFromPointer(getLoc().getPointer() + getLength())};
  }

  tok getKind() const { return Kind; }

  void setKind(tok K) { Kind = K; }

  llvm::StringRef getText() const { return Text; }

  void setText(llvm::StringRef T) { Text = T; }

  /// Set the token to the specified kind and source range.
  void setToken(tok K, llvm::StringRef T) {
    Kind = K;
    Text = T;
  }
};

} // namespace kaleidoscope

namespace llvm {
template <typename T> struct isPodLike;
template <> struct isPodLike<kaleidoscope::Token> {
  static constexpr bool value = true;
};
} // namespace llvm

#endif /* KALEIDOSCOPE_TOKEN_H */
