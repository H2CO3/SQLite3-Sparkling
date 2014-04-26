/*
 * spn_sqlite3.h
 * Sparkling bindings for sqlite3
 *
 * Created by Arpad Goretity
 * on 26/04/2014
 *
 * licensed under the 2-clause BSD License
 */

#ifndef SPN_SQLITE3_H
#define SPN_SQLITE3_H

#include <spn/ctx.h>

#define SPN_LIBSIZE_SQLITE3 9

SPN_API const SpnExtFunc spnlib_sqlite3[SPN_LIBSIZE_SQLITE3];
SPN_API void spnlib_load_sqlite3(SpnContext *ctx);

#endif /* SPN_SQLITE3_H */
