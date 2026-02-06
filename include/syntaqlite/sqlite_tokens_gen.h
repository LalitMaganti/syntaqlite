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
#define SYNTAQLITE_TOKEN_ALTER                            2
#define SYNTAQLITE_TOKEN_EXPLAIN                          3
#define SYNTAQLITE_TOKEN_ADD                              4
#define SYNTAQLITE_TOKEN_QUERY                            5
#define SYNTAQLITE_TOKEN_WINDOW                           6
#define SYNTAQLITE_TOKEN_PLAN                             7
#define SYNTAQLITE_TOKEN_OVER                             8
#define SYNTAQLITE_TOKEN_BEGIN                            9
#define SYNTAQLITE_TOKEN_FILTER                          10
#define SYNTAQLITE_TOKEN_TRANSACTION                     11
#define SYNTAQLITE_TOKEN_COLUMN                          12
#define SYNTAQLITE_TOKEN_DEFERRED                        13
#define SYNTAQLITE_TOKEN_AGG_FUNCTION                    14
#define SYNTAQLITE_TOKEN_IMMEDIATE                       15
#define SYNTAQLITE_TOKEN_AGG_COLUMN                      16
#define SYNTAQLITE_TOKEN_EXCLUSIVE                       17
#define SYNTAQLITE_TOKEN_TRUEFALSE                       18
#define SYNTAQLITE_TOKEN_COMMIT                          19
#define SYNTAQLITE_TOKEN_FUNCTION                        20
#define SYNTAQLITE_TOKEN_END                             21
#define SYNTAQLITE_TOKEN_UPLUS                           22
#define SYNTAQLITE_TOKEN_ROLLBACK                        23
#define SYNTAQLITE_TOKEN_UMINUS                          24
#define SYNTAQLITE_TOKEN_SAVEPOINT                       25
#define SYNTAQLITE_TOKEN_TRUTH                           26
#define SYNTAQLITE_TOKEN_RELEASE                         27
#define SYNTAQLITE_TOKEN_REGISTER                        28
#define SYNTAQLITE_TOKEN_TO                              29
#define SYNTAQLITE_TOKEN_VECTOR                          30
#define SYNTAQLITE_TOKEN_TABLE                           31
#define SYNTAQLITE_TOKEN_SELECT_COLUMN                   32
#define SYNTAQLITE_TOKEN_CREATE                          33
#define SYNTAQLITE_TOKEN_IF_NULL_ROW                     34
#define SYNTAQLITE_TOKEN_IF                              35
#define SYNTAQLITE_TOKEN_ASTERISK                        36
#define SYNTAQLITE_TOKEN_NOT                             37
#define SYNTAQLITE_TOKEN_SPAN                            38
#define SYNTAQLITE_TOKEN_EXISTS                          39
#define SYNTAQLITE_TOKEN_ERROR                           40
#define SYNTAQLITE_TOKEN_TEMP                            41
#define SYNTAQLITE_TOKEN_QNUMBER                         42
#define SYNTAQLITE_TOKEN_AS                              43
#define SYNTAQLITE_TOKEN_WITHOUT                         44
#define SYNTAQLITE_TOKEN_ABORT                           45
#define SYNTAQLITE_TOKEN_ACTION                          46
#define SYNTAQLITE_TOKEN_AFTER                           47
#define SYNTAQLITE_TOKEN_ANALYZE                         48
#define SYNTAQLITE_TOKEN_ASC                             49
#define SYNTAQLITE_TOKEN_ATTACH                          50
#define SYNTAQLITE_TOKEN_BEFORE                          51
#define SYNTAQLITE_TOKEN_BY                              52
#define SYNTAQLITE_TOKEN_CASCADE                         53
#define SYNTAQLITE_TOKEN_CAST                            54
#define SYNTAQLITE_TOKEN_CONFLICT                        55
#define SYNTAQLITE_TOKEN_DATABASE                        56
#define SYNTAQLITE_TOKEN_DESC                            57
#define SYNTAQLITE_TOKEN_DETACH                          58
#define SYNTAQLITE_TOKEN_EACH                            59
#define SYNTAQLITE_TOKEN_FAIL                            60
#define SYNTAQLITE_TOKEN_OR                              61
#define SYNTAQLITE_TOKEN_AND                             62
#define SYNTAQLITE_TOKEN_IS                              63
#define SYNTAQLITE_TOKEN_ISNOT                           64
#define SYNTAQLITE_TOKEN_MATCH                           65
#define SYNTAQLITE_TOKEN_LIKE_KW                         66
#define SYNTAQLITE_TOKEN_BETWEEN                         67
#define SYNTAQLITE_TOKEN_IN                              68
#define SYNTAQLITE_TOKEN_ISNULL                          69
#define SYNTAQLITE_TOKEN_NOTNULL                         70
#define SYNTAQLITE_TOKEN_ESCAPE                          71
#define SYNTAQLITE_TOKEN_COLUMNKW                        72
#define SYNTAQLITE_TOKEN_DO                              73
#define SYNTAQLITE_TOKEN_FOR                             74
#define SYNTAQLITE_TOKEN_IGNORE                          75
#define SYNTAQLITE_TOKEN_INITIALLY                       76
#define SYNTAQLITE_TOKEN_INSTEAD                         77
#define SYNTAQLITE_TOKEN_NO                              78
#define SYNTAQLITE_TOKEN_KEY                             79
#define SYNTAQLITE_TOKEN_OF                              80
#define SYNTAQLITE_TOKEN_OFFSET                          81
#define SYNTAQLITE_TOKEN_PRAGMA                          82
#define SYNTAQLITE_TOKEN_RAISE                           83
#define SYNTAQLITE_TOKEN_RECURSIVE                       84
#define SYNTAQLITE_TOKEN_REPLACE                         85
#define SYNTAQLITE_TOKEN_RESTRICT                        86
#define SYNTAQLITE_TOKEN_ROW                             87
#define SYNTAQLITE_TOKEN_ROWS                            88
#define SYNTAQLITE_TOKEN_TRIGGER                         89
#define SYNTAQLITE_TOKEN_VACUUM                          90
#define SYNTAQLITE_TOKEN_VIEW                            91
#define SYNTAQLITE_TOKEN_VIRTUAL                         92
#define SYNTAQLITE_TOKEN_WITH                            93
#define SYNTAQLITE_TOKEN_NULLS                           94
#define SYNTAQLITE_TOKEN_FIRST                           95
#define SYNTAQLITE_TOKEN_LAST                            96
#define SYNTAQLITE_TOKEN_CURRENT                         97
#define SYNTAQLITE_TOKEN_FOLLOWING                       98
#define SYNTAQLITE_TOKEN_PARTITION                       99
#define SYNTAQLITE_TOKEN_PRECEDING                      100
#define SYNTAQLITE_TOKEN_RANGE                          101
#define SYNTAQLITE_TOKEN_UNBOUNDED                      102
#define SYNTAQLITE_TOKEN_EXCLUDE                        103
#define SYNTAQLITE_TOKEN_GROUPS                         104
#define SYNTAQLITE_TOKEN_OTHERS                         105
#define SYNTAQLITE_TOKEN_TIES                           106
#define SYNTAQLITE_TOKEN_GENERATED                      107
#define SYNTAQLITE_TOKEN_ALWAYS                         108
#define SYNTAQLITE_TOKEN_MATERIALIZED                   109
#define SYNTAQLITE_TOKEN_REINDEX                        110
#define SYNTAQLITE_TOKEN_RENAME                         111
#define SYNTAQLITE_TOKEN_CTIME_KW                       112
#define SYNTAQLITE_TOKEN_ANY                            113
#define SYNTAQLITE_TOKEN_COLLATE                        114
#define SYNTAQLITE_TOKEN_ON                             115
#define SYNTAQLITE_TOKEN_INDEXED                        116
#define SYNTAQLITE_TOKEN_JOIN_KW                        117
#define SYNTAQLITE_TOKEN_CONSTRAINT                     118
#define SYNTAQLITE_TOKEN_DEFAULT                        119
#define SYNTAQLITE_TOKEN_NULL                           120
#define SYNTAQLITE_TOKEN_PRIMARY                        121
#define SYNTAQLITE_TOKEN_UNIQUE                         122
#define SYNTAQLITE_TOKEN_CHECK                          123
#define SYNTAQLITE_TOKEN_REFERENCES                     124
#define SYNTAQLITE_TOKEN_AUTOINCR                       125
#define SYNTAQLITE_TOKEN_INSERT                         126
#define SYNTAQLITE_TOKEN_DELETE                         127
#define SYNTAQLITE_TOKEN_UPDATE                         128
#define SYNTAQLITE_TOKEN_SET                            129
#define SYNTAQLITE_TOKEN_DEFERRABLE                     130
#define SYNTAQLITE_TOKEN_FOREIGN                        131
#define SYNTAQLITE_TOKEN_DROP                           132
#define SYNTAQLITE_TOKEN_UNION                          133
#define SYNTAQLITE_TOKEN_ALL                            134
#define SYNTAQLITE_TOKEN_EXCEPT                         135
#define SYNTAQLITE_TOKEN_INTERSECT                      136
#define SYNTAQLITE_TOKEN_SELECT                         137
#define SYNTAQLITE_TOKEN_VALUES                         138
#define SYNTAQLITE_TOKEN_DISTINCT                       139
#define SYNTAQLITE_TOKEN_FROM                           140
#define SYNTAQLITE_TOKEN_JOIN                           141
#define SYNTAQLITE_TOKEN_USING                          142
#define SYNTAQLITE_TOKEN_ORDER                          143
#define SYNTAQLITE_TOKEN_GROUP                          144
#define SYNTAQLITE_TOKEN_HAVING                         145
#define SYNTAQLITE_TOKEN_LIMIT                          146
#define SYNTAQLITE_TOKEN_WHERE                          147
#define SYNTAQLITE_TOKEN_RETURNING                      148
#define SYNTAQLITE_TOKEN_INTO                           149
#define SYNTAQLITE_TOKEN_NOTHING                        150
#define SYNTAQLITE_TOKEN_CASE                           151
#define SYNTAQLITE_TOKEN_WHEN                           152
#define SYNTAQLITE_TOKEN_THEN                           153
#define SYNTAQLITE_TOKEN_ELSE                           154
#define SYNTAQLITE_TOKEN_NE                             155
#define SYNTAQLITE_TOKEN_EQ                             156
#define SYNTAQLITE_TOKEN_GT                             157
#define SYNTAQLITE_TOKEN_LE                             158
#define SYNTAQLITE_TOKEN_LT                             159
#define SYNTAQLITE_TOKEN_GE                             160
#define SYNTAQLITE_TOKEN_BITAND                         161
#define SYNTAQLITE_TOKEN_BITOR                          162
#define SYNTAQLITE_TOKEN_LSHIFT                         163
#define SYNTAQLITE_TOKEN_RSHIFT                         164
#define SYNTAQLITE_TOKEN_PLUS                           165
#define SYNTAQLITE_TOKEN_MINUS                          166
#define SYNTAQLITE_TOKEN_STAR                           167
#define SYNTAQLITE_TOKEN_SLASH                          168
#define SYNTAQLITE_TOKEN_REM                            169
#define SYNTAQLITE_TOKEN_CONCAT                         170
#define SYNTAQLITE_TOKEN_PTR                            171
#define SYNTAQLITE_TOKEN_BITNOT                         172
#define SYNTAQLITE_TOKEN_ID                             173
#define SYNTAQLITE_TOKEN_LP                             174
#define SYNTAQLITE_TOKEN_RP                             175
#define SYNTAQLITE_TOKEN_COMMA                          176
#define SYNTAQLITE_TOKEN_STRING                         177
#define SYNTAQLITE_TOKEN_DOT                            178
#define SYNTAQLITE_TOKEN_FLOAT                          179
#define SYNTAQLITE_TOKEN_BLOB                           180
#define SYNTAQLITE_TOKEN_INTEGER                        181
#define SYNTAQLITE_TOKEN_VARIABLE                       182
#define SYNTAQLITE_TOKEN_SEMI                           183
#define SYNTAQLITE_TOKEN_SPACE                          184
#define SYNTAQLITE_TOKEN_COMMENT                        185
#define SYNTAQLITE_TOKEN_ILLEGAL                        186

#endif /* SYNTAQLITE_CUSTOM_TOKENS */

#endif /* SYNTAQLITE_SQLITE_TOKENS_GEN_H */
