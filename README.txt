Minimalistic SQLite3 bindings for Sparkling
-------------------------------------------

All functions are in the sqlite3 "namespace" (i. e. global array).
Also refer to the SQLite3 C API to get a grasp as to how to use
the following functions.

    userinfo open(string filename): opens specified database file.
    returns connection handle on succes, nil on error


    close(userinfo db): closes the handle returned by sqlite3.open()

    userinfo prepare(userinfo db, string query): compiles a
    prepared statement out of the query string. placeholders
    such as '?' may be used (in the same manner that the SQLite3
    C API specifies it). Returns prepared statement, or nil on error.

    finalize(userinfo stmt): destroys a prepared statement
    after use. statement may not be used subsequently.


    bool bind(userinfo stmt, [integer | string] index, value):
    binds to host parameter at index 'index' (index may be a string
    in which case it's assumed to be a parameter name, not an index) the
    value 'value'. 'value' may be of type nil, boolean, number or string.
    binding arrays and user info values is not supported.
    Returns true on success, false on failure.

    array row(userinfo stmt, bool isAssociative): returns
    the result of the statement (if any). Returns empty array if
    the statement doesn't generate results (i. e. "INSERT").
    If done, or on error, returns nil.
    If 'isAssociative' is true, then uses column names as keys
    in the returned row array, order of columns won't be preserved.
    Else it uses integer indices as keys; column names won't be returned.

    reset(userinfo stmt): resets the 'stmt' statement to its
    initial state so it can be executed again using 'sqlite3.row()'.

    int errcode(userinfo db): returns the error code set by
    the last operation on the database.

    string errmsg(userinfo db): returns the error message
    associated with the last error.
