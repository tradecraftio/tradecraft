/***********************************************************************
 * Copyright (c) 2014-2015 Pieter Wuille                               *
 * Copyright (c) 2018-2024 The Freicoin Developers                     *
 *                                                                     *
 * This program is free software: you can redistribute it and/or       *
 * modify it under the terms of version 3 of the GNU Affero General    *
 * Public License as published by the Free Software Foundation.        *
 *                                                                     *
 * This program is distributed in the hope that it will be useful,     *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of      *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU   *
 * Affero General Public License for more details.                     *
 *                                                                     *
 * You should have received a copy of the GNU Affero General Public    *
 * License along with this program.  If not, see                       *
 * <https://www.gnu.org/licenses/>.                                    *
 ***********************************************************************/

#ifndef SECP256K1_MODULE_RECOVERY_BENCH_H
#define SECP256K1_MODULE_RECOVERY_BENCH_H

#include "../../../include/secp256k1_recovery.h"

typedef struct {
    secp256k1_context *ctx;
    unsigned char msg[32];
    unsigned char sig[64];
} bench_recover_data;

static void bench_recover(void* arg, int iters) {
    int i;
    bench_recover_data *data = (bench_recover_data*)arg;
    secp256k1_pubkey pubkey;
    unsigned char pubkeyc[33];

    for (i = 0; i < iters; i++) {
        int j;
        size_t pubkeylen = 33;
        secp256k1_ecdsa_recoverable_signature sig;
        CHECK(secp256k1_ecdsa_recoverable_signature_parse_compact(data->ctx, &sig, data->sig, i % 2));
        CHECK(secp256k1_ecdsa_recover(data->ctx, &pubkey, &sig, data->msg));
        CHECK(secp256k1_ec_pubkey_serialize(data->ctx, pubkeyc, &pubkeylen, &pubkey, SECP256K1_EC_COMPRESSED));
        for (j = 0; j < 32; j++) {
            data->sig[j + 32] = data->msg[j];    /* Move former message to S. */
            data->msg[j] = data->sig[j];         /* Move former R to message. */
            data->sig[j] = pubkeyc[j + 1];       /* Move recovered pubkey X coordinate to R (which must be a valid X coordinate). */
        }
    }
}

static void bench_recover_setup(void* arg) {
    int i;
    bench_recover_data *data = (bench_recover_data*)arg;

    for (i = 0; i < 32; i++) {
        data->msg[i] = 1 + i;
    }
    for (i = 0; i < 64; i++) {
        data->sig[i] = 65 + i;
    }
}

static void run_recovery_bench(int iters, int argc, char** argv) {
    bench_recover_data data;
    int d = argc == 1;

    data.ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);

    if (d || have_flag(argc, argv, "ecdsa") || have_flag(argc, argv, "recover") || have_flag(argc, argv, "ecdsa_recover")) run_benchmark("ecdsa_recover", bench_recover, bench_recover_setup, NULL, &data, 10, iters);

    secp256k1_context_destroy(data.ctx);
}

#endif /* SECP256K1_MODULE_RECOVERY_BENCH_H */
