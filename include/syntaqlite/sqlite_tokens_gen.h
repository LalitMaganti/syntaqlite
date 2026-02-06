/*
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
** Token definitions from SQLite's parse.y via Lemon.
** DO NOT EDIT - regenerate with: python3 python/tools/extract_sqlite.py
*/
#ifndef SYNTAQLITE_SQLITE_TOKENS_GEN_H
#define SYNTAQLITE_SQLITE_TOKENS_GEN_H

#ifdef SYNTAQLITE_CUSTOM_TOKENS
#include SYNTAQLITE_CUSTOM_TOKENS
#else

#define SYNTAQLITE_TOKEN_INDEX                            1
#define SYNTAQLITE_TOKEN_SEMI                             2
#define SYNTAQLITE_TOKEN_ALTER                            3
#define SYNTAQLITE_TOKEN_EXPLAIN                          4
#define SYNTAQLITE_TOKEN_ADD                              5
#define SYNTAQLITE_TOKEN_QUERY                            6
#define SYNTAQLITE_TOKEN_WINDOW                           7
#define SYNTAQLITE_TOKEN_PLAN                             8
#define SYNTAQLITE_TOKEN_OVER                             9
#define SYNTAQLITE_TOKEN_BEGIN                           10
#define SYNTAQLITE_TOKEN_FILTER                          11
#define SYNTAQLITE_TOKEN_TRANSACTION                     12
#define SYNTAQLITE_TOKEN_COLUMN                          13
#define SYNTAQLITE_TOKEN_DEFERRED                        14
#define SYNTAQLITE_TOKEN_AGG_FUNCTION                    15
#define SYNTAQLITE_TOKEN_IMMEDIATE                       16
#define SYNTAQLITE_TOKEN_AGG_COLUMN                      17
#define SYNTAQLITE_TOKEN_EXCLUSIVE                       18
#define SYNTAQLITE_TOKEN_TRUEFALSE                       19
#define SYNTAQLITE_TOKEN_COMMIT                          20
#define SYNTAQLITE_TOKEN_FUNCTION                        21
#define SYNTAQLITE_TOKEN_END                             22
#define SYNTAQLITE_TOKEN_UPLUS                           23
#define SYNTAQLITE_TOKEN_ROLLBACK                        24
#define SYNTAQLITE_TOKEN_UMINUS                          25
#define SYNTAQLITE_TOKEN_SAVEPOINT                       26
#define SYNTAQLITE_TOKEN_TRUTH                           27
#define SYNTAQLITE_TOKEN_RELEASE                         28
#define SYNTAQLITE_TOKEN_REGISTER                        29
#define SYNTAQLITE_TOKEN_TO                              30
#define SYNTAQLITE_TOKEN_VECTOR                          31
#define SYNTAQLITE_TOKEN_TABLE                           32
#define SYNTAQLITE_TOKEN_SELECT_COLUMN                   33
#define SYNTAQLITE_TOKEN_CREATE                          34
#define SYNTAQLITE_TOKEN_IF_NULL_ROW                     35
#define SYNTAQLITE_TOKEN_IF                              36
#define SYNTAQLITE_TOKEN_ASTERISK                        37
#define SYNTAQLITE_TOKEN_NOT                             38
#define SYNTAQLITE_TOKEN_SPAN                            39
#define SYNTAQLITE_TOKEN_EXISTS                          40
#define SYNTAQLITE_TOKEN_ERROR                           41
#define SYNTAQLITE_TOKEN_TEMP                            42
#define SYNTAQLITE_TOKEN_QNUMBER                         43
#define SYNTAQLITE_TOKEN_LP                              44
#define SYNTAQLITE_TOKEN_SPACE                           45
#define SYNTAQLITE_TOKEN_RP                              46
#define SYNTAQLITE_TOKEN_COMMENT                         47
#define SYNTAQLITE_TOKEN_AS                              48
#define SYNTAQLITE_TOKEN_ILLEGAL                         49
#define SYNTAQLITE_TOKEN_COMMA                           50
#define SYNTAQLITE_TOKEN_WITHOUT                         51
#define SYNTAQLITE_TOKEN_ABORT                           52
#define SYNTAQLITE_TOKEN_ACTION                          53
#define SYNTAQLITE_TOKEN_AFTER                           54
#define SYNTAQLITE_TOKEN_ANALYZE                         55
#define SYNTAQLITE_TOKEN_ASC                             56
#define SYNTAQLITE_TOKEN_ATTACH                          57
#define SYNTAQLITE_TOKEN_BEFORE                          58
#define SYNTAQLITE_TOKEN_BY                              59
#define SYNTAQLITE_TOKEN_CASCADE                         60
#define SYNTAQLITE_TOKEN_CAST                            61
#define SYNTAQLITE_TOKEN_CONFLICT                        62
#define SYNTAQLITE_TOKEN_DATABASE                        63
#define SYNTAQLITE_TOKEN_DESC                            64
#define SYNTAQLITE_TOKEN_DETACH                          65
#define SYNTAQLITE_TOKEN_EACH                            66
#define SYNTAQLITE_TOKEN_FAIL                            67
#define SYNTAQLITE_TOKEN_OR                              68
#define SYNTAQLITE_TOKEN_AND                             69
#define SYNTAQLITE_TOKEN_IS                              70
#define SYNTAQLITE_TOKEN_ISNOT                           71
#define SYNTAQLITE_TOKEN_MATCH                           72
#define SYNTAQLITE_TOKEN_LIKE_KW                         73
#define SYNTAQLITE_TOKEN_BETWEEN                         74
#define SYNTAQLITE_TOKEN_IN                              75
#define SYNTAQLITE_TOKEN_ISNULL                          76
#define SYNTAQLITE_TOKEN_NOTNULL                         77
#define SYNTAQLITE_TOKEN_NE                              78
#define SYNTAQLITE_TOKEN_EQ                              79
#define SYNTAQLITE_TOKEN_GT                              80
#define SYNTAQLITE_TOKEN_LE                              81
#define SYNTAQLITE_TOKEN_LT                              82
#define SYNTAQLITE_TOKEN_GE                              83
#define SYNTAQLITE_TOKEN_ESCAPE                          84
#define SYNTAQLITE_TOKEN_ID                              85
#define SYNTAQLITE_TOKEN_COLUMNKW                        86
#define SYNTAQLITE_TOKEN_DO                              87
#define SYNTAQLITE_TOKEN_FOR                             88
#define SYNTAQLITE_TOKEN_IGNORE                          89
#define SYNTAQLITE_TOKEN_INITIALLY                       90
#define SYNTAQLITE_TOKEN_INSTEAD                         91
#define SYNTAQLITE_TOKEN_NO                              92
#define SYNTAQLITE_TOKEN_KEY                             93
#define SYNTAQLITE_TOKEN_OF                              94
#define SYNTAQLITE_TOKEN_OFFSET                          95
#define SYNTAQLITE_TOKEN_PRAGMA                          96
#define SYNTAQLITE_TOKEN_RAISE                           97
#define SYNTAQLITE_TOKEN_RECURSIVE                       98
#define SYNTAQLITE_TOKEN_REPLACE                         99
#define SYNTAQLITE_TOKEN_RESTRICT                       100
#define SYNTAQLITE_TOKEN_ROW                            101
#define SYNTAQLITE_TOKEN_ROWS                           102
#define SYNTAQLITE_TOKEN_TRIGGER                        103
#define SYNTAQLITE_TOKEN_VACUUM                         104
#define SYNTAQLITE_TOKEN_VIEW                           105
#define SYNTAQLITE_TOKEN_VIRTUAL                        106
#define SYNTAQLITE_TOKEN_WITH                           107
#define SYNTAQLITE_TOKEN_NULLS                          108
#define SYNTAQLITE_TOKEN_FIRST                          109
#define SYNTAQLITE_TOKEN_LAST                           110
#define SYNTAQLITE_TOKEN_CURRENT                        111
#define SYNTAQLITE_TOKEN_FOLLOWING                      112
#define SYNTAQLITE_TOKEN_PARTITION                      113
#define SYNTAQLITE_TOKEN_PRECEDING                      114
#define SYNTAQLITE_TOKEN_RANGE                          115
#define SYNTAQLITE_TOKEN_UNBOUNDED                      116
#define SYNTAQLITE_TOKEN_EXCLUDE                        117
#define SYNTAQLITE_TOKEN_GROUPS                         118
#define SYNTAQLITE_TOKEN_OTHERS                         119
#define SYNTAQLITE_TOKEN_TIES                           120
#define SYNTAQLITE_TOKEN_GENERATED                      121
#define SYNTAQLITE_TOKEN_ALWAYS                         122
#define SYNTAQLITE_TOKEN_MATERIALIZED                   123
#define SYNTAQLITE_TOKEN_REINDEX                        124
#define SYNTAQLITE_TOKEN_RENAME                         125
#define SYNTAQLITE_TOKEN_CTIME_KW                       126
#define SYNTAQLITE_TOKEN_ANY                            127
#define SYNTAQLITE_TOKEN_BITAND                         128
#define SYNTAQLITE_TOKEN_BITOR                          129
#define SYNTAQLITE_TOKEN_LSHIFT                         130
#define SYNTAQLITE_TOKEN_RSHIFT                         131
#define SYNTAQLITE_TOKEN_PLUS                           132
#define SYNTAQLITE_TOKEN_MINUS                          133
#define SYNTAQLITE_TOKEN_STAR                           134
#define SYNTAQLITE_TOKEN_SLASH                          135
#define SYNTAQLITE_TOKEN_REM                            136
#define SYNTAQLITE_TOKEN_CONCAT                         137
#define SYNTAQLITE_TOKEN_PTR                            138
#define SYNTAQLITE_TOKEN_COLLATE                        139
#define SYNTAQLITE_TOKEN_BITNOT                         140
#define SYNTAQLITE_TOKEN_ON                             141
#define SYNTAQLITE_TOKEN_INDEXED                        142
#define SYNTAQLITE_TOKEN_STRING                         143
#define SYNTAQLITE_TOKEN_JOIN_KW                        144
#define SYNTAQLITE_TOKEN_CONSTRAINT                     145
#define SYNTAQLITE_TOKEN_DEFAULT                        146
#define SYNTAQLITE_TOKEN_NULL                           147
#define SYNTAQLITE_TOKEN_PRIMARY                        148
#define SYNTAQLITE_TOKEN_UNIQUE                         149
#define SYNTAQLITE_TOKEN_CHECK                          150
#define SYNTAQLITE_TOKEN_REFERENCES                     151
#define SYNTAQLITE_TOKEN_AUTOINCR                       152
#define SYNTAQLITE_TOKEN_INSERT                         153
#define SYNTAQLITE_TOKEN_DELETE                         154
#define SYNTAQLITE_TOKEN_UPDATE                         155
#define SYNTAQLITE_TOKEN_SET                            156
#define SYNTAQLITE_TOKEN_DEFERRABLE                     157
#define SYNTAQLITE_TOKEN_FOREIGN                        158
#define SYNTAQLITE_TOKEN_DROP                           159
#define SYNTAQLITE_TOKEN_UNION                          160
#define SYNTAQLITE_TOKEN_ALL                            161
#define SYNTAQLITE_TOKEN_EXCEPT                         162
#define SYNTAQLITE_TOKEN_INTERSECT                      163
#define SYNTAQLITE_TOKEN_SELECT                         164
#define SYNTAQLITE_TOKEN_VALUES                         165
#define SYNTAQLITE_TOKEN_DISTINCT                       166
#define SYNTAQLITE_TOKEN_DOT                            167
#define SYNTAQLITE_TOKEN_FROM                           168
#define SYNTAQLITE_TOKEN_JOIN                           169
#define SYNTAQLITE_TOKEN_USING                          170
#define SYNTAQLITE_TOKEN_ORDER                          171
#define SYNTAQLITE_TOKEN_GROUP                          172
#define SYNTAQLITE_TOKEN_HAVING                         173
#define SYNTAQLITE_TOKEN_LIMIT                          174
#define SYNTAQLITE_TOKEN_WHERE                          175
#define SYNTAQLITE_TOKEN_RETURNING                      176
#define SYNTAQLITE_TOKEN_INTO                           177
#define SYNTAQLITE_TOKEN_NOTHING                        178
#define SYNTAQLITE_TOKEN_FLOAT                          179
#define SYNTAQLITE_TOKEN_BLOB                           180
#define SYNTAQLITE_TOKEN_INTEGER                        181
#define SYNTAQLITE_TOKEN_VARIABLE                       182
#define SYNTAQLITE_TOKEN_CASE                           183
#define SYNTAQLITE_TOKEN_WHEN                           184
#define SYNTAQLITE_TOKEN_THEN                           185
#define SYNTAQLITE_TOKEN_ELSE                           186

#endif /* SYNTAQLITE_CUSTOM_TOKENS */

#endif /* SYNTAQLITE_SQLITE_TOKENS_GEN_H */
