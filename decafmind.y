%{
  #include "Tree.hpp"
  #include "std.hpp"
  #include "lab3_exec.hpp"
  #include "lab4_exec.hpp"
  #include "GenSymbolTable.hpp"
  #include "TypeCheck.hpp"
  #include "Print.hpp"
  extern FILE *yyin;
  extern char *yytext;
  extern Info info;
  extern Location loc;
  extern stack<Location> idStack;
  #ifdef YYLEX_PARAM
  extern int yylex (YYLEX_PARAM);
  #else
  extern int yylex ();
  #endif
  void yyerror(std::string);
  shared_ptr<Program> program;

  Location pop(stack<Location> &stk)
  {
      Location loc=stk.top();
      stk.pop();
      return loc;
  }

template <class T,class...Args>
Wrap * init(Args...args){
  Wrap * w = new Wrap;
  w->bn.reset(new T(args...));
  return w;
}

template <class...Args>
Wrap * initVec(Args...args)
{
  Wrap *w=new Wrap;
  w->vec.reset(new vector<shared_ptr<BaseNode> >(args...));
  return w;
}
%}

%union {
  char buf[1024];
  unsigned int idNameIndex;
  unsigned int intConst;
  unsigned char boolConst;
  struct Wrap * wrap;
};

%type <wrap> Program VariableDef Variable Type Formals FunctionDef ClassDef ClassDef_II
%type <wrap> Field StmtBlock StmtBlock_II Stmt SimpleStmt LValue Call Actuals ForStmt WhileStmt
%type <wrap> IfStmt ReturnStmt BreakStmt PrintStmt PrintStmt_II BoolExpr Expr Constant

%token <intConst> INTCONST
%token <boolConst> BOOLCONST
%token <buf> IDENTIFIER STRINGCONST

%token KEY_BOOL KEY_BREAK KEY_CLASS KEY_ELSE KEY_EXTENDS KEY_FOR KEY_IF
%token KEY_INT KEY_NEW KEY_NULL KEY_RETURN KEY_STRING KEY_THIS KEY_VOID
%token KEY_WHILE KEY_STATIC KEY_PRINT KEY_READINTEGER KEY_READLINE KEY_INSTANCEOF

%token SY_ADD SY_SUB SY_MUL SY_DIV SY_MOD SY_SM SY_SM_EQ SY_BG SY_BG_EQ SY_EQ SY_EQ_EQ SY_UMINUS
%token SY_NOT_EQ SY_AND_AND SY_OR_OR SY_NOT SY_SEMICOLON SY_COMMA SY_DOT
%token SY_L_MID SY_R_MID SY_L_SM SY_R_SM SY_L_BG SY_R_BG


%right SY_EQ
%nonassoc SY_OR_OR
%nonassoc SY_AND_AND
%nonassoc SY_EQ_EQ SY_NOT_EQ
%nonassoc SY_SM SY_BG SY_SM_EQ SY_BG_EQ
%left SY_ADD SY_SUB
%left SY_MUL SY_DIV SY_MOD
%right SY_NOT SY_UMINUS
%nonassoc KEY_INSTANCEOF
%left SY_DOT SY_L_MID SY_R_MID
%nonassoc SY_L_SM SY_R_SM
%nonassoc KEY_ELSE

%start Program

%%
Program:Program ClassDef
{
  $$->vec->push_back($2->bn);
  program.reset(new Program(loc,$$->vec));
}
| ClassDef
{
    $$=initVec(1,$1->bn);
    program.reset(new Program(loc,$$->vec));
}
;

VariableDef: Variable SY_SEMICOLON
{$$=$1;}
;

Variable: Type IDENTIFIER
{$$=init<Variable>(pop(idStack),$1->bn,string($2));}
;

Type: KEY_INT
{$$=init<BuildInType>(loc,TYPE_INT);}
| KEY_BOOL
{$$=init<BuildInType>(loc,TYPE_BOOL);}
| KEY_STRING
{$$=init<BuildInType>(loc,TYPE_STRING);}
| KEY_VOID
{$$=init<BuildInType>(loc,TYPE_VOID);}
| KEY_CLASS IDENTIFIER
{$$=init<ClassType>(pop(idStack),string($2));}
| Type SY_L_MID SY_R_MID
{$$=init<ArrayType>(loc,$1->bn);}
;

Formals:/*empty*/
{$$=initVec();}
| Formals SY_COMMA Variable
{$$->vec->push_back($3->bn);}
| Variable
{$$=initVec(1,$1->bn);}
;

FunctionDef: Type IDENTIFIER SY_L_SM Formals SY_R_SM StmtBlock
{$$=init<FunctionDef>(pop(idStack),false,$1->bn,string($2),$4->vec,$6->bn);}
| KEY_STATIC Type IDENTIFIER SY_L_SM Formals SY_R_SM StmtBlock
{$$=init<FunctionDef>(pop(idStack),true,$2->bn,string($3),$5->vec,$7->bn);}
;

ClassDef: KEY_CLASS IDENTIFIER SY_L_BG ClassDef_II SY_R_BG
{$$=init<ClassDef>(pop(idStack),string($2),$4->vec);}
| KEY_CLASS IDENTIFIER KEY_EXTENDS IDENTIFIER SY_L_BG ClassDef_II SY_R_BG
{pop(idStack);$$=init<ClassDef>(pop(idStack),string($2),string($4),$6->vec);}
;

ClassDef_II:
{$$=initVec();}
| ClassDef_II Field
{$$->vec->push_back($2->bn);}
| Field
{$$=initVec(1,$1->bn);}
;

Field: VariableDef
{$$=$1;}
| FunctionDef
{$$=$1;}
;

StmtBlock: SY_L_BG StmtBlock_II SY_R_BG
{$$=init<StmtBlock>(loc,$2->vec);}
;

StmtBlock_II:
{$$=initVec();}
| StmtBlock_II Stmt
{$$->vec->push_back($2->bn);}
| Stmt
{$$=initVec(1,$1->bn);}
;

Stmt: VariableDef
{$$=$1;}
| SimpleStmt SY_SEMICOLON
{$$=$1;}
| IfStmt
{$$=$1;}
| WhileStmt
{$$=$1;}
| ForStmt
{$$=$1;}
| BreakStmt SY_SEMICOLON
{$$=$1;}
| ReturnStmt SY_SEMICOLON
{$$=$1;}
| PrintStmt SY_SEMICOLON
{$$=$1;}
| StmtBlock
{$$=$1;}
;

SimpleStmt:
{$$=init<Empty>(loc);}
| LValue SY_EQ Expr
{$$=init<AssignStmt>(loc,$1->bn,$3->bn);}
| Call
{$$=$1;}
;

LValue: IDENTIFIER
{$$=init<IDLValue>(pop(idStack),string($1));}
| Expr SY_DOT IDENTIFIER
{$$=init<IDLValue>(pop(idStack),$1->bn,string($3));}
| Expr SY_L_MID Expr SY_R_MID
{$$=init<ArrayLValue>(loc,$1->bn,$3->bn);}
;

Call: IDENTIFIER SY_L_SM Actuals SY_R_SM
{$$=init<Call>(pop(idStack),string($1),$3->vec);}
| Expr SY_DOT IDENTIFIER SY_L_SM Actuals SY_R_SM
{$$=init<Call>(pop(idStack),$1->bn,string($3),$5->vec);}
;

Actuals:
{$$=initVec();}
| Actuals SY_COMMA Expr
{$$->vec->push_back($3->bn);}
| Expr
{$$=initVec(1,$1->bn);}
;

ForStmt: KEY_FOR SY_L_SM SimpleStmt SY_SEMICOLON BoolExpr SY_SEMICOLON SimpleStmt SY_R_SM Stmt
{$$=init<ForStmt>(loc,$3->bn,$5->bn,$7->bn,$9->bn);}
;

WhileStmt: KEY_WHILE SY_L_SM BoolExpr SY_R_SM Stmt
{$$=init<WhileStmt>(loc,$3->bn,$5->bn);}
;

IfStmt: KEY_IF SY_L_SM BoolExpr SY_R_SM Stmt
{$$=init<IfStmt>(loc,$3->bn,$5->bn);}
| KEY_IF SY_L_SM BoolExpr SY_R_SM Stmt KEY_ELSE Stmt
{$$=init<IfStmt>(loc,$3->bn,$5->bn,$7->bn);}
;

ReturnStmt: KEY_RETURN
{$$=init<ReturnStmt>(loc);}
| KEY_RETURN Expr
{$$=init<ReturnStmt>(loc,$2->bn);}
;

BreakStmt: KEY_BREAK
{$$=init<BreakStmt>(loc);}
;

PrintStmt: KEY_PRINT SY_L_SM PrintStmt_II SY_R_SM
{$$=init<PrintStmt>(loc,$3->vec);}
;

PrintStmt_II:PrintStmt_II SY_COMMA Expr
{$$->vec->push_back($3->bn);}
| Expr
{$$=initVec(1,$1->bn);}
;

BoolExpr: Expr
{$$=$1;}
;

Expr: Constant
{$$=$1;}
| LValue
{$$=$1;}
| KEY_THIS
{$$=init<ThisExpr>(loc);}
| Call
{$$=$1;}
| KEY_READINTEGER SY_L_SM SY_R_SM
{$$=init<ReadInteger>(loc);}
| KEY_READLINE SY_L_SM SY_R_SM
{$$=init<ReadLine>(loc);}
| KEY_NEW IDENTIFIER SY_L_SM SY_R_SM
{$$=init<NewObject>(pop(idStack),string($2));}
| KEY_NEW Type SY_L_MID Expr SY_R_MID
{$$=init<NewArray>(loc,$2->bn,$4->bn);}
| Expr SY_ADD Expr
{$$=init<TwoExpr>(loc,TWO_EXPR_ADD,$1->bn,$3->bn);}
| Expr SY_SUB Expr
{$$=init<TwoExpr>(loc,TWO_EXPR_SUB,$1->bn,$3->bn);}
| Expr SY_MUL Expr
{$$=init<TwoExpr>(loc,TWO_EXPR_MUL,$1->bn,$3->bn);}
| Expr SY_DIV Expr
{$$=init<TwoExpr>(loc,TWO_EXPR_DIV,$1->bn,$3->bn);}
| Expr SY_MOD Expr
{$$=init<TwoExpr>(loc,TWO_EXPR_MOD,$1->bn,$3->bn);}
| Expr SY_SM Expr
{$$=init<TwoExpr>(loc,TWO_EXPR_SM,$1->bn,$3->bn);}
| Expr SY_SM_EQ Expr
{$$=init<TwoExpr>(loc,TWO_EXPR_SM_EQ,$1->bn,$3->bn);}
| Expr SY_BG Expr
{$$=init<TwoExpr>(loc,TWO_EXPR_BG,$1->bn,$3->bn);}
| Expr SY_BG_EQ Expr
{$$=init<TwoExpr>(loc,TWO_EXPR_BG_EQ,$1->bn,$3->bn);}
| Expr SY_EQ_EQ Expr
{$$=init<TwoExpr>(loc,TWO_EXPR_EQ_EQ,$1->bn,$3->bn);}
| Expr SY_NOT_EQ Expr
{$$=init<TwoExpr>(loc,TWO_EXPR_NOT_EQ,$1->bn,$3->bn);}
| Expr SY_AND_AND Expr
{$$=init<TwoExpr>(loc,TWO_EXPR_AND_AND,$1->bn,$3->bn);}
| Expr SY_OR_OR Expr
{$$=init<TwoExpr>(loc,TWO_EXPR_OR_OR,$1->bn,$3->bn);}
| SY_L_SM Expr SY_R_SM
{$$=init<OneExpr>(loc,ONE_EXPR_BRACK,$2->bn);}
| SY_SUB Expr %prec SY_MUL
{$$=init<OneExpr>(loc,ONE_EXPR_UMINUS,$2->bn);}
| SY_NOT Expr
{$$=init<OneExpr>(loc,ONE_EXPR_NOT,$2->bn);}
| KEY_INSTANCEOF SY_L_SM Expr SY_COMMA IDENTIFIER SY_R_SM
{$$=init<ClassTest>(pop(idStack),$3->bn,string($5));}
| SY_L_SM KEY_CLASS IDENTIFIER SY_R_SM Expr
{$$=init<ClassCast>(pop(idStack),string($3),$5->bn);}
;

Constant: INTCONST
{$$=init<IntConstant>(loc,$1);}
| BOOLCONST
{$$=init<BoolConstant>(loc,$1==1?true:false);}
| STRINGCONST
{$$=init<StringConstant>(loc,string($1));}
| KEY_NULL
{$$=init<NullConstant>(loc);}
;
%%

bool Trace::openTrace=false;
bool showAST=false;
bool showST=false;
bool showTac=false;
bool showAsm=false;
void help()
{
    cout<<"\n-----help-----\n";
    cout<<"1.使用 [-1] 输出语法树"<<endl;
    cout<<"2.使用 [-2] 输出符号表"<<endl;
    cout<<"3.使用 [-3] 输出Tac序列"<<endl;
    cout<<"4.使用 [-4] 输出x86汇编代码"<<endl;
    cout<<"一个简单的例子:\n ./dm test.decaf -4 > test.s"<<endl;
    cout<<"如果想要获得汇编代码，请必须开启 [-4] 编译选项，并将输出重定向到一个文件"<<endl;
    exit(0);
}
void option(int argc,char **args)
{
    if(argc<=1)
        help();
    for(int i=1;i<argc;++i)
    {
        if(args[i][0]!='-')
        continue;
        if(strlen(args[i])<2)
        continue;
        switch(args[i][1])
        {
            case 't':
            Trace::openTrace=true;
            break;
            case '1':
            showAST=true;
            break;
            case '2':
            showST=true;
            break;
            case '3':
            showTac=true;
            break;
            case '4':
            showAsm=true;
            break;
        }
    }
}

int main(int argc,char **args){
    option(argc,args);
    yyin = fopen(args[1],"r");
    yyparse();
    if(showAST)
        program->display(0);//显示语法树
    GenSymbolTable::genSymbolTable(program);//生成符号表并进行符号表检查,如果有错误，就会输出错误，并且退出
    TypeCheck::typeCheck(program);//进行类型检查，如果有错，就会输出错误然后退出
    if(showST)
        Print::printGST(program->gst);
    lab3::GenTac::genTac(program);
    if(showTac)
        lab3::Print::print(program);
    if(showAsm)
        lab4::GenCode::genCode(program);
    return 0;
}

void yyerror(std::string s){
  std::cout<<"\033[01;40;31m语法错误,出错行 "<<info.lineNum+1<<"\033[0m\n"<<std::endl;
  std::cout<<s<<std::endl;
  exit(0);
}
