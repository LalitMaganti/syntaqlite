// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Tests for the format recipe interpreter (synq_fmt_interpret).
//
// Manually builds AST nodes, defines format recipes as static op arrays,
// runs the interpreter, lays out the document, and compares with expected
// output strings.

#include <gtest/gtest.h>

extern "C" {
#include "src/formatter/fmt_helpers.h"
#include "src/parser/ast_builder_gen.h"
}

#include <cstring>
#include <string>

// ============ Test Fixture ============

class FmtInterpretTest : public ::testing::Test {
 protected:
  SynqAstContext ast_ctx_;
  const char *source_ = "";

  void Init(const char *source) {
    source_ = source;
    synq_ast_context_init(&ast_ctx_, source_, (uint32_t)strlen(source_));
    initialized_ = true;
  }

  void TearDown() override {
    if (initialized_) synq_ast_context_cleanup(&ast_ctx_);
  }

  bool initialized_ = false;

  // Run the interpreter on a node and return the formatted string.
  std::string Interpret(uint32_t node_id, const SynqFmtOp *ops,
                        uint32_t width = 80) {
    SynqFmtOptions options = {width, 2};
    SynqFmtCtx ctx;
    synq_doc_context_init(&ctx.docs);
    ctx.ast = &ast_ctx_.ast;
    ctx.source = source_;
    ctx.token_list = nullptr;
    ctx.options = &options;
    ctx.comment_att = nullptr;

    SyntaqliteNode *node = synq_ast_node(&ast_ctx_.ast, node_id);
    uint32_t doc = synq_fmt_interpret_ops(&ctx, node, ops);

    std::string result;
    if (doc == SYNQ_NULL_DOC) {
      result = "";
    } else {
      char *laid_out = synq_doc_layout(&ctx.docs, doc, options.target_width);
      result = laid_out ? laid_out : "";
      free(laid_out);
    }
    synq_doc_context_cleanup(&ctx.docs);
    return result;
  }
};

// ============ Test: Literal ============
// Python DSL: fmt=span("source")

static const SynqFmtOp literal_fmt[] = {
    FOP_SPAN(Literal, source),
};

TEST_F(FmtInterpretTest, Literal) {
  Init("42");
  uint32_t id = synq_ast_literal(&ast_ctx_, SYNTAQLITE_LITERAL_TYPE_INTEGER,
                                 {0, 2});
  EXPECT_EQ(Interpret(id, literal_fmt), "42");
}

TEST_F(FmtInterpretTest, LiteralString) {
  Init("'hello'");
  uint32_t id = synq_ast_literal(&ast_ctx_, SYNTAQLITE_LITERAL_TYPE_STRING,
                                 {0, 7});
  EXPECT_EQ(Interpret(id, literal_fmt), "'hello'");
}

// ============ Test: LimitClause ============
// Python DSL:
//   fmt=seq(child("limit"), if_set("offset", seq(kw(" OFFSET "), child("offset"))))

static const SynqFmtOp limit_clause_fmt[] = {
    FOP_SEQ(2),
        FOP_CHILD(LimitClause, limit),
        FOP_IF_SET(LimitClause, offset),
            FOP_SEQ(2),
                FOP_KW(" OFFSET "),
                FOP_CHILD(LimitClause, offset),
};

TEST_F(FmtInterpretTest, LimitClauseWithOffset) {
  Init("10 OFFSET 5");
  uint32_t limit = synq_ast_literal(&ast_ctx_, SYNTAQLITE_LITERAL_TYPE_INTEGER,
                                    {0, 2});
  uint32_t offset = synq_ast_literal(&ast_ctx_, SYNTAQLITE_LITERAL_TYPE_INTEGER,
                                     {10, 1});
  uint32_t id = synq_ast_limit_clause(&ast_ctx_, limit, offset);
  EXPECT_EQ(Interpret(id, limit_clause_fmt), "10 OFFSET 5");
}

TEST_F(FmtInterpretTest, LimitClauseWithoutOffset) {
  Init("10");
  uint32_t limit = synq_ast_literal(&ast_ctx_, SYNTAQLITE_LITERAL_TYPE_INTEGER,
                                    {0, 2});
  uint32_t id = synq_ast_limit_clause(&ast_ctx_, limit, SYNTAQLITE_NULL_NODE);
  EXPECT_EQ(Interpret(id, limit_clause_fmt), "10");
}

// ============ Test: OrderingTerm ============
// Python DSL:
//   fmt=seq(child("expr"),
//       if_enum("sort_order", "DESC", kw(" DESC")),
//       if_enum("nulls_order", "FIRST", kw(" NULLS FIRST")),
//       if_enum("nulls_order", "LAST", kw(" NULLS LAST")))

static const SynqFmtOp ordering_term_fmt[] = {
    FOP_SEQ(4),
        FOP_CHILD(OrderingTerm, expr),
        FOP_IF_ENUM(OrderingTerm, sort_order, SYNTAQLITE_SORT_ORDER_DESC),
            FOP_KW(" DESC"),
        FOP_IF_ENUM(OrderingTerm, nulls_order, SYNTAQLITE_NULLS_ORDER_FIRST),
            FOP_KW(" NULLS FIRST"),
        FOP_IF_ENUM(OrderingTerm, nulls_order, SYNTAQLITE_NULLS_ORDER_LAST),
            FOP_KW(" NULLS LAST"),
};

TEST_F(FmtInterpretTest, OrderingTermDefault) {
  Init("col1");
  uint32_t col = synq_ast_column_ref(&ast_ctx_, {0, 4}, SYNQ_NO_SPAN,
                                     SYNQ_NO_SPAN);
  uint32_t id = synq_ast_ordering_term(&ast_ctx_, col, SYNTAQLITE_SORT_ORDER_ASC,
                                       SYNTAQLITE_NULLS_ORDER_NONE);
  EXPECT_EQ(Interpret(id, ordering_term_fmt), "col1");
}

TEST_F(FmtInterpretTest, OrderingTermDescNullsFirst) {
  Init("col1");
  uint32_t col = synq_ast_column_ref(&ast_ctx_, {0, 4}, SYNQ_NO_SPAN,
                                     SYNQ_NO_SPAN);
  uint32_t id = synq_ast_ordering_term(&ast_ctx_, col, SYNTAQLITE_SORT_ORDER_DESC,
                                       SYNTAQLITE_NULLS_ORDER_FIRST);
  EXPECT_EQ(Interpret(id, ordering_term_fmt), "col1 DESC NULLS FIRST");
}

TEST_F(FmtInterpretTest, OrderingTermAscNullsLast) {
  Init("col1");
  uint32_t col = synq_ast_column_ref(&ast_ctx_, {0, 4}, SYNQ_NO_SPAN,
                                     SYNQ_NO_SPAN);
  uint32_t id = synq_ast_ordering_term(&ast_ctx_, col, SYNTAQLITE_SORT_ORDER_ASC,
                                       SYNTAQLITE_NULLS_ORDER_LAST);
  EXPECT_EQ(Interpret(id, ordering_term_fmt), "col1 NULLS LAST");
}

// ============ Test: UnaryExpr with ENUM_DISPLAY ============
// Python DSL:
//   fmt=seq(enum_display("op", {...}), child("operand"))

static const SynqFmtEnumEntry unary_op_display[] = {
    {SYNTAQLITE_UNARY_OP_MINUS, "-"},
    {SYNTAQLITE_UNARY_OP_PLUS, "+"},
    {SYNTAQLITE_UNARY_OP_BITNOT, "~"},
    {SYNTAQLITE_UNARY_OP_NOT, "NOT "},
};

static const SynqFmtOp unary_expr_fmt[] = {
    FOP_SEQ(2),
        FOP_ENUM_DISPLAY(UnaryExpr, op, unary_op_display, 4),
        FOP_CHILD(UnaryExpr, operand),
};

TEST_F(FmtInterpretTest, UnaryMinus) {
  Init("-42");
  uint32_t lit = synq_ast_literal(&ast_ctx_, SYNTAQLITE_LITERAL_TYPE_INTEGER,
                                  {1, 2});
  uint32_t id = synq_ast_unary_expr(&ast_ctx_, SYNTAQLITE_UNARY_OP_MINUS, lit);
  EXPECT_EQ(Interpret(id, unary_expr_fmt), "-42");
}

TEST_F(FmtInterpretTest, UnaryNot) {
  Init("NOT true");
  uint32_t lit = synq_ast_literal(&ast_ctx_, SYNTAQLITE_LITERAL_TYPE_INTEGER,
                                  {4, 4});
  uint32_t id = synq_ast_unary_expr(&ast_ctx_, SYNTAQLITE_UNARY_OP_NOT, lit);
  EXPECT_EQ(Interpret(id, unary_expr_fmt), "NOT true");
}

// ============ Test: BinaryExpr with SWITCH ============
// Simplified version of the Python DSL switch on op.

static const SynqFmtEnumEntry binary_op_display[] = {
    {SYNTAQLITE_BINARY_OP_PLUS, "+"},
    {SYNTAQLITE_BINARY_OP_MINUS, "-"},
    {SYNTAQLITE_BINARY_OP_EQ, "="},
};

static const SynqFmtOp binary_expr_fmt[] = {
    FOP_SWITCH_DEFAULT(BinaryExpr, op, 2),
        // case AND: seq(left, line, "AND ", right)
        FOP_CASE(SYNTAQLITE_BINARY_OP_AND),
            FOP_SEQ(4),
                FOP_CHILD(BinaryExpr, left),
                FOP_LINE,
                FOP_KW("AND "),
                FOP_CHILD(BinaryExpr, right),
        // case OR: seq(left, line, "OR ", right)
        FOP_CASE(SYNTAQLITE_BINARY_OP_OR),
            FOP_SEQ(4),
                FOP_CHILD(BinaryExpr, left),
                FOP_LINE,
                FOP_KW("OR "),
                FOP_CHILD(BinaryExpr, right),
        // default: group(seq(left, " ", op_display, " ", right))
        FOP_GROUP,
            FOP_SEQ(5),
                FOP_CHILD(BinaryExpr, left),
                FOP_KW(" "),
                FOP_ENUM_DISPLAY(BinaryExpr, op, binary_op_display, 3),
                FOP_KW(" "),
                FOP_CHILD(BinaryExpr, right),
};

TEST_F(FmtInterpretTest, BinaryPlusDefault) {
  Init("1 + 2");
  uint32_t left = synq_ast_literal(&ast_ctx_, SYNTAQLITE_LITERAL_TYPE_INTEGER,
                                   {0, 1});
  uint32_t right = synq_ast_literal(&ast_ctx_, SYNTAQLITE_LITERAL_TYPE_INTEGER,
                                    {4, 1});
  uint32_t id = synq_ast_binary_expr(&ast_ctx_, SYNTAQLITE_BINARY_OP_PLUS,
                                     left, right);
  EXPECT_EQ(Interpret(id, binary_expr_fmt), "1 + 2");
}

TEST_F(FmtInterpretTest, BinaryAnd) {
  // AND/OR use line() which becomes space in flat mode inside a group.
  // Without an enclosing group, line() becomes a newline at top level.
  // Wrap in a group to test flat behavior.
  Init("1 AND 2");
  uint32_t left = synq_ast_literal(&ast_ctx_, SYNTAQLITE_LITERAL_TYPE_INTEGER,
                                   {0, 1});
  uint32_t right = synq_ast_literal(&ast_ctx_, SYNTAQLITE_LITERAL_TYPE_INTEGER,
                                    {6, 1});
  uint32_t id = synq_ast_binary_expr(&ast_ctx_, SYNTAQLITE_BINARY_OP_AND,
                                     left, right);
  // Without enclosing group, line() breaks -> newline
  EXPECT_EQ(Interpret(id, binary_expr_fmt), "1\nAND 2");
}

// ============ Test: ResultColumn with IF_FLAG ============
// Python DSL:
//   fmt=seq(
//       if_flag("flags.star",
//           if_set("expr", seq(child("expr"), kw(".*")), kw("*")),
//           child("expr")),
//       if_span("alias", seq(kw(" AS "), span("alias"))))

static const SynqFmtOp result_column_fmt[] = {
    FOP_SEQ(2),
        FOP_IF_FLAG_ELSE(ResultColumn, flags, 0x01),
            // then: star
            FOP_IF_SET_ELSE(ResultColumn, expr),
                // table.*
                FOP_SEQ(2),
                    FOP_CHILD(ResultColumn, expr),
                    FOP_KW(".*"),
                // bare *
                FOP_KW("*"),
            // else: normal column
            FOP_CHILD(ResultColumn, expr),
        FOP_IF_SPAN(ResultColumn, alias),
            FOP_SEQ(2),
                FOP_KW(" AS "),
                FOP_SPAN(ResultColumn, alias),
};

TEST_F(FmtInterpretTest, ResultColumnBareStar) {
  Init("*");
  SyntaqliteResultColumnFlags star = {.raw = 0x01};
  uint32_t id = synq_ast_result_column(&ast_ctx_, star, SYNQ_NO_SPAN,
                                       SYNTAQLITE_NULL_NODE);
  EXPECT_EQ(Interpret(id, result_column_fmt), "*");
}

TEST_F(FmtInterpretTest, ResultColumnExprWithAlias) {
  Init("t AS alias");
  SyntaqliteResultColumnFlags no_flag = {.raw = 0};
  uint32_t lit = synq_ast_literal(&ast_ctx_, SYNTAQLITE_LITERAL_TYPE_INTEGER,
                                  {0, 1});
  uint32_t id = synq_ast_result_column(&ast_ctx_, no_flag, {5, 5}, lit);
  EXPECT_EQ(Interpret(id, result_column_fmt), "t AS alias");
}

TEST_F(FmtInterpretTest, ResultColumnExprNoAlias) {
  Init("42");
  SyntaqliteResultColumnFlags no_flag = {.raw = 0};
  uint32_t lit = synq_ast_literal(&ast_ctx_, SYNTAQLITE_LITERAL_TYPE_INTEGER,
                                  {0, 2});
  uint32_t id = synq_ast_result_column(&ast_ctx_, no_flag, SYNQ_NO_SPAN,
                                       lit);
  EXPECT_EQ(Interpret(id, result_column_fmt), "42");
}

// ============ Test: FOR_EACH with separator ============
// for_each_child(child("_item"), separator=seq(kw(","), line()))

static const SynqFmtOp comma_list_fmt[] = {
    FOP_GROUP,
        FOP_FOR_EACH_SEP,
            FOP_CHILD_ITEM,
            FOP_SEQ(2),
                FOP_KW(","),
                FOP_LINE,
};

TEST_F(FmtInterpretTest, ForEachCommaList) {
  Init("1, 2, 3");
  uint32_t a = synq_ast_literal(&ast_ctx_, SYNTAQLITE_LITERAL_TYPE_INTEGER, {0, 1});
  uint32_t b = synq_ast_literal(&ast_ctx_, SYNTAQLITE_LITERAL_TYPE_INTEGER, {3, 1});
  uint32_t c = synq_ast_literal(&ast_ctx_, SYNTAQLITE_LITERAL_TYPE_INTEGER, {6, 1});

  uint32_t list = synq_ast_expr_list(&ast_ctx_, a);
  list = synq_ast_expr_list_append(&ast_ctx_, list, b);
  list = synq_ast_expr_list_append(&ast_ctx_, list, c);
  synq_ast_list_flush(&ast_ctx_);

  EXPECT_EQ(Interpret(list, comma_list_fmt), "1, 2, 3");
}

TEST_F(FmtInterpretTest, ForEachSingleItem) {
  Init("42");
  uint32_t a = synq_ast_literal(&ast_ctx_, SYNTAQLITE_LITERAL_TYPE_INTEGER, {0, 2});

  uint32_t list = synq_ast_expr_list(&ast_ctx_, a);
  synq_ast_list_flush(&ast_ctx_);

  EXPECT_EQ(Interpret(list, comma_list_fmt), "42");
}

// ============ Test: FOR_EACH without separator ============

static const SynqFmtOp concat_list_fmt[] = {
    FOP_FOR_EACH,
        FOP_CHILD_ITEM,
};

TEST_F(FmtInterpretTest, ForEachNoSeparator) {
  Init("abc");
  uint32_t a = synq_ast_literal(&ast_ctx_, SYNTAQLITE_LITERAL_TYPE_INTEGER, {0, 1});
  uint32_t b = synq_ast_literal(&ast_ctx_, SYNTAQLITE_LITERAL_TYPE_INTEGER, {1, 1});
  uint32_t c = synq_ast_literal(&ast_ctx_, SYNTAQLITE_LITERAL_TYPE_INTEGER, {2, 1});

  uint32_t list = synq_ast_expr_list(&ast_ctx_, a);
  list = synq_ast_expr_list_append(&ast_ctx_, list, b);
  list = synq_ast_expr_list_append(&ast_ctx_, list, c);
  synq_ast_list_flush(&ast_ctx_);

  EXPECT_EQ(Interpret(list, concat_list_fmt), "abc");
}

// ============ Test: Nested group + nest ============
// group(nest(seq(kw("("), softline, child("expr"), softline, kw(")"))))

static const SynqFmtOp grouped_expr_fmt[] = {
    FOP_GROUP,
        FOP_NEST,
            FOP_SEQ(5),
                FOP_KW("("),
                FOP_SOFTLINE,
                FOP_CHILD(SubqueryExpr, select),
                FOP_SOFTLINE,
                FOP_KW(")"),
};

TEST_F(FmtInterpretTest, GroupNestSoftline) {
  Init("(inner)");
  // Reuse SubqueryExpr as a container with a single 'select' child
  uint32_t inner = synq_ast_literal(&ast_ctx_, SYNTAQLITE_LITERAL_TYPE_INTEGER,
                                    {1, 5});
  uint32_t id = synq_ast_subquery_expr(&ast_ctx_, inner);
  // Fits in 80 cols -> softlines become empty
  EXPECT_EQ(Interpret(id, grouped_expr_fmt), "(inner)");
}

// ============ Test: CLAUSE (leaf op) ============
// clause("WHERE", child("where")) -- CLAUSE calls synq_format_clause directly

static const SynqFmtOp clause_test_fmt[] = {
    FOP_SEQ(2),
        FOP_KW("SELECT *"),
        FOP_CLAUSE(DeleteStmt, where, "WHERE"),
};

TEST_F(FmtInterpretTest, ClausePresent) {
  Init("SELECT * WHERE 1");
  uint32_t cond = synq_ast_literal(&ast_ctx_, SYNTAQLITE_LITERAL_TYPE_INTEGER,
                                   {15, 1});
  // Using DeleteStmt since it has a 'where' field
  uint32_t table = synq_ast_literal(&ast_ctx_, SYNTAQLITE_LITERAL_TYPE_INTEGER,
                                    {7, 1});
  uint32_t id = synq_ast_delete_stmt(&ast_ctx_, table, cond);
  std::string result = Interpret(id, clause_test_fmt);
  // CLAUSE produces: line + kw + group(nest(line + body))
  // Outer line() -> newline. Inner group stays flat since "WHERE 1" fits.
  EXPECT_EQ(result, "SELECT *\nWHERE 1");
}

TEST_F(FmtInterpretTest, ClauseNull) {
  Init("SELECT *");
  uint32_t table = synq_ast_literal(&ast_ctx_, SYNTAQLITE_LITERAL_TYPE_INTEGER,
                                    {7, 1});
  uint32_t id = synq_ast_delete_stmt(&ast_ctx_, table, SYNTAQLITE_NULL_NODE);
  std::string result = Interpret(id, clause_test_fmt);
  // CLAUSE returns NULL_DOC when body_id is NULL_NODE; concat skips it
  EXPECT_EQ(result, "SELECT *");
}

// ============ Test: IF_SET with else ============

static const SynqFmtOp if_set_else_fmt[] = {
    FOP_IF_SET_ELSE(LimitClause, offset),
        FOP_KW("HAS_OFFSET"),
        FOP_KW("NO_OFFSET"),
};

TEST_F(FmtInterpretTest, IfSetElseThen) {
  Init("10 5");
  uint32_t limit = synq_ast_literal(&ast_ctx_, SYNTAQLITE_LITERAL_TYPE_INTEGER,
                                    {0, 2});
  uint32_t offset = synq_ast_literal(&ast_ctx_, SYNTAQLITE_LITERAL_TYPE_INTEGER,
                                     {3, 1});
  uint32_t id = synq_ast_limit_clause(&ast_ctx_, limit, offset);
  EXPECT_EQ(Interpret(id, if_set_else_fmt), "HAS_OFFSET");
}

TEST_F(FmtInterpretTest, IfSetElseElse) {
  Init("10");
  uint32_t limit = synq_ast_literal(&ast_ctx_, SYNTAQLITE_LITERAL_TYPE_INTEGER,
                                    {0, 2});
  uint32_t id = synq_ast_limit_clause(&ast_ctx_, limit, SYNTAQLITE_NULL_NODE);
  EXPECT_EQ(Interpret(id, if_set_else_fmt), "NO_OFFSET");
}
