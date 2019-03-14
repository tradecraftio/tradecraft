/**********************************************************************
 * Copyright (c) 2014 Pieter Wuille                                   *
 * Copyright (c) 2018-2019 The Freicoin Developers                    *
 *                                                                    *
 * This program is free software: you can redistribute it and/or      *
 * modify it under the conjunctive terms of BOTH version 3 of the GNU *
 * Affero General Public License as published by the Free Software    *
 * Foundation AND the MIT/X11 software license.                       *
 *                                                                    *
 * This program is distributed in the hope that it will be useful,    *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of     *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU  *
 * Affero General Public License and the MIT/X11 software license for *
 * more details.                                                      *
 *                                                                    *
 * You should have received a copy of both licenses along with this   *
 * program.  If not, see <https://www.gnu.org/licenses/> and          *
 * <http://www.opensource.org/licenses/mit-license.php>               *
 **********************************************************************/

#include "include/secp256k1.h"
#include "util.h"
#include "bench.h"

typedef struct {
    secp256k1_context_t* ctx;
    unsigned char msg[32];
    unsigned char key[32];
} bench_sign_t;

static void bench_sign_setup(void* arg) {
    int i;
    bench_sign_t *data = (bench_sign_t*)arg;

    for (i = 0; i < 32; i++) data->msg[i] = i + 1;
    for (i = 0; i < 32; i++) data->key[i] = i + 65;
}

static void bench_sign(void* arg) {
    int i;
    bench_sign_t *data = (bench_sign_t*)arg;

    unsigned char sig[64];
    for (i = 0; i < 20000; i++) {
        int j;
        int recid = 0;
        CHECK(secp256k1_ecdsa_sign_compact(data->ctx, data->msg, sig, data->key, NULL, NULL, &recid));
        for (j = 0; j < 32; j++) {
            data->msg[j] = sig[j];             /* Move former R to message. */
            data->key[j] = sig[j + 32];        /* Move former S to key.     */
        }
    }
}

int main(void) {
    bench_sign_t data;

    data.ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN);

    run_benchmark("ecdsa_sign", bench_sign, bench_sign_setup, NULL, &data, 10, 20000);

    secp256k1_context_destroy(data.ctx);
    return 0;
}
