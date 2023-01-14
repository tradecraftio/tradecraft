// Copyright (c) 2022 The Bitcoin Core developers
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

#include <kernel/checks.h>

#include <key.h>
#include <random.h>
#include <util/time.h>
#include <util/translation.h>

#include <memory>

namespace kernel {

std::optional<bilingual_str> SanityChecks(const Context&)
{
    if (!ECC_InitSanityCheck()) {
        return Untranslated("Elliptic curve cryptography sanity check failure. Aborting.");
    }

    if (!Random_SanityCheck()) {
        return Untranslated("OS cryptographic RNG sanity check failure. Aborting.");
    }

    if (!ChronoSanityCheck()) {
        return Untranslated("Clock epoch mismatch. Aborting.");
    }

    return std::nullopt;
}

}
