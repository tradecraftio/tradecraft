// Copyright (c) 2023 Bitcoin Developers
// Copyright (c) 2011-2024 The Freicoin Developers
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

#include "nontrivial-threadlocal.h"

#include <clang/AST/ASTContext.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>


// Copied from clang-tidy's UnusedRaiiCheck
namespace {
AST_MATCHER(clang::CXXRecordDecl, hasNonTrivialDestructor) {
    // TODO: If the dtor is there but empty we don't want to warn either.
    return Node.hasDefinition() && Node.hasNonTrivialDestructor();
}
} // namespace

namespace bitcoin {

void NonTrivialThreadLocal::registerMatchers(clang::ast_matchers::MatchFinder* finder)
{
    using namespace clang::ast_matchers;

    /*
      thread_local std::string foo;
    */

    finder->addMatcher(
        varDecl(
            hasThreadStorageDuration(),
            hasType(hasCanonicalType(recordType(hasDeclaration(cxxRecordDecl(hasNonTrivialDestructor())))))
        ).bind("nontrivial_threadlocal"),
        this);
}

void NonTrivialThreadLocal::check(const clang::ast_matchers::MatchFinder::MatchResult& Result)
{
    if (const clang::VarDecl* var = Result.Nodes.getNodeAs<clang::VarDecl>("nontrivial_threadlocal")) {
        const auto user_diag = diag(var->getBeginLoc(), "Variable with non-trivial destructor cannot be thread_local.");
    }
}

} // namespace bitcoin
