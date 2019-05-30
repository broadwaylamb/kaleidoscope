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

} // namespace

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
  default:
    if (isalpha(*TokStart)) {
      lexIdentifier();
      return;
    } else if (isdigit(*TokStart) || *TokStart == '.') {
      lexNumber();
      return;
    } else {
      formToken(tok::unknown, TokStart);
      return;
    }
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
    if (isalpha(CurPtr[-1]) || isdigit(CurPtr[-1]) || CurPtr[-1] == '.') {
      break;
    } else {
      goto Restart;
    }
  }

  // Reset the cursor.
  --CurPtr;
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
