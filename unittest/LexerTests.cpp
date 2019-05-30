//
// Created by Sergej Jaskiewicz on 2019-05-30.
//

#include "kaleidoscope/Lexer.h"
#include "llvm/ADT/ArrayRef.h"
#include "gtest/gtest.h"

using namespace kaleidoscope;
using namespace llvm;

static void diagnosticHandler(const SMDiagnostic &Diagnostic, void *Context) {
  if (Context) {
    static_cast<std::vector<SMDiagnostic> *>(Context)->push_back(Diagnostic);
  }
}

// The test fixture.
class LexerTest : public testing::Test {
public:

  SourceManager SourceMgr;

  std::vector<Token> tokenize(unsigned BufferID) {
    Lexer L(SourceMgr, BufferID);
    std::vector<Token> Tokens;
    do {
      Tokens.push_back(L.lex());
    } while (Tokens.back().isNot(tok::eof));
    return Tokens;
  }

  void collectDiagnostics(std::vector<SMDiagnostic>& Diags) {
    SourceMgr.getLLVMSourceMgr().setDiagHandler(diagnosticHandler, &Diags);
  }

  std::vector<Token> checkLex(StringRef Source, ArrayRef<tok> ExpectedTokens) {

    unsigned BufID = SourceMgr.addMemBufferCopy(Source);

    std::vector<Token> Toks = tokenize(BufID);
    EXPECT_EQ(ExpectedTokens.size(), Toks.size());

    for (unsigned i = 0, e = ExpectedTokens.size(); i != e; ++i) {
      EXPECT_EQ(ExpectedTokens[i], Toks[i].getKind()) << "i = " << i;
    }

    return Toks;
  }
};

TEST_F(LexerTest, TokenizeSkipComments) {
  StringRef Source = "# Comment\n"
                       "(1)";
  std::vector<tok> ExpectedTokens{tok::l_paren, tok::floating_literal,
                                  tok::r_paren, tok::eof};
  checkLex(Source, ExpectedTokens);
}

TEST_F(LexerTest, EOFTokenLengthIsZero) {
  StringRef Source = "meow";
  std::vector<tok> ExpectedTokens{ tok::identifier, tok::eof };
  std::vector<Token> Toks = checkLex(Source, ExpectedTokens);
  EXPECT_EQ(Toks[1].getLength(), 0);
}

TEST_F(LexerTest, PrefixOperator) {
  StringRef Source = "  \t+a";
  std::vector<tok> ExpectedTokens{tok::prefix_operator, tok::identifier,
                                  tok::eof};
  std::vector<Token> Toks = checkLex(Source, ExpectedTokens);
  EXPECT_EQ("+", Toks[0].getText());
}

TEST_F(LexerTest, PostfixOperator) {
  StringRef Source = "123!=#hello";
  std::vector<tok> ExpectedTokens{tok::floating_literal, tok::postfix_operator,
                                  tok::eof};
  std::vector<Token> Toks = checkLex(Source, ExpectedTokens);
  EXPECT_EQ("!=", Toks[1].getText());
}

TEST_F(LexerTest, InfixOperatorSpaced) {
  StringRef Source = "123 != extern";
  std::vector<tok> ExpectedTokens{tok::floating_literal, tok::infix_operator,
                                  tok::kw_extern, tok::eof};
  std::vector<Token> Toks = checkLex(Source, ExpectedTokens);
  EXPECT_EQ("!=", Toks[1].getText());
}

TEST_F(LexerTest, InfixOperatorNonSpaced) {
  StringRef Source = "123!=extern";
  std::vector<tok> ExpectedTokens{tok::floating_literal, tok::infix_operator,
                                  tok::kw_extern, tok::eof};
  std::vector<Token> Toks = checkLex(Source, ExpectedTokens);
  EXPECT_EQ("!=", Toks[1].getText());
}

TEST_F(LexerTest, UnexpectedToken) {
  StringRef Source = "123!=\x80";
  std::vector<SMDiagnostic> Diags;
  collectDiagnostics(Diags);
  std::vector<tok> ExpectedTokens{tok::floating_literal, tok::infix_operator,
                                  tok::unknown, tok::eof};
  checkLex(Source, ExpectedTokens);

  ASSERT_EQ(Diags.size(), 1);
  EXPECT_STREQ(Diags[0].getLoc().getPointer(), Source.begin() + 5);
  EXPECT_EQ(Diags[0].getKind(), SourceMgr::DK_Error);
}

TEST_F(LexerTest, TokenizePlaceholder) {
  StringRef Source = "aa <#one#> bb <# two #>";
  std::vector<SMDiagnostic> Diags;
  collectDiagnostics(Diags);
  std::vector<tok> ExpectedTokens{tok::identifier, tok::identifier,
                                  tok::identifier, tok::identifier, tok::eof};
  std::vector<Token> Toks = checkLex(Source, ExpectedTokens);

  EXPECT_EQ("aa", Toks[0].getText());
  EXPECT_EQ("<#one#>", Toks[1].getText());
  EXPECT_EQ("bb", Toks[2].getText());
  EXPECT_EQ("<# two #>", Toks[3].getText());

  ASSERT_EQ(Diags.size(), 2);
  EXPECT_STREQ(Diags[0].getLoc().getPointer(), Source.begin() + 3);
  EXPECT_EQ(Diags[0].getKind(), SourceMgr::DK_Error);
  EXPECT_STREQ(Diags[1].getLoc().getPointer(), Source.begin() + 14);
  EXPECT_EQ(Diags[1].getKind(), SourceMgr::DK_Error);
}

TEST_F(LexerTest, NoPlaceholder) {

  auto checkTok = [&](StringRef Source) {
    unsigned BufID = SourceMgr.addMemBufferCopy(Source);
    std::vector<Token> Toks = tokenize(BufID);
    ASSERT_FALSE(Toks.empty());
    EXPECT_NE(tok::identifier, Toks[0].getKind());
  };

  checkTok("<#");
  checkTok("<#a#");
  checkTok("<#a\n#>");
  checkTok("< #a#>");
}

TEST_F(LexerTest, NestedPlaceholder) {
  const char *Source = "<#<#aa#>#>";

  std::vector<tok> ExpectedTokens{tok::infix_operator, tok::eof};
  std::vector<Token> Toks = checkLex(Source, ExpectedTokens);
  EXPECT_EQ("<", Toks[0].getText());
}