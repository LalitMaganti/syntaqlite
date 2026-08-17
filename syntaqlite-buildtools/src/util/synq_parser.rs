// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// ── Public API ───────────────────────────────────────────────────────────

/// A structured parse error from a `.synq` file.
#[derive(Debug, Clone)]
pub(crate) struct SynqParseError {
    pub(crate) message: String,
}

impl std::fmt::Display for SynqParseError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str(&self.message)
    }
}

/// Parse a single .synq file's contents into a list of items.
pub(crate) fn parse_synq_file(input: &str) -> Result<Vec<Item>, SynqParseError> {
    let tokens = tokenize(input).map_err(|message| SynqParseError { message })?;
    Parser::new(tokens)
        .parse_file()
        .map_err(|message| SynqParseError { message })
}

// ── AST ──────────────────────────────────────────────────────────────────

/// A semantic role for a node.
///
/// Covers catalog, expression, source, and scope roles described in
/// docs/semantic-roles-plan.md.
#[derive(Debug, Clone)]
pub(crate) enum SemanticRole {
    // ── Catalog roles ─────────────────────────────────────────────────────
    DefineTable {
        name: String,
        columns: Option<String>,
        select: Option<String>,
        /// Resolved flag reference: `(field_name, bit_name)` from `flags.without_rowid`.
        without_rowid: Option<(String, String)>,
    },
    DefineView {
        name: String,
        columns: Option<String>,
        select: String,
    },
    DefineFunction {
        name: String,
        args: Option<String>,
        return_type: Option<String>,
        select: Option<String>,
    },
    ReturnSpec {
        columns: Option<String>,
    },
    Import {
        module: String,
    },

    // ── Column-list items ─────────────────────────────────────────────────
    ColumnDef {
        name: String,
        type_name: Option<String>,
        constraints: Option<String>,
    },

    // ── Result columns ────────────────────────────────────────────────────
    ResultColumn {
        flags: String,
        alias: String,
        expr: String,
    },

    // ── Expressions ───────────────────────────────────────────────────────
    Call {
        name: String,
        args: String,
    },
    ColumnRef {
        column: String,
        table: String,
    },

    // ── Sources ───────────────────────────────────────────────────────────
    SourceRef {
        kind: String, // literal RelationKind value: "table", "view", etc.
        name: String,
        alias: String,
    },
    ScopedSource {
        body: String,
        alias: String,
    },

    // ── Scope structure ───────────────────────────────────────────────────
    Query {
        from: String,
        columns: String,
        where_clause: String,
        groupby: String,
        having: String,
        orderby: String,
        limit_clause: String,
    },
    CteBinding {
        name: String,
        columns: Option<String>,
        body: String,
    },
    CteScope {
        recursive: String,
        bindings: String,
        body: String,
    },
    TriggerScope {
        target: String,
        when: String,
        body: String,
    },
    DmlScope {
        with_recursive: Option<String>,
        with_ctes: Option<String>,
    },
    UpsertScope {
        columns: Option<String>,
        target_where: Option<String>,
        setlist: Option<String>,
        update_where: Option<String>,
    },
    CompoundScope {
        left: String,
        right: String,
        orderby: Option<String>,
        limit_clause: Option<String>,
    },

    // ── Schema mutation ───────────────────────────────────────────────────
    AlterTable {
        op: String,
        target: String,
        new_name: String,
        old_name: String,
        column: Option<String>,
    },
}

/// A `semantic { ... }` annotation on a node.
#[derive(Debug, Clone)]
pub(crate) struct SemanticAnnotation {
    pub(crate) role: SemanticRole,
}

/// A `macro_def { ... }` annotation marking a node as a template macro definition.
///
/// Tells the formatter which fields contain the macro name, body, and parameter
/// names so it can auto-register macros for expansion in subsequent statements.
#[derive(Debug, Clone)]
pub(crate) struct MacroDefAnnotation {
    /// Field name for the macro name span.
    pub(crate) name: String,
    /// Field name for the macro body span.
    pub(crate) body: String,
    /// Field name for the args list node (None if the macro takes no params).
    pub(crate) args: Option<String>,
    /// Within each args list item, the field name for the param name span.
    /// Required when `args` is Some.
    pub(crate) arg_name: Option<String>,
}

#[derive(Debug)]
#[expect(clippy::large_enum_variant)]
pub(crate) enum Item {
    Node {
        name: String,
        fields: Vec<Field>,
        fmt: Option<Vec<Fmt>>,
        semantic: Option<SemanticAnnotation>,
        macro_def: Option<MacroDefAnnotation>,
        /// `fmt_precedence(N)` — fixed precedence for this node type.
        fmt_precedence: Option<u8>,
        /// `fmt_group(N)` — fixed operator group for this node type.
        fmt_group: Option<u8>,
    },
    Enum {
        name: String,
        variants: Vec<String>,
        /// `fmt_precedence { VARIANT=N ... }` — per-variant precedence values.
        fmt_precedence: Vec<(String, u8)>,
        /// `fmt_group { VARIANT=N ... }` — per-variant operator group values.
        fmt_group: Vec<(String, u8)>,
        /// `fmt_paren_boundary { VARIANT ... }` — variants that trigger readability parens.
        fmt_paren_boundary: Vec<String>,
    },
    Flags {
        name: String,
        flags: Vec<(String, u32)>,
    },
    List {
        name: String,
        child_type: String,
        fmt: Option<Vec<Fmt>>,
        prepend: bool,
    },
    Abstract {
        name: String,
        members: Vec<String>,
    },
}

#[derive(Debug, Clone, PartialEq)]
pub(crate) enum Storage {
    Index,
    Inline,
}

#[derive(Debug)]
pub(crate) struct Field {
    pub(crate) name: String,
    pub(crate) storage: Storage,
    pub(crate) type_name: String,
}

#[derive(Debug)]
pub(crate) enum Fmt {
    Text(String),
    Child(String),
    Span(String),
    Line,
    SoftLine,
    HardLine,
    Space,
    Group(Vec<Self>),
    Nest(Vec<Self>),
    IfSet {
        field: String,
        then: Vec<Self>,
        els: Option<Vec<Self>>,
    },
    IfFlag {
        field: String,
        bit: Option<String>,
        then: Vec<Self>,
        els: Option<Vec<Self>>,
    },
    IfEnum {
        field: String,
        variant: String,
        then: Vec<Self>,
        els: Option<Vec<Self>>,
    },
    IfSpan {
        field: String,
        then: Vec<Self>,
        els: Option<Vec<Self>>,
    },
    Clause {
        keywords: Vec<String>,
        field: String,
    },
    Switch {
        field: String,
        cases: Vec<(String, Vec<Self>)>,
        default: Option<Vec<Self>>,
    },
    EnumDisplay {
        field: String,
        mappings: Vec<(String, String)>,
    },
    ForEach {
        sep: Option<Vec<Self>>,
        body: Vec<Self>,
    },
    /// `child_prec(child_field, op_field)` or `child_prec(child_field, op_field, right)`.
    ChildPrec {
        child_field: String,
        op_field: String,
        is_right: bool,
    },
    /// `child_paren_list(field)` — wrap if child is a list node.
    ChildParenList(String),
    /// `child_prec_fixed(child_field)` or `child_prec_fixed(child_field, right)`.
    /// Uses the node's own `fmt_precedence` / `fmt_group` as parent prec.
    ChildPrecFixed {
        child_field: String,
        is_right: bool,
    },
}

// ── Tokens ───────────────────────────────────────────────────────────────

#[derive(Debug, Clone, PartialEq)]
enum Token {
    Ident(String),
    Str(String),
    Int(u32),
    LBrace,
    RBrace,
    LParen,
    RParen,
    Colon,
    Dot,
    Comma,
    Eq,
    Eof,
}

// ── Tokenizer ────────────────────────────────────────────────────────────

#[expect(clippy::too_many_lines)]
fn tokenize(input: &str) -> Result<Vec<Token>, String> {
    let b = input.as_bytes();
    let (mut i, mut line, mut col) = (0, 1usize, 1usize);

    let advance = |i: &mut usize, line: &mut usize, col: &mut usize| -> Option<u8> {
        let ch = *b.get(*i)?;
        *i += 1;
        if ch == b'\n' {
            *line += 1;
            *col = 1;
        } else {
            *col += 1;
        }
        Some(ch)
    };

    let mut tokens = Vec::new();
    loop {
        // skip whitespace and // comments
        loop {
            match b.get(i) {
                Some(b' ' | b'\t' | b'\n' | b'\r') => {
                    advance(&mut i, &mut line, &mut col);
                }
                Some(b'/') if b.get(i + 1) == Some(&b'/') => {
                    while let Some(ch) = advance(&mut i, &mut line, &mut col) {
                        if ch == b'\n' {
                            break;
                        }
                    }
                }
                _ => break,
            }
        }
        let Some(&ch) = b.get(i) else {
            tokens.push(Token::Eof);
            break;
        };
        match ch {
            b'{' => {
                advance(&mut i, &mut line, &mut col);
                tokens.push(Token::LBrace);
            }
            b'}' => {
                advance(&mut i, &mut line, &mut col);
                tokens.push(Token::RBrace);
            }
            b'(' => {
                advance(&mut i, &mut line, &mut col);
                tokens.push(Token::LParen);
            }
            b')' => {
                advance(&mut i, &mut line, &mut col);
                tokens.push(Token::RParen);
            }
            b':' => {
                advance(&mut i, &mut line, &mut col);
                tokens.push(Token::Colon);
            }
            b'.' => {
                advance(&mut i, &mut line, &mut col);
                tokens.push(Token::Dot);
            }
            b',' => {
                advance(&mut i, &mut line, &mut col);
                tokens.push(Token::Comma);
            }
            b'=' => {
                advance(&mut i, &mut line, &mut col);
                tokens.push(Token::Eq);
            }
            b'"' => {
                let str_line = line;
                let str_col = col;
                advance(&mut i, &mut line, &mut col);
                let mut s = String::new();
                loop {
                    match advance(&mut i, &mut line, &mut col) {
                        Some(b'"') => break,
                        Some(b'\\') => match advance(&mut i, &mut line, &mut col) {
                            Some(b'"') => s.push('"'),
                            Some(b'\\') => s.push('\\'),
                            Some(b'n') => s.push('\n'),
                            _ => return Err(format!("{line}:{col}: bad escape")),
                        },
                        Some(ch) => s.push(ch as char),
                        None => return Err(format!("{line}:{col}: unterminated string")),
                    }
                }
                if s.chars().any(char::is_whitespace) {
                    return Err(format!(
                        "{str_line}:{str_col}: keyword literal {s:?} contains whitespace; \
                         split into separate single-word literals (use the `space` keyword \
                         for explicit inter-token spaces). See issue #5."
                    ));
                }
                tokens.push(Token::Str(s));
            }
            _ if ch.is_ascii_alphabetic() || ch == b'_' => {
                let start = i;
                while b
                    .get(i)
                    .is_some_and(|c| c.is_ascii_alphanumeric() || *c == b'_')
                {
                    advance(&mut i, &mut line, &mut col);
                }
                tokens.push(Token::Ident(String::from_utf8_lossy(&b[start..i]).into()));
            }
            _ if ch.is_ascii_digit() => {
                let start = i;
                while b.get(i).is_some_and(u8::is_ascii_digit) {
                    advance(&mut i, &mut line, &mut col);
                }
                let n = std::str::from_utf8(&b[start..i])
                    .expect("digit sequence is valid UTF-8")
                    .parse()
                    .expect("digit sequence is a valid u32");
                tokens.push(Token::Int(n));
            }
            _ => return Err(format!("{}:{}: unexpected '{}'", line, col, ch as char)),
        }
    }
    Ok(tokens)
}

// ── Semantic annotation helpers ──────────────────────────────────────────

fn get_param<'p>(params: &'p [(String, String)], key: &str) -> Option<&'p str> {
    params
        .iter()
        .find(|(k, _)| k == key)
        .map(|(_, v)| v.as_str())
}

fn require_param(
    params: &[(String, String)],
    key: &str,
    node_name: &str,
    role: &str,
) -> Result<String, String> {
    get_param(params, key).map(str::to_string).ok_or_else(|| {
        format!("semantic role '{role}' in node '{node_name}' requires '{key}' parameter")
    })
}

// ── Parser ───────────────────────────────────────────────────────────────

struct Parser {
    tokens: Vec<Token>,
    pos: usize,
}

impl Parser {
    const fn new(tokens: Vec<Token>) -> Self {
        Self { tokens, pos: 0 }
    }
    fn peek(&self) -> &Token {
        &self.tokens[self.pos]
    }
    fn advance(&mut self) -> Token {
        let t = self.tokens[self.pos].clone();
        if t != Token::Eof {
            self.pos += 1;
        }
        t
    }
    fn at(&self, s: &str) -> bool {
        matches!(self.peek(), Token::Ident(n) if n == s)
    }
    fn at_tok(&self, t: &Token) -> bool {
        self.peek() == t
    }
    fn expect(&mut self, t: &Token) -> Result<(), String> {
        let got = self.advance();
        if got == *t {
            Ok(())
        } else {
            Err(format!("expected {t:?}, got {got:?}"))
        }
    }
    fn ident(&mut self) -> Result<String, String> {
        match self.advance() {
            Token::Ident(s) => Ok(s),
            t => Err(format!("expected ident, got {t:?}")),
        }
    }
    fn string(&mut self) -> Result<String, String> {
        match self.advance() {
            Token::Str(s) => Ok(s),
            t => Err(format!("expected string, got {t:?}")),
        }
    }
    fn int(&mut self) -> Result<u32, String> {
        match self.advance() {
            Token::Int(n) => Ok(n),
            t => Err(format!("expected int, got {t:?}")),
        }
    }

    fn parse_file(&mut self) -> Result<Vec<Item>, String> {
        let mut items = Vec::new();
        while !self.at_tok(&Token::Eof) {
            items.push(self.parse_item()?);
        }
        Ok(items)
    }

    fn parse_item(&mut self) -> Result<Item, String> {
        if self.at("node") {
            self.advance();
            return self.parse_node_item();
        }
        if self.at("enum") {
            self.advance();
            return self.parse_enum();
        }
        if self.at("flags") {
            self.advance();
            return self.parse_flags();
        }
        if self.at("list") {
            self.advance();
            return self.parse_list();
        }
        if self.at("abstract") {
            self.advance();
            return self.parse_abstract();
        }
        Err(format!("expected item keyword, got {:?}", self.peek()))
    }

    fn parse_node_item(&mut self) -> Result<Item, String> {
        let name = self.ident()?;
        self.expect(&Token::LBrace)?;
        let mut fields = Vec::new();
        let mut fmt = None;
        let mut semantic = None;
        let mut macro_def = None;
        let mut fmt_precedence = None;
        let mut fmt_group = None;
        loop {
            if self.at_tok(&Token::RBrace) {
                self.advance();
                break;
            }
            if self.at("fmt") {
                self.advance();
                self.expect(&Token::LBrace)?;
                fmt = Some(self.parse_fmt_seq()?);
                self.expect(&Token::RBrace)?;
            } else if self.at("fmt_precedence") {
                self.advance();
                self.expect(&Token::LParen)?;
                let n = self.int()?;
                fmt_precedence = Some(u8::try_from(n).map_err(|_| "fmt_precedence must fit u8")?);
                self.expect(&Token::RParen)?;
            } else if self.at("fmt_group") {
                self.advance();
                self.expect(&Token::LParen)?;
                let n = self.int()?;
                fmt_group = Some(u8::try_from(n).map_err(|_| "fmt_group must fit u8")?);
                self.expect(&Token::RParen)?;
            } else if self.at("semantic") {
                self.advance();
                self.expect(&Token::LBrace)?;
                semantic = Some(self.parse_semantic(&name, &fields)?);
                self.expect(&Token::RBrace)?;
            } else if self.at("macro_def") {
                self.advance();
                macro_def = Some(self.parse_macro_def(&name, &fields)?);
            } else {
                let name = self.ident()?;
                self.expect(&Token::Colon)?;
                let storage = if self.at("index") {
                    self.advance();
                    Storage::Index
                } else if self.at("inline") {
                    self.advance();
                    Storage::Inline
                } else {
                    return Err(format!("expected storage class, got {:?}", self.peek()));
                };
                let type_name = self.ident()?;
                fields.push(Field {
                    name,
                    storage,
                    type_name,
                });
            }
        }
        Ok(Item::Node {
            name,
            fields,
            fmt,
            semantic,
            macro_def,
            fmt_precedence,
            fmt_group,
        })
    }

    /// Parse a `semantic { role(...) }` block (new syntax).
    #[expect(clippy::too_many_lines)]
    fn parse_semantic(
        &mut self,
        node_name: &str,
        fields: &[Field],
    ) -> Result<SemanticAnnotation, String> {
        let role_name = self.ident()?;
        // For source_ref, `kind` is a literal enum value (not a field name).
        let literal_keys: &[&str] = if role_name == "source_ref" {
            &["kind"]
        } else {
            &[]
        };
        let params = self.parse_semantic_params(node_name, fields, literal_keys)?;
        let role = match role_name.as_str() {
            // ── Catalog roles ────────────────────────────────────────────
            "define_table" => {
                let without_rowid = get_param(&params, "without_rowid").map(|v| {
                    let (field, bit) = v.split_once('.').unwrap_or_else(|| {
                        panic!(
                            "define_table without_rowid must use dotted syntax \
                                (e.g. flags.without_rowid), got '{v}'"
                        )
                    });
                    (field.to_string(), bit.to_string())
                });
                SemanticRole::DefineTable {
                    name: require_param(&params, "name", node_name, "define_table")?,
                    columns: get_param(&params, "columns").map(str::to_string),
                    select: get_param(&params, "select").map(str::to_string),
                    without_rowid,
                }
            }
            "define_view" => SemanticRole::DefineView {
                name: require_param(&params, "name", node_name, "define_view")?,
                columns: get_param(&params, "columns").map(str::to_string),
                select: require_param(&params, "select", node_name, "define_view")?,
            },
            "define_function" => SemanticRole::DefineFunction {
                name: require_param(&params, "name", node_name, "define_function")?,
                args: get_param(&params, "args").map(str::to_string),
                return_type: get_param(&params, "return_type").map(str::to_string),
                select: get_param(&params, "select").map(str::to_string),
            },
            "return_spec" => SemanticRole::ReturnSpec {
                columns: get_param(&params, "table_columns").map(str::to_string),
            },
            "import" => SemanticRole::Import {
                module: require_param(&params, "module", node_name, "import")?,
            },
            // ── Column-list items ─────────────────────────────────────────
            "column_def" => SemanticRole::ColumnDef {
                name: require_param(&params, "name", node_name, "column_def")?,
                type_name: get_param(&params, "type").map(str::to_string),
                constraints: get_param(&params, "constraints").map(str::to_string),
            },
            // ── Result columns ────────────────────────────────────────────
            "result_column" => SemanticRole::ResultColumn {
                flags: require_param(&params, "flags", node_name, "result_column")?,
                alias: require_param(&params, "alias", node_name, "result_column")?,
                expr: require_param(&params, "expr", node_name, "result_column")?,
            },
            // ── Expressions ───────────────────────────────────────────────
            "call" => SemanticRole::Call {
                name: require_param(&params, "name", node_name, "call")?,
                args: require_param(&params, "args", node_name, "call")?,
            },
            "column_ref" => SemanticRole::ColumnRef {
                column: require_param(&params, "column", node_name, "column_ref")?,
                table: require_param(&params, "table", node_name, "column_ref")?,
            },
            // ── Sources ───────────────────────────────────────────────────
            "source_ref" => SemanticRole::SourceRef {
                kind: require_param(&params, "kind", node_name, "source_ref")?,
                name: require_param(&params, "name", node_name, "source_ref")?,
                alias: require_param(&params, "alias", node_name, "source_ref")?,
            },
            "scoped_source" => SemanticRole::ScopedSource {
                body: require_param(&params, "body", node_name, "scoped_source")?,
                alias: require_param(&params, "alias", node_name, "scoped_source")?,
            },
            // ── Scope structure ───────────────────────────────────────────
            "query" => SemanticRole::Query {
                from: require_param(&params, "from", node_name, "query")?,
                columns: require_param(&params, "columns", node_name, "query")?,
                where_clause: require_param(&params, "where_clause", node_name, "query")?,
                groupby: require_param(&params, "groupby", node_name, "query")?,
                having: require_param(&params, "having", node_name, "query")?,
                orderby: require_param(&params, "orderby", node_name, "query")?,
                limit_clause: require_param(&params, "limit_clause", node_name, "query")?,
            },
            "cte_binding" => SemanticRole::CteBinding {
                name: require_param(&params, "name", node_name, "cte_binding")?,
                columns: get_param(&params, "columns").map(str::to_string),
                body: require_param(&params, "body", node_name, "cte_binding")?,
            },
            "cte_scope" => SemanticRole::CteScope {
                recursive: require_param(&params, "recursive", node_name, "cte_scope")?,
                bindings: require_param(&params, "bindings", node_name, "cte_scope")?,
                body: require_param(&params, "body", node_name, "cte_scope")?,
            },
            "trigger_scope" => SemanticRole::TriggerScope {
                target: require_param(&params, "target", node_name, "trigger_scope")?,
                when: require_param(&params, "when", node_name, "trigger_scope")?,
                body: require_param(&params, "body", node_name, "trigger_scope")?,
            },
            "dml_scope" => SemanticRole::DmlScope {
                with_recursive: get_param(&params, "with_recursive").map(str::to_string),
                with_ctes: get_param(&params, "with_ctes").map(str::to_string),
            },
            "upsert_scope" => SemanticRole::UpsertScope {
                columns: get_param(&params, "columns").map(str::to_string),
                target_where: get_param(&params, "target_where").map(str::to_string),
                setlist: get_param(&params, "setlist").map(str::to_string),
                update_where: get_param(&params, "update_where").map(str::to_string),
            },
            "compound_scope" => SemanticRole::CompoundScope {
                left: require_param(&params, "left", node_name, "compound_scope")?,
                right: require_param(&params, "right", node_name, "compound_scope")?,
                orderby: get_param(&params, "orderby").map(str::to_string),
                limit_clause: get_param(&params, "limit_clause").map(str::to_string),
            },
            "alter_table" => SemanticRole::AlterTable {
                op: require_param(&params, "op", node_name, "alter_table")?,
                target: require_param(&params, "target", node_name, "alter_table")?,
                new_name: require_param(&params, "new_name", node_name, "alter_table")?,
                old_name: require_param(&params, "old_name", node_name, "alter_table")?,
                column: get_param(&params, "column").map(str::to_string),
            },
            _ => {
                return Err(format!(
                    "unknown semantic role '{role_name}' in node '{node_name}'"
                ));
            }
        };
        Ok(SemanticAnnotation { role })
    }

    /// Parse a `macro_def(name: ..., body: ..., args: ..., arg_name: ...)` block.
    ///
    /// Marks the node as a template macro definition. The formatter uses this
    /// to extract macro name/params/body and register them for expansion.
    fn parse_macro_def(
        &mut self,
        node_name: &str,
        fields: &[Field],
    ) -> Result<MacroDefAnnotation, String> {
        let params = self.parse_semantic_params(node_name, fields, &["arg_name"])?;
        let name = require_param(&params, "name", node_name, "macro_def")?;
        let body = require_param(&params, "body", node_name, "macro_def")?;
        let args = get_param(&params, "args").map(str::to_string);
        let arg_name = get_param(&params, "arg_name").map(str::to_string);
        if args.is_some() && arg_name.is_none() {
            return Err(format!(
                "macro_def in '{node_name}' specifies 'args' but missing 'arg_name' \
                 (the field within each arg list item that holds the param name)"
            ));
        }
        Ok(MacroDefAnnotation {
            name,
            body,
            args,
            arg_name,
        })
    }

    /// Parse `(key: value, ...)` parameter list.
    ///
    /// Values listed in `literal_keys` are accepted as-is (any ident); all
    /// other values are validated as field names declared in the node.
    fn parse_semantic_params(
        &mut self,
        node_name: &str,
        fields: &[Field],
        literal_keys: &[&str],
    ) -> Result<Vec<(String, String)>, String> {
        self.expect(&Token::LParen)?;
        let mut params = Vec::new();
        while !self.at_tok(&Token::RParen) {
            if !params.is_empty() {
                self.expect(&Token::Comma)?;
            }
            let key = self.ident()?;
            self.expect(&Token::Colon)?;
            let value = self.ident()?;
            // Support dotted flag references: `key: field.bit_name`.
            if self.at_tok(&Token::Dot) {
                self.advance();
                let bit = self.ident()?;
                if !fields.iter().any(|f| f.name == value) {
                    return Err(format!(
                        "semantic annotation in '{node_name}' references unknown field '{value}'"
                    ));
                }
                params.push((key, format!("{value}.{bit}")));
            } else {
                if !literal_keys.contains(&key.as_str()) && !fields.iter().any(|f| f.name == value)
                {
                    return Err(format!(
                        "semantic annotation in '{node_name}' references unknown field '{value}'"
                    ));
                }
                params.push((key, value));
            }
        }
        self.advance(); // consume RParen
        Ok(params)
    }

    fn parse_enum(&mut self) -> Result<Item, String> {
        let name = self.ident()?;
        self.expect(&Token::LBrace)?;
        let mut variants = Vec::new();
        let mut fmt_precedence = Vec::new();
        let mut fmt_group = Vec::new();
        let mut fmt_paren_boundary = Vec::new();
        while !self.at_tok(&Token::RBrace) {
            if self.at("fmt_precedence") {
                self.advance();
                self.expect(&Token::LBrace)?;
                while !self.at_tok(&Token::RBrace) {
                    let v = self.ident()?;
                    self.expect(&Token::Eq)?;
                    let n = self.int()?;
                    fmt_precedence.push((v, u8::try_from(n).map_err(|_| "prec must fit u8")?));
                }
                self.advance();
            } else if self.at("fmt_group") {
                self.advance();
                self.expect(&Token::LBrace)?;
                while !self.at_tok(&Token::RBrace) {
                    let v = self.ident()?;
                    self.expect(&Token::Eq)?;
                    let n = self.int()?;
                    fmt_group.push((v, u8::try_from(n).map_err(|_| "group must fit u8")?));
                }
                self.advance();
            } else if self.at("fmt_paren_boundary") {
                self.advance();
                self.expect(&Token::LBrace)?;
                while !self.at_tok(&Token::RBrace) {
                    fmt_paren_boundary.push(self.ident()?);
                }
                self.advance();
            } else {
                variants.push(self.ident()?);
            }
        }
        self.advance();
        Ok(Item::Enum {
            name,
            variants,
            fmt_precedence,
            fmt_group,
            fmt_paren_boundary,
        })
    }

    fn parse_flags(&mut self) -> Result<Item, String> {
        let name = self.ident()?;
        self.expect(&Token::LBrace)?;
        let mut flags = Vec::new();
        while !self.at_tok(&Token::RBrace) {
            let n = self.ident()?;
            self.expect(&Token::Eq)?;
            let v = self.int()?;
            flags.push((n, v));
        }
        self.advance();
        Ok(Item::Flags { name, flags })
    }

    fn parse_abstract(&mut self) -> Result<Item, String> {
        let name = self.ident()?;
        self.expect(&Token::LBrace)?;
        let mut members = Vec::new();
        while !self.at_tok(&Token::RBrace) {
            members.push(self.ident()?);
        }
        self.advance();
        Ok(Item::Abstract { name, members })
    }

    fn parse_list(&mut self) -> Result<Item, String> {
        let name = self.ident()?;
        self.expect(&Token::LBrace)?;
        let child_type = self.ident()?;
        let mut fmt = None;
        let mut prepend = false;
        loop {
            if self.at("fmt") {
                self.advance();
                self.expect(&Token::LBrace)?;
                let items = self.parse_fmt_seq()?;
                self.expect(&Token::RBrace)?;
                fmt = Some(items);
            } else if self.at("prepend") {
                self.advance();
                prepend = true;
            } else {
                break;
            }
        }
        self.expect(&Token::RBrace)?;
        Ok(Item::List {
            name,
            child_type,
            fmt,
            prepend,
        })
    }

    // ── fmt parsing ──────────────────────────────────────────────────

    fn parse_fmt_seq(&mut self) -> Result<Vec<Fmt>, String> {
        let mut items = Vec::new();
        while !matches!(self.peek(), Token::RBrace | Token::Eof) {
            items.push(self.parse_fmt()?);
        }
        Ok(items)
    }

    fn braced(&mut self) -> Result<Vec<Fmt>, String> {
        self.expect(&Token::LBrace)?;
        let items = self.parse_fmt_seq()?;
        self.expect(&Token::RBrace)?;
        Ok(items)
    }

    fn optional_else(&mut self) -> Result<Option<Vec<Fmt>>, String> {
        if self.at("else") {
            self.advance();
            Ok(Some(self.braced()?))
        } else {
            Ok(None)
        }
    }

    #[expect(clippy::too_many_lines)]
    fn parse_fmt(&mut self) -> Result<Fmt, String> {
        match self.peek().clone() {
            Token::Str(s) => {
                self.advance();
                Ok(Fmt::Text(s))
            }
            Token::Ident(ref s) => match s.as_str() {
                "child" => {
                    self.advance();
                    self.expect(&Token::LParen)?;
                    let f = self.ident()?;
                    self.expect(&Token::RParen)?;
                    Ok(Fmt::Child(f))
                }
                "span" => {
                    self.advance();
                    self.expect(&Token::LParen)?;
                    let f = self.ident()?;
                    self.expect(&Token::RParen)?;
                    Ok(Fmt::Span(f))
                }
                "line" => {
                    self.advance();
                    Ok(Fmt::Line)
                }
                "softline" => {
                    self.advance();
                    Ok(Fmt::SoftLine)
                }
                "hardline" => {
                    self.advance();
                    Ok(Fmt::HardLine)
                }
                "space" => {
                    self.advance();
                    Ok(Fmt::Space)
                }
                "group" => {
                    self.advance();
                    Ok(Fmt::Group(self.braced()?))
                }
                "nest" => {
                    self.advance();
                    Ok(Fmt::Nest(self.braced()?))
                }
                "if_set" => {
                    self.advance();
                    self.expect(&Token::LParen)?;
                    let field = self.ident()?;
                    self.expect(&Token::RParen)?;
                    let then = self.braced()?;
                    let els = self.optional_else()?;
                    Ok(Fmt::IfSet { field, then, els })
                }
                "if_flag" => {
                    self.advance();
                    self.expect(&Token::LParen)?;
                    let field = self.ident()?;
                    let bit = if self.at_tok(&Token::Dot) {
                        self.advance();
                        Some(self.ident()?)
                    } else {
                        None
                    };
                    self.expect(&Token::RParen)?;
                    let then = self.braced()?;
                    let els = self.optional_else()?;
                    Ok(Fmt::IfFlag {
                        field,
                        bit,
                        then,
                        els,
                    })
                }
                "if_enum" => {
                    self.advance();
                    self.expect(&Token::LParen)?;
                    let field = self.ident()?;
                    self.expect(&Token::Comma)?;
                    let variant = self.ident()?;
                    self.expect(&Token::RParen)?;
                    let then = self.braced()?;
                    let els = self.optional_else()?;
                    Ok(Fmt::IfEnum {
                        field,
                        variant,
                        then,
                        els,
                    })
                }
                "if_span" => {
                    self.advance();
                    self.expect(&Token::LParen)?;
                    let field = self.ident()?;
                    self.expect(&Token::RParen)?;
                    let then = self.braced()?;
                    let els = self.optional_else()?;
                    Ok(Fmt::IfSpan { field, then, els })
                }
                "clause" => {
                    self.advance();
                    self.expect(&Token::LParen)?;
                    let mut keywords = vec![self.string()?];
                    self.expect(&Token::Comma)?;
                    while let Token::Str(_) = self.peek() {
                        keywords.push(self.string()?);
                        self.expect(&Token::Comma)?;
                    }
                    let field = self.ident()?;
                    self.expect(&Token::RParen)?;
                    Ok(Fmt::Clause { keywords, field })
                }
                "switch" => {
                    self.advance();
                    self.expect(&Token::LParen)?;
                    let field = self.ident()?;
                    self.expect(&Token::RParen)?;
                    self.expect(&Token::LBrace)?;
                    let mut cases = Vec::new();
                    let mut default = None;
                    while !self.at_tok(&Token::RBrace) {
                        if self.at("default") {
                            self.advance();
                            default = Some(self.braced()?);
                        } else {
                            let v = self.ident()?;
                            cases.push((v, self.braced()?));
                        }
                    }
                    self.advance();
                    Ok(Fmt::Switch {
                        field,
                        cases,
                        default,
                    })
                }
                "enum_display" => {
                    self.advance();
                    self.expect(&Token::LParen)?;
                    let field = self.ident()?;
                    self.expect(&Token::Comma)?;
                    self.expect(&Token::LBrace)?;
                    let mut mappings = Vec::new();
                    while !self.at_tok(&Token::RBrace) {
                        let v = self.ident()?;
                        self.expect(&Token::Eq)?;
                        let d = self.string()?;
                        mappings.push((v, d));
                    }
                    self.advance();
                    self.expect(&Token::RParen)?;
                    Ok(Fmt::EnumDisplay { field, mappings })
                }
                "child_prec" => {
                    self.advance();
                    self.expect(&Token::LParen)?;
                    let child_field = self.ident()?;
                    self.expect(&Token::Comma)?;
                    let op_field = self.ident()?;
                    let is_right = if self.at_tok(&Token::Comma) {
                        self.advance();
                        let tag = self.ident()?;
                        if tag != "right" {
                            return Err(format!(
                                "child_prec third arg must be 'right', got '{tag}'"
                            ));
                        }
                        true
                    } else {
                        false
                    };
                    self.expect(&Token::RParen)?;
                    Ok(Fmt::ChildPrec {
                        child_field,
                        op_field,
                        is_right,
                    })
                }
                "child_paren_list" => {
                    self.advance();
                    self.expect(&Token::LParen)?;
                    let f = self.ident()?;
                    self.expect(&Token::RParen)?;
                    Ok(Fmt::ChildParenList(f))
                }
                "child_prec_fixed" => {
                    self.advance();
                    self.expect(&Token::LParen)?;
                    let child_field = self.ident()?;
                    let is_right = if self.at_tok(&Token::Comma) {
                        self.advance();
                        let tag = self.ident()?;
                        if tag != "right" {
                            return Err(format!(
                                "child_prec_fixed second arg must be 'right', got '{tag}'"
                            ));
                        }
                        true
                    } else {
                        false
                    };
                    self.expect(&Token::RParen)?;
                    Ok(Fmt::ChildPrecFixed {
                        child_field,
                        is_right,
                    })
                }
                "for_each" => {
                    self.advance();
                    let sep = if self.at_tok(&Token::LParen) {
                        self.advance();
                        if !self.at("sep") {
                            return Err(format!("expected 'sep', got {:?}", self.peek()));
                        }
                        self.advance();
                        self.expect(&Token::Colon)?;
                        let mut items = Vec::new();
                        while !self.at_tok(&Token::RParen) {
                            items.push(self.parse_fmt()?);
                        }
                        self.advance();
                        Some(items)
                    } else {
                        None
                    };
                    let body = self.braced()?;
                    Ok(Fmt::ForEach { sep, body })
                }
                _ => Err(format!("unexpected in fmt: {:?}", self.peek())),
            },
            other => Err(format!("unexpected in fmt: {other:?}")),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn parse_node(synq: &str) -> Item {
        let items = parse_synq_file(synq).expect("parse failed");
        assert_eq!(items.len(), 1);
        items.into_iter().next().unwrap()
    }

    fn node_semantic(item: Item) -> Option<SemanticAnnotation> {
        match item {
            Item::Node { semantic, .. } => semantic,
            _ => panic!("expected Node"),
        }
    }

    #[test]
    fn keyword_literal_with_whitespace_is_rejected() {
        let err = parse_synq_file(r#"node Foo { field: inline X fmt { "ORDER BY" } }"#)
            .expect_err("expected error");
        assert!(
            err.message.contains("contains whitespace"),
            "unexpected error: {}",
            err.message
        );
    }

    #[test]
    fn keyword_literal_with_trailing_space_is_rejected() {
        let err = parse_synq_file(r#"node Foo { field: inline X fmt { "FROM " } }"#)
            .expect_err("expected error");
        assert!(err.message.contains("contains whitespace"));
    }

    #[test]
    fn keyword_literal_in_clause_with_whitespace_is_rejected() {
        let err = parse_synq_file(r#"node Foo { x: index Y fmt { clause("ORDER BY", x) } }"#)
            .expect_err("expected error");
        assert!(err.message.contains("contains whitespace"));
    }

    #[test]
    fn semantic_define_table_parses() {
        let item = parse_node(
            r"node CreateTableStmt {
                table_name: inline SyntaqliteTextSpan
                columns: index ColumnDefList
                as_select: index Select
                semantic { define_table(name: table_name, columns: columns, select: as_select) }
            }",
        );
        let ann = node_semantic(item).expect("expected semantic annotation");
        match ann.role {
            SemanticRole::DefineTable {
                name,
                columns,
                select,
                without_rowid,
            } => {
                assert_eq!(name, "table_name");
                assert_eq!(columns.as_deref(), Some("columns"));
                assert_eq!(select.as_deref(), Some("as_select"));
                assert!(without_rowid.is_none());
            }
            other => panic!("expected DefineTable, got {other:?}"),
        }
    }

    #[test]
    fn semantic_import_uses_module_key() {
        let item = parse_node(
            r"node IncludePerfettoModuleStmt {
                module_name: inline SyntaqliteTextSpan
                semantic { import(module: module_name) }
            }",
        );
        let ann = node_semantic(item).expect("expected semantic annotation");
        match ann.role {
            SemanticRole::Import { module } => assert_eq!(module, "module_name"),
            other => panic!("expected Import, got {other:?}"),
        }
    }

    #[test]
    fn semantic_unknown_field_is_error() {
        let result = parse_synq_file(
            r"node Foo {
                bar: inline SyntaqliteTextSpan
                semantic { define_table(name: nonexistent) }
            }",
        );
        assert!(result.is_err());
        assert!(result.unwrap_err().message.contains("unknown field"));
    }

    #[test]
    fn semantic_unknown_role_is_error() {
        let result = parse_synq_file(
            r"node Foo {
                bar: inline SyntaqliteTextSpan
                semantic { frobnicate(name: bar) }
            }",
        );
        assert!(result.is_err());
        assert!(
            result
                .unwrap_err()
                .message
                .contains("unknown semantic role")
        );
    }

    #[test]
    fn node_without_semantic_has_none() {
        let item = parse_node(
            r"node Foo {
                bar: inline SyntaqliteTextSpan
            }",
        );
        assert!(node_semantic(item).is_none());
    }
}
