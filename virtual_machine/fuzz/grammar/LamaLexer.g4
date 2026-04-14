lexer grammar LamaLexer;

DELIM : '---DELIM---' ;

LINE_COMMENT : '--' ~[\r\n]* -> skip ;
BLOCK_COMMENT : '(*' (BLOCK_COMMENT | .)*? '*)' -> skip ;

WS : [ \t\r\n]+ -> skip ;

AFTER   : 'after' ;
ARRAY   : 'array' ;
AT      : 'at' ;
BEFORE  : 'before' ;
BOX     : 'box' ;
CASE    : 'case' ;
DO      : 'do' ;
ELIF    : 'elif' ;
ELSE    : 'else' ;
ESAC    : 'esac' ;
ETA     : 'eta' ;
FALSE   : 'false' ;
FI      : 'fi' ;
FOR     : 'for' ;
FUN     : 'fun' ;
IF      : 'if' ;
IMPORT  : 'import' ;
INFIX_KW: 'infix' ;
INFIXL  : 'infixl' ;
INFIXR  : 'infixr' ;
LAZY    : 'lazy' ;
OD      : 'od' ;
OF      : 'of' ;
PUBLIC  : 'public' ;
SEXP    : 'sexp' ;
SKIP_   : 'skip' ;
STR     : 'str' ;
SYNTAX  : 'syntax' ;
THEN    : 'then' ;
TRUE    : 'true' ;
VAL     : 'val' ;
VAR     : 'var' ;
WHILE   : 'while' ;
LET     : 'let' ;
IN      : 'in' ;

ARROW : '->' ;

SEMI       : ';' ;
COMMA      : ',' ;
DOT        : '.' ;
LPAREN     : '(' ;
RPAREN     : ')' ;
LBRACE     : '{' ;
RBRACE     : '}' ;
LBRACK     : '[' ;
RBRACK     : ']' ;
UNDERSCORE : '_' ;

HASH    : '#' ;
PIPE    : '|' ;
EQUALS  : '=' ;
COLON   : ':' ;
MINUS   : '-' ;
AT_SIGN : '@' ;

INFIX
    : [+*/%$!&^?<>]
    | [-+*/%$#@!|&^?<>:=] [-+*/%$#@!|&^?<>:=]+
    ;

UIDENT : [A-Z] [a-zA-Z_0-9]* ;
LIDENT : [a-z] [a-zA-Z_0-9]* ;

DECIMAL : [0-9]+ ;
STRING  : '"' (~["\r\n] | '""')* '"' ;
CHAR    : '\'' (~['\r\n] | '\'\'' | '\\n' | '\\t') '\'' ;
