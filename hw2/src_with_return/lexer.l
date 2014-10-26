%option noyywrap
%{
#include <stdio.h>
#include "header.h"
int linenumber;
symtab * lookup();
symtab * ptr;
void     insertID();
void 	 printSymTab();
/* You need to define for all tokens in C--, here are some examples */
/* reserve word */
#define R_RETURN 11
#define R_TYPEDEF 12
#define R_IF 13
#define R_ELSE 14
#define R_INT 15
#define R_FLOAT 16
#define R_FOR 17
#define R_STRUCT 18
#define R_UNION 19
#define R_VOID 20
#define R_WHILE 21
/* Constant Literal */
#define C_INT 22
#define C_FLOAT 23
#define C_STRING 24
/* Identifier and comment */
#define R_ID 25
#define R_COMMENT 26
/* Arithmetic operators: +. -. *. /, % */
#define R_OPADD 31
#define R_OPSUB 32
#define R_OPMUL 33
#define R_OPDIV 34
#define R_OPMOD 35
/* Relational operator: > < >= <= != == */
#define R_ROLT 41
#define R_ROGT 42
#define R_ROLTE 43
#define R_ROGTE 44
#define R_RONE 45
#define R_ROEQ 46
/* Logical operator: ||, &&, ! */
#define R_LOOR 47
#define R_LOAND 48
#define R_LONOT 49
/* Assign operator: = */
#define R_OPASS 50
/* Separator () {} [] ' , . */
#define R_MK_LPAREN 61
#define R_MK_RPAREN 62
#define R_MK_LBRACE 63
#define R_MK_RBRACE 64
#define R_MK_LBRAKET 65
#define R_MK_RBRAKET 66
#define R_MK_COMMA 67
#define R_MK_SEMICOL 68
#define R_MK_DOT 68
#define R_ERROR 		100
%}

letter	[A-Za-z]
digit	[0-9]
E	[Ee][+-]?{digit}+
FS	(f|F|l|L)
IS	(u|U|l|L)*

RES_return	"return"
RES_typedef "typedef"
RES_if	"if"
RES_else "else"
RES_int "int"
RES_float "float"
RES_for "for"
RES_struct "struct"
RES_union "union"
RES_void "void"
RES_while "while"

WS	 [ \t]+

Int_constant {digit}+({E})?{IS}?
Float_constant {digit}*\.{digit}+({E})?{FS}?
String_constant {letter}?\"(\\.|[^\\"])*\"

ID	 {letter}({letter}|{digit}|"_")*

OP_add        "+"
OP_sub        "-"
OP_mul        "*"
OP_div       "/"
OP_mod       "%"

RO_lt	"<"
RO_gt	">"
RO_lte "<="
RO_gte ">="
RO_ne "!="
RO_eq "=="

LO_or   "||"
LO_and	"&&"
LO_not "!"

OP_assign        "="

/* Other operators appear here */

newline  "\n"

DL_lparen "("
DL_rparen ")"
DL_lbraket "["
DL_rbraket "]"
DL_lbrace "{"
DL_rbrace "}"
DL_comma  ","
DL_semicol ";"
DL_dot  "."

comment \/\*([^*]|[\r\n]|(\*+([^*/]|[\r\n])))*\*+\/
line_comment L?'(\\.|[^\\'])+'

/* Other separators appear here */

error    .

%%

{WS}            {}

{RES_return} {return R_RETURN;}
{RES_typedef} {return R_TYPEDEF;}
{RES_if}	{return R_IF;}
{RES_else} {return R_ELSE;}
{RES_int} {return R_INT;}
{RES_float} {return R_FLOAT;}
{RES_for} {return R_FOR;}
{RES_struct} {return R_STRUCT;}
{RES_union} {return R_UNION;}
{RES_void} {return R_VOID;}
{RES_while} {return R_WHILE;}

{Int_constant}         {return C_INT;}
{Float_constant}       {return C_FLOAT;}
{String_constant}			 {return C_STRING;}
{ID} {
			printf("%s\n", yytext);
			ptr = lookup(yytext);
			if (ptr == NULL)
			     insertID(yytext);
			else
			     ptr->counter++;
			return R_ID;
		 }

{comment}	{printf("%s\n", yytext); return R_COMMENT;}
{line_comment} {printf("%s\n", yytext); return R_COMMENT;}

{OP_add} {return R_OPADD;}
{OP_sub} {return R_OPSUB;}
{OP_mul} {return R_OPMUL;}
{OP_div} {return R_OPDIV;}
{OP_mod} {return R_OPMOD;}

{RO_lt} {return R_ROLT;}
{RO_gt} {return R_ROGT;}
{RO_lte} {return R_ROLTE;}
{RO_gte} {return R_ROGTE;}
{RO_ne} {return R_RONE;}
{RO_eq} {return R_ROEQ;}

{LO_or} {return R_LOOR;}
{LO_and} {return R_LOAND;}
{LO_not} {return R_LONOT;}

{OP_assign} {return R_OPASS;}


{DL_lparen}     {return R_MK_LPAREN;}
{DL_rparen}     {return R_MK_RPAREN;}
{DL_lbrace}     {return R_MK_LBRACE;}
{DL_rbrace}     {return R_MK_RBRACE;}
{DL_lbraket}     {return R_MK_LBRAKET;}
{DL_rbraket}     {return R_MK_RBRAKET;}

{DL_comma}      {return R_MK_COMMA;}
{DL_semicol}    {return R_MK_SEMICOL;}
{DL_dot} {return R_MK_DOT;}
{newline}       {linenumber += 1;}
{error}	{printf("Unrecognize token on %d : %s\n", linenumber,  yytext); return R_ERROR;}

%%

int main(int argc, char **argv)
{
 argc--; ++argv;
   if (argc > 0)
      yyin = fopen(argv[0], "r");
   else
      yyin = stdin;
   yylex();
   printSymTab();
}
