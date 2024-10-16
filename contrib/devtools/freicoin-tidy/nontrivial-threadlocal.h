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

#ifndef NONTRIVIAL_THREADLOCAL_CHECK_H
#define NONTRIVIAL_THREADLOCAL_CHECK_H

#include <clang-tidy/ClangTidyCheck.h>

namespace bitcoin {

// Warn about any thread_local variable with a non-trivial destructor.
class NonTrivialThreadLocal final : public clang::tidy::ClangTidyCheck
{
public:
    NonTrivialThreadLocal(clang::StringRef Name, clang::tidy::ClangTidyContext* Context)
        : clang::tidy::ClangTidyCheck(Name, Context) {}

    bool isLanguageVersionSupported(const clang::LangOptions& LangOpts) const override
    {
        return LangOpts.CPlusPlus;
    }
    void registerMatchers(clang::ast_matchers::MatchFinder* Finder) override;
    void check(const clang::ast_matchers::MatchFinder::MatchResult& Result) override;
};

} // namespace bitcoin

#endif // NONTRIVIAL_THREADLOCAL_CHECK_H
