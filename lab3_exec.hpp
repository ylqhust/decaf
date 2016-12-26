#ifndef _COMPILE_LAB3_H_
#define _COMPILE_LAB3_H_
#include "std.hpp"
#include "Visitor.hpp"
#include "Tree.hpp"
#include "lab3_define.hpp"
namespace lab3
{
    class GenTac : public Visitor
    {
        shared_ptr<GlobalSymbolTable> gst;
        map<const string,shared_ptr<VirtualTable> > vtMap; //所有的虚函数表
        shared_ptr<vector<shared_ptr<VirtualTable> > > vts;//按照先后顺序排列的虚函数表
        map<const string,shared_ptr<Function> > staticFuncMap;//所有的静态函数
        shared_ptr<vector<shared_ptr<Function> > > staticFuncs;//按照先后顺序排列的静态函数表

        shared_ptr<ClassSymbolTable> currentCST;
        shared_ptr<VirtualTable> currentVT;
        shared_ptr<FunctionSymbolTable> currentFST;
        shared_ptr<Function> currentF;
        vector<shared_ptr<LocalSymbolTable> > lstStack;
        stack<shared_ptr<Label> > loopStopStack;//循环停止的标号，供break 语句使用
    public:
        static void genTac(shared_ptr<Program> &program)
        {
            GenTac gt;
            gt.vts.reset(new vector<shared_ptr<VirtualTable> >);
            gt.staticFuncs.reset(new vector<shared_ptr<Function> >);
            gt.gst=program->gst;
            program->accept(&gt);
            program->vts=gt.vts;
            program->staticFuncs=gt.staticFuncs;
        }
    private:
        virtual void visitProgram(const Program * program)
        {
            #ifdef _YLQ_DEBUG_
            Trace t(__LINE__,__FILE__);
            #endif
            const shared_ptr<GlobalSymbolTable> &gst=program->gst;
            vector<shared_ptr<ClassSymbolTable> > cstList=gst->getCST();
            for(auto cst : cstList)
                createVTBL(cst);//为每一个类创建虚函数表
            for(auto cst : cstList)
                createNewObjectFunction(cst);//为每一个类创建 New 函数
            for(auto cd : *(program->clist))
                cd->accept(this);
        }
        virtual void visitClassDef(const ClassDef * classDef)
        {
            #ifdef _YLQ_DEBUG_
            Trace t(__LINE__,__FILE__);
            #endif
            currentCST=const_cast<ClassDef *>(classDef)->cst;
            currentVT=vtMap.find(currentCST->name)->second;

            const shared_ptr<vector<shared_ptr<BaseNode> > > &fields=classDef->fields;
            for(auto field:(*fields))
                field->accept(this);
        }
        virtual void visitFunctionDef(const FunctionDef * functionDef)
        {
            #ifdef _YLQ_DEBUG_
            Trace t(__LINE__,__FILE__);
            #endif
            currentFST=const_cast<FunctionDef *>(functionDef)->fst;
            currentF.reset(new Function(currentCST->name,currentFST->name));
            functionDef->stmtBlock->accept(this);
            (*currentF)<<Tac::genReturn();//强行插入一条return语句，确保函数能够返回
            if(currentFST->isStatic)
            {
                staticFuncMap.insert(pair<const string,shared_ptr<Function> >(currentF->getUniqueFuncId(),currentF));
                staticFuncs->push_back(currentF);
            }
            else
                currentVT->addFunc(currentF);
        }
        virtual void visitStmtBlock(const StmtBlock * stmtBlock)
        {
            #ifdef _YLQ_DEBUG_
            Trace t(__LINE__,__FILE__);
            #endif
            lstStack.push_back(stmtBlock->lst);
            if(stmtBlock->lst->vvst.size()!=0)
                (*currentF)<<Tac::genAllocStack(Temp::getIntConstTemp(4*(stmtBlock->lst->vvst.size())));
            for(auto stmt:(*stmtBlock->stmts))
                stmt->accept(this);
            if(stmtBlock->lst->vvst.size()!=0)
                (*currentF)<<Tac::genReleaseStack(Temp::getIntConstTemp(4*(stmtBlock->lst->vvst.size())));
            lstStack.pop_back();
        }
        virtual void visitAssignStmt(const AssignStmt * assignStmt)
        {
            #ifdef _YLQ_DEBUG_
            Trace tr(__LINE__,__FILE__);
            #endif
            assignStmt->lvalue->accept(this);
            assignStmt->expr->accept(this);
            Temp t=getValue(assignStmt->expr);
            switch(assignStmt->lvalue->ea3.vi)
            {
                case VALUE_IN_HEAP:
                (*currentF)<<Tac::genStore(assignStmt->lvalue->ea3.pointer,assignStmt->lvalue->ea3.offset,t);
                break;
                case VALUE_IN_STACK:
                (*currentF)<<Tac::genStoreToStack(t,assignStmt->lvalue->ea3.offset);
                break;
                default:
                //左值应该是有地址的
                SEE;
            }
        }
        virtual void visitIDLValue(const IDLValue * idLValue)
        {
            #ifdef _YLQ_DEBUG_
            stringstream ss;
            ss<<" 源码："<<idLValue->loc.row;
            Trace tr(__LINE__,__FILE__,ss.str());
            #endif
            if(idLValue->haveExprDot)
            {
                idLValue->exprDot->accept(this);
                Temp currentObjectAddress=getValue(idLValue->exprDot);
                const_cast<IDLValue *>(idLValue)->ea3.pointer=currentObjectAddress;
                const_cast<IDLValue *>(idLValue)->ea3.offset=getVarOffsetInClass(dynamic_pointer_cast<ClassExprType>(idLValue->exprDot->ea.et)->name,idLValue->idName);
                const_cast<IDLValue *>(idLValue)->ea3.vi=VALUE_IN_HEAP;
            }
            else
            {
                //idLValue->idName 有可能是局部定义的变量，有可能是形参中的参数，也有可能是当前类或者他的父类中的成员
                //局部变量和形参中的数据都在栈中，只有类中的成员才在堆中
                bool success=false;
                Temp offset=getVarOffsetInLocal(idLValue->idName,success);//在已存在的局部作用域中查找这个变量
                if(success)
                {
                    const_cast<IDLValue *>(idLValue)->ea3.offset=offset;
                    const_cast<IDLValue *>(idLValue)->ea3.vi=VALUE_IN_STACK;
                    return;
                }
                offset=getVarOffsetInFormal(idLValue->idName,success);//在当前函数的形参中查找
                if(success)
                {
                    const_cast<IDLValue *>(idLValue)->ea3.offset=offset;
                    const_cast<IDLValue *>(idLValue)->ea3.vi=VALUE_IN_STACK;
                    return;
                }
                const_cast<IDLValue *>(idLValue)->ea3.pointer=Temp::genNewTemp();
                const_cast<IDLValue *>(idLValue)->ea3.offset=getVarOffsetInClass(currentCST->name,idLValue->idName);
                const_cast<IDLValue *>(idLValue)->ea3.vi=VALUE_IN_HEAP;
                (*currentF)<<Tac::genLoadFromStack(idLValue->ea3.pointer,getVarOffsetInFormal("this",success));//获取 this 的地址,this 是作为非静态函数的第一个隐含参数的
            }
        }
        virtual void visitArrayLValue(const ArrayLValue *arrayLValue)
        {
            #ifdef _YLQ_DEBUG_
            Trace tr(__LINE__,__FILE__);
            #endif
            arrayLValue->expr1->accept(this);
            arrayLValue->expr2->accept(this);
            Temp t=getValue(arrayLValue->expr1);
            const_cast<ArrayLValue *>(arrayLValue)->ea3.pointer=t;
            const_cast<ArrayLValue *>(arrayLValue)->ea3.vi=VALUE_IN_HEAP;
            Temp arrayMaxLength=Temp::genNewTemp();//这个数组可以访问的最大长度
            (*currentF)<<Tac::genLoad(arrayMaxLength,arrayLValue->ea3.pointer,getSaveArrayLengthUnitOffset());
            Temp index=getValue(arrayLValue->expr2);//用户想要访问的长度
            //判断 index 是否小余 arrayLenght
            shared_ptr<Label> l=Label::genLabel();
            Temp cmp1=Temp::genNewTemp();
            Temp cmp2=Temp::genNewTemp();
            Temp res=Temp::genNewTemp();
            (*currentF)<<Tac::genLess(cmp1,index,arrayMaxLength)
            <<Tac::genLessEqual(cmp2,Temp::getIntConstTemp(0),index)
            <<Tac::genAndAnd(res,cmp1,cmp2)
            <<Tac::genJmpNotEqual(res,Temp::getIntConstTemp(0),l);
            //下面是 index>=arrayLength 时的处理代码
            Temp unique=Temp::genNewTemp();
            //printf("第%d行数组访问越界",arrayLValue->loc.row)
            (*currentF)
            <<Tac::genPush(Temp::getIntConstTemp(arrayLValue->loc.row))
            <<Tac::genLoadStr(unique," 第%d行数组访问越界\\n")
            <<Tac::genPush(unique)
            <<Tac::genCall(PRINTF)
            <<Tac::genReleaseStack(Temp::getIntConstTemp(8))
            <<Tac::genPush(Temp::getIntConstTemp(1))
            <<Tac::genCall(EXIT)//退出
            <<Tac::genReleaseStack(Temp::getIntConstTemp(4))
            <<Tac::genMark(l);
            Temp offset=Temp::genNewTemp();
            (*currentF)<<Tac::genMul(offset,index,Temp::getIntConstTemp(4));
            const_cast<ArrayLValue *>(arrayLValue)->ea3.offset=offset;
        }
        void parmCallRelease(const shared_ptr<vector<shared_ptr<BaseNode> > > &actuals,const string &funcName,const Temp &ret)
        {
            //(*currentF)<<Tac::genPushACD();
            for(int i=actuals->size()-1;i>=0;--i)
            {
                (*actuals)[i]->accept(this);
                Temp t=getValue((*actuals)[i]);
                (*currentF)<<Tac::genPush(t);//压入参数
            }
            (*currentF)<<Tac::genCall(ret,funcName);
            if(actuals->size()!=0)
            (*currentF)<<Tac::genReleaseStack(Temp::getIntConstTemp(4*actuals->size()));//释放形参所占用的栈空间
        //    (*currentF)<<Tac::genPopACD();
        }
        void parmCallRelease(const Temp &thisAddr,
            const shared_ptr<vector<shared_ptr<BaseNode> > > &actuals,
            const Temp &virtualTableAddress,const Temp &funcOffsetInVTB,
            const Temp &ret)
        {
            //从右向左压入参数
        //    (*currentF)<<Tac::genPushACD();
            for(int i=actuals->size()-1;i>=0;--i)
            {
                (*actuals)[i]->accept(this);
                Temp t=getValue((*actuals)[i]);
                (*currentF)<<Tac::genPush(t);//压入参数
            }
            (*currentF)<<Tac::genPush(thisAddr);
            //参数全部压入，开始调用函数
            (*currentF)<<Tac::genCall(ret,virtualTableAddress,funcOffsetInVTB);
            (*currentF)<<Tac::genReleaseStack(Temp::getIntConstTemp(4*(actuals->size()+1)));//释放形参所占用的栈空间,这里多加了一个1是因为还压入了当前对象的地址
        //    (*currentF)<<Tac::genPopACD();
        }
        virtual void visitCall(const Call * call)
        {
            const_cast<Call *>(call)->ea3.vi=VALUE_IN_REGISTER;
            const_cast<Call *>(call)->ea3.reg=Temp::genNewTemp();
            if(call->haveExprDot)
            {
                if(call->exprDot->ea.justClassName)
                    //通过类名调用静态函数
                    parmCallRelease(call->actuals, Function::genUniqueFuncId(dynamic_pointer_cast<ClassExprType>(call->exprDot->ea.et)->name,call->idName),call->ea3.reg);
                else
                {
                    call->exprDot->accept(this);
                    //调用非静态函数
                    if(call->exprDot->ea.et->isArrayType())
                    {
                        //调用 length 函数，这里直接获取长度数据，而不是真的调用函数
                        Temp arrayAddr=getValue(call->exprDot);
                        (*currentF)<<Tac::genLoad(call->ea3.reg,arrayAddr,Temp::getIntConstTemp(-4));
                    }
                    else
                    {
                        //通过类实例调用函数
                        Temp currentObjectAddress=getValue(call->exprDot);
                        //现在已经获取了这个对象在内存中的地址，那么直接访问这个地址，就可以获取到该对象的虚函数表的地址
                        Temp virtualTableAddress=Temp::genNewTemp();
                        (*currentF)<<Tac::genLoad(virtualTableAddress,currentObjectAddress,Temp::getIntConstTemp(0));
                        //现在虚函数表的地址也有了，那么就要在虚函数表中找到这个函数的偏移，这个函数的偏移应该是一个常量，可以在编译期间确定下来
                        Temp funcOffsetInVTB = getFuncOffsetInClass(dynamic_pointer_cast<ClassExprType>(call->exprDot->ea.et)->name,call->idName);
                        //现在函数地址也有了，那么开始压入参数，最后压入 当前对象的地址
                        parmCallRelease(currentObjectAddress,
                             call->actuals, virtualTableAddress,funcOffsetInVTB, call->ea3.reg);
                    }
                }
            }
            else
            {
                if(currentFST->isStatic)
                {
                    //调用的是一个静态函数，并且是当前类或者他的父类的一个静态函数
                    //首先得到静态函数的名字，这个可以在编译期间确定
                    bool success=false;
                    string staticFuncFullName = getStaticFuncFullName(call->idName,success);
                    if(!success)
                    SEE;
                    parmCallRelease(call->actuals, staticFuncFullName, call->ea3.reg);
                }
                else
                {
                    //调用的是当前类或它的父类的一个函数,这个函数可能是静态函数，也可能不是静态函数
                    bool success=false;
                    string staticFuncFullName= getStaticFuncFullName(call->idName,success);
                    if(success)
                        //调用一个静态函数
                        parmCallRelease(call->actuals, staticFuncFullName, call->ea3.reg);
                    else
                    {
                        //调用一个非静态函数
                        Temp currentObjectAddress=Temp::genNewTemp();
                        bool success=false;
                        (*currentF)<<Tac::genLoadFromStack(currentObjectAddress,getVarOffsetInFormal("this",success));
                        //现在已经获取了这个对象在内存中的地址，那么直接访问这个地址，就可以获取到该对象的虚函数表的地址
                        Temp virtualTableAddress=Temp::genNewTemp();
                        (*currentF)<<Tac::genLoad(virtualTableAddress,currentObjectAddress,Temp::getIntConstTemp(0));
                        //现在虚函数表的地址也有了，那么就要在虚函数表中找到这个函数的偏移，这个函数的偏移应该是一个常量，可以在编译期间确定下来
                        Temp funcOffsetInVTB = getFuncOffsetInClass(currentCST->name,call->idName);
                        parmCallRelease(currentObjectAddress,
                             call->actuals, virtualTableAddress,funcOffsetInVTB, call->ea3.reg);
                    }
                }
            }
        }
        virtual void visitForStmt(const ForStmt * forStmt)
        {
            forStmt->simpleStmt1->accept(this);
            shared_ptr<Label> loop=Label::genLabel();
            shared_ptr<Label> stop=Label::genLabel();
            forStmt->boolExpr->accept(this);
            Temp boolValue=getValue(forStmt->boolExpr);
            (*currentF)<<Tac::genJmpNotEqual(boolValue,Temp::getIntConstTemp(1),stop);
            (*currentF)<<Tac::genMark(loop);
            loopStopStack.push(stop);
            forStmt->stmt->accept(this);
            loopStopStack.pop();
            forStmt->simpleStmt2->accept(this);
            forStmt->boolExpr->accept(this);
            getValue(boolValue,forStmt->boolExpr);
            (*currentF)<<Tac::genJmpNotEqual(boolValue,Temp::getIntConstTemp(0),loop);
            (*currentF)<<Tac::genMark(stop);
        }
        virtual void visitWhileStmt(const WhileStmt * whileStmt)
        {
            shared_ptr<Label> loop=Label::genLabel();
            shared_ptr<Label> stop=Label::genLabel();
            whileStmt->boolExpr->accept(this);
            Temp boolValue=getValue(whileStmt->boolExpr);
            (*currentF)<<Tac::genJmpNotEqual(boolValue,Temp::getIntConstTemp(1) ,stop);
            (*currentF)<<Tac::genMark(loop);
            loopStopStack.push(stop);
            whileStmt->stmt->accept(this);
            loopStopStack.pop();
            whileStmt->boolExpr->accept(this);
            getValue(boolValue,whileStmt->boolExpr);
            (*currentF)<<Tac::genJmpNotEqual(boolValue,Temp::getIntConstTemp(0),loop);
            (*currentF)<<Tac::genMark(stop);
        }
        virtual void visitIfStmt(const IfStmt * ifStmt)
        {
            ifStmt->boolExpr->accept(this);
            Temp boolValue=getValue(ifStmt->boolExpr);
            shared_ptr<Label> elseLabel=Label::genLabel();
            shared_ptr<Label> stop=Label::genLabel();
            if(ifStmt->haveElse)
                (*currentF)<<Tac::genJmpNotEqual(boolValue,Temp::getIntConstTemp(1),elseLabel);
            else
                (*currentF)<<Tac::genJmpNotEqual(boolValue,Temp::getIntConstTemp(1),stop);
            ifStmt->stmt1->accept(this);
            (*currentF)<<Tac::genJmp(stop);
            if(ifStmt->haveElse)
            {
                (*currentF)<<Tac::genMark(elseLabel);
                ifStmt->stmt2->accept(this);
            }
            (*currentF)<<Tac::genMark(stop);
        }
        virtual void visitReturnStmt(const ReturnStmt * returnStmt)
        {
            //计算要释放的局部变量的个数
            int i=0;
            for(auto lst:lstStack)
                    i+=lst->vvst.size();
            i*=4;
            if(returnStmt->haveExpr)
            {
                returnStmt->expr->accept(this);
                Temp value=getValue(returnStmt->expr);
                if(i!=0)
                (*currentF)<<Tac::genReleaseStack(Temp::getIntConstTemp(i));
                (*currentF)<<Tac::genReturn(value);
            }
            else
            {
                if(i!=0)
                (*currentF)<<Tac::genReleaseStack(Temp::getIntConstTemp(i));
                (*currentF)<<Tac::genReturn();
            }
        }
        virtual void visitBreakStmt(const BreakStmt * breakStmt)
        {
            if(loopStopStack.empty())
            SEE;//讲道理，这里是不应该为空的，如果为空的话，只能说明前面哪里的代码有问题，这是编译期间的问题
            (*currentF)<<Tac::genJmp(loopStopStack.top());
        }
        virtual void visitPrintStmt(const PrintStmt * printStmt)
        {
            const shared_ptr<vector<shared_ptr<BaseNode> > > &exprs=printStmt->exprs;
            Temp value=Temp::genNewTemp();
            Temp pv=Temp::genNewTemp();
            for(int i=0;i<exprs->size();++i)
            {
                (*exprs)[i]->accept(this);
                getValue(value,(*exprs)[i]);
                if((*exprs)[i]->ea.et->isStringType())
                {
                    (*currentF)<<Tac::genPush(value)
                    <<Tac::genLoadStr(pv,"%s")
                    <<Tac::genPush(pv)
                    <<Tac::genCall(PRINTF)
                    <<Tac::genReleaseStack(Temp::getIntConstTemp(8));
                }
                else if((*exprs)[i]->ea.et->isBoolType())
                {
                    shared_ptr<Label> trueLabel = Label::genLabel();
                    shared_ptr<Label> endLabel = Label::genLabel();
                    (*currentF)<<Tac::genJmpNotEqual(value,Temp::getIntConstTemp(0),trueLabel)
                    <<Tac::genLoadStr(pv,"false")
                    <<Tac::genJmp(endLabel)
                    <<Tac::genMark(trueLabel)
                    <<Tac::genLoadStr(pv,"true")
                    <<Tac::genMark(endLabel)
                    <<Tac::genPush(pv)
                    <<Tac::genCall(PRINTF)
                    <<Tac::genReleaseStack(Temp::getIntConstTemp(4));
                }
                else
                {
                    (*currentF)<<Tac::genPush(value)
                    <<Tac::genLoadStr(pv,"%d")
                    <<Tac::genPush(pv)
                    <<Tac::genCall(PRINTF)
                    <<Tac::genReleaseStack(Temp::getIntConstTemp(8));
                }
            }
        }
        virtual void visitIntConstant(const IntConstant * ic)
        {
            const_cast<IntConstant *>(ic)->ea3.vi=VALUE_IN_REGISTER;
            const_cast<IntConstant *>(ic)->ea3.reg=Temp::getIntConstTemp(ic->value);
        }
        virtual void visitBoolConstant(const BoolConstant * bc)
        {
            const_cast<BoolConstant *>(bc)->ea3.vi=VALUE_IN_REGISTER;
            const_cast<BoolConstant *>(bc)->ea3.reg=Temp::getIntConstTemp(bc->value?1:0);
        }
        virtual void visitStringConstant(const StringConstant * sc)
        {
            const_cast<StringConstant *>(sc)->ea3.vi=VALUE_IN_REGISTER;
            const_cast<StringConstant *>(sc)->ea3.reg=Temp::genNewTemp();
            (*currentF)<<Tac::genLoadStr(sc->ea3.reg,sc->value);
        }
        virtual void visitNullConstant(const NullConstant * nc)
        {
            const_cast<NullConstant *>(nc)->ea3.vi=VALUE_IN_REGISTER;
            const_cast<NullConstant *>(nc)->ea3.reg=Temp::getIntConstTemp(0);
        }
        virtual void visitThisExpr(const ThisExpr * te)
        {
            const_cast<ThisExpr *>(te)->ea3.vi=VALUE_IN_STACK;
            bool success;//useless
            const_cast<ThisExpr *>(te)->ea3.offset=getVarOffsetInFormal("this",success);
        }
        virtual void visitOneExpr(const OneExpr * oe)
        {
            oe->expr->accept(this);
            const_cast<OneExpr *>(oe)->ea3.vi=VALUE_IN_REGISTER;
            const_cast<OneExpr *>(oe)->ea3.reg=getValue(oe->expr);
            switch(oe->kind)
            {
                case ONE_EXPR_BRACK:
                break;
                case ONE_EXPR_UMINUS:
                if(oe->ea3.reg.isconst())
                const_cast<OneExpr *>(oe)->ea3.reg=Temp::getIntConstTemp((0-oe->ea3.reg.getValue()));
                else
                (*currentF)<<Tac::genNeg(oe->ea3.reg,oe->ea3.reg);
                break;
                case ONE_EXPR_NOT:
                if(oe->ea3.reg.isconst())
                const_cast<OneExpr *>(oe)->ea3.reg=Temp::getIntConstTemp(oe->ea3.reg.getValue()==1?0:1);
                else
                (*currentF)<<Tac::genNot(oe->ea3.reg,oe->ea3.reg);
                break;
                default:
                SEE;
            }
        }
        virtual void visitTwoExpr(const TwoExpr * te)
        {
            te->leftExpr->accept(this);
            te->rightExpr->accept(this);
            const_cast<TwoExpr *>(te)->ea3.vi=VALUE_IN_REGISTER;
            const_cast<TwoExpr *>(te)->ea3.reg=Temp::genNewTemp();
            switch(te->kind)
            {
                case TWO_EXPR_ADD:
                if(te->leftExpr->ea3.vi==VALUE_IN_REGISTER &&
                    te->leftExpr->ea3.reg.isconst() &&
                    te->rightExpr->ea3.vi==VALUE_IN_REGISTER &&
                    te->rightExpr->ea3.reg.isconst())
                    const_cast<TwoExpr *>(te)->ea3.reg=Temp::getIntConstTemp(te->leftExpr->ea3.reg.getValue()+te->rightExpr->ea3.reg.getValue());
                else
                (*currentF)<<Tac::genAdd(te->ea3.reg,getValue(te->leftExpr),getValue(te->rightExpr));
                break;
                case TWO_EXPR_SUB:
                if(te->leftExpr->ea3.vi==VALUE_IN_REGISTER &&
                    te->leftExpr->ea3.reg.isconst() &&
                    te->rightExpr->ea3.vi==VALUE_IN_REGISTER &&
                    te->rightExpr->ea3.reg.isconst())
                    const_cast<TwoExpr *>(te)->ea3.reg=Temp::getIntConstTemp(te->leftExpr->ea3.reg.getValue()-te->rightExpr->ea3.reg.getValue());
                else
                (*currentF)<<Tac::genSub(te->ea3.reg,getValue(te->leftExpr),getValue(te->rightExpr));
                break;
                case TWO_EXPR_MUL:
                if(te->leftExpr->ea3.vi==VALUE_IN_REGISTER &&
                    te->leftExpr->ea3.reg.isconst() &&
                    te->rightExpr->ea3.vi==VALUE_IN_REGISTER &&
                    te->rightExpr->ea3.reg.isconst())
                    const_cast<TwoExpr *>(te)->ea3.reg=Temp::getIntConstTemp((te->leftExpr->ea3.reg.getValue())*(te->rightExpr->ea3.reg.getValue()));
                else
                (*currentF)<<Tac::genMul(te->ea3.reg,getValue(te->leftExpr),getValue(te->rightExpr));
                break;
                case TWO_EXPR_DIV:
                if(te->leftExpr->ea3.vi==VALUE_IN_REGISTER &&
                    te->leftExpr->ea3.reg.isconst() &&
                    te->rightExpr->ea3.vi==VALUE_IN_REGISTER &&
                    te->rightExpr->ea3.reg.isconst())
                    const_cast<TwoExpr *>(te)->ea3.reg=Temp::getIntConstTemp((te->leftExpr->ea3.reg.getValue())/(te->rightExpr->ea3.reg.getValue()));
                else
                (*currentF)<<Tac::genDiv(te->ea3.reg,getValue(te->leftExpr),getValue(te->rightExpr));
                break;
                case TWO_EXPR_MOD:
                if(te->leftExpr->ea3.vi==VALUE_IN_REGISTER &&
                    te->leftExpr->ea3.reg.isconst() &&
                    te->rightExpr->ea3.vi==VALUE_IN_REGISTER &&
                    te->rightExpr->ea3.reg.isconst())
                    const_cast<TwoExpr *>(te)->ea3.reg=Temp::getIntConstTemp((te->leftExpr->ea3.reg.getValue())%(te->rightExpr->ea3.reg.getValue()));
                else
                (*currentF)<<Tac::genMod(te->ea3.reg,getValue(te->leftExpr),getValue(te->rightExpr));
                break;
                case TWO_EXPR_SM:
                if(te->leftExpr->ea3.vi==VALUE_IN_REGISTER &&
                    te->leftExpr->ea3.reg.isconst() &&
                    te->rightExpr->ea3.vi==VALUE_IN_REGISTER &&
                    te->rightExpr->ea3.reg.isconst())
                    const_cast<TwoExpr *>(te)->ea3.reg=Temp::getIntConstTemp(((te->leftExpr->ea3.reg.getValue())<(te->rightExpr->ea3.reg.getValue()))?1:0);
                else
                (*currentF)<<Tac::genLess(te->ea3.reg,getValue(te->leftExpr),getValue(te->rightExpr));
                break;
                case TWO_EXPR_SM_EQ:
                if(te->leftExpr->ea3.vi==VALUE_IN_REGISTER &&
                    te->leftExpr->ea3.reg.isconst() &&
                    te->rightExpr->ea3.vi==VALUE_IN_REGISTER &&
                    te->rightExpr->ea3.reg.isconst())
                    const_cast<TwoExpr *>(te)->ea3.reg=Temp::getIntConstTemp(((te->leftExpr->ea3.reg.getValue())<=(te->rightExpr->ea3.reg.getValue()))?1:0);
                else
                (*currentF)<<Tac::genLessEqual(te->ea3.reg,getValue(te->leftExpr),getValue(te->rightExpr));
                break;
                case TWO_EXPR_BG:
                if(te->leftExpr->ea3.vi==VALUE_IN_REGISTER &&
                    te->leftExpr->ea3.reg.isconst() &&
                    te->rightExpr->ea3.vi==VALUE_IN_REGISTER &&
                    te->rightExpr->ea3.reg.isconst())
                    const_cast<TwoExpr *>(te)->ea3.reg=Temp::getIntConstTemp(((te->leftExpr->ea3.reg.getValue())>(te->rightExpr->ea3.reg.getValue()))?1:0);
                else
                (*currentF)<<Tac::genGreat(te->ea3.reg,getValue(te->leftExpr),getValue(te->rightExpr));
                break;
                case TWO_EXPR_BG_EQ:
                if(te->leftExpr->ea3.vi==VALUE_IN_REGISTER &&
                    te->leftExpr->ea3.reg.isconst() &&
                    te->rightExpr->ea3.vi==VALUE_IN_REGISTER &&
                    te->rightExpr->ea3.reg.isconst())
                    const_cast<TwoExpr *>(te)->ea3.reg=Temp::getIntConstTemp(((te->leftExpr->ea3.reg.getValue())>=(te->rightExpr->ea3.reg.getValue()))?1:0);
                else
                (*currentF)<<Tac::genGreatEqual(te->ea3.reg,getValue(te->leftExpr),getValue(te->rightExpr));
                break;
                case TWO_EXPR_EQ_EQ:
                if(te->leftExpr->ea3.vi==VALUE_IN_REGISTER &&
                    te->leftExpr->ea3.reg.isconst() &&
                    te->rightExpr->ea3.vi==VALUE_IN_REGISTER &&
                    te->rightExpr->ea3.reg.isconst())
                    const_cast<TwoExpr *>(te)->ea3.reg=Temp::getIntConstTemp(((te->leftExpr->ea3.reg.getValue())==(te->rightExpr->ea3.reg.getValue()))?1:0);
                else
                (*currentF)<<Tac::genEqualEqual(te->ea3.reg,getValue(te->leftExpr),getValue(te->rightExpr));
                break;
                case TWO_EXPR_NOT_EQ:
                if(te->leftExpr->ea3.vi==VALUE_IN_REGISTER &&
                    te->leftExpr->ea3.reg.isconst() &&
                    te->rightExpr->ea3.vi==VALUE_IN_REGISTER &&
                    te->rightExpr->ea3.reg.isconst())
                    const_cast<TwoExpr *>(te)->ea3.reg=Temp::getIntConstTemp(((te->leftExpr->ea3.reg.getValue())!=(te->rightExpr->ea3.reg.getValue()))?1:0);
                else
                (*currentF)<<Tac::genNotEqual(te->ea3.reg,getValue(te->leftExpr),getValue(te->rightExpr));
                break;
                case TWO_EXPR_AND_AND:
                if(te->leftExpr->ea3.vi==VALUE_IN_REGISTER &&
                    te->leftExpr->ea3.reg.isconst() &&
                    te->rightExpr->ea3.vi==VALUE_IN_REGISTER &&
                    te->rightExpr->ea3.reg.isconst())
                    const_cast<TwoExpr *>(te)->ea3.reg=Temp::getIntConstTemp(((te->leftExpr->ea3.reg.getValue())&&(te->rightExpr->ea3.reg.getValue()))?1:0);
                else
                (*currentF)<<Tac::genAndAnd(te->ea3.reg,getValue(te->leftExpr),getValue(te->rightExpr));
                break;
                case TWO_EXPR_OR_OR:
                if(te->leftExpr->ea3.vi==VALUE_IN_REGISTER &&
                    te->leftExpr->ea3.reg.isconst() &&
                    te->rightExpr->ea3.vi==VALUE_IN_REGISTER &&
                    te->rightExpr->ea3.reg.isconst())
                    const_cast<TwoExpr *>(te)->ea3.reg=Temp::getIntConstTemp(((te->leftExpr->ea3.reg.getValue())||(te->rightExpr->ea3.reg.getValue()))?1:0);
                else
                (*currentF)<<Tac::genOrOr(te->ea3.reg,getValue(te->leftExpr),getValue(te->rightExpr));
                break;
                default:
                SEE;
            }
        }
        virtual void visitReadInteger(const ReadInteger * ri)
        {
            //scanf("%d",address)
            const_cast<ReadInteger *>(ri)->ea3.vi=VALUE_IN_REGISTER;
            const_cast<ReadInteger *>(ri)->ea3.reg=Temp::genNewTemp();
            Temp address=Temp::genNewTemp();
            Temp t=Temp::genNewTemp();
            (*currentF)
            <<Tac::genPush(Temp::getIntConstTemp(4))
            <<Tac::genCall(address,MALLOC)
            <<Tac::genPush(address)
            <<Tac::genLoadStr(t,"%d")
            <<Tac::genPush(t)
            <<Tac::genCall(SCANF)
            <<Tac::genReleaseStack(Temp::getIntConstTemp(12))
            <<Tac::genLoad(ri->ea3.reg,address,Temp::getIntConstTemp(0));
        }
        virtual void visitReadLine(const ReadLine * rl)
        {
            //scanf("%s",s);
            const_cast<ReadLine *>(rl)->ea3.vi=VALUE_IN_REGISTER;
            const_cast<ReadLine *>(rl)->ea3.reg=Temp::genNewTemp();
            Temp t=Temp::genNewTemp();
            (*currentF)
            <<Tac::genPush(Temp::getIntConstTemp(1024))
            <<Tac::genCall(rl->ea3.reg,MALLOC)
            <<Tac::genPush(rl->ea3.reg)
            <<Tac::genLoadStr(t,"%s")
            <<Tac::genPush(t)
            <<Tac::genCall(SCANF)
            <<Tac::genReleaseStack(Temp::getIntConstTemp(12));
        }
        virtual void visitNewObject(const NewObject * no)
        {
            const_cast<NewObject *>(no)->ea3.vi=VALUE_IN_REGISTER;
            const_cast<NewObject *>(no)->ea3.reg=Temp::genNewTemp();
            (*currentF)<<Tac::genCall(no->ea3.reg,Function::genUniqueFuncId(no->idName,"New"));
        }
        virtual void visitNewArray(const NewArray * na)
        {
            const_cast<NewArray *>(na)->ea3.vi=VALUE_IN_REGISTER;
            const_cast<NewArray *>(na)->ea3.reg=Temp::genNewTemp();
            na->expr->accept(this);
            Temp size=getValue(na->expr);//获取要分配的大小
            //下面是检查 size 是否是负数的代码
            shared_ptr<Label> l=Label::genLabel();
            (*currentF)<<Tac::genJmpLess(Temp::getIntConstTemp(0),size,l);//如果0<size 跳转到正常流程
            Temp t=Temp::genNewTemp();
            //printf("%d行不能用负数初始化数组\n",na->loc.row);
            (*currentF)
            <<Tac::genPush(Temp::getIntConstTemp(na->loc.row))
            <<Tac::genLoadStr(t,"第%d行不能用负数初始化数组\\n")
            <<Tac::genPush(t)
            <<Tac::genCall(PRINTF)
            <<Tac::genReleaseStack(Temp::getIntConstTemp(8))
            <<Tac::genPush(Temp::getIntConstTemp(1))
            <<Tac::genCall(EXIT)//退出
            <<Tac::genReleaseStack(Temp::getIntConstTemp(4))
            <<Tac::genMark(l);
            //调用 malloc 函数
            Temp address=Temp::genNewTemp();
            Temp rsize=Temp::genNewTemp();
            (*currentF)<<Tac::genAdd(rsize,size,Temp::getIntConstTemp(1))//多一个空间用于存放数组长度
            <<Tac::genMul(rsize,rsize,Temp::getIntConstTemp(4))// 每个单元都是4个字节
            <<Tac::genPush(rsize)
            <<Tac::genCall(address,MALLOC)
            <<Tac::genReleaseStack(Temp::getIntConstTemp(4))
            //将address 的后面一个单元的内容设置为数组长度
            <<Tac::genDiv(rsize,rsize,Temp::getIntConstTemp(4))
            <<Tac::genSub(rsize,rsize,Temp::getIntConstTemp(1))
            <<Tac::genStore(address,Temp::getIntConstTemp(0),rsize)
            <<Tac::genAdd(na->ea3.reg,address,Temp::getIntConstTemp(4));
        }
        virtual void visitClassTest(const ClassTest * ct)
        {
            const_cast<ClassTest *>(ct)->ea3.vi=VALUE_IN_REGISTER;
            const_cast<ClassTest *>(ct)->ea3.reg=Temp::genNewTemp();
            Temp distClassVirtualTBLAddress=Temp::genNewTemp();
            (*currentF)<<Tac::genLoadVTBL(distClassVirtualTBLAddress,vtMap.find(ct->idName)->second);
            ct->expr->accept(this);
            Temp theObjectAddress=getValue(ct->expr);
            Temp testClassVirtualTBLAddress=Temp::genNewTemp();
            (*currentF)<<Tac::genLoad(testClassVirtualTBLAddress,theObjectAddress,Temp::getIntConstTemp(0));
            (*currentF)<<Tac::genLessEqual(ct->ea3.reg,distClassVirtualTBLAddress,testClassVirtualTBLAddress);//父类的虚函数表地址在子类的前面
        }
        virtual void visitClassCast(const ClassCast * cc)
        {
            const_cast<ClassCast *>(cc)->ea3.vi=VALUE_IN_REGISTER;
            const_cast<ClassCast *>(cc)->ea3.reg=Temp::genNewTemp();
            Temp distClassVirtualTBLAddress=Temp::genNewTemp();
            (*currentF)<<Tac::genLoadVTBL(distClassVirtualTBLAddress,vtMap.find(cc->idName)->second);
            cc->expr->accept(this);
            Temp theObjectAddress=getValue(cc->expr);
            Temp theClassVirtualTBLAddress=Temp::genNewTemp();
            (*currentF)<<Tac::genLoad(theClassVirtualTBLAddress,theObjectAddress,Temp::getIntConstTemp(0));
            shared_ptr<Label> l=Label::genLabel();
            (*currentF)<<Tac::genJmpLessEqual(distClassVirtualTBLAddress,theClassVirtualTBLAddress,l);
            //转换错误代码
            Temp t=Temp::genNewTemp();
            //printf("第%d行类型转换异常\n",cc->loc.row)
            (*currentF)
            <<Tac::genPush(Temp::getIntConstTemp(cc->loc.row))//先输出错误所在行数
            <<Tac::genLoadStr(t,"第%d行类型转换异常\\n")
            <<Tac::genPush(t)
            <<Tac::genCall(PRINTF)
            <<Tac::genReleaseStack(Temp::getIntConstTemp(8))
            <<Tac::genPush(Temp::getIntConstTemp(1))
            <<Tac::genCall(EXIT)//退出
            <<Tac::genReleaseStack(Temp::getIntConstTemp(4))
            <<Tac::genMark(l);
            getValue(const_cast<ClassCast *>(cc)->ea3.reg,cc->expr);//直接将地址拷贝过来
        }

    private:
        void createVTBL(shared_ptr<ClassSymbolTable> &cst)
        {
            if(vtMap.find(cst->name)!=vtMap.end())
                return;//说明这个表已经被创建过了
            shared_ptr<VirtualTable> vt(new VirtualTable(cst->name));
            if(cst->haveParent && vtMap.find(cst->parentName)==vtMap.end())
                createVTBL(cst->parentClass);//创建父类的虚函数表
            if(cst->haveParent)
            {
                //设置父类的虚函数表
                auto parentVT = vtMap.find(cst->parentName)->second;
                vt->setParentVT(parentVT);
            }
            vtMap.insert(pair<const string,shared_ptr<VirtualTable> >(cst->name,vt));
            vts->push_back(vt);
        }
        void createNewObjectFunction(shared_ptr<ClassSymbolTable> &cst)
        {
            //每一个 New 函数都相当于一个静态函数
            shared_ptr<Function> newFunc(new Function(cst->name,"New"));
            int classSize=calcClassSize(cst);//类的大小，每一个成员变量都占4个字节，整数，bool 类型，指针，都是4个字节
            classSize+=4;//还要加上4个字节，用于存放虚函数表的地址
            Temp address=Temp::genNewTemp();
            (*newFunc)
            <<Tac::genPush(Temp::getIntConstTemp(classSize))
            <<Tac::genCall(address,MALLOC)
            <<Tac::genReleaseStack(Temp::getIntConstTemp(4));//传入一个参数，在函数返回后，就要释放一个参数的空间
            Temp t=Temp::genNewTemp();
            (*newFunc)<<Tac::genLoadVTBL(t,vtMap.find(cst->name)->second)
            <<Tac::genStore(address,Temp::getIntConstTemp(0),t)//将虚函数表地址保存到由 MALLOC 申请的空间中去
            <<Tac::genReturn(address);
            staticFuncMap.insert(pair<const string,decltype(newFunc)>(newFunc->getUniqueFuncId(),newFunc));
            staticFuncs->push_back(newFunc);
        }
        int calcClassSize(shared_ptr<ClassSymbolTable> &cst)
        {
            if(cst.get()==nullptr)
                return 0;
            int size = cst->vvst.size()*4;
            if(cst->haveParent)
                return size+calcClassSize(cst->parentClass);
            else
                return size;
        }
        const Temp &getVarOffsetInClass(const string &className,const string &varName)
        {
            //获取成员变量在类中的偏移
            shared_ptr<ClassSymbolTable> cst=gst->getCSTByName(className);
            for(int i=0;i<cst->vvst.size();++i)
                if(cst->vvst[i]->name==varName)
                    return Temp::getIntConstTemp(4+calcClassSize(cst->parentClass)+i*4);//第一个4的意思是虚函数表占用的4个字节
            return getVarOffsetInClass(cst->parentClass->name, varName);
        }
        const Temp &getVarOffsetInLocal(const string &varName,bool &success)
        {
            int max=-1,count=0;
            for(auto pos=lstStack.begin();pos!=lstStack.end();++pos)
            {
                vector<shared_ptr<VariableSymbolTable> > &vars=(*pos)->vvst;
                for(auto var:vars)
                {
                    if(var->name==varName)
                        max=count;
                    ++count;
                }
            }
            if(max!=-1)
            {
                success=true;
                return Temp::getIntConstTemp((max+1)*(-4));//第一个变量相对于 ebp 的偏移是-4
            }
            else
            {
                success=false;
                return Temp::getIntConstTemp(0);
            }
        }
        const Temp &getVarOffsetInFormal(const string &varName,bool &success)
        {
            if(varName=="this")
            {
                if(currentFST->isStatic)
                    SEE;
                success=true;
                return Temp::getIntConstTemp(8);//this 的地址总是 ebp+8,要注意，参数入栈的时候，是从右向左的，最后压入 this 地址
            }
            else
            {
                for(int i=0;i<currentFST->vvst.size();++i)
                    if(currentFST->vvst[i]->name==varName)
                    {
                        success=true;
                        if(currentFST->isStatic)
                        return Temp::getIntConstTemp(8+i*4);
                        return Temp::getIntConstTemp(12+i*4);
                        //上面两个表达式相差4是因为非静态函数有一个隐含参数 this
                    }
                success=false;
                return Temp::getIntConstTemp(0);
            }
        }
        const Temp getSaveArrayLengthUnitOffset()
        {
            //这里之所以返回-4是因为保存数组长度的那个单元在这个数组首地址的前面一个单元，因此必须使用数组的首地址减去4才能够获得这个单元的内容
            return Temp::getIntConstTemp(-4);
        }
        const Temp getFuncOffsetInClass(const string &className,const string &funcName)
        {
            /**
            获取函数在虚函数表中的偏移
            1.如果没有父类，那么久按照先后顺序找即可
            2.如果有父类
                2.1 如果没有 override，那么还是按照顺序找
                2.2 如果有 override，那么子类重复出现的函数名就不再计算在内了
            **/
            shared_ptr<ClassSymbolTable> cst=gst->getCSTByName(className);
            stack<shared_ptr<ClassSymbolTable> > cstList;
            vector<string> funcs;
            cstList.push(cst);
            while(cst->haveParent)
            {
                cst=cst->parentClass;
                cstList.push(cst);
            }
            while(!cstList.empty())
            {
                for(auto func:cstList.top()->vfst)
                {
                    if(func->isStatic)
                        continue;
                    else
                    {
                        bool find=false;
                        for(auto name:funcs)
                            if(name==func->name)
                                find=true;
                        if(!find)
                            funcs.push_back(func->name);
                    }
                }
                cstList.pop();
            }
            for(int i=0;i<funcs.size();++i)
                if(funcs[i]==funcName)
                    return Temp::getIntConstTemp(i*4);
            SEE;
        }
        Temp getValue(const shared_ptr<BaseNode> &expr)
        {
            Temp t=Temp::genNewTemp();
            switch(expr->ea3.vi)
            {
                case VALUE_IN_HEAP:
                (*currentF)<<Tac::genLoad(t,expr->ea3.pointer,expr->ea3.offset);
                break;
                case VALUE_IN_STACK:
                (*currentF)<<Tac::genLoadFromStack(t,expr->ea3.offset);
                break;
                case VALUE_IN_REGISTER:
                return expr->ea3.reg;
                break;
                default:
                SEE;
            }
            return t;
        }
        void getValue(Temp &t,const shared_ptr<BaseNode> &expr)
        {
            switch(expr->ea3.vi)
            {
                case VALUE_IN_HEAP:
                (*currentF)<<Tac::genLoad(t,expr->ea3.pointer,expr->ea3.offset);
                break;
                case VALUE_IN_STACK:
                (*currentF)<<Tac::genLoadFromStack(t,expr->ea3.offset);
                break;
                case VALUE_IN_REGISTER:
                t=expr->ea3.reg;
                break;
                default:
                SEE;
            }
        }
        string getStaticFuncFullName(const string &funcName,bool &success)
        {
            shared_ptr<ClassSymbolTable> cst=currentCST;
            while(true)
            {
                for(auto func:cst->vfst)
                    if(func->isStatic && func->name==funcName)
                    {
                        success=true;
                        return Function::genUniqueFuncId(cst->name,funcName);
                    }
                if(cst->haveParent)
                    cst=cst->parentClass;
                else
                    break;
            }
            success=false;
            return "";
        }
    };

    class Print
    {
    private:
        static void printVT(const shared_ptr<VirtualTable> &vt)
        {
            cout<<"VirtualTable:"<<vt->className<<endl;
            cout<<"{"<<endl;
            cout<<"parentVT:"<<((vt->parentVT.get()==nullptr)?"null":(vt->parentVT->className))<<endl;
            for(auto func:vt->funcs)
            cout<<"Func:"<<func->getUniqueFuncId()<<endl;
            cout<<"}"<<endl<<endl;
            for(auto func:vt->funcs)
            printFunc(func);
        }
        static void printFunc(const shared_ptr<Function> &func)
        {
            cout<<"Function:"<<func->getUniqueFuncId()<<endl;
            for(auto tac:func->tacList)
                tac->print();
            cout<<"End"<<endl<<endl;
        }
    public:
        static void print(const shared_ptr<Program> &program)
        {
            for(auto vt:*(program->vts))
            printVT(vt);
            for(auto func:*(program->staticFuncs))
            printFunc(func);
//            cout<<"MAX:"<<Temp::getMaxCount()<<endl;
        }
    };

};



#endif /* end of include guard: _COMPILE_LAB3_H_ */
