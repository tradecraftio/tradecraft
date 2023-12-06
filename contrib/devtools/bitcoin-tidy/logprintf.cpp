// Copyright (c) 2023 Bitcoin Developers
// Copyright (c) 2011-2023 The Freicoin Developers
//
// This program is free software: you can redistribute it and/or modify it under
// the terms of version 3 of the GNU Affero General Public License as published
// by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License for more
// details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "logprintf.h"

#include <clang/AST/ASTContext.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>


namespace {
AST_MATCHER(clang::StringLiteral, unterminated)
{
    size_t len = Node.getLength();
    if (len > 0 && Node.getCodeUnit(len - 1) == '\n') {
        return false;
    }
    return true;
}
} // namespace

namespace freicoin {

void LogPrintfCheck::registerMatchers(clang::ast_matchers::MatchFinder* finder)
{
    using namespace clang::ast_matchers;

    /*
      Logprintf(..., ..., ..., ..., ..., "foo", ...)
    */

    finder->addMatcher(
        callExpr(
            callee(functionDecl(hasName("LogPrintf_"))),
            hasArgument(5, stringLiteral(unterminated()).bind("logstring"))),
        this);

    /*
      auto walletptr = &wallet;
      wallet.WalletLogPrintf("foo");
      wallet->WalletLogPrintf("foo");
    */
    finder->addMatcher(
        cxxMemberCallExpr(
            callee(cxxMethodDecl(hasName("WalletLogPrintf"))),
            hasArgument(0, stringLiteral(unterminated()).bind("logstring"))),
        this);
}

void LogPrintfCheck::check(const clang::ast_matchers::MatchFinder::MatchResult& Result)
{
    if (const clang::StringLiteral* lit = Result.Nodes.getNodeAs<clang::StringLiteral>("logstring")) {
        const clang::ASTContext& ctx = *Result.Context;
        const auto user_diag = diag(lit->getEndLoc(), "Unterminated format string used with LogPrintf");
        const auto& loc = lit->getLocationOfByte(lit->getByteLength(), *Result.SourceManager, ctx.getLangOpts(), ctx.getTargetInfo());
        user_diag << clang::FixItHint::CreateInsertion(loc, "\\n");
    }
}

} // namespace freicoin
