// Copyright 2014 BitPay Inc.
// Copyright (c) 2011-2021 The Freicoin Developers
//
// This program is free software: you can redistribute it and/or
// modify it under the conjunctive terms of BOTH version 3 of the GNU
// Affero General Public License as published by the Free Software
// Foundation AND the MIT/X11 software license.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Affero General Public License and the MIT/X11 software license for
// more details.
//
// You should have received a copy of both licenses along with this
// program.  If not, see <https://www.gnu.org/licenses/> and
// <http://www.opensource.org/licenses/mit-license.php>

//
// To re-create univalue_escapes.h:
// $ g++ -o gen gen.cpp
// $ ./gen > univalue_escapes.h
//

#include <stdio.h>
#include <string.h>
#include "univalue.h"

using namespace std;

static bool initEscapes;
static std::string escapes[256];

static void initJsonEscape()
{
    // Escape all lower control characters (some get overridden with smaller sequences below)
    for (int ch=0x00; ch<0x20; ++ch) {
        char tmpbuf[20];
        snprintf(tmpbuf, sizeof(tmpbuf), "\\u%04x", ch);
        escapes[ch] = std::string(tmpbuf);
    }

    escapes[(int)'"'] = "\\\"";
    escapes[(int)'\\'] = "\\\\";
    escapes[(int)'\b'] = "\\b";
    escapes[(int)'\f'] = "\\f";
    escapes[(int)'\n'] = "\\n";
    escapes[(int)'\r'] = "\\r";
    escapes[(int)'\t'] = "\\t";
    escapes[(int)'\x7f'] = "\\u007f"; // U+007F DELETE

    initEscapes = true;
}

static void outputEscape()
{
	printf(	"// Automatically generated file. Do not modify.\n"
		"#ifndef FREICOIN_UNIVALUE_UNIVALUE_ESCAPES_H\n"
		"#define FREICOIN_UNIVALUE_UNIVALUE_ESCAPES_H\n"
		"static const char *escapes[256] = {\n");

	for (unsigned int i = 0; i < 256; i++) {
		if (escapes[i].empty()) {
			printf("\tNULL,\n");
		} else {
			printf("\t\"");

			unsigned int si;
			for (si = 0; si < escapes[i].size(); si++) {
				char ch = escapes[i][si];
				switch (ch) {
				case '"':
					printf("\\\"");
					break;
				case '\\':
					printf("\\\\");
					break;
				default:
					printf("%c", escapes[i][si]);
					break;
				}
			}

			printf("\",\n");
		}
	}

	printf(	"};\n"
		"#endif // FREICOIN_UNIVALUE_UNIVALUE_ESCAPES_H\n");
}

int main (int argc, char *argv[])
{
	initJsonEscape();
	outputEscape();
	return 0;
}

