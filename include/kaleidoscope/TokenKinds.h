//
// Created by Sergej Jaskiewicz on 2019-05-28.
//

#ifndef KALEIDOSCOPE_TOKENKINDS_H
#define KALEIDOSCOPE_TOKENKINDS_H

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"

namespace kaleidoscope {
enum class tok {
#define TOKEN(X) X,
#include "kaleidoscope/TokenKinds.def"

  NUM_TOKENS
};

/// Check whether a token kind is known to have any specific text content.
/// e.g., tol::l_paren has determined text however tok::identifier doesn't.
bool isTokenTextDetermined(tok Kind);

/// If a token kind has determined text, return the text; otherwise assert.
llvm::StringRef getTokenText(tok Kind);

void dumpTokenKind(tok Kind);
} // end namespace swift

#endif /* KALEIDOSCOPE_TOKENKINDS_H */
