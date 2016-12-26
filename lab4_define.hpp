#ifndef _LAB4_DEFINE_H_
#define _LAB4_DEFINE_H_
#include "lab3_define.hpp"
namespace lab4
{
    struct Reg
    {
        const string registerName;
        static Reg al;
        static Reg eax;
        static Reg ebx;
        static Reg ecx;
        static Reg edx;
        static Reg esp;
        static Reg ebp;
        static Reg esi;
        Reg(const string &name):registerName(name){}
    };
    Reg Reg::al("al");
    Reg Reg::eax("eax");
    Reg Reg::ebx("ebx");
    Reg Reg::ecx("ecx");
    Reg Reg::edx("edx");
    Reg Reg::esp("esp");
    Reg Reg::ebp("ebp");
    Reg Reg::esi("esi");

    struct Ins
    {
        const string insName;
        static Ins  movl;
        static Ins  leal;
        static Ins  addl;
        static Ins  subl;
        static Ins  imull;
        static Ins  cltd;
        static Ins  idivl;
        static Ins  cmpl;
        static Ins  sete;
        static Ins  setne;
        static Ins  movzbl;
        static Ins  negl;
        static Ins  setl;
        static Ins  setle;
        static Ins  setg;
        static Ins  setge;
        static Ins  jmp;
        static Ins  jl;
        static Ins  jle;
        static Ins  jne;
        static Ins  je;
        static Ins  pushl;
        static Ins  popl;
        static Ins  ret;
        static Ins  call;
        Ins(const string &name):insName(name){}
    };

    Ins Ins::movl("movl");
    Ins Ins::leal("leal");
    Ins Ins::addl("addl");
    Ins Ins::subl("subl");
    Ins Ins::imull("imull");
    Ins Ins::cltd("cltd");
    Ins Ins::idivl("idivl");
    Ins Ins::cmpl("cmpl");
    Ins Ins::sete("sete");
    Ins Ins::movzbl("movzbl");
    Ins Ins::negl("negl");
    Ins Ins::setne("setne");
    Ins Ins::setl("setl");
    Ins Ins::setle("setle");
    Ins Ins::setg("setg");
    Ins Ins::setge("setge");
    Ins Ins::jmp("jmp");
    Ins Ins::jl("jl");
    Ins Ins::jle("jle");
    Ins Ins::jne("jne");
    Ins Ins::je("je");
    Ins Ins::pushl("pushl");
    Ins Ins::popl("popl");
    Ins Ins::ret("ret");
    Ins Ins::call("call");

    class AsmCode
    {
        static void asmcodeMark(const string &label,stringstream &ss){ss<<label<<":"<<endl;}
        static void asmcode(const Ins &ins,const string &label,stringstream &ss){ss<<ins.insName<<"\t"<<label<<endl;}
        static void asmcode(const Ins &ins,stringstream &ss){ss<<ins.insName<<endl;}
        static void asmcode(const Ins &ins,const int value,stringstream &ss)
        {
            ss<<ins.insName<<"\t$"<<value<<endl;
        }
        static void asmcode(const Ins &ins,const lab3::Temp &op,stringstream &ss)
        {
            if(op.isconst())
                ss<<ins.insName<<"\t$"<<op.getValue()<<endl;
            else
                ss<<ins.insName<<"\t("<<lab3::Temp::getCommName()<<"+"<<op.getOffsetInDataSegment()<<")"<<endl;
        }
        static void asmcode(const Ins &ins,const Reg &reg,stringstream &ss)
        {ss<<ins.insName<<"\t%"<<reg.registerName<<endl;}
        static void asmcode(const Ins &ins,const Reg &reg1,const Reg &reg2,stringstream &ss){ss<<ins.insName<<"\t%"<<reg1.registerName<<",%"<<reg2.registerName<<endl;}
        static void asmcode(const Ins &ins,const int value,const Reg &reg,stringstream &ss)
        {
            ss<<ins.insName<<"\t$"<<value<<",%"<<reg.registerName<<endl;
        }
        static void asmcode(const Ins &ins,const lab3::Temp &op,const Reg &reg,stringstream &ss)
        {
            if(op.isconst())
                ss<<ins.insName<<"\t$"<<op.getValue()<<",%"<<reg.registerName<<endl;
            else
                ss<<ins.insName<<"\t("<<lab3::Temp::getCommName()<<"+"<<op.getOffsetInDataSegment()<<"),%"<<reg.registerName<<endl;
        }
        static void asmcode(const Ins &ins,const string &labelName,const Reg &reg,stringstream &ss){ss<<ins.insName<<"\t"<<labelName<<",%"<<reg.registerName<<endl;}
        static void asmcode(const Ins &ins,const Reg &reg,const lab3::Temp &op,stringstream &ss)
        {
            if(op.isconst())SEE;
            ss<<ins.insName<<"\t%"<<reg.registerName<<",("<<lab3::Temp::getCommName()<<"+"<<op.getOffsetInDataSegment()<<")"<<endl;
        }
        static void asmcode(const Ins &ins,const int value,const lab3::Temp &op,stringstream &ss)
        {
            if(op.isconst())SEE;
            ss<<ins.insName<<"\t$"<<value<<",("<<lab3::Temp::getCommName()<<"+"<<op.getOffsetInDataSegment()<<")"<<endl;
        }


    public:
        static const string asmCode(const lab3::Tac &tac)
        {
            stringstream ss;
            ss.clear();
            switch(tac.kind)
            {
                case lab3::TK_LOAD_VTBL:
                //将虚函数表的首地址加载到指定的临时变量中去,中间寄存器使用 edx
                ss.clear();
                asmcode(Ins::leal,tac.vt->className,Reg::edx,ss);
                asmcode(Ins::movl,Reg::edx,tac.op0,ss);
                return ss.str();

                case lab3::TK_LOAD_STR_CONST:
                //将字符串的首地址加载到指定的临时变量中
                ss.clear();
                //首先获取到这个字符串的名字
                asmcode(Ins::leal,tac.rodataStrs.find(tac.str)->second,Reg::edx,ss);
                asmcode(Ins::movl,Reg::edx,tac.op0,ss);
                return ss.str();

                case lab3::TK_ADD:
                return asmCalcCode(Ins::addl, tac.op0, tac.op1, tac.op2,Reg::edx, ss);
                case lab3::TK_SUB:
                return asmCalcCode(Ins::subl, tac.op0, tac.op1, tac.op2,Reg::edx, ss);
                case lab3::TK_MUL:
                return asmCalcCode(Ins::imull, tac.op0, tac.op1, tac.op2,Reg::edx, ss);
                break;
                case lab3::TK_DIV:
                ss.clear();
                //首先将第一个操作数放到 eax 中去
                asmcode(Ins::movl,tac.op1,Reg::eax,ss);
                asmcode(Ins::movl,tac.op2,Reg::ecx,ss);
                asmcode(Ins::cltd,ss);//调用 cltd 指令
                asmcode(Ins::idivl,Reg::ecx,ss);
                //最终商在 eax 中，余数在 edx 中
                asmcode(Ins::movl,Reg::eax,tac.op0,ss);
                return ss.str();
                case lab3::TK_MOD:
                ss.clear();
                asmcode(Ins::movl,tac.op1,Reg::eax,ss);
                asmcode(Ins::movl,tac.op2,Reg::ecx,ss);
                asmcode(Ins::cltd,ss);
                asmcode(Ins::idivl,Reg::ecx,ss);
                //最终商在 eax 中，余数在 edx 中
                asmcode(Ins::movl,Reg::edx,tac.op0,ss);
                return ss.str();
                case lab3::TK_NEG:
                ss.clear();
                asmcode(Ins::movl,tac.op1,Reg::edx,ss);
                asmcode(Ins::negl,Reg::edx,ss);
                asmcode(Ins::movl,Reg::edx,tac.op0,ss);
                return ss.str();
                case lab3::TK_EQ_EQ:
                return asmRelaCode(Ins::sete,tac.op0,tac.op1,tac.op2, ss);
                case lab3::TK_NOT_EQ:
                return asmRelaCode(Ins::setne,tac.op0,tac.op1,tac.op2, ss);
                case lab3::TK_LESS:
                return asmRelaCode(Ins::setl,tac.op0,tac.op1,tac.op2,ss);
                case lab3::TK_LESS_EQ:
                return asmRelaCode(Ins::setle,tac.op0,tac.op1,tac.op2,ss);
                case lab3::TK_GREAT:
                return asmRelaCode(Ins::setg,tac.op0,tac.op1,tac.op2,ss);
                case lab3::TK_GREAT_EQ:
                return asmRelaCode(Ins::setge,tac.op0,tac.op1,tac.op2,ss);
                case lab3::TK_AND_AND:
                return asmAndAndCode(tac.op0,tac.op1,tac.op2,ss);
                case lab3::TK_OR_OR:
                return asmOrOrCode(tac.op0,tac.op1,tac.op2,ss);
                case lab3::TK_NOT:
                ss.clear();
                asmcode(Ins::cmpl,0,tac.op1,ss);//将0和源操作数比较
                asmcode(Ins::sete,Reg::al,ss);
                asmcode(Ins::movzbl,Reg::al,Reg::eax,ss);
                asmcode(Ins::movl,Reg::eax,tac.op0,ss);
                return ss.str();
                case lab3::TK_JMP:
                ss.clear();
                asmcode(Ins::jmp,tac.label->getLabelName(),ss);
                return ss.str();
                case lab3::TK_JL:
                return asmJmpCode(Ins::jl,tac.op0, tac.op1, tac.label->getLabelName(), ss);
                case lab3::TK_JLE:
                return asmJmpCode(Ins::jle,tac.op0,tac.op1,tac.label->getLabelName(),ss);
                case lab3::TK_JNE:
                return asmJmpCode(Ins::jne,tac.op0,tac.op1,tac.label->getLabelName(),ss);
                case lab3::TK_RETURN_VALUE:
                ss.clear();
                ss<<Ins::popl.insName<<"\t%"<<Reg::ebp.registerName<<endl;
                asmcode(Ins::movl,tac.op0,Reg::eax,ss);
                asmcode(Ins::ret,ss);
                return ss.str();
                case lab3::TK_RETURN_NO_VALUE:
                ss.clear();
                ss<<Ins::popl.insName<<"\t%"<<Reg::ebp.registerName<<endl;
                asmcode(Ins::ret,ss);
                return ss.str();
                case lab3::TK_CALL_NO_RETURN:
                ss.clear();
                asmcode(Ins::call,tac.str,ss);
                return ss.str();
                case lab3::TK_CALL_RETURN:
                ss.clear();
                asmcode(Ins::call,tac.str,ss);
                asmcode(Ins::movl,Reg::eax,tac.op0,ss);
                return ss.str();
                case lab3::TK_CALL_RETURN_VT:
                ss.clear();
                asmcode(Ins::movl,tac.op1,Reg::ebx,ss);//将虚函数表的地址放到 ebx 中
                if(!tac.op2.isconst())SEE;
                ss<<Ins::movl.insName<<"\t"<<tac.op2.getValue()<<"(%"<<Reg::ebx.registerName<<"),%"<<Reg::eax.registerName<<endl;//获取函数地址
                asmcode(Ins::call,Reg::eax,ss);
                asmcode(Ins::movl,Reg::eax,tac.op0,ss);//将返回值放到 op0中
                return ss.str();
                case lab3::TK_CALL_NO_RETURN_VT:
                ss.clear();
                asmcode(Ins::movl,tac.op1,Reg::ebx,ss);//将虚函数表的地址放到 ebx 中
                if(!tac.op2.isconst())SEE;
                ss<<Ins::movl.insName<<"\t"<<tac.op2.getValue()<<"(%"<<Reg::ebx.registerName<<"),%"<<Reg::eax.registerName<<endl;//获取函数地址
                asmcode(Ins::call,Reg::eax,ss);
                return ss.str();
                case lab3::TK_LOAD:
                return asmLoadCode(tac.op0,tac.op1,tac.op2,ss);
                case lab3::TK_STORE:
                return asmStoreCode(tac.op0,tac.op1,tac.op2,ss);
                case lab3::TK_PUSH:
                ss.clear();
                asmcode(Ins::pushl,tac.op0,ss);
                return ss.str();
                case lab3::TK_ALLOC_STACK:
                ss.clear();
                ss<<Ins::subl.insName<<"\t$"<<tac.op0.getValue()<<",%"<<Reg::esp.registerName<<endl;
                return ss.str();
                case lab3::TK_RELEASE_STACK:
                ss.clear();
                ss<<Ins::addl.insName<<"\t$"<<tac.op0.getValue()<<",%"<<Reg::esp.registerName<<endl;
                return ss.str();
                case lab3::TK_LOAD_FROM_STACK:
                return asmLoadFromStackCode(tac.op0,tac.op1,ss);
                case lab3::TK_STORE_TO_STACK:
                return asmStoreToStackCode(tac.op0,tac.op1,ss);
                case lab3::TK_MARK:
                ss.clear();
                asmcodeMark(tac.label->getLabelName(), ss);
                return ss.str();
                case lab3::TK_EMPTY:
                return "";
                default:
                SEE;
            }
            SEE;
        }

        static string beginFuncAsmCode()
        {
            stringstream ss;
            ss<<Ins::pushl.insName<<"\t%"<<Reg::ebp.registerName<<endl;
            ss<<Ins::movl.insName<<"\t%"<<Reg::esp.registerName<<",%"<<Reg::ebp.registerName<<endl;
            return ss.str();
        }

    private:
        static const string asmCalcCode(const Ins &ins,const lab3::Temp &op0,const lab3::Temp &op1,const lab3::Temp &op2,const Reg &reg,stringstream &ss)
        {
            ss.clear();
            asmcode(Ins::movl,op1,reg,ss);
            asmcode(ins,op2,reg,ss);
            asmcode(Ins::movl,reg,op0,ss);
            return ss.str();
        }
        static const string asmRelaCode(const Ins &relaIns,const lab3::Temp &op0,const lab3::Temp &op1,const lab3::Temp &op2,stringstream &ss)
        {
            ss.clear();
            asmcode(Ins::movl,op1,Reg::eax,ss);
            asmcode(Ins::cmpl,op2,Reg::eax,ss);
            asmcode(relaIns,Reg::al,ss);
            asmcode(Ins::movzbl,Reg::al,Reg::eax,ss);
            asmcode(Ins::movl,Reg::eax,op0,ss);
            return ss.str();
        }
        static const string asmAndAndCode(const lab3::Temp &op0,const lab3::Temp &op1,const lab3::Temp &op2,stringstream &ss)
        {
            ss.clear();
            asmcode(Ins::movl,op1,Reg::eax,ss);
            asmcode(Ins::cmpl,0,Reg::eax,ss);
            string label1=lab3::Label::genLabel()->getLabelName();
            string label2=lab3::Label::genLabel()->getLabelName();
            asmcode(Ins::je,label1,ss);
            asmcode(Ins::movl,op2,Reg::eax,ss);
            asmcode(Ins::cmpl,0,Reg::eax,ss);
            asmcode(Ins::je,label1,ss);
            asmcode(Ins::movl,1,Reg::eax,ss);
            asmcode(Ins::jmp,label2,ss);
            asmcodeMark(label1,ss);
            asmcode(Ins::movl,0,Reg::eax,ss);
            asmcodeMark(label2,ss);
            asmcode(Ins::movl,Reg::eax,op0,ss);
            return ss.str();
        }

        static const string asmOrOrCode(const lab3::Temp &op0,const lab3::Temp &op1,const lab3::Temp &op2,stringstream &ss)
        {
            ss.clear();
            string label1=lab3::Label::genLabel()->getLabelName();
            string label2=lab3::Label::genLabel()->getLabelName();
            string label3=lab3::Label::genLabel()->getLabelName();
            asmcode(Ins::movl,op1,Reg::eax,ss);
            asmcode(Ins::cmpl,0,Reg::eax,ss);
            asmcode(Ins::jne,label1,ss);
            asmcode(Ins::movl,op2,Reg::eax,ss);
            asmcode(Ins::cmpl,0,Reg::eax,ss);
            asmcode(Ins::je,label2,ss);
            asmcodeMark(label1,ss);
            asmcode(Ins::movl,1,Reg::eax,ss);
            asmcode(Ins::jmp,label3,ss);
            asmcodeMark(label2,ss);
            asmcode(Ins::movl,0,Reg::eax,ss);
            asmcodeMark(label3,ss);
            asmcode(Ins::movl,Reg::eax,op0,ss);
            return ss.str();
        }

        static const string asmJmpCode(const Ins &jmpIns,const lab3::Temp &op0,const lab3::Temp &op1,const string &label,stringstream &ss)
        {
            ss.clear();
            asmcode(Ins::movl,op0,Reg::edx,ss);
            asmcode(Ins::cmpl,op1,Reg::edx,ss);
            asmcode(jmpIns,label,ss);
            return ss.str();
        }

        static const string asmLoadCode(const lab3::Temp &dist,const lab3::Temp &offset,const lab3::Temp &base,stringstream &ss)
        {
            ss.clear();
            asmcode(Ins::movl,base,Reg::ebx,ss);
            asmcode(Ins::movl,offset,Reg::esi,ss);
            ss<<Ins::movl.insName<<"\t(%"<<Reg::ebx.registerName<<",%"<<Reg::esi.registerName<<"),%"<<Reg::eax.registerName<<endl;
            asmcode(Ins::movl,Reg::eax,dist,ss);
            return ss.str();
        }

        static const string asmStoreCode(const lab3::Temp &offset,const lab3::Temp &base,const lab3::Temp &src,stringstream &ss)
        {
            ss.clear();
            asmcode(Ins::movl,src,Reg::eax,ss);
            asmcode(Ins::movl,base,Reg::ebx,ss);
            asmcode(Ins::movl,offset,Reg::esi,ss);
            ss<<Ins::movl.insName<<"\t%"<<Reg::eax.registerName<<",(%"<<Reg::ebx.registerName<<",%"<<Reg::esi.registerName<<")"<<endl;
            return ss.str();
        }

        static const string asmLoadFromStackCode(const lab3::Temp &dist,const lab3::Temp &offset,stringstream &ss)
        {
            ss.clear();
            ss<<Ins::movl.insName<<"\t"<<offset.getValue()<<"(%"<<Reg::ebp.registerName<<"),%"<<Reg::eax.registerName<<endl;
            asmcode(Ins::movl,Reg::eax,dist,ss);
            return ss.str();
        }

        static const string asmStoreToStackCode(const lab3::Temp &src,const lab3::Temp &offset,stringstream &ss)
        {
            ss.clear();
            asmcode(Ins::movl,src,Reg::eax,ss);
            ss<<Ins::movl.insName<<"\t%"<<Reg::eax.registerName<<","<<offset.getValue()<<"(%"<<Reg::ebp.registerName<<")"<<endl;
            return ss.str();
        }

    };
};




#endif /* end of include guard: _LAB4_DEFINE_H_ */
