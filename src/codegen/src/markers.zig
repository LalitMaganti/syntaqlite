// Shared Lemon-generated section markers.
//
// Constants identifying well-known sections in Lemon parser output,
// used by both lempar_transform.zig and parser_extract.zig.

// Section boundaries
pub const TOKEN_DEFS_START = "/* These constants specify the various numeric values for terminal symbols.";
pub const TOKEN_DEFS_END = "/**************** End token definitions ***************************************/";
pub const CONTROL_DEFS_START = "/************* Begin control #defines *****************************************/";
pub const CONTROL_DEFS_END = "/************* End control #defines *******************************************/";
pub const PARSING_TABLES_START = "*********** Begin parsing tables **********************************************/";
pub const PARSING_TABLES_END = "/********** End of lemon-generated parsing tables *****************************/";
pub const REDUCE_ACTIONS_START = "/********** Begin reduce actions **********************************************/";
pub const REDUCE_ACTIONS_END = "/********** End reduce actions ************************************************/";

// Struct markers
pub const STRUCT_COMMENT_START = "/* The following structure represents a single element of the";

// Array declarations
pub const FALLBACK_ARRAY_DECL = "static const YYCODETYPE yyFallback[] = {";
pub const TOKEN_NAMES_ARRAY_DECL = "static const char *const yyTokenName[] = {";
pub const RULE_NAMES_ARRAY_DECL = "static const char *const yyRuleName[] = {";
pub const RULE_INFO_LHS_ARRAY_DECL = "static const YYCODETYPE yyRuleInfoLhs[] = {";
pub const RULE_INFO_NRHS_ARRAY_DECL = "static const signed char yyRuleInfoNRhs[] = {";

// Conditional compilation markers
pub const YYFALLBACK_IFDEF = "#ifdef YYFALLBACK";
pub const YYCOVERAGE_NDEBUG_IFDEF = "#if defined(YYCOVERAGE) || !defined(NDEBUG)";
pub const NDEBUG_IFNDEF = "#ifndef NDEBUG";

// parser_extract-specific markers
pub const CONTROL_DEFS_COMMENT_START = "/* The next sections is a series of control #defines.";
pub const PARSING_TABLES_COMMENT_START = "/* Next are the tables used to determine what action to take";
pub const FALLBACK_COMMENT_START = "/* The next table maps tokens (terminal symbols) into fallback tokens.";
pub const INCLUDE_END = "/**************** End of %include directives **********************************/";

// lempar_transform-specific markers
pub const STRUCT_END = "typedef struct yyParser yyParser;";
