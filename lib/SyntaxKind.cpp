//
// Created by Sergej Jaskiewicz on 2019-05-28.
//

#include <kaleidoscope/TokenKinds.h>
#include "kaleidoscope/TokenKinds.h"
#include "llvm/Support/raw_ostream.h"

using namespace kaleidoscope;
using namespace llvm;

static StringRef getTokenTextInternal(tok Kind) {
  switch (Kind) {
#define PUNCTUATOR(name, str) case tok::name: return str;
#define KEYWORD(name) case tok::kw_##name: return #name;
#include "kaleidoscope/TokenKinds.def"
    default:
      return StringRef();
  }
}

bool kaleidoscope::isTokenTextDetermined(tok Kind) {
  return !getTokenTextInternal(Kind).empty();
}

StringRef getTokenText(tok Kind) {
  auto text = getTokenTextInternal(Kind);
  assert(!text.empty() && "token text cannot be determined");
  return text;
}

void kaleidoscope::dumpTokenKind(tok Kind) {
  switch (Kind) {
#define TOKEN(X)                                                               \
  case tok::X:                                                                 \
    errs() << #X;                                                              \
    break;
#include "kaleidoscope/TokenKinds.def"
  case tok::NUM_TOKENS:
    errs() << "NUM_TOKENS (unset)";
    break;
  }
}
