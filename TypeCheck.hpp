#ifndef _TYPE_CHECK_H_
#define _TYPE_CHECK_H_
#include "std.hpp"
#include "Visitor.hpp"
#include "Error.hpp"
#include "SymbolTable.hpp"
#include "Tree.hpp"

class TypeCheck : public Visitor
{
    stack<shared_ptr<LocalSymbolTable> >  lstStack;
    shared_ptr<GlobalSymbolTable> gst;
    unsigned int canBreakStmt=0;
    ErrorContainer ec;
public:
    static void typeCheck(shared_ptr<Program> &program)
    {
        #ifdef _YLQ_DEBUG_
        Trace t(__LINE__,__FILE__);
        #endif
        TypeCheck tc;
        tc.gst=program->gst;
        program->accept(&tc);
        if(!tc.ec.noError())
            tc.ec.printErrorAndExit();
    }
    TypeCheck(){}

    virtual void visitProgram(const Program * program)
    {
        #ifdef _YLQ_DEBUG_
        Trace t(__LINE__,__FILE__);
        #endif
        for(auto pos=program->clist->begin();pos!=program->clist->end();++pos)
        {
            (*pos)->accept(this);
        }
    }
    virtual void visitClassDef(const ClassDef * classDef)
    {
        #ifdef _YLQ_DEBUG_
        Trace t(__LINE__,__FILE__);
        #endif
        for(auto pos=classDef->fields->begin();pos!=classDef->fields->end();++pos)
        {
            (*pos)->accept(this);
        }
    }
    virtual void visitFunctionDef(const FunctionDef * functionDef)
    {
        #ifdef _YLQ_DEBUG_
        Trace t(__LINE__,__FILE__);
        #endif
        functionDef->stmtBlock->accept(this);
        if(!functionDef->fst->returnType->isVoidType() && !functionDef->fst->localST->returnStmtMustCanBeExec)
            ec.addLocAndString(functionDef->fst->loc,"函数 "+functionDef->fst->name+" 的局部作用域中最少要有一个与之匹配的返回语句");
    }
    virtual void visitStmtBlock(const StmtBlock * stmtBlock)
    {
        #ifdef _YLQ_DEBUG_
        Trace t(__LINE__,__FILE__);
        #endif
        lstStack.push(stmtBlock->lst);
        for(auto pos=stmtBlock->stmts->begin();pos!=stmtBlock->stmts->end();++pos)
            (*pos)->accept(this);
        lstStack.pop();
        if(stmtBlock->lst->returnStmtMustCanBeExec && !lstStack.empty())
            lstStack.top()->returnStmtMustCanBeExec=true;
    }
    virtual void visitAssignStmt(const AssignStmt * assignStmt)
    {
        #ifdef _YLQ_DEBUG_
        Trace t(__LINE__,__FILE__);
        #endif
        assignStmt->lvalue->accept(this);
        assignStmt->expr->accept(this);
        if(!checkTypeCompatible(assignStmt->lvalue->ea.et,assignStmt->expr->ea.et))
        {
            ec.addLocAndString(assignStmt->loc,"等号左边类型不兼容等号右边类型");
            return;
        }
        if(!assignStmt->lvalue->ea.isLValue && !assignStmt->lvalue->ea.et->isErrorType())
            ec.addLocAndString(assignStmt->lvalue->loc,"等号左边的表达式不是一个左值");
    }
    virtual void visitForStmt(const ForStmt * forStmt)
    {
        #ifdef _YLQ_DEBUG_
        Trace t(__LINE__,__FILE__);
        #endif
        forStmt->simpleStmt1->accept(this);
        forStmt->boolExpr->accept(this);
        if(!forStmt->boolExpr->ea.et->isBoolType())
            ec.addLocAndString(forStmt->boolExpr->loc,"for 语句的条件表达式必须是 bool 类型的");
        forStmt->simpleStmt2->accept(this);
        canBreakStmt++;
        forStmt->stmt->accept(this);
        canBreakStmt--;
    }
    virtual void visitWhileStmt(const WhileStmt * whileStmt)
    {
        #ifdef _YLQ_DEBUG_
        Trace t(__LINE__,__FILE__);
        #endif
        whileStmt->boolExpr->accept(this);
        if(!whileStmt->boolExpr->ea.et->isBoolType())
            ec.addLocAndString(whileStmt->boolExpr->loc,"while 语句的条件表达式必须是 bool 类型的");
        canBreakStmt++;
        whileStmt->stmt->accept(this);
        canBreakStmt--;
    }
    virtual void visitIfStmt(const IfStmt * ifStmt)
    {
        #ifdef _YLQ_DEBUG_
        Trace t(__LINE__,__FILE__);
        #endif
        ifStmt->boolExpr->accept(this);
        if(!ifStmt->boolExpr->ea.et->isBoolType())
            ec.addLocAndString(ifStmt->boolExpr->loc,"if 语句的条件表达式必须是 bool 类型的");
        ifStmt->stmt1->accept(this);
        if(ifStmt->haveElse)
            ifStmt->stmt2->accept(this);
        if(dynamic_pointer_cast<StmtBlock>(ifStmt->stmt1)->lst->returnStmtMustCanBeExec && ifStmt->haveElse && dynamic_pointer_cast<StmtBlock>(ifStmt->stmt2)->lst->returnStmtMustCanBeExec)
            lstStack.top()->returnStmtMustCanBeExec=true;
    }
    virtual void visitReturnStmt(const ReturnStmt * returnStmt)
    {
        #ifdef _YLQ_DEBUG_
        Trace t(__LINE__,__FILE__);
        #endif
        lstStack.top()->returnStmtMustCanBeExec=true;
        shared_ptr<FunctionSymbolTable> parent=dynamic_pointer_cast<FunctionSymbolTable>(lstStack.top()->getFSTFromChild());
        if(returnStmt->haveExpr)
        {
            returnStmt->expr->accept(this);
            if(!checkTypeCompatible(parent->returnType,returnStmt->expr->ea.et))
                ec.addLocAndString(returnStmt->expr->loc,"返回值类型与此 return 语句所在函数的返回类型不兼容");
        }
        else if(!parent->returnType->isVoidType())
            ec.addLocAndString(returnStmt->loc,"return 语句的返回类型是 void，但是return 语句所在的函数返回类型不是 void");
    }
    virtual void visitBreakStmt(const BreakStmt * breakStmt)
    {
        #ifdef _YLQ_DEBUG_
        Trace t(__LINE__,__FILE__);
        #endif
        if(canBreakStmt==0)//如果 break 语句没有出现在 for 语句或者 while 语句中，就报错
            ec.addLocAndString(breakStmt->loc,"break 语句不应该出现在这");
    }
    virtual void visitPrintStmt(const PrintStmt * printStmt)
    {
        #ifdef _YLQ_DEBUG_
        Trace t(__LINE__,__FILE__);
        #endif
        //传递给 Print 函数的参数只能是 string ，int ，bool 这三种类型，所以只需要检查表达式的类型是否是
        //这三者之一就行了
        stringstream ss;
        int i=1;
        for(auto pos=printStmt->exprs->begin();pos!=printStmt->exprs->end();++pos)
        {
            (*pos)->accept(this);
            bool b1=(*pos)->ea.et->isBoolType();
            bool b2=(*pos)->ea.et->isIntType();
            bool b3=(*pos)->ea.et->isStringType();
            bool b4=(*pos)->ea.et->isErrorType();
            if(!b1 && !b2 && !b3 && !b4)
                ss<<"第"<<i<<"个 ";
            ++i;
        }
        string s = ss.str();
        if(s.size()!=0)
            ec.addLocAndString(printStmt->loc,"Print 函数的 "+s+"参数类型非法，只支持 int，bool，string");
    }
    virtual void visitIDLValue(const IDLValue * idLValue)
    {
        #ifdef _YLQ_DEBUG_
        Trace t(__LINE__,__FILE__);
        #endif
        if(idLValue->haveExprDot)
        {
            idLValue->exprDot->accept(this);
            if(!idLValue->exprDot->ea.et->isClassType())
            {
                if(!idLValue->exprDot->ea.et->isErrorType())
                ec.addError(shared_ptr<NotClassTypeExprError>(new NotClassTypeExprError(
                    idLValue->exprDot->loc
                )));
                const_cast<IDLValue *>(idLValue)->setError();
                return;
            }
            shared_ptr<ClassExprType> cet=dynamic_pointer_cast<ClassExprType>(idLValue->exprDot->ea.et);
            const string &className=cet->name;
            if(className=="null")
            {
                ec.addLocAndString(idLValue->exprDot->loc,"试图访问 null");
                const_cast<IDLValue *>(idLValue)->setError();
            }
            else
            {
                    //判断这个类名表示的类是否是当前类或者当前类的父类，如果不是，就报错，访问权限有问题，如果是，那么再检查变量是否存在
                    shared_ptr<ClassSymbolTable> cst=dynamic_pointer_cast<ClassSymbolTable>(lstStack.top()->getCSTFromChild());
                    shared_ptr<VariableSymbolTable> vst;
                    do {
                        if(cst->name==className)
                            break;
                        if(!cst->haveParent)
                            break;
                        cst=getCSTByName(cst->parentName);
                    } while(1);
                    if(cst->name!=className)
                    {
                        //说明 className 表示的这个类不是当前类或者当前类的父类，作用域有问题,不能访问其他类的私有成员变量
                        ec.addLocAndString(idLValue->exprDot->loc,"访问类 "+className+" 中的私有成员变量");
                        const_cast<IDLValue *>(idLValue)->setError();
                    }
                    else
                    {
                        //找到了这个类，那么就在这个类和他的父类中找 idLValue->idName 这个符号
                        vst=cst->findVSTByName(idLValue->idName);
                        if(vst.get()==nullptr)
                        {
                            //在当前类和父类中都没有找到idLValue->idName 这个符号，报错
                            ec.addLocAndString(idLValue->loc,"符号 "+idLValue->idName+" 未定义");
                            const_cast<IDLValue *>(idLValue)->setError();
                        }
                        else if(idLValue->exprDot->ea.justClassName)
                        {
                            ec.addLocAndString(idLValue->exprDot->loc,"无法通过类名调用成员变量");
                            const_cast<IDLValue *>(idLValue)->setError();
                        }
                        else
                        {
                            //找到了这个符号
                            const_cast<IDLValue *>(idLValue)->ea.et=vst->type;
                            const_cast<IDLValue *>(idLValue)->ea.isLValue=true;
                        }
                    }
                }
        }
        else
        {
            //从当前作用域开始向上查找 idLValue->idName 这个标识符
            shared_ptr<VariableSymbolTable> vst=dynamic_pointer_cast<VariableSymbolTable>(lstStack.top()->findVSTByNameAndLoc(idLValue->idName,idLValue->loc));
            if(vst.get()==nullptr)
            {
                //说明没有找到，那么判断idLValue->idName 表示的是否是一个类名
                if(getCSTByName(idLValue->idName).get()!=nullptr)
                {
                    const_cast<IDLValue *>(idLValue)->ea.et.reset(new ClassExprType(idLValue->idName));
                    const_cast<IDLValue *>(idLValue)->ea.isLValue=false;
                    const_cast<IDLValue *>(idLValue)->ea.justClassName=true;
                }
                else
                {
                    ec.addLocAndString(idLValue->loc,"未定义个符号 "+idLValue->idName);
                    const_cast<IDLValue *>(idLValue)->setError();
                }
            }
            else
            {
                //找到了
                const_cast<IDLValue *>(idLValue)->ea.et=vst->type;
                const_cast<IDLValue *>(idLValue)->ea.isLValue=true;
            }
        }
    }
    virtual void visitClassType(const ClassType * ct)
    {
        #ifdef _YLQ_DEBUG_
        Trace t(__LINE__,__FILE__);
        #endif
        //在全局符号表中找，是否有 ct->idName 这个类符号，如果没有，就报错
        if(getCSTByName(ct->idName).get()==nullptr)
        {
            ec.addLocAndString(ct->loc,"未定义的类 "+ct->idName);
            const_cast<ClassType *>(ct)->ea.et.reset(new BuildInExprType(BIST_ERROR));
        }
        else
            const_cast<ClassType *>(ct)->ea.et.reset(new ClassExprType(ct->idName));
    }
    virtual void visitArrayType(const ArrayType * at)
    {
        #ifdef _YLQ_DEBUG_
        Trace t(__LINE__,__FILE__);
        #endif
        at->type->accept(this);
        if(at->type->ea.et->isErrorType())
            const_cast<ArrayType *>(at)->ea.et.reset(new BuildInExprType(BIST_ERROR));
        else
        {
            ArrayExprType *aet=new ArrayExprType();
            aet->type=at->type->ea.et;
            const_cast<ArrayType *>(at)->ea.et.reset(aet);
        }
    }
    virtual void visitArrayLValue(const ArrayLValue * arrayLValue)
    {
        #ifdef _YLQ_DEBUG_
        Trace t(__LINE__,__FILE__);
        #endif
        /**
        1.expr1的类型必须是数组类型，否则就报错
        2.expr2的类型必须是整型，否则就报错
        3.最终类型为expr1中元素的类型
        **/
        arrayLValue->expr1->accept(this);
        if(!arrayLValue->expr1->ea.et->isArrayType())
        {
            ec.addLocAndString(arrayLValue->expr1->loc,"表达式不是数组类型，无法使用[]符号");
            const_cast<ArrayLValue *>(arrayLValue)->setError();
            return;//直接返回，不在继续向下
        }
        const_cast<ArrayLValue *>(arrayLValue)->ea.et=dynamic_pointer_cast<ArrayExprType>(arrayLValue->expr1->ea.et)->type;

        /**
        考虑如下情况
        int a[][];
        a[0]应该是一个非左值,这里是必须的，因为内存分配的数组空间是连续的，如果能够对 a[0]赋值的话，那么会打破数组的连续性，所以这里就让它不能赋值
        a[0][0]才是一个左值
        if(arrayLValue->ea.et->isArrayType() || arrayLValue->ea.et->isErrorType() || arrayLValue->ea.et->isVoidType())
            const_cast<ArrayLValue *>(arrayLValue)->ea.isLValue=false;
        else
            const_cast<ArrayLValue *>(arrayLValue)->ea.isLValue=true;
            **/
        const_cast<ArrayLValue *>(arrayLValue)->ea.isLValue=true;
        //对 expr2是否是整型进行检查
        arrayLValue->expr2->accept(this);
        if(!arrayLValue->expr2->ea.et->isIntType() && !arrayLValue->expr2->ea.et->isErrorType())
            ec.addLocAndString(arrayLValue->expr2->loc,"数组下标必须是 int 类型表达式");
    }
    virtual void visitNewArray(const NewArray * na)
    {
        #ifdef _YLQ_DEBUG_
        Trace t(__LINE__,__FILE__);
        #endif
        na->type->accept(this);
        na->expr->accept(this);
        if(na->type->ea.et->isErrorType())
        {
            const_cast<NewArray *>(na)->ea.et.reset(new BuildInExprType(BIST_ERROR));
            const_cast<NewArray *>(na)->ea.isLValue=false;
        }
        else
        {
            ArrayExprType *aet=new ArrayExprType();
            aet->type=na->type->ea.et;
            const_cast<NewArray *>(na)->ea.et.reset(aet);
            const_cast<NewArray *>(na)->ea.isLValue=false;
        }
        if(!na->expr->ea.et->isIntType() && !na->expr->ea.et->isErrorType())
            ec.addLocAndString(na->expr->loc,"数组下标必须是 int 类型表达式");
    }
    virtual void visitCall(const Call * call)
    {
        #ifdef _YLQ_DEBUG_
        Trace t(__LINE__,__FILE__);
        #endif
        if(call->haveExprDot)
        {
            call->exprDot->accept(this);
            if(call->exprDot->ea.et->isArrayType())
            {
                if(call->idName!="length")
                {
                    //数组类型只支持一个无参函数 length
                    ec.addLocAndString(call->loc,"数组 "+call->idName+" 只支持一个 length()函数");
                    const_cast<Call *>(call)->setError();
                }
                else if(call->actuals->size()!=0)
                {
                    //length 函数必须是无参数的
                    ec.addLocAndString(call->loc,"length()函数必须是无参数的");
                    const_cast<Call *>(call)->setError();
                }
                else
                {
                    //length 函数返回一个整数
                    const_cast<Call *>(call)->ea.et.reset(new BuildInExprType(BIST_INT));
                    const_cast<Call *>(call)->ea.isLValue=false;
                }
            }
            else if(call->exprDot->ea.et->isClassType())
            {
                shared_ptr<ClassExprType> cet=dynamic_pointer_cast<ClassExprType>(call->exprDot->ea.et);
                const string &className=cet->name;
                if(className=="null")
                {
                    ec.addLocAndString(call->exprDot->loc,"对 null 进行访问");
                    const_cast<Call *>(call)->setError();
                }
                else
                {
                    shared_ptr<ClassSymbolTable> cst=getCSTByName(className);
                    shared_ptr<FunctionSymbolTable> fst=cst->findFSTByName(call->idName);
                    if(fst.get()==nullptr)
                    {
                        //在cst和其父类中都没有找到call->idName 这个符号，报错
                        ec.addError(shared_ptr<UnDefineFuncError>(new UnDefineFuncError(
                            call->idName,call->loc
                        )));
                        const_cast<Call *>(call)->setError();
                    }
                    else
                    {
                        //找到了这个符号,首先判断是否是直接使用类名调用静态函数
                        //判断形参和实参的类型是否吻合
                        if(call->exprDot->ea.justClassName && !fst->isStatic)
                            ec.addLocAndString(call->exprDot->loc,"试图通过类名访问非静态函数");
                        else if(checkFormalsAndActuals(fst,call))
                        {
                            const_cast<Call *>(call)->ea.et=fst->returnType;
                            const_cast<Call *>(call)->ea.isLValue=false;
                        }
                        else
                        {
                            const_cast<Call *>(call)->setError();
                        }
                    }

                }
            }
            else
            {
                //既不是数组类型，也不是累类型，直接报错
                ec.addLocAndString(call->exprDot->loc,"无法对该表达式进行 '.' 运算");
                const_cast<Call *>(call)->setError();
            }
        }
        else
        {
            //从当前作用域开始向上查找 call->idName 表示的函数，静态函数只能调用静态函数
            shared_ptr<FunctionSymbolTable> currentFST=dynamic_pointer_cast<FunctionSymbolTable>(lstStack.top()->getFSTFromChild());
            shared_ptr<ClassSymbolTable> currentCST=dynamic_pointer_cast<ClassSymbolTable>(currentFST->parent);
            shared_ptr<FunctionSymbolTable> fst=currentCST->findFSTByName(call->idName);
            if(fst.get()==nullptr)
            {
                //在当前类和父类中都没有找到call->idName 这个符号，报错
                ec.addLocAndString(call->loc,"函数 "+call->idName+" 未定义");
                const_cast<Call *>(call)->setError();
            }
            else if(currentFST->isStatic && !fst->isStatic)
            {
                //静态函数只能够调用静态函数
                ec.addLocAndString(call->loc," 静态函数中不能直接调用非静态函数");
                const_cast<Call *>(call)->setError();
            }
            else
            {
                //找到了这个符号
                //判断形参和实参的类型是否吻合
                checkFormalsAndActuals(fst,call);
                const_cast<Call *>(call)->ea.et=fst->returnType;
                const_cast<Call *>(call)->ea.isLValue=false;
            }
        }
    }
    virtual void visitOneExpr(const OneExpr * oe)
    {
        #ifdef _YLQ_DEBUG_
        Trace t(__LINE__,__FILE__);
        #endif
        oe->expr->accept(this);
        switch(oe->kind)
        {
            case ONE_EXPR_BRACK:
            const_cast<OneExpr *>(oe)->ea.et=oe->expr->ea.et;
            const_cast<OneExpr *>(oe)->ea.isLValue=oe->expr->ea.isLValue;
            const_cast<OneExpr *>(oe)->ea.justClassName=oe->expr->ea.justClassName;
            break;
            case ONE_EXPR_UMINUS:
            if(oe->expr->ea.et->isIntType()||oe->expr->ea.et->isErrorType())
            {
                const_cast<OneExpr *>(oe)->ea.et=oe->expr->ea.et;
                const_cast<OneExpr *>(oe)->ea.isLValue=false;
            }
            else
            {
                const_cast<OneExpr *>(oe)->ea.et.reset(new BuildInExprType(BIST_ERROR));
                const_cast<OneExpr *>(oe)->ea.isLValue=false;
                ec.addLocAndString(oe->expr->loc,"负号只能对整数型类型表达式使用");
            }
            break;
            case ONE_EXPR_NOT:
            if(oe->expr->ea.et->isBoolType()||oe->expr->ea.et->isErrorType())
            {
                const_cast<OneExpr *>(oe)->ea.et=oe->expr->ea.et;
                const_cast<OneExpr *>(oe)->ea.isLValue=false;
            }
            else
            {
                const_cast<OneExpr *>(oe)->ea.et.reset(new BuildInExprType(BIST_ERROR));
                const_cast<OneExpr *>(oe)->ea.isLValue=false;
                ec.addLocAndString(oe->expr->loc," '!' 只能对布尔型表达式使用");
            }
            break;
            default:
            SeriousErrorHandler::seriousError("TypeCheck.h->visitOneExpr()");
        }
    }
    virtual void visitTwoExpr(const TwoExpr * te)
    {
        #ifdef _YLQ_DEBUG_
        Trace t(__LINE__,__FILE__);
        #endif

        te->leftExpr->accept(this);
        te->rightExpr->accept(this);
        switch(te->kind)
        {
            case TWO_EXPR_ADD:
            case TWO_EXPR_SUB:
            case TWO_EXPR_MUL:
            case TWO_EXPR_DIV:
            case TWO_EXPR_MOD:
            if(!te->leftExpr->ea.et->isIntType() || !te->rightExpr->ea.et->isIntType())
            {
                ec.addError(shared_ptr<CalcNotIntTypeError>(new CalcNotIntTypeError(
                    te->loc
                )));
            }
            break;
            case TWO_EXPR_SM:
            case TWO_EXPR_SM_EQ:
            case TWO_EXPR_BG:
            case TWO_EXPR_BG_EQ:
            if(!te->leftExpr->ea.et->isIntType() || !te->rightExpr->ea.et->isIntType())
            {
                ec.addError(shared_ptr<CompNotIntTypeError>(new CompNotIntTypeError(
                    te->loc
                )));
            }
            break;
            case TWO_EXPR_EQ_EQ:
            case TWO_EXPR_NOT_EQ:
            if((!checkTypeCompatible(te->leftExpr->ea.et,te->rightExpr->ea.et)) &&
            (!checkTypeCompatible(te->rightExpr->ea.et,te->leftExpr->ea.et)))
                ec.addLocAndString(te->loc,"表达式两边的类型不兼容");
            break;
            case TWO_EXPR_AND_AND:
            case TWO_EXPR_OR_OR:
            if(!te->leftExpr->ea.et->isBoolType() || !te->rightExpr->ea.et->isBoolType())
            {
                ec.addError(shared_ptr<RelaNotBoolTypeError>(new RelaNotBoolTypeError(
                    te->loc
                )));
            }
            break;
            default:
            SeriousErrorHandler::seriousError("TypeCheck.h->TwoExpr()");
        }
    }
    virtual void visitThisExpr(const ThisExpr *te)
    {
        #ifdef _YLQ_DEBUG_
        Trace t(__LINE__,__FILE__);
        #endif
        shared_ptr<FunctionSymbolTable> fst=dynamic_pointer_cast<FunctionSymbolTable>(lstStack.top()->getFSTFromChild());
        if(fst->isStatic)
        {
            ec.addLocAndString(te->loc,"静态函数中不能够使用 this");
            const_cast<ThisExpr *>(te)->ea.et.reset(new BuildInExprType(BIST_ERROR));
            const_cast<ThisExpr *>(te)->ea.isLValue=false;
        }
        else
        {
            shared_ptr<ClassSymbolTable> cst=dynamic_pointer_cast<ClassSymbolTable>(fst->getCSTFromChild());
            const_cast<ThisExpr *>(te)->ea.et.reset(new ClassExprType(cst->name));
            const_cast<ThisExpr *>(te)->ea.isLValue=false;
        }
    }
    virtual void visitClassTest(const ClassTest * ct)
    {
        #ifdef _YLQ_DEBUG_
        Trace t(__LINE__,__FILE__);
        #endif
        ct->expr->accept(this);
        if(!ct->expr->ea.et->isClassType())
        {
            ec.addError(shared_ptr<ClassTestUseToNotClassTypeError>(new ClassTestUseToNotClassTypeError(
                ct->expr->loc,ct->expr->ea.exprTypeToString()
            )));
        }
        else
        {
            shared_ptr<ClassExprType> cet=dynamic_pointer_cast<ClassExprType>(ct->expr->ea.et);
            if(cet->name=="null")
            {
                ec.addError(shared_ptr<ClassTestUseToNullError>(new ClassTestUseToNullError(
                    ct->expr->loc
                )));
            }
        }
        if(getCSTByName(ct->idName).get()==nullptr)
        {
            ec.addError(shared_ptr<ClassTestCantFindTheClassError>(new ClassTestCantFindTheClassError(
                ct->loc,ct->idName
            )));
        }
    }
    virtual void visitClassCast(const ClassCast * cc)
    {
        #ifdef _YLQ_DEBUG_
        Trace t(__LINE__,__FILE__);
        #endif
        cc->expr->accept(this);
        if(!cc->expr->ea.et->isClassType())
        {
            if(!cc->expr->ea.et->isErrorType())
            ec.addError(shared_ptr<ClassCastUseToNotClassTypeError>(new ClassCastUseToNotClassTypeError(
                cc->expr->ea.exprTypeToString(),cc->expr->loc
            )));
            return;
        }
        else if(dynamic_pointer_cast<ClassExprType>(cc->expr->ea.et)->name=="null")
        {
            ec.addError(shared_ptr<ClassCastUseToNullError>(new ClassCastUseToNullError(
                cc->expr->loc
            )));
            return;
        }
        if(!checkTypeCompatible(cc->ea.et,cc->expr->ea.et) && !checkTypeCompatible(cc->expr->ea.et,cc->ea.et))
            ec.addLocAndString(cc->loc,"类型无法转化");
    }

private:
    bool checkFormalsAndActuals(shared_ptr<FunctionSymbolTable> fst,const Call *call)
    {
        vector<shared_ptr<VariableSymbolTable> > &formals=fst->vvst;
        shared_ptr<vector<shared_ptr<BaseNode> > > actuals = call->actuals;
        if(formals.size()!=actuals->size())
        {
            ec.addError(shared_ptr<FormalsActualsCountNotSameError>(new FormalsActualsCountNotSameError(
                fst->name,fst->loc,formals.size(),call->loc,actuals->size()
            )));
            return false;
        }
        auto formal=formals.begin();
        auto actual=actuals->begin();
        int i=1;
        stringstream ss;
        for(;formal!=formals.end();++formal,++actual,++i)
        {
            (*actual)->accept(this);
            if(!checkTypeCompatible((*formal)->type,(*actual)->ea.et))
                ss<<"第"<<i<<"个 ";
        }
        if(ss.str().size()!=0)
        {
            ec.addError(shared_ptr<FormalActualTypeNotSameError>(new FormalActualTypeNotSameError(
                fst->name,fst->loc,call->loc,ss.str()
            )));
            return false;
        }
        return true;
    }
    shared_ptr<ClassSymbolTable> getCSTByName(string const &name)
    {
        shared_ptr<shared_ptr<ClassSymbolTable> > null_cst(new shared_ptr<ClassSymbolTable>());
        if(name=="null")
        return *null_cst;
        return gst->getCSTByName(name);
    }
    bool checkTypeCompatible(shared_ptr<SymbolType> left,shared_ptr<SymbolType> right)
    {
        //left 必须兼容 right 才返回 true
        if(left->isErrorType() || right->isErrorType())
            return true;
        if(left->isBuildInType() && right->isBuildInType())
            return dynamic_pointer_cast<BuildInExprType>(left)->bist==dynamic_pointer_cast<BuildInExprType>(right)->bist;
        if(left->isClassType() && right->isClassType())
        {
            shared_ptr<ClassExprType> leftc=dynamic_pointer_cast<ClassExprType>(left);
            shared_ptr<ClassExprType> rightc=dynamic_pointer_cast<ClassExprType>(right);
            if(rightc->name=="null")
                return true;
            if(leftc->name==rightc->name)
                return true;
            shared_ptr<ClassSymbolTable> p=getCSTByName(rightc->name);
            if(p.get()==nullptr)
                SeriousErrorHandler::seriousError("TypeCheck.h->checkTypeCompatible()");
            while(p->haveParent)
                if(p->parentClass->name==leftc->name)
                    return true;
                else
                    p=p->parentClass;
        }
        if(left->isArrayType())
        {
            if(right->isClassType() && dynamic_pointer_cast<ClassExprType>(right)->name=="null")
                return true;
            return left->symbolTypeToString()==right->symbolTypeToString();//现在就先这样吧
        }
        return false;
    }
};
#endif /* end of include guard: _TYPE_CHECK_H_ */
