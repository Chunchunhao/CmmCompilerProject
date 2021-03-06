#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "header.h"


int main( int argc, char *argv[] )
{
    FILE *source, *target;
    Program program;
    SymbolTable symtab;

    if( argc == 3){
        source = fopen(argv[1], "r");
        target = fopen(argv[2], "w");
        if( !source ){
            printf("can't open the source file\n");
            exit(2);
        }
        else if( !target ){
            printf("can't open the target file\n");
            exit(2);
        }
        else{
            program = parser(source);
            fclose(source);
            symtab = build(program);
            check(&program, &symtab);
            gencode(program, &symtab, target);
        }
    }
    else{
        printf("Usage: %s source_file target_file\n", argv[0]);
    }


    return 0;
}

/********************************************* 
  Scanning 
 *********************************************/
Token getNumericToken( FILE *source, char c )
{
    Token token;
    int i = 0;

    while( isdigit(c) ) {
        token.tok[i++] = c;
        c = fgetc(source);
    }

    if( c != '.' ){
        ungetc(c, source);
        token.tok[i] = '\0';
        token.type = IntValue;
        return token;
    }

    token.tok[i++] = '.';

    c = fgetc(source);
    if( !isdigit(c) ){
        ungetc(c, source);
        printf("Expect a digit : %c\n", c);
        exit(1);
    }

    while( isdigit(c) ){
        token.tok[i++] = c;
        c = fgetc(source);
    }

    ungetc(c, source);
    token.tok[i] = '\0';
    token.type = FloatValue;
    return token;
}

Token getAlphabeticToken( FILE *source, char c )
{
    Token token;
    int i = 0;

    while( isalpha(c) ) {
        token.tok[i++] = c;
        c = fgetc(source);
    }

    ungetc(c, source);
    token.tok[i] = '\0';
    if( strcmp( token.tok, "f" ) == 0 ) // c == 'f' -> strcmp(token.tok, "f") == 0
        token.type = FloatDeclaration;
    else if( strcmp( token.tok, "i" ) == 0 )
        token.type = IntegerDeclaration;
    else if( strcmp( token.tok, "p" ) == 0 )
        token.type = PrintOp;
    else
        token.type = Alphabet;
    return token;
}

Token ungettedToken;    // new, for ungetting whole identifier
int someTokenUngetted = 0;  // new, flag of ungetting
void ungetToken(Token token)
{
    if(someTokenUngetted)
    {
        printf("Two tokens ungetted successively\n");
        exit(1);
    }
    ungettedToken = token;
    someTokenUngetted = 1;
    return;
}
Token scanner( FILE *source )
{
    if(someTokenUngetted)
    {
        someTokenUngetted = 0;
        return ungettedToken;
    }

    char c;
    Token token;

    while( !feof(source) ){
        c = fgetc(source);

// CHECK FOR SPACES
        while( isspace(c) ) 
        {
            c = fgetc(source);
        }
// MODIFY FIN
        if( isdigit(c) )
            return getNumericToken(source, c);

//
//
        if( isalpha(c) )// islower -> isalpha
            return getAlphabeticToken(source, c);


        switch(c){
            case '=':
                token.type = AssignmentOp;
                return token;
            case '+':
                token.type = PlusOp;
                return token;
            case '-':
                token.type = MinusOp;
                return token;
            case '*':
                token.type = MulOp;
                return token;
            case '/':
                token.type = DivOp;
                return token;
            case EOF:
                token.type = EOFsymbol;
                token.tok[0] = '\0';
                return token;
            default:
                printf("Invalid character : %c\n", c);
                exit(1);
        }
    }

    token.tok[0] = '\0';
    token.type = EOFsymbol;
    return token;
}
int nameCount = 0;
char nameTable[50][257];
char* storeName(char* name)
{
    int i;
    for(i = 0; i < nameCount; i ++)
    {
        if(strcmp(nameTable[i], name) == 0)
            return nameTable[i];
    }
    if(nameCount >= 50)
    {
        printf("There are too many different identifiers (> 50)\n");
        exit(1);
    }
    strcpy(nameTable[i], name);
    nameCount ++;
    return nameTable[i];
}

/********************************************************
  Parsing
 *********************************************************/
Declaration parseDeclaration( FILE *source, Token token )
{
    Token token2;
    switch(token.type){
        case FloatDeclaration:
        case IntegerDeclaration:
            token2 = scanner(source);
            if (strcmp(token2.tok, "f") == 0 ||
                    strcmp(token2.tok, "i") == 0 ||
                    strcmp(token2.tok, "p") == 0) {
                printf("Syntax Error: %s cannot be used as id\n", token2.tok);
                exit(1);
            }
            return makeDeclarationNode( token, token2 );
        default:
            printf("Syntax Error: Expect Declaration %s\n", token.tok);
            exit(1);
    }
}

Declarations *parseDeclarations( FILE *source )
{
    Token token = scanner(source);
    Declaration decl;
    Declarations *decls;
    switch(token.type){
        case FloatDeclaration:
        case IntegerDeclaration:
            decl = parseDeclaration(source, token);
            decls = parseDeclarations(source);
            return makeDeclarationTree( decl, decls );
        case PrintOp:
        case Alphabet:
            ungetc(token.tok[0], source);
            return NULL;
        case EOFsymbol:
            return NULL;
        default:
            printf("Syntax Error: Expect declarations %s\n", token.tok);
            exit(1);
    }
}

Expression *parseValue( FILE *source )
{
    Token next_token = scanner(source);
    float signedbit = 1;
    while(1){
        if(next_token.type == PlusOp){
            signedbit *= 1;
            next_token = scanner(source);
        }
        else if(next_token.type == MinusOp){
            signedbit *= -1;
            next_token = scanner(source);
        }
        else {
            ungetToken(next_token);
            break;
        }
    }

    Token token = scanner(source);
    Expression *value = (Expression *)malloc( sizeof(Expression) );
    value->leftOperand = value->rightOperand = NULL;

    switch(token.type){
        case Alphabet:
            (value->v).type = Identifier; ///////////////////////////////////
            (value->v).val.id = storeName( token.tok );
            if( signedbit == -1)
                (value->v).si = -1;
            else
                (value->v).si = 1;
            // (value->v).val.id = token.tok[0];
            break;
        case IntValue:
            (value->v).type = IntConst;
            (value->v).val.ivalue = atoi(token.tok) * signedbit;
            break;
        case FloatValue:
            (value->v).type = FloatConst;
            (value->v).val.fvalue = atof(token.tok) * signedbit;
            break;
        default:
            printf("Syntax Error: Expect Identifier or a Number %s\n", token.tok);
            exit(1);
    }

    return value;
}
// HW 1
Expression *parseTermTail( FILE *source, Expression *lvalue ) {
   
    Token token = scanner(source);
    Expression *expr;

    switch(token.type){
        case MulOp:
            expr = (Expression *)malloc( sizeof(Expression) );
            (expr->v).type = MulNode;
            (expr->v).val.op = Mul;
            expr->leftOperand = lvalue;
            expr->rightOperand = parseValue(source);
            return parseTermTail(source, expr);
        case DivOp:
            expr = (Expression *)malloc( sizeof(Expression) );
            (expr->v).type = DivNode;
            (expr->v).val.op = Div;
            expr->leftOperand = lvalue;
            expr->rightOperand = parseValue(source);
            return parseTermTail(source, expr);
        case PlusOp:
        case MinusOp:
        case Alphabet:
        case PrintOp:
            ungetToken(token); // ungetc(token.tok[0], source);
            return lvalue;
        case EOFsymbol:
            return lvalue;
        default:
            printf("Syntax Error: Expect a numeric value or an identifier %s\n", token.tok);
            exit(1);
    }
}

// HW 1
// Expression *parseTerm( FILE *source, Expression *lvalue );

Expression *parseExpressionTail( FILE *source, Expression *lvalue )
 {
    Token token = scanner(source);
    Expression *expr;

    switch(token.type){
        case PlusOp:
            expr = (Expression *)malloc( sizeof(Expression) );
            (expr->v).type = PlusNode;
            (expr->v).val.op = Plus;
            expr->leftOperand = lvalue;
            // expr->rightOperand = parseValue(source);
            expr->rightOperand = parseTermTail(source, parseValue(source));
            return parseExpressionTail(source, expr);
        case MinusOp:
            expr = (Expression *)malloc( sizeof(Expression) );
            (expr->v).type = MinusNode;
            (expr->v).val.op = Minus;
            expr->leftOperand = lvalue;
            // expr->rightOperand = parseValue(source);
            expr->rightOperand = parseTermTail(source, parseValue(source));
            return parseExpressionTail(source, expr);
        case MulOp:
        case DivOp:
            return parseExpressionTail(source, parseTermTail(source, expr));
        case Alphabet:
        case PrintOp:
            ungetToken(token); // ungetc(token.tok[0], source); -> ungetToken(token);
            //ungetc(token.tok[0], source);
            return lvalue;
        case EOFsymbol:
            return lvalue;
        default:
            printf("Syntax Error: Expect a numeric value or an identifier %s\n", token.tok);
            exit(1);
    }
}

Expression *parseExpression( FILE *source, Expression *lvalue )
{
    Token token = scanner(source);
    Expression *expr;
    switch(token.type){
        case PlusOp:
            expr = (Expression *)malloc( sizeof(Expression) );
            (expr->v).type = PlusNode;
            (expr->v).val.op = Plus;
            expr->leftOperand = lvalue; // expr->rightOperand = parseValue(source);
            expr->rightOperand = parseTermTail(source, parseValue(source));
            return parseExpressionTail(source, expr);
        case MinusOp:
            expr = (Expression *)malloc( sizeof(Expression) );
            (expr->v).type = MinusNode;
            (expr->v).val.op = Minus;
            expr->leftOperand = lvalue;// expr->rightOperand = parseValue(source);
            expr->rightOperand = parseTermTail(source, parseValue(source));
            return parseExpressionTail(source, expr);
        // I should write a parse Del But so tired
        case MulOp:
            expr = (Expression *)malloc( sizeof(Expression) );
            (expr->v).type = MulNode;
            (expr->v).val.op = Mul;
            expr->leftOperand = lvalue;
            expr->rightOperand = parseValue(source);
            return parseExpressionTail(source, expr);
        case DivOp:
            expr = (Expression *)malloc( sizeof(Expression) );
            (expr->v).type = DivNode;
            (expr->v).val.op = Div;
            expr->leftOperand = lvalue;
            expr->rightOperand = parseValue(source);
            return parseExpressionTail(source, expr);
        case Alphabet:
        case PrintOp:
            ungetToken(token); // ungetc(token.tok[0], source); -> ungetToken(token);
            return NULL;
        case EOFsymbol:
            return NULL;
        default:
            printf("Syntax Error: Expect a numeric value or an identifier %s\n", token.tok);
            exit(1);
    }
}

Statement parseStatement( FILE *source, Token token )
{
    Token next_token;
    Expression *value, *expr;

    switch(token.type){
        case Alphabet:
            next_token = scanner(source);
            if(next_token.type == AssignmentOp){
                value = parseValue(source);
                expr = parseExpression(source, value);
                return makeAssignmentNode(token.tok, value, expr); // return makeAssignmentNode(token.tok[0], value, expr);
            }
            else{
                printf("Syntax Error: Expect an assignment op %s\n", next_token.tok);
                exit(1);
            }
        case PrintOp:
            next_token = scanner(source);
            if(next_token.type == Alphabet)
                return makePrintNode(next_token.tok); // return makePrintNode(next_token.tok[0]);
            else{
                printf("Syntax Error: Expect an identifier %s\n", next_token.tok);
                exit(1);
            }
            break;
        default:
            printf("Syntax Error: Expect a statement %s\n", token.tok);
            exit(1);
    }
}

Statements *parseStatements( FILE * source )
{

    Token token = scanner(source);
    Statement stmt;
    Statements *stmts;

    switch(token.type){
        case Alphabet:
        case PrintOp:
            stmt = parseStatement(source, token);
            stmts = parseStatements(source);
            return makeStatementTree(stmt , stmts);
        case EOFsymbol:
            return NULL;
        default:
            printf("Syntax Error: Expect statements %s\n", token.tok);
            exit(1);
    }
}


/*********************************************************************
  Build AST
 **********************************************************************/
Declaration makeDeclarationNode( Token declare_type, Token identifier )
{
    Declaration tree_node;

    switch(declare_type.type){
        case FloatDeclaration:
            tree_node.type = Float;
            break;
        case IntegerDeclaration:
            tree_node.type = Int;
            break;
        default:
            break;
    }
    // tree_node.name = identifier.tok[0];
    tree_node.name = storeName(identifier.tok); // .tok[0] -> .tok
    //strcpy(tree_node.name, identifier.tok);
    return tree_node;
}

Declarations *makeDeclarationTree( Declaration decl, Declarations *decls )
{
    Declarations *new_tree = (Declarations *)malloc( sizeof(Declarations) );
    new_tree->first = decl;
    new_tree->rest = decls;

    return new_tree;
}


Statement makeAssignmentNode( char* id, Expression *v, Expression *expr_tail )
{
    Statement stmt;
    AssignmentStatement assign;

    stmt.type = Assignment;
    assign.id = storeName(id); // assign.id = id;
    if(expr_tail == NULL)
        assign.expr = v;
    else
        assign.expr = expr_tail;
    stmt.stmt.assign = assign;

    return stmt;
}

Statement makePrintNode( char* id )
{
    Statement stmt;
    stmt.type = Print;
    stmt.stmt.variable = storeName(id); // stmt.stmt.variable = id;

    return stmt;
}

Statements *makeStatementTree( Statement stmt, Statements *stmts )
{
    Statements *new_tree = (Statements *)malloc( sizeof(Statements) );
    new_tree->first = stmt;
    new_tree->rest = stmts;

    return new_tree;
}

/* parser */
Program parser( FILE *source )
{
    Program program;

    program.declarations = parseDeclarations(source);
    program.statements = parseStatements(source);

    return program;
}


/********************************************************
  Build symbol table
 *********************************************************/
void InitializeTable( SymbolTable *table )
{
    int i;

    table->count = 0;
    for(i = 0 ; i < 23; i++) 
    {
        table->type[i] = Notype;
        table->name[i] = NULL;
    }
}

int getHashValue( char* str , int len ) // new
{
    int i, h = 0;
    for(i = 0; i < len; i ++)
    {
        
    }
    return h;
}

int getVariableIndex(SymbolTable *table, char* s) // redundent to lookupTable?
{
    int i;
    for(i = 0; i < table->count; i ++)
    {
        //if(strcmp(table->name[i], s) == 0)
        if(table->name[i] == s) // they are both pointers to NameTable
            return i;
    }
    return -1;
}

void add_table( SymbolTable *table, char* s, DataType t )// char -> char*
{
    int tc = table->count;
    if(tc >= 23)
    {
        printf("More than 23 variables declared\n");
        exit(1);
    }
    if(getVariableIndex(table, s) >= 0)
    {
        printf("Error : id %s has been declared\n", s);//erro
        exit(1);
    }
    table->type[tc] = t;
    //strcpy(table->name[tc], s);
    table->name[tc] = s;
    table->count += 1;
}

SymbolTable build( Program program )
{
    SymbolTable table;
    Declarations *decls = program.declarations;
    Declaration current;

    InitializeTable(&table);

    while(decls !=NULL){
        current = decls->first;
        add_table(&table, current.name, current.type);
        decls = decls->rest;
    }

    return table;
}


/********************************************************************
  Type checking
 *********************************************************************/

void convertType( Expression * old, DataType type )
{
    if(old->type == Float && type == Int){
        printf("error : can't convert float to integer\n");
        return;
    }
    if(old->type == Int && type == Float){
        Expression *tmp = (Expression *)malloc( sizeof(Expression) );
        if(old->v.type == Identifier)
            printf("convert to float %s \n",old->v.val.id);
        else
            printf("convert to float %d \n", old->v.val.ivalue);
        tmp->v = old->v;
        tmp->leftOperand = old->leftOperand;
        tmp->rightOperand = old->rightOperand;
        tmp->type = old->type;

        Value v;
        v.type = IntToFloatConvertNode;
        v.val.op = IntToFloatConvert;
        old->v = v;
        old->type = Int;
        old->leftOperand = tmp;
        old->rightOperand = NULL;
    }
}

DataType generalize( Expression *left, Expression *right )
{
    if(left->type == Float || right->type == Float){
        printf("generalize : float\n");
        return Float;
    }
    printf("generalize : int\n");
    return Int;
}

DataType lookup_table( SymbolTable *table, char* s )
{
    /* reserve for later use
    int id = c-'a';
    if( table->table[id] != Int && table->table[id] != Float)
        printf("Error : identifier %c is not declared\n", c);//error
    return table->table[id];
    */
    int i = getVariableIndex(table, s);
    if(i >= 0)
        return table->type[i];

    printf("Error : identifier %s is not declared\n", s);//error
    return Notype;
}

void checkexpression( Expression * expr, SymbolTable * table )
{
    char* s;
    if(expr->leftOperand == NULL && expr->rightOperand == NULL){
        switch(expr->v.type){
            case Identifier:
                s = expr->v.val.id;
                printf("identifier : %s\n",s);
                expr->type = lookup_table(table, s);
                break;
            case IntConst:
                printf("constant : int\n");
                expr->type = Int;
                break;
            case FloatConst:
                printf("constant : float\n");
                expr->type = Float;
                break;
                //case PlusNode: case MinusNode: case MulNode: case DivNode:
            default:
                break;
        }
    }
    else{
        Expression *left = expr->leftOperand;
        Expression *right = expr->rightOperand;

        checkexpression(left, table);
        checkexpression(right, table);

        DataType type = generalize(left, right);
        convertType(left, type);//left->type = type;//converto
        convertType(right, type);//right->type = type;//converto
        expr->type = type;
        
        // print_expr(expr);
        if( ((left->v).type == IntConst && 
            (right->v).type == IntConst )||
            ((left->v).type == FloatConst && 
            (right->v).type == FloatConst )) {
            // printf("merge");
            switch((expr->v).type){
                case PlusNode:
                    (expr->v).type = (left->v).type;
                    if( (expr->v).type == IntConst)
                        (expr->v).val.ivalue = (left->v).val.ivalue + (right->v).val.ivalue;
                    else if( expr->v.type == FloatConst )
                        (expr->v).val.fvalue = (left->v).val.fvalue + (right->v).val.fvalue;
                    break;
                case MinusNode:
                    (expr->v.type) = (left->v).type;
                    if( (expr->v).type == IntConst)
                        (expr->v).val.ivalue = (left->v).val.ivalue - (right->v).val.ivalue;
                    else if( expr->v.type == FloatConst )
                        (expr->v).val.fvalue = (left->v).val.fvalue - (right->v).val.fvalue;
                    break;
                case MulNode:
                    (expr->v.type) = (left->v).type;
                    if( (expr->v).type == IntConst)
                        (expr->v).val.ivalue = (left->v).val.ivalue * (right->v).val.ivalue;
                    else if( expr->v.type == FloatConst )
                        (expr->v).val.fvalue = (left->v).val.fvalue * (right->v).val.fvalue;
                     break;
                case DivNode:
                    (expr->v.type) = (left->v).type;
                    if( (expr->v).type == IntConst)
                        (expr->v).val.ivalue = (left->v).val.ivalue / (right->v).val.ivalue;
                    else if( expr->v.type == FloatConst )
                        (expr->v).val.fvalue = (left->v).val.fvalue / (right->v).val.fvalue;
                    break;
                default:
                    printf("error ");
                    break;
            }
            expr->leftOperand = NULL;
            expr->rightOperand = NULL;
        }
        else if( ((left->v).type == IntToFloatConvertNode ) &&
                 (((left->leftOperand)->v).type == IntConst ) &&
                 ((right->v).type == FloatConst)) {
            // printf("mergeLL(float)\n");
            switch((expr->v).type){
                case PlusNode:
                    (expr->v).type = (right->v).type;
                    (expr->v).val.fvalue = (float)((left->leftOperand)->v).val.ivalue + (right->v).val.fvalue;
                    break;
                case MinusNode:
                    (expr->v).type = (right->v).type;
                    (expr->v).val.fvalue = (float)((left->leftOperand)->v).val.ivalue - (right->v).val.fvalue;
                    break;
                case MulNode:
                    (expr->v).type = (right->v).type;
                    (expr->v).val.fvalue = (float)((left->leftOperand)->v).val.ivalue * (right->v).val.fvalue;
                    break;
                case DivNode:
                    (expr->v).type = (right->v).type;
                    (expr->v).val.fvalue = (float)((left->leftOperand)->v).val.ivalue / (right->v).val.fvalue;
                    break;
                default:
                    printf("error ");
                    break;
            }
            expr->leftOperand = NULL;
            expr->rightOperand = NULL;
        }
        else if( ((right->v).type == IntToFloatConvertNode )&&
                 (((right->leftOperand)->v).type == IntConst ) &&
                 ((left->v).type == FloatConst)){
            // printf("mergeLL(float)\n");
            switch((expr->v).type){
                case PlusNode:
                    (expr->v).type = (left->v).type;
                    (expr->v).val.fvalue = (left->v).val.fvalue + (float)((right->leftOperand)->v).val.ivalue;
                    break;
                case MinusNode:
                    (expr->v).type = (left->v).type;
                    (expr->v).val.fvalue = (left->v).val.fvalue - (float)((right->leftOperand)->v).val.ivalue;
                    break;
                case MulNode:
                    (expr->v).type = (left->v).type;
                    (expr->v).val.fvalue = (left->v).val.fvalue * (float)((right->leftOperand)->v).val.ivalue;
                    break;
                case DivNode:
                    (expr->v).type = (left->v).type;
                    (expr->v).val.fvalue = (left->v).val.fvalue / (float)((right->leftOperand)->v).val.ivalue;
                    break;
                default:
                    printf("error ");
                    break;
            }
            expr->leftOperand = NULL;
            expr->rightOperand = NULL;
        }
    }
}

void checkstmt( Statement *stmt, SymbolTable * table )
{
    if(stmt->type == Assignment){
        AssignmentStatement assign = stmt->stmt.assign;
        printf("assignment : %s \n",assign.id);
        checkexpression(assign.expr, table);
        stmt->stmt.assign.type = lookup_table(table, assign.id);
        if (assign.expr->type == Float && stmt->stmt.assign.type == Int) {
            printf("error : can't convert float to integer\n");
        } else {
            convertType(assign.expr, stmt->stmt.assign.type);
        }
    }
    else if (stmt->type == Print){
        printf("print : %s \n",stmt->stmt.variable);
        lookup_table(table, stmt->stmt.variable);
    }
    else printf("error : statement error\n");//error
}

void check( Program *program, SymbolTable * table )
{
    Statements *stmts = program->statements;
    while(stmts != NULL){
        checkstmt(&stmts->first,table);
        stmts = stmts->rest;
    }
}


/***********************************************************************
  Code generation
 ************************************************************************/
char getRegister(SymbolTable* table, char* variable)
{
    int index = getVariableIndex(table, variable);
    if(index < 0)
        printf("Error : identifier %s is not declared\n", variable);
    return ('a' + index);
}
void fprint_op( FILE *target, ValueType op )
{
    switch(op){
        case MinusNode:
            fprintf(target,"-\n");
            break;
        case PlusNode:
            fprintf(target,"+\n");
            break;
        case MulNode:
            fprintf(target,"*\n");
            break;
        case DivNode:
            fprintf(target,"/\n");
            break;
        default:
            fprintf(target,"Error in fprintf_op ValueType = %d\n",op);
            break;
    }
}

void fprint_expr( FILE *target, Expression *expr, SymbolTable* table)
{
    print_expr(expr);
    if(expr->leftOperand == NULL){
        switch( (expr->v).type ){
            case Identifier:
                if( (expr->v).si == -1){
                    fprintf(target,"-1.0\n" );
                    fprintf(target,"l%c\n", getRegister(table, (expr->v).val.id));
                    fprintf(target,"5 k\n" );
                    fprintf(target,"*\n" );
                }
                else {
                    fprintf(target,"l%c\n", getRegister(table, (expr->v).val.id));
                }
                break;
            case IntConst:
                fprintf(target,"%d\n",(expr->v).val.ivalue);
                break;
            case FloatConst:
                fprintf(target,"%f\n", (expr->v).val.fvalue);
                break;
            default:
                fprintf(target,"Error In fprint_left_expr. (expr->v).type=%d\n",(expr->v).type);
                break;
        }
    }
    else{
        fprint_expr(target, expr->leftOperand, table);
        if(expr->rightOperand == NULL){
            fprintf(target,"5 k\n");
        }
        else{
            //  fprint_right_expr(expr->rightOperand);
            fprint_expr(target, expr->rightOperand, table);
            fprint_op(target, (expr->v).type);
        }
    }
}

void gencode(Program prog, SymbolTable* table, FILE * target)
{
    Statements *stmts = prog.statements;
    Statement stmt;

    while(stmts != NULL){
        stmt = stmts->first;
        switch(stmt.type){
            case Print:
                fprintf(target,"l%c\n", getRegister(table, stmt.stmt.variable));
                fprintf(target,"p\n");
                break;
            case Assignment:
                fprint_expr(target, stmt.stmt.assign.expr, table);
                
                   if(stmt.stmt.assign.type == Int){
                   fprintf(target,"0 k\n");
                   }
                   else if(stmt.stmt.assign.type == Float){
                   fprintf(target,"5 k\n");
                   }

                fprintf(target,"s%c\n",getRegister(table, stmt.stmt.variable));
                fprintf(target,"0 k\n");
                break;
        }
        stmts=stmts->rest;
    }

}


/***************************************
  For our debug,
  you can omit them.
 ****************************************/
void print_expr(Expression *expr)
{
    if(expr == NULL)
        return;
    else{
        print_expr(expr->leftOperand);
        switch((expr->v).type){
            case Identifier:
                printf("%s ", (expr->v).val.id);
                break;
            case IntConst:
                printf("%d ", (expr->v).val.ivalue);
                break;
            case FloatConst:
                printf("%f ", (expr->v).val.fvalue);
                break;
            case PlusNode:
                printf("\'+\' ");
                break;
            case MinusNode:
                printf("\'-\' ");
                break;
            case MulNode:
                printf("\'*\' ");
                break;
            case DivNode:
                printf("\'/\' ");
                break;
            case IntToFloatConvertNode:
                printf("(float) ");
                break;
            default:
                printf("error ");
                break;
        }
        print_expr(expr->rightOperand);
    }
}

void test_parser( FILE *source )
{
    Declarations *decls;
    Statements *stmts;
    Declaration decl;
    Statement stmt;
    Program program = parser(source);

    decls = program.declarations;

    while(decls != NULL){
        decl = decls->first;
        if(decl.type == Int)
            printf("i ");
        if(decl.type == Float)
            printf("f ");
        printf("%s ",decl.name);// %c -> %s
        decls = decls->rest;
    }

    stmts = program.statements;

    while(stmts != NULL){
        stmt = stmts->first;
        /*
        if(stmt.type == Print){
            printf("p %s ", stmt.stmt.variable);
        }

        if(stmt.type == Assignment){
            printf("%s = ", stmt.stmt.assign.id);
            print_expr(stmt.stmt.assign.expr);
        }
        */
        stmts = stmts->rest;
    }

}
