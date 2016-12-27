#ifndef _LAB3_DEFINE_H_
#define _LAB3_DEFINE_H_
#include "std.hpp"
namespace lab3
{
    enum TacKind
    {
        TK_EMPTY,
        TK_LOAD_VTBL,//加载虚函数表
        TK_LOAD_STR_CONST,//加载 字符串常量
        //算数运算
        TK_ADD,TK_SUB,TK_MUL,TK_DIV,TK_MOD,
        TK_NEG,//取负数

        //逻辑运算
        TK_EQ_EQ,TK_NOT_EQ,TK_LESS,TK_LESS_EQ,TK_GREAT,TK_GREAT_EQ,TK_AND_AND,TK_OR_OR,TK_NOT,

        //控制流
        TK_JMP,//无条件跳转
        TK_JL,//小余跳转
        TK_JLE,//小于等于时跳转
        TK_JNE,//不等于跳转
        TK_RETURN_VALUE,
        TK_RETURN_NO_VALUE,

        //函数调用
        TK_CALL_NO_RETURN,//调用函数
        TK_CALL_RETURN,
        TK_CALL_RETURN_VT,//调用虚函数表中的函数
        TK_CALL_NO_RETURN_VT,
        //内存访问
        TK_LOAD,//加载
        TK_STORE,//保存

        //栈操作
        TK_PUSH,//申请栈中的4个字节
        TK_ALLOC_STACK,
        TK_RELEASE_STACK,
        TK_LOAD_FROM_STACK,//从栈获取数据
        TK_STORE_TO_STACK,//保存到栈

        TK_MARK //定义一个行号
    };
    #define MALLOC  "malloc" // 由 malloc 函数申请的内存中的原始数据都是0
    #define EXIT    "exit"
    #define PRINTF  "printf"
    #define SCANF   "scanf"

    class Temp
    {
        string name;
        int offset=999999999;//表示在 .data 段中相对于临时变量首地址的偏移
        bool isConst=false;
        int value;
        static unsigned int count;
        static unsigned int constCount;
        static unsigned int maxCount;
        static map<int,Temp> intConstTmps;
        Temp(const string &name,const int offset):name(name),offset(offset){}
    public:
        Temp(){}
        Temp(const Temp &t):name(t.name),offset(t.offset),isConst(t.isConst),value(t.value){}
        static Temp genNewTemp()
        {
            stringstream ss;
            ss<<"_T"<<count;
            ++count;
            if(count>maxCount)
                maxCount=count;
            return Temp(ss.str(),count-1);
        }
        static void resetCount(){Temp::count=0;}
        static Temp &getIntConstTemp(int value)
        {
            if(intConstTmps.find(value)==intConstTmps.end())
            {
                stringstream ss;
                ss<<"_TC"<<constCount;
                ++constCount;
                Temp t(ss.str(),constCount-1);
                t.isConst=true;
                t.value=value;
                intConstTmps.insert(pair<int,Temp>(value,t));
            }
            return intConstTmps.find(value)->second;
        }
        Temp &operator=(const Temp &tmp)
        {
            this->name=tmp.name;
            this->offset=tmp.offset;
            this->isConst=tmp.isConst;
            this->value=tmp.value;
            return *this;
        }
        const string display()
        {
            if(isConst)
            {
                stringstream ss;
                ss<<value;
                return ss.str();
            }
            return name;
        }
        const bool isconst()const{return isConst;}
        const int getValue()const{return value;}
        const int getOffsetInDataSegment()const{return offset*4;}
        static const int getMaxCount(){return maxCount;}
        static const string getCommName(){return "_Temp";}

    };
    class Label
    {
        string label;
        Label(const string &label):label(label){}
        Label(){}
        static unsigned int count;
    public:
        static shared_ptr<Label> genLabel()
        {
            stringstream ss;
            ss<<".L"<<count;
            ++count;
            shared_ptr<Label> l(new Label(ss.str()));
            return l;
        }
        Label &operator=(const Label &l)
        {
            this->label=l.label;
            return *this;
        }
        const string &getLabelName()const{return label;}
    };

    unsigned int Temp::count=0;
    unsigned int Temp::maxCount=0;
    unsigned int Temp::constCount=0;
    unsigned int Label::count=0;
    map<int,Temp> Temp::intConstTmps;

    class Tac;
    class Function
    {
    public:
        const string className;
        const string funName;
        const string uniqueFuncId;
        vector<shared_ptr<Tac> > tacList;//函数执行体，一堆 tac
    public:
        Function(const string className,const string &funcName):className(className),funName(funcName),uniqueFuncId(genUniqueFuncId(className,funName))
        {
        //    Temp::resetCount();//每次重新开始一个函数，都重新开始计数
        }
        Function &operator<<(const shared_ptr<Tac> &tac)
        {
            tacList.push_back(tac);
            return *this;
        }
        const string &getUniqueFuncId()const
        {
            return uniqueFuncId;
        }
        static const string genUniqueFuncId(const string &className,const string &funName)
        {
            if(className=="Main" && funName=="main")
                return funName;
            return "_"+className+"_"+funName;
        }
    };
    class VirtualTable
    {
    public:
        shared_ptr<VirtualTable> parentVT;
        const string className;
        vector<shared_ptr<Function> > funcs;
    public:
        VirtualTable(const string &cn):className(cn){}
        void setParentVT(shared_ptr<VirtualTable> &pvt){parentVT=pvt;}
        void addFunc(shared_ptr<Function> &func){funcs.push_back(func);}
    };


    class Tac
    {
    public:
        enum TacKind kind;
        Temp op0;
        Temp op1;
        Temp op2;
        string str;//该字符串的实际内容
        string strName;//该字符串的名字，用于汇编语言时调用
        static map<const string,const string> rodataStrs;
        static int strNameCount;
        shared_ptr<VirtualTable> vt;
        shared_ptr<Label> label;
        Tac(enum TacKind kind):kind(kind){}
        Tac *setOp(int id,const Temp &op)
        {
            switch(id)
            {
                case 0:op0=op;break;
                case 1:op1=op;break;
                case 2:op2=op;break;
                default:SeriousErrorHandler::seriousError("lab3_define.h->Tac->setOp()");
            }
            return this;
        }
        Tac *setVT(shared_ptr<VirtualTable> &vt)
        {
            this->vt=vt;
            return this;
        }
        Tac *setStr(const string &str)
        {
            this->str=str;
            return this;
        }
        Tac *setLabel(const shared_ptr<Label> &l)
        {
            this->label=l;
            return this;
        }
        Tac *setStrName(const string &strName)
        {
            this->strName=strName;
            return this;
        }
        static const string &genUniqueStrName(const string &str)
        {
            if(rodataStrs.find(str)==rodataStrs.end())
            {
                stringstream ss;
                ss<<".LStr.S"<<strNameCount;
                ++strNameCount;
                rodataStrs.insert(pair<const string,const string>(str,ss.str()));
            }
            return rodataStrs.find(str)->second;
        }
    public:
        static const map<const string,const string> &getRodataStrs()
        {
            return rodataStrs;
        }
        static shared_ptr<Tac> genCall(const string &funcName)
        {
            shared_ptr<Tac> call((new Tac(TK_CALL_NO_RETURN))->setStr(funcName));
            return call;
        }
        static shared_ptr<Tac> genCall(const Temp &dist,const string &funcName)
        {
            shared_ptr<Tac> call((new Tac(TK_CALL_RETURN))->setOp(0,dist)->setStr(funcName));
            return call;
        }
        static shared_ptr<Tac> genCall(const Temp &dist,const Temp &vtbl,const Temp &offset)
        {
            shared_ptr<Tac> call((new Tac(TK_CALL_RETURN_VT))->setOp(0,dist)->setOp(1,vtbl)->setOp(2,offset));
            return call;
        }
        static shared_ptr<Tac> genCall(const Temp &vtbl,const Temp &offset)
        {
            shared_ptr<Tac> call((new Tac(TK_CALL_NO_RETURN_VT))->setOp(0,vtbl)->setOp(1,offset));
            return call;
        }
        static shared_ptr<Tac> genStore(const Temp &pointer,const Temp &offset,const Temp &src)
        {
            shared_ptr<Tac> store((new Tac(TK_STORE))->setOp(0,pointer)->setOp(1,offset)->setOp(2,src));
            return store;
        }
        static shared_ptr<Tac> genLoad(const Temp &dist,const Temp &pointer,const Temp &offset)
        {
            shared_ptr<Tac> load((new Tac(TK_LOAD))->setOp(0,dist)->setOp(1,pointer)->setOp(2,offset));
            return load;
        }
        static shared_ptr<Tac> genLoadVTBL(const Temp &dist,shared_ptr<VirtualTable> &vt)
        {
            shared_ptr<Tac> lv((new Tac(TK_LOAD_VTBL))->setOp(0,dist)->setVT(vt));
            return lv;
        }
        static shared_ptr<Tac> genReturn()
        {
            shared_ptr<Tac> r((new Tac(TK_RETURN_NO_VALUE)));
            return r;
        }
        static shared_ptr<Tac> genReturn(const Temp &value)
        {
            shared_ptr<Tac> rv((new Tac(TK_RETURN_VALUE))->setOp(0,value));
            return rv;
        }
        static shared_ptr<Tac> genAllocStack(const Temp &count)
        {
            shared_ptr<Tac> t((new Tac(TK_ALLOC_STACK))->setOp(0,count));
            return t;
        }
        static shared_ptr<Tac> genReleaseStack(const Temp &count)
        {
            shared_ptr<Tac> t((new Tac(TK_RELEASE_STACK))->setOp(0,count));
            return t;
        }
        static shared_ptr<Tac> genPush(const Temp &src)
        {
            shared_ptr<Tac> p((new Tac(TK_PUSH))->setOp(0,src));
            return p;
        }
        static shared_ptr<Tac> genLoadFromStack(const Temp &dist,const Temp &offset)
        {
            //从 bp+offset 处获取数据，保存到 dist 中
            shared_ptr<Tac> t((new Tac(TK_LOAD_FROM_STACK))->setOp(0,dist)->setOp(1,offset));
            return t;
        }
        static shared_ptr<Tac> genStoreToStack(const Temp &src,const Temp &offset)
        {
            //将数据保存到 bp+offset 中
            shared_ptr<Tac> t((new Tac(TK_STORE_TO_STACK))->setOp(0,src)->setOp(1,offset));
            return t;
        }
        static shared_ptr<Tac> genJmpLess(const Temp &left,const Temp &right,const shared_ptr<Label> &l)
        {
            //当 left<right 时跳转
            if(left.isconst() && right.isconst())
            {
                if(left.getValue() < right.getValue())
                    return genJmp(l);
                else
                    return shared_ptr<Tac>(new Tac(TK_EMPTY));
            }
            else
            {
                shared_ptr<Tac> t((new Tac(TK_JL))->setOp(0,left)->setOp(1,right)->setLabel(l));
                return t;
            }
        }
        static shared_ptr<Tac> genJmpLessEqual(const Temp &left,const Temp &right,const shared_ptr<Label> &l)
        {
            //当 left<=right 时跳转
            if(left.isconst() && right.isconst())
            {
                if(left.getValue() <= right.getValue())
                    return genJmp(l);
                else
                    return shared_ptr<Tac>(new Tac(TK_EMPTY));
            }
            else
            {
                shared_ptr<Tac> t((new Tac(TK_JLE))->setOp(0,left)->setOp(1,right)->setLabel(l));
                return t;
            }

        }
        static shared_ptr<Tac> genLoadStr(const Temp &dist,const string &str)
        {
            shared_ptr<Tac> t((new Tac(TK_LOAD_STR_CONST))->setOp(0,dist)->setStr(str)->setStrName(genUniqueStrName(str)));
            return t;
        }
        static shared_ptr<Tac> genMark(const shared_ptr<Label> &l)
        {
            shared_ptr<Tac> t((new Tac(TK_MARK))->setLabel(l));
            return t;
        }
        static shared_ptr<Tac> genMul(const Temp &dist,const Temp &op1,const Temp &op2)
        {
            shared_ptr<Tac> t((new Tac(TK_MUL))->setOp(0,dist)->setOp(1,op1)->setOp(2,op2));
            return t;
        }
        static shared_ptr<Tac> genAdd(const Temp &dist,const Temp &op1,const Temp &op2)
        {
            shared_ptr<Tac> t((new Tac(TK_ADD))->setOp(0,dist)->setOp(1,op1)->setOp(2,op2));
            return t;
        }
        static shared_ptr<Tac> genDiv(const Temp &dist,const Temp &op1,const Temp &op2)
        {
            shared_ptr<Tac> t((new Tac(TK_DIV))->setOp(0,dist)->setOp(1,op1)->setOp(2,op2));
            return t;
        }
        static shared_ptr<Tac> genSub(const Temp &dist,const Temp &op1,const Temp &op2)
        {
            shared_ptr<Tac> t((new Tac(TK_SUB))->setOp(0,dist)->setOp(1,op1)->setOp(2,op2));
            return t;
        }
        static shared_ptr<Tac> genMod(const Temp &dist,const Temp &op1,const Temp &op2)
        {
            shared_ptr<Tac> t((new Tac(TK_MOD))->setOp(0,dist)->setOp(1,op1)->setOp(2,op2));
            return t;
        }

        static shared_ptr<Tac> genLess(const Temp &dist,const Temp &op1,const Temp &op2)
        {
            shared_ptr<Tac> t((new Tac(TK_LESS))->setOp(0,dist)->setOp(1,op1)->setOp(2,op2));
            return t;
        }
        static shared_ptr<Tac> genLessEqual(const Temp &dist,const Temp &op1,const Temp &op2)
        {
            shared_ptr<Tac> t((new Tac(TK_LESS_EQ))->setOp(0,dist)->setOp(1,op1)->setOp(2,op2));
            return t;
        }
        static shared_ptr<Tac> genGreat(const Temp &dist,const Temp &op1,const Temp &op2)
        {
            shared_ptr<Tac> t((new Tac(TK_GREAT))->setOp(0,dist)->setOp(1,op1)->setOp(2,op2));
            return t;
        }
        static shared_ptr<Tac> genGreatEqual(const Temp &dist,const Temp &op1,const Temp &op2)
        {
            shared_ptr<Tac> t((new Tac(TK_GREAT_EQ))->setOp(0,dist)->setOp(1,op1)->setOp(2,op2));
            return t;
        }
        static shared_ptr<Tac> genEqualEqual(const Temp &dist,const Temp &op1,const Temp &op2)
        {
            shared_ptr<Tac> t((new Tac(TK_EQ_EQ))->setOp(0,dist)->setOp(1,op1)->setOp(2,op2));
            return t;
        }
        static shared_ptr<Tac> genNotEqual(const Temp &dist,const Temp &op1,const Temp &op2)
        {
            shared_ptr<Tac> t((new Tac(TK_NOT_EQ))->setOp(0,dist)->setOp(1,op1)->setOp(2,op2));
            return t;
        }
        static shared_ptr<Tac> genAndAnd(const Temp &dist,const Temp &op1,const Temp &op2)
        {
            shared_ptr<Tac> t((new Tac(TK_AND_AND))->setOp(0,dist)->setOp(1,op1)->setOp(2,op2));
            return t;
        }
        static shared_ptr<Tac> genOrOr(const Temp &dist,const Temp &op1,const Temp &op2)
        {
            shared_ptr<Tac> t((new Tac(TK_OR_OR))->setOp(0,dist)->setOp(1,op1)->setOp(2,op2));
            return t;
        }

        static shared_ptr<Tac> genJmpNotEqual(const Temp &left,const Temp &right,const shared_ptr<Label> &l)
        {
            //op1!=op2时，跳转到 l
            if(left.isconst() && right.isconst())
            {
                if(left.getValue()!=right.getValue())
                    return genJmp(l);
                else
                    return shared_ptr<Tac>(new Tac(TK_EMPTY));
            }
            else
            {
                shared_ptr<Tac> t((new Tac(TK_JNE))->setOp(0,left)->setOp(1,right)->setLabel(l));
                return t;
            }
        }
        static shared_ptr<Tac> genJmp(const shared_ptr<Label> &l)
        {
            shared_ptr<Tac> t((new Tac(TK_JMP))->setLabel(l));
            return t;
        }
        static shared_ptr<Tac> genNot(const Temp &dist,const Temp &src)
        {
            shared_ptr<Tac> t((new Tac(TK_NOT))->setOp(0,dist)->setOp(1,src));
            return t;
        }
        static shared_ptr<Tac> genNeg(const Temp &dist,const Temp &src)
        {
            shared_ptr<Tac> t((new Tac(TK_NEG))->setOp(0,dist)->setOp(1,src));
            return t;
        }

        void print()
        {
            switch(kind)
            {
                case TK_LOAD_VTBL:
                cout<<op0.display()<<" = AddrOfVT("<<vt->className<<")"<<endl;
                break;
                case TK_LOAD_STR_CONST:
                cout<<op0.display()<<" = AddrOfStr("<<str<<")"<<endl;
                break;
                case TK_ADD:
                cout<<op0.display()<<" = "<<op1.display()<<" + "<<op2.display()<<endl;
                break;
                case TK_SUB:
                cout<<op0.display()<<" = "<<op1.display()<<" - "<<op2.display()<<endl;
                break;
                case TK_MUL:
                cout<<op0.display()<<" = "<<op1.display()<<" * "<<op2.display()<<endl;
                break;
                case TK_DIV:
                cout<<op0.display()<<" = "<<op1.display()<<" / "<<op2.display()<<endl;
                break;
                case TK_MOD:
                cout<<op0.display()<<" = "<<op1.display()<<" % "<<op2.display()<<endl;
                break;
                case TK_NEG:
                cout<<op0.display()<<" = -"<<op1.display()<<endl;
                break;
                case TK_EQ_EQ:
                cout<<op0.display()<<" = ("<<op1.display()<<" == "<<op2.display()<<")"<<endl;
                break;
                case TK_NOT_EQ:
                cout<<op0.display()<<" = ("<<op1.display()<<" != "<<op2.display()<<")"<<endl;
                break;
                case TK_LESS:
                cout<<op0.display()<<" = ("<<op1.display()<<" < "<<op2.display()<<")"<<endl;
                break;
                case TK_LESS_EQ:
                cout<<op0.display()<<" = ("<<op1.display()<<" <= "<<op2.display()<<")"<<endl;
                break;
                case TK_GREAT:
                cout<<op0.display()<<" = ("<<op1.display()<<" > "<<op2.display()<<")"<<endl;
                break;
                case TK_GREAT_EQ:
                cout<<op0.display()<<" = ("<<op1.display()<<" >= "<<op2.display()<<")"<<endl;
                break;
                case TK_AND_AND:
                cout<<op0.display()<<" = ("<<op1.display()<<" && "<<op2.display()<<")"<<endl;
                break;
                case TK_OR_OR:
                cout<<op0.display()<<" = ("<<op1.display()<<" || "<<op2.display()<<")"<<endl;
                break;
                case TK_NOT:
                cout<<op0.display()<<" = !"<<op1.display()<<endl;
                break;
                case TK_JMP:
                cout<<"jmp "<<label->getLabelName()<<endl;
                break;
                case TK_JL:
                cout<<"jmp "<<label->getLabelName()<<" ("<<op0.display()<<" < "<<op1.display()<<")"<<endl;
                break;
                case TK_JLE:
                cout<<"jmp "<<label->getLabelName()<<" ("<<op0.display()<<" <= "<<op1.display()<<")"<<endl;
                break;
                case TK_JNE:
                cout<<"jmp "<<label->getLabelName()<<" ("<<op0.display()<<" != "<<op1.display()<<")"<<endl;
                break;
                case TK_RETURN_VALUE:
                cout<<"ret "<<op0.display()<<endl;
                break;
                case TK_RETURN_NO_VALUE:
                cout<<"ret"<<endl;
                break;
                case TK_CALL_NO_RETURN:
                cout<<"call "<<str<<endl;
                break;
                case TK_CALL_RETURN:
                cout<<"call "<<str<<endl;
                cout<<"mov "<<op0.display()<<",eax"<<endl;
                break;
                case TK_CALL_RETURN_VT:
                cout<<"call "<<op1.display()<<"["<<op2.display()<<"]"<<endl;
                cout<<"mov "<<op0.display()<<",eax"<<endl;
                break;
                case TK_CALL_NO_RETURN_VT:
                cout<<"call "<<op1.display()<<"["<<op2.display()<<"]"<<endl;
                break;
                case TK_LOAD:
                cout<<"load "<<op0.display()<<" = "<<op1.display()<<"["<<op2.display()<<"]"<<endl;
                break;
                case TK_STORE:
                cout<<"store "<<op0.display()<<"["<<op1.display()<<"] = "<<op2.display()<<endl;
                break;
                case TK_PUSH:
                cout<<"push "<<op0.display()<<endl;
                break;
                case TK_ALLOC_STACK:
                cout<<"allocStack "<<op0.display()<<endl;
                break;
                case TK_RELEASE_STACK:
                cout<<"releaseStack "<<op0.display()<<endl;
                break;
                case TK_LOAD_FROM_STACK:
                cout<<"loadFromStack "<<op0.display()<<" = ebp["<<op1.display()<<"]"<<endl;
                break;
                case TK_STORE_TO_STACK:
                cout<<"storeToStack ebp["<<op1.display()<<"] = "<<op0.display()<<endl;
                break;
                case TK_MARK:
                cout<<label->getLabelName()<<":"<<endl;
                break;
                case TK_EMPTY:
                break;
                default:
                SEE;
            }
        }

    };

    map<const string,const string> Tac::rodataStrs;
    int Tac::strNameCount=0;


    enum ValueIn
    {
        VALUE_IN_HEAP,VALUE_IN_STACK,VALUE_IN_REGISTER,VALUE_IN_UNKOWN
    };
    struct ExprAttributeLab3
    {
        Temp pointer;//该表达式的段地址
        Temp offset=Temp::getIntConstTemp(0);//该表达式相对于段地址的偏移，
        Temp reg;//寄存器
        enum ValueIn vi=VALUE_IN_UNKOWN;//该表达式所表示的值在哪里，如果是在 heap 中，那么使用1，2.如果是在栈中，那么使用2，如果是在 register 中，那么使用3
    };
};



#endif /* end of include guard: _LAB3_DEFINE_H_ */
