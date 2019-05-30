//
// Created by Sergej Jaskiewicz on 2019-05-30.
//

#include "kaleidoscope/Lexer.h"
#include "llvm/ADT/ArrayRef.h"
#include "gtest/gtest.h"

using namespace kaleidoscope;
using namespace llvm;

// The test fixture.
class LexerTest : public testing::Test {
public:
  std::vector<Token> tokenize(StringRef Buf) {
    Lexer L(Buf.begin(), Buf.end());
    std::vector<Token> Tokens;
    do {
      Tokens.push_back(L.lex());
    } while (Tokens.back().isNot(tok::eof));
    return Tokens;
  }

  std::vector<Token> checkLex(StringRef Source, ArrayRef<tok> ExpectedTokens) {

    std::vector<Token> Toks = tokenize(Source);
    EXPECT_EQ(ExpectedTokens.size(), Toks.size());

    for (unsigned i = 0, e = ExpectedTokens.size(); i != e; ++i) {
      EXPECT_EQ(ExpectedTokens[i], Toks[i].getKind()) << "i = " << i;
    }

    return Toks;
  }
};

TEST_F(LexerTest, TokenizeSkipComments) {
  const char *Source = "# Comment\n"
                       "(1)";
  std::vector<tok> ExpectedTokens{tok::l_paren, tok::floating_literal,
                                  tok::r_paren, tok::eof};
  checkLex(Source, ExpectedTokens);
}

TEST_F(LexerTest, EOFTokenLengthIsZero) {
  const char *Source = "meow";
  std::vector<tok> ExpectedTokens{ tok::identifier, tok::eof };
  std::vector<Token> Toks = checkLex(Source, ExpectedTokens);
  EXPECT_EQ(Toks[1].getLength(), 0U);
}
