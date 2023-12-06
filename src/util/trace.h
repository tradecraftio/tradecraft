// Copyright (c) 2020-2021 The Bitcoin Core developers
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

#ifndef FREICOIN_UTIL_TRACE_H
#define FREICOIN_UTIL_TRACE_H

#if defined(HAVE_CONFIG_H)
#include <config/freicoin-config.h>
#endif

#ifdef ENABLE_TRACING

#include <sys/sdt.h>

// Disabling this warning can be removed once we switch to C++20
#if defined(__clang__) && __cplusplus < 202002L
#define FREICOIN_DISABLE_WARN_ZERO_VARIADIC_PUSH _Pragma("clang diagnostic push") _Pragma("clang diagnostic ignored \"-Wgnu-zero-variadic-macro-arguments\"")
#define FREICOIN_DISABLE_WARN_ZERO_VARIADIC_POP _Pragma("clang diagnostic pop")
#else
#define FREICOIN_DISABLE_WARN_ZERO_VARIADIC_PUSH
#define FREICOIN_DISABLE_WARN_ZERO_VARIADIC_POP
#endif

#define TRACE(context, event) FREICOIN_DISABLE_WARN_ZERO_VARIADIC_PUSH DTRACE_PROBE(context, event) FREICOIN_DISABLE_WARN_ZERO_VARIADIC_POP
#define TRACE1(context, event, a) FREICOIN_DISABLE_WARN_ZERO_VARIADIC_PUSH DTRACE_PROBE1(context, event, a) FREICOIN_DISABLE_WARN_ZERO_VARIADIC_POP
#define TRACE2(context, event, a, b) FREICOIN_DISABLE_WARN_ZERO_VARIADIC_PUSH DTRACE_PROBE2(context, event, a, b) FREICOIN_DISABLE_WARN_ZERO_VARIADIC_POP
#define TRACE3(context, event, a, b, c) FREICOIN_DISABLE_WARN_ZERO_VARIADIC_PUSH DTRACE_PROBE3(context, event, a, b, c) FREICOIN_DISABLE_WARN_ZERO_VARIADIC_POP
#define TRACE4(context, event, a, b, c, d) FREICOIN_DISABLE_WARN_ZERO_VARIADIC_PUSH DTRACE_PROBE4(context, event, a, b, c, d) FREICOIN_DISABLE_WARN_ZERO_VARIADIC_POP
#define TRACE5(context, event, a, b, c, d, e) FREICOIN_DISABLE_WARN_ZERO_VARIADIC_PUSH DTRACE_PROBE5(context, event, a, b, c, d, e) FREICOIN_DISABLE_WARN_ZERO_VARIADIC_POP
#define TRACE6(context, event, a, b, c, d, e, f) FREICOIN_DISABLE_WARN_ZERO_VARIADIC_PUSH DTRACE_PROBE6(context, event, a, b, c, d, e, f) FREICOIN_DISABLE_WARN_ZERO_VARIADIC_POP
#define TRACE7(context, event, a, b, c, d, e, f, g) FREICOIN_DISABLE_WARN_ZERO_VARIADIC_PUSH DTRACE_PROBE7(context, event, a, b, c, d, e, f, g) FREICOIN_DISABLE_WARN_ZERO_VARIADIC_POP
#define TRACE8(context, event, a, b, c, d, e, f, g, h) FREICOIN_DISABLE_WARN_ZERO_VARIADIC_PUSH DTRACE_PROBE8(context, event, a, b, c, d, e, f, g, h) FREICOIN_DISABLE_WARN_ZERO_VARIADIC_POP
#define TRACE9(context, event, a, b, c, d, e, f, g, h, i) FREICOIN_DISABLE_WARN_ZERO_VARIADIC_PUSH DTRACE_PROBE9(context, event, a, b, c, d, e, f, g, h, i) FREICOIN_DISABLE_WARN_ZERO_VARIADIC_POP
#define TRACE10(context, event, a, b, c, d, e, f, g, h, i, j) FREICOIN_DISABLE_WARN_ZERO_VARIADIC_PUSH DTRACE_PROBE10(context, event, a, b, c, d, e, f, g, h, i, j) FREICOIN_DISABLE_WARN_ZERO_VARIADIC_POP
#define TRACE11(context, event, a, b, c, d, e, f, g, h, i, j, k) FREICOIN_DISABLE_WARN_ZERO_VARIADIC_PUSH DTRACE_PROBE11(context, event, a, b, c, d, e, f, g, h, i, j, k) FREICOIN_DISABLE_WARN_ZERO_VARIADIC_POP
#define TRACE12(context, event, a, b, c, d, e, f, g, h, i, j, k, l) FREICOIN_DISABLE_WARN_ZERO_VARIADIC_PUSH DTRACE_PROBE12(context, event, a, b, c, d, e, f, g, h, i, j, k, l) FREICOIN_DISABLE_WARN_ZERO_VARIADIC_POP

#else

#define TRACE(context, event)
#define TRACE1(context, event, a)
#define TRACE2(context, event, a, b)
#define TRACE3(context, event, a, b, c)
#define TRACE4(context, event, a, b, c, d)
#define TRACE5(context, event, a, b, c, d, e)
#define TRACE6(context, event, a, b, c, d, e, f)
#define TRACE7(context, event, a, b, c, d, e, f, g)
#define TRACE8(context, event, a, b, c, d, e, f, g, h)
#define TRACE9(context, event, a, b, c, d, e, f, g, h, i)
#define TRACE10(context, event, a, b, c, d, e, f, g, h, i, j)
#define TRACE11(context, event, a, b, c, d, e, f, g, h, i, j, k)
#define TRACE12(context, event, a, b, c, d, e, f, g, h, i, j, k, l)

#endif


#endif // FREICOIN_UTIL_TRACE_H
