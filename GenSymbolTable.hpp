#ifndef _GEN_SYMBOL_TABLE_H_
#define _GEN_SYMBOL_TABLE_H_
#include "std.hpp"
#include "Visitor.hpp"
#include "SymbolTable.hpp"
#include "Error.hpp"

#ifdef _YLQ_DEBUG_
int depth;
#endif

class GenSymbolTable : public Visitor
{
    shared_ptr<GlobalSymbolTable> gst;
    stack<shared_ptr<ClassSymbolTable> > cstStack;
    stack<shared_ptr<VariableSymbolTable> > vstStack;
    stack<shared_ptr<SymbolType> > stStack;
    stack<shared_ptr<FunctionSymbolTable> > fstStack;
    stack<shared_ptr<LocalSymbolTable> > lstStack;
    ErrorContainer ec;
public:

    GenSymbolTable():gst(new GlobalSymbolTable()){}

    static void genSymbolTable(shared_ptr<Program> &program)
    {
        #ifdef _YLQ_DEBUG_
        Trace t(__LINE__,__FILE__);
        #endif
        GenSymbolTable gst;
        program->accept(&gst);
        gst.check();
    }
    virtual void visitProgram(const Program *program)
    {
        #ifdef _YLQ_DEBUG_
        Trace t(__LINE__,__FILE__);
        #endif
        for(auto pos=program->clist->begin();pos!=program->clist->end();++pos)
        {
            (*pos)->accept(this);
            cstStack.top()->SymbolTable::setParent(gst);
            gst->add(cstStack.top());
            cstStack.pop();
        }
        const_cast<Program *>(program)->setGST(gst);
    }

    virtual void visitBuildInType(const BuildInType *bit)
	{
        #ifdef _YLQ_DEBUG_
        Trace t(__LINE__,__FILE__);
        #endif
        switch(bit->kind)
        {
            case TYPE_INT:
            stStack.push(shared_ptr<BuildInSymbolType>(new BuildInSymbolType(BIST_INT)));
            return;
            case TYPE_STRING:
            stStack.push(shared_ptr<BuildInSymbolType>(new BuildInSymbolType(BIST_STRING)));
            return;
            case TYPE_VOID:
            stStack.push(shared_ptr<BuildInSymbolType>(new BuildInSymbolType(BIST_VOID)));
            return;
            case TYPE_BOOL:
            stStack.push(shared_ptr<BuildInSymbolType>(new BuildInSymbolType(BIST_BOOL)));
            return;
        }
        SeriousErrorHandler::seriousError("GenSymbolTable.h->visitBuildInType()");
	}
    virtual void visitClassType(const ClassType *ct)
	{
        #ifdef _YLQ_DEBUG_
        Trace t(__LINE__,__FILE__);
        #endif
        shared_ptr<ClassSymbolType> cst(new ClassSymbolType(ct->idName));
        stStack.push(cst);
	}
    virtual void visitArrayType(const ArrayType *at)
	{
        #ifdef _YLQ_DEBUG_
        Trace t(__LINE__,__FILE__);
        #endif
        shared_ptr<ArraySymbolType> ast(new ArraySymbolType());
        at->type->accept(this);
        ast->type.swap(stStack.top());
        stStack.pop();
        stStack.push(ast);
	}

    virtual void visitVariable(const Variable *variable)
    {
        #ifdef _YLQ_DEBUG_
        Trace t(__LINE__,__FILE__);
        #endif
        shared_ptr<VariableSymbolTable> vst(new VariableSymbolTable(variable->idName,variable->BaseNode::loc));
        variable->type->accept(this);
        vst->setType(stStack.top());
        stStack.pop();
        vstStack.push(vst);
        const_cast<Variable *>(variable)->setVST(vst);
    }

    virtual void visitFunctionDef(const FunctionDef *functionDef)
	{
        #ifdef _YLQ_DEBUG_
        Trace t(__LINE__,__FILE__);
        #endif
        shared_ptr<FunctionSymbolTable> fst(new FunctionSymbolTable(functionDef->isStatic,functionDef->idName,functionDef->idName==string("main"),functionDef->BaseNode::loc));
        functionDef->type->accept(this);
        fst->setReturnType(stStack.top());
        stStack.pop();
        for(auto pos=functionDef->formals->begin();pos!=functionDef->formals->end();++pos)
        {
            (*pos)->accept(this);
            vstStack.top()->SymbolTable::setParent(fst);
            fst->addFormal(vstStack.top());
            vstStack.pop();
        }

        functionDef->stmtBlock->accept(this);//收集函数体中的符号
        lstStack.top()->SymbolTable::setParent(fst);
        fst->setLocalSymbolTable(lstStack.top());
        lstStack.pop();
        fstStack.push(fst);
        const_cast<FunctionDef *>(functionDef)->setFST(fst);
	}

    virtual void visitClassDef(const ClassDef *classDef)
	{
        #ifdef _YLQ_DEBUG_
        Trace t(__LINE__,__FILE__);
        #endif
        shared_ptr<ClassSymbolTable> cst(new ClassSymbolTable(classDef->idName,classDef->haveParent,classDef->idName=="Main",classDef->parentName,classDef->BaseNode::loc));

        for(auto pos=classDef->fields->begin();pos!=classDef->fields->end();++pos)
        {
            (*pos)->accept(this);
            if(!vstStack.empty())
            {
                vstStack.top()->SymbolTable::setParent(cst);
                cst->addVST(vstStack.top());
                vstStack.pop();
            }
            if(!fstStack.empty())
            {
                fstStack.top()->SymbolTable::setParent(cst);
                cst->addFST(fstStack.top());
                fstStack.pop();
            }
        }
        cstStack.push(cst);//将这个类符号表压入栈中
        const_cast<ClassDef *>(classDef)->setCST(cst);
    }

    virtual void visitStmtBlock(const StmtBlock *stmtBlock)
    {
        #ifdef _YLQ_DEBUG_
        Trace t(__LINE__,__FILE__);
        #endif
        shared_ptr<LocalSymbolTable> lst(new LocalSymbolTable());
        for(auto pos=stmtBlock->stmts->begin();pos!=stmtBlock->stmts->end();++pos)
        {
            (*pos)->accept(this);
            if(!vstStack.empty())
            {
                vstStack.top()->SymbolTable::setParent(lst);
                lst->addVariable(vstStack.top());
                vstStack.pop();
            }
            if(!lstStack.empty())
            {
                lstStack.top()->SymbolTable::setParent(lst);
                lst->addLocalST(lstStack.top());
                lstStack.pop();
            }

        }

        lstStack.push(lst);
        const_cast<StmtBlock *>(stmtBlock)->setLST(lst);

    }
    virtual void visitForStmt(const ForStmt *forStmt)
    {
        #ifdef _YLQ_DEBUG_
        Trace t(__LINE__,__FILE__);
        #endif
        forStmt->stmt->accept(this);
    }
    virtual void visitWhileStmt(const WhileStmt *whileStmt)
    {
        #ifdef _YLQ_DEBUG_
        Trace t(__LINE__,__FILE__);
        #endif
        whileStmt->stmt->accept(this);
    }
    virtual void visitIfStmt(const IfStmt *ifStmt)
    {
        #ifdef _YLQ_DEBUG_
        Trace t(__LINE__,__FILE__);
        #endif
        ifStmt->stmt1->accept(this);
        if(ifStmt->haveElse)
        ifStmt->stmt2->accept(this);
    }






    void check()
    {
        classNameCheck();
        parentExitAndInheritCheck();
        fieldsCheck();
        functionCheck();
        localSTCheck();
        if(!ec.noError())
        ec.printErrorAndExit();
    }
private:
    void classNameCheck()
    {
        #ifdef _YLQ_DEBUG_
        Trace t(__LINE__,__FILE__);
        #endif
        //类名检查，是否有Main类和是否有名称冲突
        bool haveMainClass=false;
        const vector<shared_ptr<ClassSymbolTable> > &cst=gst->getCST();
        map<const string,decltype(cst.begin())> nameMap;
        for(auto pos=cst.begin();pos!=cst.end();++pos)
        {
            if((*pos)->isMain)//判断是否是类Main
            haveMainClass=true;

            auto before=nameMap.find((*pos)->name);
            if(before!=nameMap.end())
            {
                //存在重名冲突
                ec.addError(shared_ptr<ClassNameConflictError>(new ClassNameConflictError(
                    (*pos)->name,(*(before->second))->SymbolTable::loc,(*pos)->SymbolTable::loc)));
            }
            else
            nameMap.insert(pair<const string,decltype(cst.begin())>((*pos)->name,pos));
        }

        if(!haveMainClass)
        ec.addError(shared_ptr<NoMainClassError>(new NoMainClassError));
    }
    void parentExitAndInheritCheck()
    {
        #ifdef _YLQ_DEBUG_
        Trace t(__LINE__,__FILE__);
        #endif
        //继承关系检查，父类是否存在&父类是否继承了子类
        const vector<shared_ptr<ClassSymbolTable> > &cst=gst->getCST();
        map<const string,decltype(cst.begin())> nameMap;
        for(auto pos=cst.begin();pos!=cst.end();++pos)
            nameMap.insert(
                pair<const string,decltype(cst.begin())>((*pos)->name,pos)
            );
        for(auto pos=cst.begin();pos!=cst.end();++pos)
            if((*pos)->haveParent)
            {
                auto parentClass=nameMap.find((*pos)->parentName);
                if(parentClass==nameMap.end())
                {
                    ec.addError(shared_ptr<ParentClassNotFoundError>(
                        new ParentClassNotFoundError((*pos)->name,(*pos)->parentName,(*pos)->SymbolTable::loc))
                    );
                }
                else
                {
                    //在这里将他的父类设置好
                    (*pos)->parentClass=*(parentClass->second);
                }
            }
        if(!ec.noError())
            ec.printErrorAndExit();

        //只有在前面两步没有出错的情况下，才检查继承关系是否正常
        for(auto pos=cst.begin();pos!=cst.end();++pos)
        {
            auto p=pos;
            stringstream ss;
            ss<<(*pos)->name<<" [行号："<<(*pos)->SymbolTable::loc.row<<"]";
            while((*p)->haveParent)
            {
                p=nameMap.find((*p)->parentName)->second;
                ss<<"->"<<(*p)->name<<"[行号："<<(*p)->SymbolTable::loc.row<<"]";
                if((*p)->name==(*pos)->name)
                {
                    ec.addError(shared_ptr<InheritError>(new InheritError(
                        (*pos)->name,
                        ss.str()
                    )));
                    break;
                }
            }
        }
    }
    void fieldsCheck()
    {
        #ifdef _YLQ_DEBUG_
        Trace t(__LINE__,__FILE__);
        #endif
        /**
        对每个类中的成员进行检查
        1.成员变量的type不能是void
        2.函数形参类型不能是void,函数返回值类型如果有标识符，那么那个类必须要有
        3.每个类中的成员变量或者成员函数的名字不能一样，并且不能够和类名相同,每个函数的形参中的形参名不能够重复
        4.main方法必须有，并且main函数必须是static void main()
        **/
        const vector<shared_ptr<ClassSymbolTable> > &cst=gst->getCST();
        map<string const ,Location const > classNameMap;
        for(auto pos=cst.begin();pos!=cst.end();++pos)
            classNameMap.insert(pair<string const ,Location const >(
                (*pos)->name,(*pos)->SymbolTable::loc
            ));

        bool havemainFunc = false;
        for(auto pos=cst.begin();pos!=cst.end();++pos)
        {
            map<string const ,Location const > fieldMap;
            const vector<shared_ptr<VariableSymbolTable> > &vvst=(*pos)->vvst;
            const vector<shared_ptr<FunctionSymbolTable> > &vfst=(*pos)->vfst;
            for(auto vst=vvst.begin();vst!=vvst.end();++vst)
            {
                //先检查成员变量的类型是否是void
                if((*vst)->type->isVoidType())
                {
                    ec.addError(shared_ptr<VariableTypeIsVoidError>(new VariableTypeIsVoidError(
                       (*vst)->name,(*vst)->SymbolTable::loc
                    )));
                }
                classExistCheck((*vst)->type,(*vst)->loc);
                auto exist=fieldMap.find((*vst)->name);
                auto exist2=classNameMap.find((*vst)->name);//检查是否和类名冲突
                if(exist!=fieldMap.end() || exist2!=classNameMap.end())
                {
                    if(exist!=fieldMap.end())
                    {
                        //发现重名
                        //auto before=exist->second;这里发现一个auto的问题，如果使用auto的话，后面的错误输出会有问题，但是如果直接指名before的类型的话，就不会有这个问题
                        Location const before=exist->second;
                        ec.addError(shared_ptr<NameConflictError>(new NameConflictError(
                            (*vst)->name,before,
                            (*vst)->SymbolTable::loc
                        )));
                    }
                    if(exist2!=classNameMap.end())
                    {
                        Location const before=exist2->second;
                        ec.addError(shared_ptr<NameConflictError>(new NameConflictError(
                            (*vst)->name,before,
                            (*vst)->SymbolTable::loc
                        )));
                    }
                }
                else
                {
                    fieldMap.insert(pair<string const ,Location const >(
                        (*vst)->name,(*vst)->SymbolTable::loc
                    ));
                }
            }


            for(auto fst=vfst.begin();fst!=vfst.end();++fst)
            {
                if((*pos)->isMain && (*fst)->isMain)
                {
                    havemainFunc=true;
                    //检查这个main函数是否是static，返回值是否是void，形参列表是否为空
                    if(!(*fst)->isStatic)
                        ec.addError(shared_ptr<MainFuncStaticError>(new MainFuncStaticError(
                            (*fst)->SymbolTable::loc
                        )));
                    if(!(*fst)->returnType->isVoidType())
                        ec.addError(shared_ptr<MainFuncReturnTypeError>(new MainFuncReturnTypeError(
                            (*fst)->SymbolTable::loc
                        )));
                    if((*fst)->vvst.size()!=0)
                        ec.addError(shared_ptr<MainFuncFormalError>(new MainFuncFormalError(
                            (*fst)->SymbolTable::loc
                        )));
                }
                classExistCheck((*fst)->returnType,(*fst)->loc);
                auto exist=fieldMap.find((*fst)->name);//检查函数名冲突
                auto exist2=classNameMap.find((*fst)->name);//检查是否和类名冲突
                if(exist!=fieldMap.end() || exist2!=classNameMap.end())
                {
                    //发现重名
                    if(exist!=fieldMap.end())
                    {
                        Location const before=exist->second;
                        ec.addError(shared_ptr<NameConflictError>(new NameConflictError(
                            (*fst)->name,before,
                            (*fst)->SymbolTable::loc
                        )));
                    }
                    if(exist2!=classNameMap.end())
                    {
                        Location const before=exist2->second;
                        ec.addError(shared_ptr<NameConflictError>(new NameConflictError(
                            (*fst)->name,before,
                            (*fst)->SymbolTable::loc
                        )));
                    }
                }
                else
                {
                    fieldMap.insert(pair<string const ,Location const >(
                        (*fst)->name,(*fst)->SymbolTable::loc
                    ));
                }
                //检查函数形参列表中的形参类型和形参是否重复
                map<string const ,Location const > formalsMap;
                const vector<shared_ptr<VariableSymbolTable> > &formals=(*fst)->vvst;
                for(auto formal=formals.begin();formal!=formals.end();++formal)
                {
                    if((*formal)->type->isVoidType())
                        ec.addError(shared_ptr<VariableTypeIsVoidError>(new VariableTypeIsVoidError(
                            (*formal)->name,(*formal)->SymbolTable::loc
                        )));
                    classExistCheck((*formal)->type,(*formal)->loc);
                    auto exist=formalsMap.find((*formal)->name);
                    if(exist!=formalsMap.end())
                    {
                        Location const before=exist->second;
                        ec.addError(shared_ptr<NameConflictError>(new NameConflictError(
                            (*formal)->name,before,
                            (*formal)->loc
                        )));
                    }
                    else
                        formalsMap.insert(pair<string const ,Location const >(
                            (*formal)->name,(*formal)->loc
                        ));
                }


            }
        }
        if(!havemainFunc)
        {
            ec.addError(shared_ptr<NoMainFuncitonError>(new NoMainFuncitonError()));
        }
    }
    void functionCheck()
    {
        #ifdef _YLQ_DEBUG_
        Trace t(__LINE__,__FILE__);
        #endif
        /**
        对于继承了父类的子类
        如果子类中有和父类中同名的函数，首先这个两个函数都不能是静态函数，其次，这两个函数的返回值，形参列表必须相同
        **/
        const vector<shared_ptr<ClassSymbolTable> > cstList=gst->getCST();
        for(auto cst:cstList)
        {
            if(!cst->haveParent)
                continue;
            shared_ptr<ClassSymbolTable> parent=cst->parentClass;
            vector<shared_ptr<FunctionSymbolTable> > cstF = cst->vfst;
            while(1)
            {
                vector<shared_ptr<FunctionSymbolTable> > parentF = parent->vfst;
                for(auto cf:cstF)
                    for(auto pf:parentF)
                    {
                        if(cf->name!=pf->name)
                            continue;
                        //这连个函数的名字相同，那就检查是否是静态函数，是否返回值相同，是否
                        if(cf->isStatic || pf->isStatic)
                        {
                            ec.addError(shared_ptr<FunctionNameConflictError>(new FunctionNameConflictError(
                                cf->loc,pf->loc,"静态函数不能够重名"
                            )));
                            continue;
                        }
                        if(cf->returnType->symbolTypeToString()!=pf->returnType->symbolTypeToString())
                        {
                            ec.addError(shared_ptr<FunctionNameConflictError>(new FunctionNameConflictError(
                                cf->loc,pf->loc,"重名函数的返回值类型必须相同"
                            )));
                            continue;
                        }
                        vector<shared_ptr<VariableSymbolTable> > formals1 = cf->vvst;
                        vector<shared_ptr<VariableSymbolTable> > formals2 = pf->vvst;
                        if(formals1.size()!=formals2.size())
                        {
                            ec.addError(shared_ptr<FunctionNameConflictError>(new FunctionNameConflictError(
                                cf->loc,pf->loc,"重名函数的形参列表必须相同"
                            )));
                            continue;
                        }
                        auto pos1=formals1.begin();
                        auto pos2=formals2.begin();
                        for(;pos1!=formals1.end();++pos1,++pos2)
                            if((*pos1)->type->symbolTypeToString()!=(*pos2)->type->symbolTypeToString())
                            {
                                ec.addError(shared_ptr<FunctionNameConflictError>(new FunctionNameConflictError(
                                    cf->loc,pf->loc,"重名函数的形参列表必须相同"
                                )));
                                continue;
                            }
                    }
                if(!parent->haveParent)
                    break;
                parent=parent->parentClass;
            }
        }
    }
    void classExistCheck(shared_ptr<SymbolType> st,Location const loc)
    {
        #ifdef _YLQ_DEBUG_
        Trace t(__LINE__,__FILE__);
        #endif
        //进行类型检查,主要是看如果出现了类名，那么这个类是否找得到
        if(st->isClassType() && gst->getCSTByName(dynamic_pointer_cast<ClassSymbolType>(st)->name).get()==nullptr)
            ec.addLocAndString(loc,"存在未定义的类 "+st->symbolTypeToString());
        else if(st->isArrayType())
            classExistCheck(dynamic_pointer_cast<ArraySymbolType>(st)->type,loc);
    }
    template <typename Map,typename Pos,typename Ec>//就当作一个宏使用吧
    void NCE(Map const &map,Pos const &pos,Ec &ec) //NameConflictError
    {
        auto exist=map.find((*pos)->name);
        if(exist!=map.end())
        {
            ec.addError(shared_ptr<NameConflictError>(new NameConflictError(
                (*pos)->name,exist->second,(*pos)->SymbolTable::loc
            )));
        }
    }

    template <typename Map,typename Vec>
    void addToMap(Map &m,Vec const &v)
    {
        for(auto pos=v.begin();pos!=v.end();++pos)
            m.insert(pair<string const ,Location const >(
                (*pos)->name,(*pos)->SymbolTable::loc
            ));
    }

    void localLocalSTCheck(map<string const ,Location const > const &classNameMap,shared_ptr<LocalSymbolTable> const &lst)
    {
        #ifdef _YLQ_DEBUG_
        Trace t(__LINE__,__FILE__);
        #endif
        //检查嵌套的作用域中的变量是否和类名有重复，是否在自己的作用域中重复
        map<string const ,Location const > localMap;
        for(auto pos=lst->vvst.begin();pos!=lst->vvst.end();++pos)
        {
            NCE(classNameMap, pos,ec);
            NCE(localMap,pos,ec);
            classExistCheck((*pos)->type,(*pos)->loc);
            localMap.insert(pair<string const ,Location const >(
                (*pos)->name,(*pos)->SymbolTable::loc
            ));
        }

        for(auto pos=lst->vlst.begin();pos!=lst->vlst.end();++pos)
            localLocalSTCheck(classNameMap,*pos);
    }

    void localSTCheck()
    {
        #ifdef _YLQ_DEBUG_
        Trace t(__LINE__,__FILE__);
        #endif
        /**
        对函数体中定义的符号进行检查
        1.类名不会被屏蔽
        //2.局部作用域中的标识符不能够和当前类中的成员同名，不能够和当前函数的参数列表中的变量同名
        2.局部作用域中的标识符不能够和当前函数的参数列表中的变量同名
        3.嵌套的作用域中的标识符不能够和类名相同
        4.同一个作用域中不能够有重名的标识符
        **/

        map<string const ,Location const > classNameMap;
        addToMap(classNameMap,gst->getCST());
        for(auto pos=gst->getCST().begin();pos!=gst->getCST().end();++pos)
        {
            map<string const ,Location const > fieldMap;
            addToMap(fieldMap,(*pos)->vvst);
            addToMap(fieldMap,(*pos)->vfst);
            for(auto fst=(*pos)->vfst.begin();fst!=(*pos)->vfst.end();++fst)
            {
                map<string const ,Location const > formalMap;
                addToMap(formalMap,(*fst)->vvst);
                const vector<shared_ptr<LocalSymbolTable> > &lstVLST = (*fst)->localST->vlst;
                map<string const ,Location const > lstVVSTMap;
                for(auto lstVST=(*fst)->localST->vvst.begin();lstVST!=(*fst)->localST->vvst.end();++lstVST)
                {
                    NCE(classNameMap,lstVST,ec);//检查是否和类名冲突
                    //NCE(fieldMap,lstVST,ec);//检查是否和类中的成员名冲突
                    NCE(formalMap,lstVST,ec);//检查是否和函数的形参列表冲突
                    NCE(lstVVSTMap,lstVST,ec);//检查是否和当前局部作用域中的其他变量有名称冲突
                    classExistCheck((*lstVST)->type,(*lstVST)->loc);
                    lstVVSTMap.insert(pair<string const ,Location const >(
                        (*lstVST)->name,(*lstVST)->SymbolTable::loc
                    ));
                }

                for(auto lst=lstVLST.begin();lst!=lstVLST.end();++lst)
                    localLocalSTCheck(classNameMap,*lst);
            }
        }
    }
};

#endif /* end of include guard: _GEN_SYMBOL_TABLE_H_ */
