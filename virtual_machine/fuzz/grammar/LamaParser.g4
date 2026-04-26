parser grammar LamaParser;
options { tokenVocab = LamaLexer; }

multiUnit
    : singleUnitProgram EOF
    | twoUnitProgram EOF
    | threeUnitProgram EOF
    ;

singleUnitProgram : standaloneUnit ;

twoUnitProgram
    : libraryUnit DELIM mainUnitAfterUnit1
    ;

threeUnitProgram
    : libraryUnit DELIM libraryUnitAfterUnit1 DELIM mainUnitAfterUnit1And2
    ;

standaloneUnit : scopeExpression ;

libraryUnit : scopeExpression ;

libraryUnitAfterUnit1 : importUnit1? scopeExpression ;

mainUnitAfterUnit1 : importUnit1? scopeExpression ;

mainUnitAfterUnit1And2 : mainImports12? scopeExpression ;

mainImports12
    : importUnit1 importUnit2?
    | importUnit2 importUnit1?
    ;

importUnit1 : IMPORT UNIT1 SEMI ;

importUnit2 : IMPORT UNIT2 SEMI ;

importDecl : IMPORT UIDENT SEMI ;

// Scope expressions

scopeExpression : definition* expression? ;

definition
    : variableDefinition
    | functionDefinition
    | infixDefinition
    ;

variableDefinition
    : (VAR | PUBLIC) variableDefinitionItem (COMMA variableDefinitionItem)* SEMI
    ;

variableDefinitionItem : LIDENT (EQUALS basicExpression)? ;

functionDefinition
    : PUBLIC? FUN LIDENT LPAREN functionArguments RPAREN functionBody
    ;

functionArguments : (pattern (COMMA pattern)*)? ;

functionBody : LBRACE scopeExpression RBRACE ;

// Infix operator definitions

infixDefinition
    : infixHead LPAREN functionArguments RPAREN functionBody
    ;

infixHead : PUBLIC? infixity infixOp level ;

infixity : INFIX_KW | INFIXL | INFIXR ;

level : (AT | BEFORE | AFTER)? infixOp ;

// Expressions

expression : basicExpression (SEMI expression)? ;

basicExpression : binaryOperand (infixOp binaryOperand)* ;

binaryOperand
    : MINUS postfixExpression
    | postfixExpression
    ;

infixOp
    : INFIX
    | HASH
    | PIPE
    | EQUALS
    | COLON
    | MINUS
    | AT_SIGN
    | ARROW
    ;

postfixExpression : primary postfix* ;

postfix
    : LPAREN (expression (COMMA expression)*)? RPAREN   // call
    | LBRACK expression RBRACK                           // index
    | DOT LIDENT                                         // dot notation
    ;

primary
    : DECIMAL
    | STRING
    | CHAR
    | LIDENT
    | TRUE
    | FALSE
    | INFIX_KW infixOp
    | FUN LPAREN functionArguments RPAREN functionBody
    | SKIP_
    | LPAREN scopeExpression RPAREN
    | LBRACK (expression (COMMA expression)*)? RBRACK          // array
    | LBRACE (expression (COMMA expression)*)? RBRACE          // list
    | UIDENT (LPAREN expression (COMMA expression)* RPAREN)?   // S-expression
    | ifExpression
    | whileDoExpression
    | doWhileExpression
    | forExpression
    | caseExpression
    | letExpression
    | LAZY basicExpression
    | ETA basicExpression
    ;

// Control flow

ifExpression
    : IF expression THEN scopeExpression elsePart? FI
    ;

elsePart
    : ELIF expression THEN scopeExpression elsePart?
    | ELSE scopeExpression
    ;

whileDoExpression : WHILE expression DO scopeExpression OD ;

doWhileExpression : DO scopeExpression WHILE expression OD ;

forExpression
    : FOR scopeExpression COMMA expression COMMA expression
      DO scopeExpression OD
    ;

caseExpression : CASE expression OF caseBranches ESAC ;

caseBranches : caseBranch (PIPE caseBranch)* ;

caseBranch : pattern ARROW scopeExpression ;

letExpression : LET pattern EQUALS expression IN expression ;

// Patterns

pattern
    : simplePattern COLON pattern   // cons
    | simplePattern
    ;

simplePattern
    : UNDERSCORE
    | UIDENT (LPAREN pattern (COMMA pattern)* RPAREN)?     // S-expr pattern
    | LBRACK (pattern (COMMA pattern)*)? RBRACK            // array pattern
    | LBRACE (pattern (COMMA pattern)*)? RBRACE            // list pattern
    | LIDENT (AT_SIGN pattern)?                            // binding / variable
    | MINUS? DECIMAL                                       // integer constant
    | STRING
    | CHAR
    | TRUE
    | FALSE
    | HASH BOX
    | HASH VAL
    | HASH STR
    | HASH ARRAY
    | HASH SEXP
    | HASH FUN
    | LPAREN pattern RPAREN                                // grouping
    ;
