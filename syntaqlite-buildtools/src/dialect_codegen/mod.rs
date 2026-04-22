// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

use std::collections::{HashMap, HashSet};

use crate::util::synq_parser::{Field, Fmt, Item, MacroDefAnnotation, SemanticAnnotation};

pub(crate) mod c_dialect;
pub(crate) mod c_meta_codegen;
pub(crate) mod c_nodes_codegen;
pub(crate) mod fmt_compiler;
pub(crate) mod python_codegen;
pub(crate) mod rust_ast;
pub(crate) mod rust_dialect;
pub(crate) mod semantic_roles_codegen;

pub(super) fn c_type_name(name: &str) -> String {
    format!("Syntaqlite{name}")
}

pub(crate) struct AstModel<'a> {
    items: &'a [Item],
    extension_items: &'a [Item],
    enum_names: HashSet<&'a str>,
    flags_names: HashSet<&'a str>,
    node_names: HashSet<&'a str>,
    list_names: HashSet<&'a str>,
    enums: Vec<EnumRef<'a>>,
    flags: Vec<FlagsRef<'a>>,
    nodes: Vec<NodeRef<'a>>,
    lists: Vec<ListRef<'a>>,
    node_like_items: Vec<NodeLikeRef<'a>>,
    abstract_items: Vec<(&'a str, &'a [String])>,
    /// Pinned tag assignments: node/list name → tag value (1-based).
    tag_map: HashMap<String, u32>,
    /// Number of base tags (for extension builds). Equal to total tag count
    /// for non-extension builds.
    base_tag_count: u32,
}

#[derive(Clone, Copy)]
pub(crate) struct EnumRef<'a> {
    pub(crate) name: &'a str,
    pub(crate) variants: &'a [String],
}

#[derive(Clone, Copy)]
pub(crate) struct FlagsRef<'a> {
    pub(crate) name: &'a str,
    pub(crate) flags: &'a [(String, u32)],
}

#[derive(Clone, Copy)]
pub(crate) struct NodeRef<'a> {
    pub(crate) name: &'a str,
    pub(crate) fields: &'a [Field],
    #[expect(dead_code)]
    pub(crate) fmt: Option<&'a [Fmt]>,
    pub(crate) semantic: Option<&'a SemanticAnnotation>,
    pub(crate) macro_def: Option<&'a MacroDefAnnotation>,
}

#[derive(Clone, Copy)]
pub(crate) struct ListRef<'a> {
    pub(crate) name: &'a str,
    pub(crate) child_type: &'a str,
    #[expect(dead_code)]
    pub(crate) fmt: Option<&'a [Fmt]>,
    pub(crate) prepend: bool,
}

#[derive(Clone, Copy)]
pub(crate) enum NodeLikeRef<'a> {
    Node(NodeRef<'a>),
    List(ListRef<'a>),
}

impl<'a> NodeLikeRef<'a> {
    pub(crate) const fn name(&self) -> &'a str {
        match self {
            NodeLikeRef::Node(n) => n.name,
            NodeLikeRef::List(l) => l.name,
        }
    }
}

impl<'a> AstModel<'a> {
    pub(crate) fn new(items: &'a [Item]) -> Self {
        let mut enum_names: HashSet<&str> = HashSet::new();
        let mut flags_names: HashSet<&str> = HashSet::new();
        let mut node_names: HashSet<&str> = HashSet::new();
        let mut list_names: HashSet<&str> = HashSet::new();
        let mut enums: Vec<EnumRef<'a>> = Vec::new();
        let mut flags: Vec<FlagsRef<'a>> = Vec::new();
        let mut nodes: Vec<NodeRef<'a>> = Vec::new();
        let mut lists: Vec<ListRef<'a>> = Vec::new();
        let mut node_like_items: Vec<NodeLikeRef<'a>> = Vec::new();
        let mut abstract_items: Vec<(&str, &[String])> = Vec::new();

        for item in items {
            match item {
                Item::Enum { name, variants, .. } => {
                    let name = name.as_str();
                    enum_names.insert(name);
                    enums.push(EnumRef {
                        name,
                        variants: variants.as_slice(),
                    });
                }
                Item::Flags {
                    name,
                    flags: values,
                } => {
                    let name = name.as_str();
                    flags_names.insert(name);
                    flags.push(FlagsRef {
                        name,
                        flags: values.as_slice(),
                    });
                }
                Item::Node {
                    name,
                    fields,
                    fmt,
                    semantic,
                    macro_def,
                    ..
                } => {
                    let node = NodeRef {
                        name: name.as_str(),
                        fields: fields.as_slice(),
                        fmt: fmt.as_deref(),
                        semantic: semantic.as_ref(),
                        macro_def: macro_def.as_ref(),
                    };
                    node_names.insert(node.name);
                    nodes.push(node);
                    node_like_items.push(NodeLikeRef::Node(node));
                }
                Item::List {
                    name,
                    child_type,
                    fmt,
                    prepend,
                } => {
                    let list = ListRef {
                        name: name.as_str(),
                        child_type: child_type.as_str(),
                        fmt: fmt.as_deref(),
                        prepend: *prepend,
                    };
                    list_names.insert(list.name);
                    lists.push(list);
                    node_like_items.push(NodeLikeRef::List(list));
                }
                Item::Abstract { name, members } => {
                    abstract_items.push((name.as_str(), members.as_slice()));
                }
            }
        }

        let mut tag_map = HashMap::new();
        for (i, item) in node_like_items.iter().enumerate() {
            let name = item.name();
            tag_map.insert(
                name.to_string(),
                u32::try_from(i + 1).expect("tag count fits in u32"),
            );
        }
        let base_tag_count = u32::try_from(node_like_items.len()).expect("tag count fits in u32");

        Self {
            items,
            extension_items: &[],
            enum_names,
            flags_names,
            node_names,
            list_names,
            enums,
            flags,
            nodes,
            lists,
            node_like_items,
            abstract_items,
            tag_map,
            base_tag_count,
        }
    }

    /// Build an `AstModel` where base items get pinned tags and extension items
    /// get tags after the base range. Extension items may redefine base nodes
    /// (append-only fields) or add entirely new nodes.
    #[expect(clippy::too_many_lines)]
    pub(crate) fn new_with_extensions(
        base_items: &'a [Item],
        extension_items: &'a [Item],
    ) -> Result<Self, String> {
        // First, build the base model to establish tag ordering.
        let base = Self::new(base_items);

        // Collect all items: start with base, then add extension items.
        let mut enum_names = base.enum_names;
        let mut flags_names = base.flags_names;
        let mut node_names = base.node_names;
        let mut list_names = base.list_names;
        let mut enums = base.enums;
        let mut flags = base.flags;
        let mut nodes = base.nodes;
        let mut lists = base.lists;
        let mut node_like_items = base.node_like_items;
        let mut abstract_items = base.abstract_items;
        let mut tag_map = base.tag_map;
        let base_tag_count = base.base_tag_count;
        let mut next_tag = base_tag_count + 1;

        // Index base nodes/lists by name for field-prefix validation.
        let mut base_node_fields: HashMap<&str, &[Field]> = HashMap::new();
        for node in &nodes {
            base_node_fields.insert(node.name, node.fields);
        }

        for item in extension_items {
            match item {
                Item::Enum { name, variants, .. } => {
                    let name = name.as_str();
                    enum_names.insert(name);
                    enums.push(EnumRef {
                        name,
                        variants: variants.as_slice(),
                    });
                }
                Item::Flags {
                    name,
                    flags: values,
                } => {
                    let name = name.as_str();
                    flags_names.insert(name);
                    flags.push(FlagsRef {
                        name,
                        flags: values.as_slice(),
                    });
                }
                Item::Node {
                    name,
                    fields,
                    fmt,
                    semantic,
                    macro_def,
                    ..
                } => {
                    let name_str = name.as_str();
                    let node = NodeRef {
                        name: name_str,
                        fields: fields.as_slice(),
                        fmt: fmt.as_deref(),
                        semantic: semantic.as_ref(),
                        macro_def: macro_def.as_ref(),
                    };

                    if let Some(base_fields) = base_node_fields.get(name_str) {
                        // Redefining a base node — validate append-only.
                        validate_append_only(name_str, base_fields, fields)?;
                        // Replace the base entry in node_like_items (keeps base tag).
                        for item in &mut node_like_items {
                            if let NodeLikeRef::Node(n) = item
                                && n.name == name_str
                            {
                                *n = node;
                                break;
                            }
                        }
                        // Replace in nodes vec too.
                        for n in &mut nodes {
                            if n.name == name_str {
                                *n = node;
                                break;
                            }
                        }
                    } else {
                        // New extension node.
                        node_names.insert(name_str);
                        nodes.push(node);
                        node_like_items.push(NodeLikeRef::Node(node));
                        tag_map.insert(name_str.to_string(), next_tag);
                        next_tag += 1;
                    }
                }
                Item::List {
                    name,
                    child_type,
                    fmt,
                    prepend,
                } => {
                    let name_str = name.as_str();
                    let list = ListRef {
                        name: name_str,
                        child_type: child_type.as_str(),
                        fmt: fmt.as_deref(),
                        prepend: *prepend,
                    };

                    if tag_map.contains_key(name_str) {
                        // Redefining a base list — replace in-place.
                        for item in &mut node_like_items {
                            if let NodeLikeRef::List(l) = item
                                && l.name == name_str
                            {
                                *l = list;
                                break;
                            }
                        }
                        for l in &mut lists {
                            if l.name == name_str {
                                *l = list;
                                break;
                            }
                        }
                    } else {
                        list_names.insert(name_str);
                        lists.push(list);
                        node_like_items.push(NodeLikeRef::List(list));
                        tag_map.insert(name_str.to_string(), next_tag);
                        next_tag += 1;
                    }
                }
                Item::Abstract { name, members } => {
                    abstract_items.push((name.as_str(), members.as_slice()));
                }
            }
        }

        Ok(Self {
            items: base_items,
            extension_items,
            enum_names,
            flags_names,
            node_names,
            list_names,
            enums,
            flags,
            nodes,
            lists,
            node_like_items,
            abstract_items,
            tag_map,
            base_tag_count,
        })
    }

    /// Iterate all items (base + extension). Use this when codegen needs the
    /// full set — e.g. fmt compilation needs fmt blocks from both base and
    /// extension items.
    pub(crate) fn all_items(&self) -> impl Iterator<Item = &'a Item> {
        self.items.iter().chain(self.extension_items.iter())
    }

    pub(crate) const fn enum_names(&self) -> &HashSet<&'a str> {
        &self.enum_names
    }

    pub(crate) const fn flags_names(&self) -> &HashSet<&'a str> {
        &self.flags_names
    }

    pub(crate) const fn node_names(&self) -> &HashSet<&'a str> {
        &self.node_names
    }

    pub(crate) const fn list_names(&self) -> &HashSet<&'a str> {
        &self.list_names
    }

    pub(crate) fn abstract_items(&self) -> &[(&'a str, &'a [String])] {
        &self.abstract_items
    }

    /// The name of the root/top-level AST node type.
    #[expect(
        clippy::unused_self,
        reason = "API consistency; may use self in future"
    )]
    pub(crate) fn root_node_name(&self) -> &'static str {
        "Stmt"
    }

    pub(crate) fn enums(&self) -> &[EnumRef<'a>] {
        &self.enums
    }

    pub(crate) fn flags(&self) -> &[FlagsRef<'a>] {
        &self.flags
    }

    pub(crate) fn nodes(&self) -> &[NodeRef<'a>] {
        &self.nodes
    }

    pub(crate) fn lists(&self) -> &[ListRef<'a>] {
        &self.lists
    }

    pub(crate) fn node_like_items(&self) -> &[NodeLikeRef<'a>] {
        &self.node_like_items
    }

    /// Return the pinned tag value for a node/list name.
    pub(crate) fn tag_for(&self, name: &str) -> u32 {
        self.tag_map[name]
    }

    /// Access the tag map (name → tag value).
    pub(crate) fn tag_map(&self) -> &HashMap<String, u32> {
        &self.tag_map
    }

    /// Number of base tags (before extension tags).
    #[cfg(test)]
    pub(crate) const fn base_tag_count(&self) -> u32 {
        self.base_tag_count
    }
}

/// Validate that extension fields are a strict append-only extension of base fields.
/// The first N fields of the extension must exactly match the base node's fields.
fn validate_append_only(
    name: &str,
    base_fields: &[Field],
    ext_fields: &[Field],
) -> Result<(), String> {
    if ext_fields.len() < base_fields.len() {
        return Err(format!(
            "extension node '{}' has fewer fields ({}) than base ({})",
            name,
            ext_fields.len(),
            base_fields.len()
        ));
    }
    for (i, (base, ext)) in base_fields.iter().zip(ext_fields.iter()).enumerate() {
        if base.name != ext.name || base.storage != ext.storage || base.type_name != ext.type_name {
            return Err(format!(
                "extension node '{}' field {} mismatch: base has '{}' ({:?} {}), extension has '{}' ({:?} {})",
                name,
                i,
                base.name,
                base.storage,
                base.type_name,
                ext.name,
                ext.storage,
                ext.type_name,
            ));
        }
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::util::synq_parser::parse_synq_file;

    fn parse_items(synq: &str) -> Vec<Item> {
        parse_synq_file(synq).expect("parse failed")
    }

    #[test]
    fn base_tags_are_sequential() {
        let items = parse_items(
            r"
            node Foo { x: index Bar }
            node Bar { y: inline Bool }
            list FooList { Foo }
            ",
        );
        let model = AstModel::new(&items);
        assert_eq!(model.tag_for("Foo"), 1);
        assert_eq!(model.tag_for("Bar"), 2);
        assert_eq!(model.tag_for("FooList"), 3);
        assert_eq!(model.base_tag_count(), 3);
    }

    #[test]
    fn extension_new_node_gets_tag_after_base() {
        let base_items = parse_items(
            r"
            node Foo { x: index Bar }
            node Bar { y: inline Bool }
            ",
        );
        let ext_items = parse_items(
            r"
            node Baz { z: inline Bool }
            ",
        );
        let model = AstModel::new_with_extensions(&base_items, &ext_items).unwrap();
        assert_eq!(model.tag_for("Foo"), 1);
        assert_eq!(model.tag_for("Bar"), 2);
        assert_eq!(model.tag_for("Baz"), 3);
        assert_eq!(model.base_tag_count(), 2);
    }

    #[test]
    fn extension_redefine_keeps_base_tag() {
        let base_items = parse_items(
            r"
            node Foo { x: index Bar }
            node Bar { y: inline Bool }
            ",
        );
        let ext_items = parse_items(
            r"
            node Foo { x: index Bar  z: inline Bool }
            ",
        );
        let model = AstModel::new_with_extensions(&base_items, &ext_items).unwrap();
        assert_eq!(model.tag_for("Foo"), 1);
        assert_eq!(model.tag_for("Bar"), 2);
        // Foo should now have 2 fields.
        let foo = model.nodes().iter().find(|n| n.name == "Foo").unwrap();
        assert_eq!(foo.fields.len(), 2);
    }

    #[test]
    fn extension_field_reorder_is_error() {
        let base_items = parse_items(
            r"
            node Foo { x: index Bar  y: inline Bool }
            node Bar { z: inline Bool }
            ",
        );
        let ext_items = parse_items(
            r"
            node Foo { y: inline Bool  x: index Bar }
            ",
        );
        match AstModel::new_with_extensions(&base_items, &ext_items) {
            Err(err) => assert!(err.contains("mismatch"), "got: {err}"),
            Ok(_) => panic!("should fail on field reorder"),
        }
    }

    #[test]
    fn extension_fewer_fields_is_error() {
        let base_items = parse_items(
            r"
            node Foo { x: index Bar  y: inline Bool }
            node Bar { z: inline Bool }
            ",
        );
        let ext_items = parse_items(
            r"
            node Foo { x: index Bar }
            ",
        );
        match AstModel::new_with_extensions(&base_items, &ext_items) {
            Err(err) => assert!(err.contains("fewer fields"), "got: {err}"),
            Ok(_) => panic!("should fail on fewer fields"),
        }
    }

    #[test]
    fn base_model_from_real_synq_files_has_stable_tags() {
        // Parse the actual base .synq files and verify some known tags.
        let base_synq = crate::base_files::base_synq_files();
        let mut all_items = Vec::new();
        for (name, content) in base_synq {
            let items =
                parse_synq_file(content).unwrap_or_else(|e| panic!("parse {name} failed: {e}"));
            all_items.extend(items);
        }
        let model = AstModel::new(&all_items);

        // These values match the current generated NodeTag enum.
        assert_eq!(model.tag_for("AggregateFunctionCall"), 1);
        assert_eq!(model.tag_for("ParenExpr"), 36);
        assert_eq!(model.tag_for("SelectStmt"), 51);
        assert_eq!(model.tag_for("FilterOver"), 79);
        assert_eq!(model.base_tag_count(), 79);
    }
}
