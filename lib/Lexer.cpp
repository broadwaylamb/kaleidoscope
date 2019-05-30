//
// Created by Sergej Jaskiewicz on 2019-05-28.
//

#include "kaleidoscope/Lexer.h"
#include "kaleidoscope/TokenKinds.h"

using namespace kaleidoscope;
using namespace llvm;

namespace {

/// Advance \p CurPtr to the end of line or the end of file. Returns \c true
/// if it stopped at the end of line, \c false if it stopped at the end of file.
bool advanceToEndOfLine(const char *&CurPtr, const char *BufferEnd) {
  while (true) {
    switch (*CurPtr++) {
    case '\n':
    case '\r':
      --CurPtr;
      return true; // If we found the end of the line, return.
    case 0:
      if (CurPtr - 1 != BufferEnd) {
        // If this is a random nul character in the middle of a buffer, skip
        // it as whitespace.
        continue;
      }
      // Otherwise, the last line of the file does not have a newline.
      --CurPtr;
      return false;
    default:
      break;
    }
  }
}

bool isIdentifierStartCharacter(char c) { return isAlpha(c) || c == '_'; }

bool isIdentifierContinuationCharacter(char c) {
  return isIdentifierStartCharacter(c) || isDigit(c);
}

bool isFloatLiteralCharacter(char c) { return isDigit(c) || c == '.'; }

bool isOperatorStartCharacter(char c) {
  switch (c) {
  case '%':
  case '!':
  case '=':
  case '<':
  case '>':
  case '-':
  case '+':
  case '*':
  case '&':
  case '|':
  case '/':
    return true;
  default:
    return false;
  }
}

bool isOperatorContinuationCharacter(char c) {
  return isOperatorStartCharacter(c);
}

/// Is the operator beginning at the given character "left-bound"?
bool isLeftBound(const char *TokBegin, const char *BufferBegin) {
  // The first character in the file is not left-bound.
  if (TokBegin == BufferBegin) return false;

  switch (TokBegin[-1]) {
  case ' ': case '\r': case '\n': case '\t': // whitespace
  case '(':                                  // opening delimiters
  case '\0':                                 // whitespace / last char in file
    return false;
  default:
    return true;
  }
}

/// Is the operator ending at the given character (actually one past the end)
/// "right-bound"?
bool isRightBound(const char *tokEnd) {
  switch (*tokEnd) {
  case ' ': case '\r': case '\n': case '\t': // whitespace
  case ')': // closing delimiters
  case '\0': // whitespace / last char in file
  case '#': // A following comment counts as whitespace, so this token is not
            // right bound.
    return false;
  default:
    return true;
  }
}

} // namespace

Lexer::Lexer(const SourceManager &SourceMgr, unsigned BufferID)
    : SourceMgr(SourceMgr), BufferID(BufferID) {

  // Initialize buffer pointers.
  StringRef contents =
      SourceMgr.getLLVMSourceMgr().getMemoryBuffer(BufferID)->getBuffer();

  BufferStart = contents.data();
  BufferEnd = contents.data() + contents.size();
  CurPtr = BufferStart;

  assert(*BufferEnd == 0);
  assert(NextToken.is(tok::NUM_TOKENS));
  lexImpl();
}

void Lexer::lexImpl() {

  assert(CurPtr >= BufferStart && CurPtr <= BufferEnd &&
         "Current pointer out of range!");

  lexTrivia();

  // Remember the start of the token so we can form the text range.
  const char *TokStart = CurPtr;

  switch ((signed char)*CurPtr++) {
  case '\n':
  case '\r':
    llvm_unreachable("Newlines should be eaten by lexTrivia");
  case ' ':
  case '\t':
  case '\f':
  case '\v':
    llvm_unreachable("Whitespaces should be eaten by lexTrivia");
  case 0:
    // This is the real end of the buffer.
    // Put CurPtr back into buffer bounds.
    --CurPtr;
    // Return EOF.
    formToken(tok::eof, TokStart);
    return;
  case '(':
    formToken(tok::l_paren, TokStart);
    return;
  case ')':
    formToken(tok::r_paren, TokStart);
    return;
  case '<':
    if (*CurPtr == '#') {
      tryLexEditorPlaceholder();
      return;
    }
  default:
    if (isIdentifierStartCharacter(*TokStart)) {
      lexIdentifier();
      return;
    }
    if (isFloatLiteralCharacter(*TokStart)) {
      lexNumber();
      return;
    }
    if (isOperatorStartCharacter((*TokStart))) {
      lexOperator();
      return;
    }

    formToken(tok::unknown, TokStart);
  }
}

void Lexer::lexTrivia() {
Restart:
  switch ((signed char)*CurPtr++) {
  case '\n':
    goto Restart;
  case '\r':
    if (*CurPtr == '\n') {
      // CRLF
      ++CurPtr;
    }
    goto Restart;
  case ' ':
    goto Restart;
  case '\t':
    goto Restart;
  case '\v':
    goto Restart;
  case '\f':
    goto Restart;
  case '#':
    skipPoundComment(/*EatNewline=*/false);
    goto Restart;
  case 0:
  case '(':
  case ')':
    break;
  default:
    if (isIdentifierStartCharacter(CurPtr[-1]) ||
        isFloatLiteralCharacter(CurPtr[-1]) ||
        isOperatorStartCharacter(CurPtr[-1])) {
      break;
    }

    diagnose(CurPtr - 1, SourceMgr::DK_Error, "Unexpected token");
  }

  // Reset the cursor.
  --CurPtr;
}

void Lexer::tryLexEditorPlaceholder() {
  assert(CurPtr[-1] == '<' && CurPtr[0] == '#');
  const char *TokStart = CurPtr - 1;
  for (const char *Ptr = CurPtr + 1; Ptr < BufferEnd - 1; ++Ptr) {
    if (*Ptr == '\n')
      break;
    if (Ptr[0] == '<' && Ptr[1] == '#')
      break;
    if (Ptr[0] == '#' && Ptr[1] == '>') {
      // Found it. Flag it as error for the rest of the compiler pipeline and
      // lex it as an identifier.
      diagnose(TokStart, SourceMgr::DK_Error,
               "editor placeholder in source file");
      CurPtr = Ptr + 2;
      formToken(tok::identifier, TokStart);
      return;
    }
  }

  // Not a well-formed placeholder.
  lexOperator();
}

void Lexer::lexOperator() {
  const char *TokStart = CurPtr - 1;
  CurPtr = TokStart;
  assert(isOperatorStartCharacter(*TokStart));
  ++CurPtr;

  while (isOperatorContinuationCharacter(*CurPtr)) {
    ++CurPtr;
  }

  // Decide between the binary, prefix, and postfix cases.
  // It's binary if either both sides are bound or both sides are not bound.
  // Otherwise, it's postfix if left-bound and prefix if right-bound.
  bool leftBound = isLeftBound(TokStart, BufferStart);
  bool rightBound = isRightBound(CurPtr);

  if (leftBound == rightBound) {
    formToken(tok::infix_operator, TokStart);
    return;
  }

  formToken(leftBound ? tok::postfix_operator
                      : tok::prefix_operator,
            TokStart);
}

void Lexer::skipPoundComment(bool EatNewline) {
  assert(CurPtr[-1] == '#' && "Not a # comment");
  skipToEndOfLine(EatNewline);
}

void Lexer::skipToEndOfLine(bool EatNewline) {
  bool isEOL = advanceToEndOfLine(CurPtr, BufferEnd);
  if (EatNewline && isEOL) {
    ++CurPtr;
  }
}

void Lexer::formToken(tok Kind, const char *TokStart) {
  assert(CurPtr >= BufferStart && CurPtr <= BufferEnd &&
         "Current pointer out of range!");

  NextToken.setToken(Kind, {TokStart, static_cast<size_t>(CurPtr - TokStart)});
}

void Lexer::lexIdentifier() {
  const char *TokStart = CurPtr - 1;
  assert((isalpha(*TokStart) || *TokStart == '_') && "Unexpected start");

  while (CurPtr != BufferEnd && isalnum(*CurPtr)) {
    ++CurPtr;
  }

  tok Kind =
      kindOfIdentifier({TokStart, static_cast<size_t>(CurPtr - TokStart)});

  formToken(Kind, TokStart);
}

void Lexer::lexNumber() {
  const char *TokStart = CurPtr - 1;
  assert((isdigit(*TokStart) || *TokStart == '.') && "Unexpected start");

  bool HasPoint = false;
  while (CurPtr != BufferEnd && (isdigit(*CurPtr) || *CurPtr == '.')) {
    if (*CurPtr == '.') {
      if (HasPoint) {
        break;
      } else {
        HasPoint = true;
      }
    }
    ++CurPtr;
  }

  formToken(tok::floating_literal, TokStart);
}

tok Lexer::kindOfIdentifier(StringRef Str) {
#define KEYWORD(kw)                                                            \
  if (Str == #kw) {                                                            \
    return tok::kw_##kw;                                                       \
  }
#include "kaleidoscope/TokenKinds.def"
  return tok::identifier;
}
