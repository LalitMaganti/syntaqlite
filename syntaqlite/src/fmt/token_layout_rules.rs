//! SQL layout policy over grammar symbols. These choices affect whitespace only.
//! Lists and operator chains stay open until a clause, item, or enclosure owns
//! their grouping. No rule identifies, inserts, removes, or reorders source text.
use super::{Break, Fragment, Shape, State};

fn list(lhs: &str) -> bool {
    matches!(
        lhs,
        "selcollist"
            | "sclp"
            | "nexprlist"
            | "exprlist"
            | "sortlist"
            | "idlist"
            | "eidlist"
            | "columnlist"
            | "conslist"
            | "conslist_opt"
            | "wqlist"
            | "setlist"
            | "windowdefn_list"
            | "seltablist"
            | "stl_prefix"
            | "selectnowith"
            | "carglist"
            | "refargs"
            | "case_exprlist"
            | "trigger_cmd_list"
            | "mvalues"
            | "vtabarglist"
    )
}
fn clause(symbol: &str) -> bool {
    matches!(
        symbol,
        "from"
            | "where_opt"
            | "where_opt_ret"
            | "groupby_opt"
            | "having_opt"
            | "window_clause"
            | "orderby_opt"
            | "limit_opt"
            | "returning"
            | "upsert"
            | "frame_opt"
            | "select"
            | "selectnowith"
    )
}
pub(super) fn boundary(lhs: &str, children: &[Fragment<'_>], index: usize, shape: Shape) -> Break {
    let left = children[index - 1];
    let right = children[index];
    if matches!(right.first, "," | ")" | ";" | ".") || left.last == "." {
        return Break::Tight;
    }
    if left.last == "(" {
        return Break::Tight;
    }
    if right.first == "(" {
        // Calls and authored name-column lists bind tightly; SQL grouping uses
        // a space. The distinction is supplied by the grammar, not token search.
        if lhs == "typetoken"
            || (lhs == "table_source" && matches!(left.symbol, "nm" | "dbnm"))
            || (lhs == "expr"
                && matches!(left.symbol, "ID" | "INDEXED" | "JOIN_KW" | "CAST" | "RAISE"))
            || matches!(
                right.symbol,
                "idlist_opt" | "eidlist_opt" | "paren_exprlist"
            )
        {
            return Break::Tight;
        }
        return Break::Space;
    }
    if matches!(left.symbol, "PLUS" | "MINUS" | "BITNOT") && (lhs != "expr" || index == 1) {
        // Adjacent minus signs would become a line-comment token.
        return if left.last == "-" && right.first == "-" {
            Break::Space
        } else {
            Break::Tight
        };
    }
    if left.last == "," {
        return Break::Line;
    }
    if lhs == "trigger_cmd_list" && left.last == ";" {
        return Break::Hard;
    }
    if lhs == "refargs" {
        return Break::Line;
    }
    if lhs == "carglist" {
        return if left.trailing_prefix {
            Break::Space
        } else {
            Break::Line
        };
    }
    if lhs == "stl_prefix" && right.symbol == "joinop" {
        return Break::Line;
    }
    if lhs == "selectnowith" && matches!(right.symbol, "multiselect_op" | "oneselect") {
        return Break::Line;
    }
    if lhs == "filter_over" {
        return Break::Line;
    }
    if matches!(shape, Shape::Chain(_)) && index == 1 {
        return Break::Line;
    }
    Break::Space
}

fn header<'a>(state: &mut State<'a>, lhs: &'static str, children: &[Fragment<'a>]) -> Fragment<'a> {
    let name = children.iter().position(|f| {
        matches!(
            f.symbol,
            "nm" | "nmorerr" | "fullname" | "xfullname" | "trnm"
        )
    });
    let end = children
        .iter()
        .position(|f| matches!(f.symbol, "enclosure" | "idlist_opt" | "eidlist_opt"))
        .unwrap_or(children.len());
    if let Some(name) = name
        && name > 0
        && name < end
    {
        let prefix = state.sequence(lhs, &children[..name], Shape::Atom);
        let body = state.sequence(lhs, &children[name..end], Shape::Atom);
        let mut result = state.hanging(prefix, body);
        for index in end..children.len() {
            let boundary = boundary(lhs, children, index, Shape::Atom);
            result = state.append(result, children[index], boundary, Shape::Atom);
        }
        return state.grouped(result);
    }
    let sequence = state.sequence(lhs, children, Shape::Atom);
    state.grouped(sequence)
}

fn sections<'a>(
    state: &mut State<'a>,
    lhs: &'static str,
    children: &[Fragment<'a>],
) -> Fragment<'a> {
    let mut result = Fragment::empty(lhs);
    let mut start = 0;
    for index in 0..=children.len() {
        if index != children.len()
            && (index == start
                || (!clause(children[index].symbol)
                    && !matches!(children[index].symbol, "SET" | "RETURNING")))
        {
            continue;
        }
        let part = &children[start..index];
        let section = if part
            .first()
            .is_some_and(|f| matches!(f.symbol, "SET" | "RETURNING"))
            && part.len() > 1
        {
            let body = state.sequence(lhs, &part[1..], Shape::List);
            state.hanging(part[0], body)
        } else {
            header(state, lhs, part)
        };
        result = state.append(result, section, Break::Line, Shape::Atom);
        start = index;
    }
    state.grouped(result)
}

pub(super) fn layout<'a>(
    state: &mut State<'a>,
    lhs: &'static str,
    rule: &'static str,
    children: &mut Vec<Fragment<'a>>,
) -> Fragment<'a> {
    if children.is_empty() {
        return Fragment::empty(lhs);
    }
    // Grammar wrappers add no group or indentation. List wrappers retain their
    // open sequence so left recursion cannot accumulate nesting or fits work.
    if children.len() == 1 {
        let mut child = children[0];
        if list(lhs) {
            child.shape = Shape::List;
        }
        return child;
    }
    if let Some(begin) = children.iter().position(|f| f.symbol == "BEGIN")
        && children.last().is_some_and(|f| f.symbol == "END")
    {
        let last = children.len() - 1;
        let body = state.sequence("trigger_cmd_list", &children[begin + 1..last], Shape::List);
        let block = state.enclosure(children[begin], body, children[last], true);
        let header = state.sequence(lhs, &children[..begin], Shape::Atom);
        return state.append(header, block, Break::Hard, Shape::Atom);
    }
    if lhs == "expr" && children[0].symbol == "CASE" {
        let arm = children
            .iter()
            .position(|f| f.symbol == "case_exprlist")
            .expect("CASE arms");
        let last = children.len() - 1;
        let head = state.sequence(lhs, &children[..arm], Shape::Atom);
        let mut body = Fragment::empty("arms");
        for &child in &children[arm..last] {
            body = state.append(body, child, Break::Line, Shape::List);
        }
        let inside = state.hanging(head, body);
        let result = state.append(inside, children[last], Break::Line, Shape::Atom);
        return state.grouped(result);
    }
    if lhs == "case_exprlist" {
        let prefix = usize::from(children[0].symbol == lhs);
        let then = children
            .iter()
            .position(|f| f.symbol == "THEN")
            .expect("CASE arm THEN");
        let head = state.sequence("case_arm", &children[prefix..=then], Shape::Atom);
        let body = state.sequence("case_arm", &children[then + 1..], Shape::Atom);
        let arm = state.hanging(head, body);
        return if prefix == 1 {
            state.append(children[0], arm, Break::Line, Shape::List)
        } else {
            Fragment {
                shape: Shape::List,
                ..arm
            }
        };
    }
    if lhs == "expr"
        && let Some(suffix) = children.iter().position(|f| f.symbol == "filter_over")
    {
        let head = state.sequence("expr", &children[..suffix], Shape::Atom);
        let body = state.sequence("function_suffix", &children[suffix..], Shape::List);
        return state.suffix(head, body);
    }
    if lhs == "selcollist" {
        let prefix = usize::from(children[0].symbol == "sclp");
        let alias = children
            .iter()
            .position(|f| f.symbol == "as")
            .unwrap_or(children.len());
        let head = state.sequence("projection_item", &children[prefix..alias], Shape::Atom);
        let body = state.sequence("projection_item", &children[alias..], Shape::Atom);
        let projection = state.suffix(head, body);
        children.truncate(prefix);
        children.push(projection);
    }
    if lhs == "columnlist" {
        let column = children
            .iter()
            .position(|f| f.symbol == "columnname")
            .expect("column declaration");
        let constraints = state.sequence("carglist", &children[column + 1..], Shape::List);
        let declaration = state.hanging(children[column], constraints);
        children.truncate(column);
        children.push(declaration);
    }
    if matches!(lhs, "sortlist" | "eidlist") {
        let prefix = if children[0].symbol == lhs { 2 } else { 0 };
        let modifiers = state.sequence("sort_modifiers", &children[prefix + 1..], Shape::Atom);
        let item = state.suffix(children[prefix], modifiers);
        children.truncate(prefix);
        children.push(item);
    }
    if lhs == "carglist" && children[0].trailing_prefix {
        let body = state.sequence("constraint_body", &children[1..], Shape::Atom);
        let result = state.suffix(children[0], body);
        return Fragment {
            shape: Shape::List,
            trailing_prefix: children.last().expect("constraint body").trailing_prefix,
            ..result
        };
    }
    if lhs == "ccons"
        && let Some(options) = children.iter().position(|f| f.symbol == "refargs")
    {
        let head = state.sequence(lhs, &children[..options], Shape::Atom);
        let body = state.sequence("refargs", &children[options..], Shape::List);
        return state.suffix(head, body);
    }
    if lhs == "trigger_decl" {
        let event = children
            .iter()
            .position(|f| matches!(f.symbol, "trigger_time" | "trigger_event"))
            .expect("trigger event");
        let predicate = children
            .iter()
            .position(|f| f.symbol == "when_clause")
            .unwrap_or(children.len());
        let declaration = header(state, lhs, &children[..event]);
        let event = header(state, "trigger_event_header", &children[event..predicate]);
        let mut out = state.append(declaration, event, Break::Line, Shape::Atom);
        for &child in &children[predicate..] {
            out = state.append(out, child, Break::Line, Shape::Atom);
        }
        return state.grouped(out);
    }
    if lhs == "setlist" {
        let prefix = if children[0].symbol == lhs { 2 } else { 0 };
        let equal = children
            .iter()
            .position(|f| f.symbol == "EQ")
            .expect("assignment equals");
        let head = state.sequence("assignment", &children[prefix..=equal], Shape::Atom);
        let body = state.sequence("assignment", &children[equal + 1..], Shape::Atom);
        let assignment = state.hanging(head, body);
        children.truncate(prefix);
        children.push(assignment);
    }
    if lhs == "seltablist" {
        let prefix = usize::from(children[0].symbol == "stl_prefix");
        let constraint = children
            .iter()
            .position(|f| f.symbol == "on_using")
            .unwrap_or(children.len());
        let alias = children
            .iter()
            .position(|f| f.symbol == "as")
            .unwrap_or(constraint);
        let head = state.sequence("table_source", &children[prefix..alias], Shape::Atom);
        let alias = state.sequence("table_source", &children[alias..constraint], Shape::Atom);
        let head = state.suffix(head, alias);
        let body = state.sequence("table_source", &children[constraint..], Shape::Atom);
        let table = state.hanging(head, body);
        children.truncate(prefix);
        children.push(table);
    }
    // BETWEEN has a pair of bounds, not a boolean AND chain. Both expression
    // bounds and window-frame bounds use the same hanging layout operation.
    if matches!(lhs, "expr" | "frame_opt")
        && let Some(between) = children
            .iter()
            .position(|f| matches!(f.symbol, "between_op" | "BETWEEN"))
        && let Some(and) = children.iter().position(|f| f.symbol == "AND")
    {
        let head = state.sequence("bound_header", &children[..=between], Shape::Atom);
        let lower = state.sequence("bound", &children[between + 1..and], Shape::Atom);
        let upper_end = children
            .iter()
            .position(|f| f.symbol == "frame_exclude_opt")
            .unwrap_or(children.len());
        let upper = state.sequence("bound", &children[and..upper_end], Shape::Atom);
        let bounds = state.append(lower, upper, Break::Line, Shape::List);
        let mut result = state.hanging(head, bounds);
        for &child in &children[upper_end..] {
            result = state.append(result, child, Break::Line, Shape::Atom);
        }
        return state.grouped(result);
    }
    if lhs == "where_opt_ret" && children.iter().any(|f| f.symbol == "RETURNING") {
        let returning = children
            .iter()
            .position(|f| f.symbol == "RETURNING")
            .expect("returning clause");
        let head = state.sequence(
            "returning",
            &children[returning..returning + 1],
            Shape::Atom,
        );
        let body = state.sequence("returning", &children[returning + 1..], Shape::List);
        let result = state.hanging(head, body);
        if returning == 0 {
            return result;
        }
        let body = state.sequence("where_opt", &children[1..returning], Shape::Atom);
        let predicate = state.hanging(children[0], body);
        let combined = state.append(predicate, result, Break::Line, Shape::Atom);
        return state.grouped(combined);
    }
    if lhs == "oneselect" && children[0].symbol == "SELECT" {
        let columns = children
            .iter()
            .position(|f| f.symbol == "selcollist")
            .expect("SELECT projection");
        let head = state.sequence(lhs, &children[..columns], Shape::Atom);
        let projection = state.hanging(head, children[columns]);
        let mut query = projection;
        for &child in &children[columns + 1..] {
            query = state.append(query, child, Break::Line, Shape::Atom);
        }
        return state.grouped(query);
    }
    if matches!(lhs, "select" | "with") && children[0].symbol == "WITH" {
        let items = children
            .iter()
            .position(|f| f.symbol == "wqlist")
            .expect("WITH items");
        let head = state.sequence(lhs, &children[..items], Shape::Atom);
        let mut out = state.hanging(head, children[items]);
        for &child in &children[items + 1..] {
            out = state.append(out, child, Break::Line, Shape::Atom);
        }
        return state.grouped(out);
    }
    // A clause's header and body share one hanging group. Subsequent clauses
    // are siblings, so their indentation does not inherit the preceding body.
    let body = match lhs {
        "from" => Some("seltablist"),
        "on_using" if children[0].symbol == "ON" => Some("expr"),
        "where_opt" | "where_opt_ret" | "having_opt" | "when_clause" => Some("expr"),
        "groupby_opt" => Some("nexprlist"),
        "orderby_opt" => Some("sortlist"),
        "window_clause" => Some("windowdefn_list"),
        "returning" => Some("selcollist"),
        "upsert" if children[0].symbol == "RETURNING" => Some("selcollist"),
        "window" if children.iter().any(|f| f.symbol == "nexprlist") => Some("nexprlist"),
        "window" if children.iter().any(|f| f.symbol == "sortlist") => Some("sortlist"),
        _ => None,
    };
    if let Some(body) = body
        && let Some(index) = children.iter().position(|f| f.symbol == body)
    {
        let head = state.sequence(lhs, &children[..index], Shape::Atom);
        let mut out = state.hanging(head, children[index]);
        for &child in &children[index + 1..] {
            out = state.append(out, child, Break::Line, Shape::Atom);
        }
        return state.grouped(out);
    }
    if lhs == "create_table" {
        return header(state, lhs, children);
    }
    if matches!(lhs, "cmd" | "trigger_cmd" | "upsert") {
        return sections(state, lhs, children);
    }
    if lhs == "expr" && children[0].symbol == "expr" && children.len() >= 3 {
        return state.sequence(lhs, children, Shape::Chain(rule));
    }
    // Even unclassified left-recursive grammar sequences must not create a
    // nested group per reduction. Their owner seals the sequence once.
    let shape = if list(lhs) || children[0].symbol == lhs {
        Shape::List
    } else {
        Shape::Atom
    };
    let result = state.sequence(lhs, children, shape);
    if lhs == "ccons" && children[0].symbol == "CONSTRAINT" {
        return Fragment {
            trailing_prefix: true,
            ..result
        };
    }
    if shape == Shape::Atom {
        state.grouped(result)
    } else {
        result
    }
}
