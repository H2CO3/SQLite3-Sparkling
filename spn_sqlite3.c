/*
 * spn_sqlite3.c
 * Sparkling bindings for sqlite3
 *
 * Created by Arpad Goretity
 * on 26/04/2014
 *
 * licensed under the 2-clause BSD License
 */

#include <sqlite3.h>
#include "spn_sqlite3.h"


static int spnlib_sqlite3_open(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	SpnString *fname;
	sqlite3 *db;

	if (argc != 1) {
		spn_ctx_runtime_error(ctx, "expecting one argument", NULL);
		return -1;
	}

	if (!spn_isstring(&argv[0])) {
		spn_ctx_runtime_error(ctx, "argument must be a file name", NULL);
		return -2;
	}

	fname = spn_stringvalue(&argv[0]);

	if (sqlite3_open(fname->cstr, &db) != SQLITE_OK) {
		return 0; /* return nil implicitly */
	}

	/* return database handle as a user info object */
	*ret = spn_makeweakuserinfo(db);
	return 0;
}

static int spnlib_sqlite3_close(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	sqlite3 *db;

	if (argc != 1) {
		spn_ctx_runtime_error(ctx, "expecting one argument", NULL);
		return -1;
	}

	if (!spn_isweakuserinfo(&argv[0])) {
		spn_ctx_runtime_error(ctx, "argument must be an SQLite3 handle", NULL);
		return -2;
	}

	db = spn_ptrvalue(&argv[0]);
	sqlite3_close(db);
	return 0;
}

static int spnlib_sqlite3_prepare(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	sqlite3 *db;
	SpnString *query;
	sqlite3_stmt *stmt;


	if (argc != 2) {
		spn_ctx_runtime_error(ctx, "expecting two arguments", NULL);
		return -1;
	}

	if (!spn_isweakuserinfo(&argv[0])) {
		spn_ctx_runtime_error(ctx, "first argument must be an SQLite3 handle", NULL);
		return -2;
	}

	if (!spn_isstring(&argv[1])) {
		spn_ctx_runtime_error(ctx, "second argument must be a query string", NULL);
		return -3;
	}

	db = spn_ptrvalue(&argv[0]);
	query = spn_stringvalue(&argv[1]);

	if (sqlite3_prepare_v2(db, query->cstr, query->len, &stmt, NULL) != SQLITE_OK) {
		return 0; /* implicitly return nil */
	}

	/* return prepared statement */
	*ret = spn_makeweakuserinfo(stmt);
	return 0;
}

static int spnlib_sqlite3_finalize(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	sqlite3_stmt *stmt;

	if (argc != 1) {
		spn_ctx_runtime_error(ctx, "expecting one argument", NULL);
		return -1;
	}

	if (!spn_isweakuserinfo(&argv[0])) {
		spn_ctx_runtime_error(ctx, "argument must be a prepared statement", NULL);
		return -2;
	}

	stmt = spn_ptrvalue(&argv[0]);
	sqlite3_finalize(stmt);
	return 0;
}

static int spnlib_sqlite3_bind(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	sqlite3_stmt *stmt;
	int parm_idx;

	SpnValue *idx_val, *parm_val;
	int status;

	const void *args[1]; /* for error reporting */

	/* typecheck arguments */
	if (argc != 3) {
		spn_ctx_runtime_error(ctx, "expecting 3 arguments", NULL);
		return -1;
	}

	if (!spn_isweakuserinfo(&argv[0])) {
		spn_ctx_runtime_error(ctx, "1st argument must be a statement", NULL);
		return -2;
	}

	stmt = spn_ptrvalue(&argv[0]);
	idx_val = &argv[1];

	if (spn_isint(idx_val)) {
		parm_idx = spn_intvalue(idx_val);
	} else if (spn_isstring(idx_val)) {
		SpnString *parm_name = spn_stringvalue(idx_val);
		parm_idx = sqlite3_bind_parameter_index(stmt, parm_name->cstr);
	} else {
		spn_ctx_runtime_error(
			ctx,
			"2nd argument must be a parameter index or name",
			NULL
		);
		return -4;
	}

	/* actually perform the binding */
	parm_val = &argv[2];

	switch (spn_valtype(parm_val)) {
	case SPN_TTAG_NIL:	{
		status = sqlite3_bind_null(stmt, parm_idx);
		break;
	}
	case SPN_TTAG_BOOL:	{
		status = sqlite3_bind_int(stmt, parm_idx, spn_boolvalue(parm_val));
		break;
	}
	case SPN_TTAG_NUMBER:	{
		if (spn_isint(parm_val)) {
			long n = spn_intvalue(parm_val);
			status = sqlite3_bind_int64(stmt, parm_idx, n);
		} else {
			double x = spn_floatvalue(parm_val);
			status = sqlite3_bind_double(stmt, parm_idx, x);
		}
		break;
	}
	case SPN_TTAG_STRING:	{
		SpnString *str = spn_stringvalue(parm_val);
		status = sqlite3_bind_text(stmt, parm_idx, str->cstr, str->len, SQLITE_TRANSIENT);
		break;
	}
	case SPN_TTAG_ARRAY:	/* fallthru */
	case SPN_TTAG_USERINFO:	/* fallthru */
	default:
		args[0] = spn_type_name(parm_val->type);
		spn_ctx_runtime_error(ctx, "cannot bind value of type %s", args);
		return -5;
		break;
	}


	*ret = spn_makebool(status == SQLITE_OK);
	return 0;
}

static int spnlib_sqlite3_row(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	sqlite3_stmt *stmt;
	int isassoc;
	int status;

	if (argc != 2) {
		spn_ctx_runtime_error(ctx, "expecting two arguments", NULL);
		return -1;
	}

	if (!spn_isweakuserinfo(&argv[0])) {
		spn_ctx_runtime_error(ctx, "1st argument must be a prepared statement", NULL);
		return -2;
	}

	if (!spn_isbool(&argv[1])) {
		spn_ctx_runtime_error(ctx, "2nd argument must be a boolean", NULL);
		return -3;
	}

	stmt = spn_ptrvalue(&argv[0]);
	isassoc = spn_boolvalue(&argv[1]);
	status = sqlite3_step(stmt);

	/* if there is data available, return it */
	if (status == SQLITE_ROW) {
		int n_cols = sqlite3_column_count(stmt);
		SpnArray *column = spn_array_new();

		int i;
		for (i = 0; i < n_cols; i++) {
			const char *col_name = sqlite3_column_name(stmt, i);
			int col_type = sqlite3_column_type(stmt, i);
			SpnValue col_value;

			switch (col_type) {
			case SQLITE_NULL:	{
				col_value = spn_makenil();
				break;
			}
			case SQLITE_INTEGER:	{
				sqlite3_int64 n = sqlite3_column_int64(stmt, i);
				col_value = spn_makeint(n);
				break;
			}
			case SQLITE_FLOAT:	{
				double x = sqlite3_column_double(stmt, i);
				col_value = spn_makefloat(x);
				break;
			}
			case SQLITE_TEXT:	{
				/* however hard I tried, casts were ugly */
				const unsigned char *utext = sqlite3_column_text(stmt, i);
				const void *p = utext;
				const char *cstr = p;

				int len = sqlite3_column_bytes(stmt, i);
				col_value.type = SPN_TYPE_STRING;
				col_value.v.o = spn_string_new_len(cstr, len);
				break;
			}
			case SQLITE_BLOB:	{
				const void *blob = sqlite3_column_blob(stmt, i);
				int len = sqlite3_column_bytes(stmt, i);
				col_value.type = SPN_TYPE_STRING;
				col_value.v.o = spn_string_new_len(blob, len);
				break;
			}
			default: break; /* unreachable */
			}

			if (isassoc) {
				spn_array_set_strkey(column, col_name, &col_value);
			} else {
				spn_array_set_intkey(column, i, &col_value);
			}

			spn_value_release(&col_value);
		}

		ret->type = SPN_TYPE_ARRAY;
		ret->v.o = column;
	}

	/* else implicitly return nil */
	return 0;
}

static int spnlib_sqlite3_reset(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	sqlite3_stmt *stmt;

	if (argc != 1) {
		spn_ctx_runtime_error(ctx, "expecting one argument", NULL);
		return -1;
	}

	if (!spn_isweakuserinfo(&argv[0])) {
		spn_ctx_runtime_error(ctx, "argument must be a prepared statement", NULL);
		return -2;
	}

	stmt = spn_ptrvalue(&argv[0]);
	sqlite3_reset(stmt);
	return 0;
}

static int spnlib_sqlite3_errcode(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	sqlite3 *db;
	int errcode;

	if (argc != 1) {
		spn_ctx_runtime_error(ctx, "expecting one argument", NULL);
		return -1;
	}

	if (!spn_isweakuserinfo(&argv[0])) {
		spn_ctx_runtime_error(ctx, "argument must be an SQLite3 handle", NULL);
		return -2;
	}

	db = spn_ptrvalue(&argv[0]);
	errcode = sqlite3_errcode(db);
	*ret = spn_makeint(errcode);
	return 0;
}

static int spnlib_sqlite3_errmsg(SpnValue *ret, int argc, SpnValue *argv, void *ctx)
{
	sqlite3 *db;
	const char *errmsg;

	if (argc != 1) {
		spn_ctx_runtime_error(ctx, "expecting one argument", NULL);
		return -1;
	}

	if (!spn_isweakuserinfo(&argv[0])) {
		spn_ctx_runtime_error(ctx, "argument must be an SQLite3 handle", NULL);
		return -2;
	}

	db = spn_ptrvalue(&argv[0]);
	errmsg = sqlite3_errmsg(db);
	*ret = spn_makestring(errmsg);
	return 0;
}

const SpnExtFunc spnlib_sqlite3[SPN_LIBSIZE_SQLITE3] = {
	{ "open",	spnlib_sqlite3_open	},
	{ "close",	spnlib_sqlite3_close	},
	{ "prepare",	spnlib_sqlite3_prepare	},
	{ "finalize",	spnlib_sqlite3_finalize	},
	{ "bind",	spnlib_sqlite3_bind	},
	{ "row",	spnlib_sqlite3_row	},
	{ "reset",	spnlib_sqlite3_reset	},
	{ "errcode",	spnlib_sqlite3_errcode	},
	{ "errmsg",	spnlib_sqlite3_errmsg	}
};

void spnlib_load_sqlite3(SpnContext *ctx)
{
	spn_ctx_addlib_cfuncs(ctx, "sqlite3", spnlib_sqlite3, SPN_LIBSIZE_SQLITE3);
}
