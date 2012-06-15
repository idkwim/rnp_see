#include "translator.h"

#include <udis86.h>

#include <cstdio>
#include <iostream>
#include <sstream>

#include "debug.h"

std::string ins_debug_str (ud_t * ud_obj)
{
    std::stringstream ss;
    
    ss << ud_insn_asm(ud_obj)
       << "{type="   << ud_type_DEBUG[ud_obj->operand[0].type] << ", "
       << " size="   << (int) ud_obj->operand[0].size << ", "
       << " offset=" << (int) ud_obj->operand[0].offset << ", "
       << " scale="  << (int) ud_obj->operand[0].scale << ", "
       << " lval="   << (int) ud_obj->operand[0].lval.uqword << ", "
       << " base="   << ud_type_DEBUG[ud_obj->operand[0].base] << "}, "
       << "{type="   << ud_type_DEBUG[ud_obj->operand[1].type] << ", "
       << " size="   << (int) ud_obj->operand[1].size << ", "
       << " offset=" << (int) ud_obj->operand[1].offset << ", "
       << " scale="  << (int) ud_obj->operand[1].scale << ", "
       << " lval="   << (int) ud_obj->operand[1].lval.uqword << ", "
       << " base="   << ud_type_DEBUG[ud_obj->operand[1].base] << "}";
   
   return ss.str();
}

std::list <Instruction *> Translator :: translate (uint64_t address, uint8_t * data, size_t size)
{
    ud_t ud_obj;
    
    ud_init(&ud_obj);
    ud_set_mode(&ud_obj, 64);
    ud_set_syntax(&ud_obj, UD_SYN_INTEL);
    
    ud_set_input_buffer(&ud_obj, (unsigned char *) data, size);
    
    while (ud_disassemble(&ud_obj)) {
        switch (ud_obj.mnemonic) {
        case UD_Iadd    : add    (&ud_obj, address + ud_insn_off(&ud_obj)); break;
        case UD_Iand    : And    (&ud_obj, address + ud_insn_off(&ud_obj)); break;
        case UD_Icall   : call   (&ud_obj, address + ud_insn_off(&ud_obj)); break;
        case UD_Icmp    : cmp    (&ud_obj, address + ud_insn_off(&ud_obj)); break;
        case UD_Ihlt    : hlt    (&ud_obj, address + ud_insn_off(&ud_obj)); break;
        case UD_Ija     : ja     (&ud_obj, address + ud_insn_off(&ud_obj)); break;
        case UD_Ijl     : jl     (&ud_obj, address + ud_insn_off(&ud_obj)); break;
        case UD_Ijmp    : jmp    (&ud_obj, address + ud_insn_off(&ud_obj)); break;
        case UD_Ijnz    : jnz    (&ud_obj, address + ud_insn_off(&ud_obj)); break;
        case UD_Ijz     : jz     (&ud_obj, address + ud_insn_off(&ud_obj)); break;
        case UD_Ilea    : lea    (&ud_obj, address + ud_insn_off(&ud_obj)); break;
        case UD_Ileave  : leave  (&ud_obj, address + ud_insn_off(&ud_obj)); break;
        case UD_Imov    : mov    (&ud_obj, address + ud_insn_off(&ud_obj)); break;
        case UD_Imovzx  : movzx  (&ud_obj, address + ud_insn_off(&ud_obj)); break;
        case UD_Imovsxd : movsxd (&ud_obj, address + ud_insn_off(&ud_obj)); break;
        case UD_Inop    : nop    (&ud_obj, address + ud_insn_off(&ud_obj)); break;
        case UD_Ipop    : pop    (&ud_obj, address + ud_insn_off(&ud_obj)); break;
        case UD_Ipush   : push   (&ud_obj, address + ud_insn_off(&ud_obj)); break;
        case UD_Iret    : ret    (&ud_obj, address + ud_insn_off(&ud_obj)); break;
        case UD_Isar    : sar    (&ud_obj, address + ud_insn_off(&ud_obj)); break;
        case UD_Ishr    : sar    (&ud_obj, address + ud_insn_off(&ud_obj)); break;
        case UD_Isub    : sub    (&ud_obj, address + ud_insn_off(&ud_obj)); break;
        case UD_Itest   : test   (&ud_obj, address + ud_insn_off(&ud_obj)); break;
        case UD_Ixor    : Xor    (&ud_obj, address + ud_insn_off(&ud_obj)); break;
        default :
            std::stringstream ss;
            ss << "unhandled instruction ["
            << ud_insn_hex(&ud_obj) << " => "
            <<  ud_insn_asm(&ud_obj) << "]"
            << " " << ins_debug_str(&ud_obj);
            std::cerr << ss.str() << std::endl;
            //throw std::runtime_error(ss.str());
        }
    }
    
    return instructions;
}


int Translator :: register_bits (int reg)
{
    if ((reg >= UD_R_AL)  && (reg <= UD_R_R15B)) return 8;
    if ((reg >= UD_R_AX)  && (reg <= UD_R_R15W)) return 16;
    if ((reg >= UD_R_EAX) && (reg <= UD_R_R15D)) return 32;
    if ((reg >= UD_R_RAX) && (reg <= UD_R_R15))  return 64;
    if (reg == UD_R_RIP) return 64;
    
    throw std::runtime_error("invalid register for register_bits");
    return -1;
}


void Translator :: operand_set (ud_t * ud_obj, int operand_i, uint64_t address, InstructionOperand & value)
{
    InstructionOperand lhs = operand(ud_obj, operand_i, address);
    if (ud_obj->operand[operand_i].type == UD_OP_MEM)
        instructions.push_back(new InstructionStore(address,
                                                    ud_insn_len(ud_obj),
                                                    ud_obj->operand[operand_i].size,
                                                    lhs,
                                                    value));
    else
        instructions.push_back(new InstructionAssign(address, ud_insn_len(ud_obj), lhs, value));
}


InstructionOperand Translator :: operand_get (ud_t * ud_obj, int operand_i, uint64_t address)
{
    InstructionOperand result = operand(ud_obj, operand_i, address);
    if (ud_obj->operand[operand_i].type == UD_OP_MEM) {
        InstructionOperand loaded_result = InstructionOperand(OPTYPE_TMPVAR, ud_obj->operand[operand_i].size);
        instructions.push_back(new InstructionLoad(address,
                                                   ud_insn_len(ud_obj),
                                                   ud_obj->operand[operand_i].size,
                                                   loaded_result,
                                                   result));
        return loaded_result;
    }
    return result;
}


uint64_t Translator :: operand_lval (int bits, struct ud_operand operand)
{
    switch (bits) {
    case 8  : return (uint64_t) operand.lval.ubyte;
    case 16 : return (uint64_t) operand.lval.uword;
    case 32 : return (uint64_t) operand.lval.udword;
    case 64 : return (uint64_t) operand.lval.uqword;
    }
    
    std::stringstream ss;
    ss << "invalid bit value passed to operand_lval: " << bits;
    throw std::runtime_error(ss.str());
    
    return 0;
}


InstructionOperand Translator :: operand (ud_t * ud_obj, int operand_i, uint64_t address)
{
    struct ud_operand operand = ud_obj->operand[operand_i];
    
    if (operand.type == UD_OP_REG)
        return InstructionOperand(OPTYPE_VAR, register_bits(operand.base), operand.base);
    
    else if (operand.type == UD_OP_MEM) {
        InstructionOperand base;
        InstructionOperand index;
        InstructionOperand scale;
        InstructionOperand displ;
        
        if (operand.base)   base  = InstructionOperand(OPTYPE_VAR,      registers.bits(operand.base),  operand.base);
        if (operand.index)  index = InstructionOperand(OPTYPE_VAR,      registers.bits(operand.index), operand.index);
        if (operand.scale)  scale = InstructionOperand(OPTYPE_CONSTANT, 8,                             operand.scale);
        if (operand.offset) displ = InstructionOperand(OPTYPE_CONSTANT,
                                                       operand.offset,
                                                       operand_lval(operand.offset, operand));
        
        InstructionOperand index_scale = index;
        if (operand.index && operand.scale) {
            index_scale = InstructionOperand(OPTYPE_TMPVAR, 64);
            instructions.push_back(new InstructionMul(address, 64, index_scale, index, scale));
        }
        
        InstructionOperand base_displacement = base;
        if (operand.base && operand.offset) {
            base_displacement = InstructionOperand(OPTYPE_TMPVAR, 64);
            instructions.push_back(new InstructionAdd(address, 64, base_displacement, base, displ));
        }
        else if ((! operand.base) && (operand.offset))
            base_displacement = displ;
        
        if (operand.index) {
            InstructionOperand result = InstructionOperand(OPTYPE_TMPVAR, 64);
            instructions.push_back(new InstructionAdd(address, 64, result, index_scale, base_displacement));
            return result;
        }
        
        return base_displacement;
    }
    else if (operand.type == UD_OP_CONST)
        return InstructionOperand(OPTYPE_CONSTANT, operand.size, operand_lval(operand.size, operand));
    else if (operand.type == UD_OP_JIMM)
        return InstructionOperand(OPTYPE_CONSTANT, operand.size, operand_lval(operand.size, operand));
    else if (operand.type == UD_OP_IMM)
        return InstructionOperand(OPTYPE_CONSTANT, operand.size, operand_lval(operand.size, operand));
    else
        throw std::runtime_error("unsupported operand type " + ins_debug_str(ud_obj));
    
    return InstructionOperand();
}


void Translator :: add (ud_t * ud_obj, uint64_t address)
{
    InstructionOperand lhs = operand_get(ud_obj, 0, address);
    InstructionOperand rhs = operand_get(ud_obj, 1, address);
    InstructionOperand tmp (OPTYPE_TMPVAR, lhs.g_bits());
        
    InstructionOperand OFTmp    (OPTYPE_TMPVAR, lhs.g_bits());
    InstructionOperand OFTmpShl (OPTYPE_CONSTANT, lhs.g_bits(), lhs.g_bits() - 1);
    InstructionOperand OF       (OPTYPE_TMPVAR, 1, "OF"); // signed overflow
    InstructionOperand CF       (OPTYPE_TMPVAR, 1, "CF"); // unsigned overflow
    InstructionOperand ZF       (OPTYPE_TMPVAR, 1, "ZF");
    InstructionOperand SF       (OPTYPE_TMPVAR, 1, "SF"); // "negative" flag
    InstructionOperand zero     (OPTYPE_CONSTANT, tmp.g_bits(), 0);
    
    instructions.push_back(new InstructionAdd(address, ud_insn_len(ud_obj), tmp, lhs, rhs));
    
    instructions.push_back(new InstructionXor(address, ud_insn_len(ud_obj), OFTmp, tmp,   lhs));
    instructions.push_back(new InstructionShr(address, ud_insn_len(ud_obj), OF,    OFTmp, OFTmpShl));
    
    instructions.push_back(new InstructionCmpLtu(address, ud_insn_len(ud_obj), CF, tmp, lhs));
    instructions.push_back(new InstructionCmpEq (address, ud_insn_len(ud_obj), ZF, tmp, zero));
    instructions.push_back(new InstructionCmpLts(address, ud_insn_len(ud_obj), SF, tmp, zero));
    
    operand_set(ud_obj, 0, address, tmp);
}

void Translator :: And (ud_t * ud_obj, uint64_t address)
{
    InstructionOperand lhs = operand_get(ud_obj, 0, address);
    InstructionOperand rhs = operand_get(ud_obj, 1, address);
    InstructionOperand tmp (OPTYPE_TMPVAR, lhs.g_bits());
    
    InstructionOperand OF   (OPTYPE_TMPVAR, 1, "OF"); // signed overflow
    InstructionOperand CF   (OPTYPE_TMPVAR, 1, "CF"); // unsigned overflow
    InstructionOperand ZF   (OPTYPE_TMPVAR, 1, "ZF");
    InstructionOperand SF   (OPTYPE_TMPVAR, 1, "SF"); // "negative" flag
    InstructionOperand zero (OPTYPE_CONSTANT, tmp.g_bits(), 0);
    
    instructions.push_back(new InstructionAnd(address, ud_insn_len(ud_obj), tmp, lhs, rhs));
    
    instructions.push_back(new InstructionAssign(address, ud_insn_len(ud_obj), OF, zero));
    instructions.push_back(new InstructionAssign(address, ud_insn_len(ud_obj), CF, zero));
    instructions.push_back(new InstructionCmpEq (address, ud_insn_len(ud_obj), ZF, tmp, zero));
    instructions.push_back(new InstructionCmpLts(address, ud_insn_len(ud_obj), SF, tmp, zero));
    
    operand_set(ud_obj, 0, address, tmp);
}

void Translator :: call (ud_t * ud_obj, uint64_t address)
{
    InstructionOperand dst = operand_get(ud_obj, 0, address);
    instructions.push_back(new InstructionCall(address, ud_insn_len(ud_obj), dst));
}


void Translator :: cmp (ud_t * ud_obj, uint64_t address)
{
    InstructionOperand lhs = operand_get(ud_obj, 0, address);
    InstructionOperand rhs = operand_get(ud_obj, 1, address);
    
    InstructionOperand CF          (OPTYPE_TMPVAR, 1, "CF");
    InstructionOperand CForZF      (OPTYPE_TMPVAR, 1, "CForZF");
    InstructionOperand SFxorOF     (OPTYPE_TMPVAR, 1, "SFxorOF");
    InstructionOperand SFxorOForZF (OPTYPE_TMPVAR, 1, "SFxorOForZF");
    InstructionOperand ZF          (OPTYPE_TMPVAR, 1, "ZF");
    InstructionOperand OF          (OPTYPE_TMPVAR, 1, "OF");
    InstructionOperand SF          (OPTYPE_TMPVAR, 1, "SF");
    InstructionOperand tmp0        (OPTYPE_TMPVAR, lhs.g_bits());
    InstructionOperand tmp0s       (OPTYPE_CONSTANT, tmp0.g_bits(), 0);
    
    instructions.push_back(new InstructionCmpLtu(address, ud_insn_len(ud_obj), CF,          lhs, rhs));
    instructions.push_back(new InstructionCmpLeu(address, ud_insn_len(ud_obj), CForZF,      lhs, rhs));
    instructions.push_back(new InstructionCmpLts(address, ud_insn_len(ud_obj), SFxorOF,     lhs, rhs));
    instructions.push_back(new InstructionCmpLes(address, ud_insn_len(ud_obj), SFxorOForZF, lhs, rhs));
    instructions.push_back(new InstructionCmpEq (address, ud_insn_len(ud_obj), ZF,          lhs, rhs));
    instructions.push_back(new InstructionSub   (address, ud_insn_len(ud_obj), tmp0,        lhs, rhs));
    instructions.push_back(new InstructionCmpLts(address, ud_insn_len(ud_obj), SF,          tmp0, tmp0s));
    instructions.push_back(new InstructionXor   (address, ud_insn_len(ud_obj), OF,          SFxorOF, SF));
}


void Translator :: hlt (ud_t * ud_obj, uint64_t address)
{
    instructions.push_back(new InstructionHlt(address, ud_insn_len(ud_obj)));
}


void Translator :: ja (ud_t * ud_obj, uint64_t address)
{
    InstructionOperand dst = operand_get(ud_obj, 0, address);
    
    InstructionOperand CF        (OPTYPE_TMPVAR, 1, "CF");
    InstructionOperand ZF        (OPTYPE_TMPVAR, 1, "ZF");
    InstructionOperand CForZF    (OPTYPE_TMPVAR, 1, "CForZF");
    InstructionOperand notCForZF (OPTYPE_TMPVAR, 1, "notCForZF");
    
    instructions.push_back(new InstructionOr(address, ud_insn_len(ud_obj), CForZF, CF, ZF));
    instructions.push_back(new InstructionNot(address, ud_insn_len(ud_obj), notCForZF, CForZF));
    instructions.push_back(new InstructionBrc(address, ud_insn_len(ud_obj), dst, notCForZF));
}


void Translator :: jl (ud_t * ud_obj, uint64_t address)
{
    InstructionOperand SFxorOF (OPTYPE_TMPVAR, 1, "SFxorOF");
    InstructionOperand dst = operand(ud_obj, 0, address);
    
    instructions.push_back(new InstructionBrc(address, ud_insn_len(ud_obj), SFxorOF, dst));
}


// making this an InstructionBrc greatly simplifies analysis as all branches are
// either InstructionBrc or InstructionCall
void Translator :: jmp (ud_t * ud_obj, uint64_t address)
{
    InstructionOperand dst = operand(ud_obj, 0, address);
    InstructionOperand t (OPTYPE_TMPVAR, 1, 1);
    
    instructions.push_back(new InstructionBrc(address, ud_insn_len(ud_obj), t, dst));
}


void Translator :: jnz (ud_t * ud_obj, uint64_t address)
{
    InstructionOperand dst = operand(ud_obj, 0, address);
    
    InstructionOperand ZF    (OPTYPE_TMPVAR,   1, "ZF");
    InstructionOperand notZF (OPTYPE_TMPVAR,   1, "notZF");

    instructions.push_back(new InstructionNot(address, ud_insn_len(ud_obj), notZF, ZF));
    instructions.push_back(new InstructionBrc(address, ud_insn_len(ud_obj), dst,   notZF));
}

void Translator :: jz (ud_t * ud_obj, uint64_t address)
{
    InstructionOperand ZF  (OPTYPE_TMPVAR, 1, "ZF");
    InstructionOperand dst = operand(ud_obj, 0, address);
    
    instructions.push_back(new InstructionBrc(address, ud_insn_len(ud_obj), ZF, dst));
}


void Translator :: lea (ud_t * ud_obj, uint64_t address)
{
    InstructionOperand dst = operand(ud_obj, 0, address);
    InstructionOperand src = operand(ud_obj, 1, address);
    
    instructions.push_back(new InstructionAssign(address, ud_insn_len(ud_obj), dst, src));
}


void Translator :: leave (ud_t * ud_obj, uint64_t address)
{
    InstructionOperand rsp   (OPTYPE_VAR, 64, UD_R_RSP);
    InstructionOperand rbp   (OPTYPE_VAR, 64, UD_R_RBP);
    InstructionOperand eight (OPTYPE_CONSTANT, 8, 8);
    
    instructions.push_back(new InstructionAssign(address, ud_insn_len(ud_obj), rsp, rbp));
    instructions.push_back(new InstructionLoad  (address, ud_insn_len(ud_obj), 64, rbp, rsp));
    instructions.push_back(new InstructionAdd   (address, ud_insn_len(ud_obj), rsp, rsp, eight));
}


void Translator :: mov (ud_t * ud_obj, uint64_t address)
{
    InstructionOperand dst = operand(ud_obj, 0, address);
    InstructionOperand src = operand(ud_obj, 1, address);
    
    if (ud_obj->operand[0].type == UD_OP_MEM) {
        instructions.push_back(new InstructionStore(address,
                                                    ud_insn_len(ud_obj),
                                                    ud_obj->operand[0].size,
                                                    dst,
                                                    src));
    }
    else if (ud_obj->operand[1].type == UD_OP_MEM) {
        instructions.push_back(new InstructionLoad(address,
                                                   ud_insn_len(ud_obj),
                                                   ud_obj->operand[1].size,
                                                   dst,
                                                   src));
    }
    else {
        instructions.push_back(new InstructionAssign(address, ud_insn_len(ud_obj), dst, src));
    }
}


void Translator :: movsxd (ud_t * ud_obj, uint64_t address)
{
    InstructionOperand dst = operand(ud_obj, 0, address);
    InstructionOperand src = operand(ud_obj, 1, address);
    
    instructions.push_back(new InstructionSignExtend(address, ud_insn_len(ud_obj), dst, src));
}


void Translator :: movzx (ud_t * ud_obj, uint64_t address)
{
    // conduct the move first
    mov(ud_obj, address);
 
    InstructionOperand dst = operand(ud_obj, 0, address);
    InstructionOperand toand(OPTYPE_CONSTANT, 8, 0xff);
    
    // x86 will put the byte in the lowest octet of the register, so AND it with 0xff
    instructions.push_back(new InstructionAnd(address, ud_insn_len(ud_obj), dst, dst, toand));
}


void Translator :: nop (ud_t * ud_obj, uint64_t address)
{
    InstructionOperand noperand(OPTYPE_TMPVAR, 1, "NOP");
    instructions.push_back(new InstructionAnd(address, ud_insn_len(ud_obj), noperand, noperand, noperand));
}


void Translator :: pop (ud_t * ud_obj, uint64_t address)
{
    size_t size;
    
    InstructionOperand dst     = operand(ud_obj, 0, address);
    InstructionOperand rsp     = InstructionOperand(OPTYPE_VAR, 64, UD_R_RSP);
    InstructionOperand addsize = InstructionOperand(OPTYPE_CONSTANT, 8, STACK_ELEMENT_SIZE);
    
    if (ud_obj->operand[0].type == UD_OP_MEM) throw std::runtime_error("popping into memory location not supported");
    
    if (ud_obj->operand[0].type == UD_OP_REG) size = register_bits(ud_obj->operand[0].base);
    else size = ud_obj->operand[0].size;
    
    instructions.push_back(new InstructionLoad(address, ud_insn_len(ud_obj), size, dst, rsp));
    instructions.push_back(new InstructionAdd(address, ud_insn_len(ud_obj), rsp, rsp, addsize));
}


void Translator :: push (ud_t * ud_obj, uint64_t address)
{
    size_t size;
    
    InstructionOperand src     = operand_get(ud_obj, 0, address);
    InstructionOperand rsp     = InstructionOperand(OPTYPE_VAR, 64, UD_R_RSP);
    InstructionOperand subsize = InstructionOperand(OPTYPE_CONSTANT, 8, STACK_ELEMENT_SIZE);
    
    if (ud_obj->operand[0].type == UD_OP_REG) size = register_bits(ud_obj->operand[0].base);
    else size = ud_obj->operand[0].size;
    
    instructions.push_back(new InstructionSub(address, ud_insn_len(ud_obj), rsp, rsp, subsize));
    instructions.push_back(new InstructionStore(address, ud_insn_len(ud_obj), size, rsp, src));
}


void Translator :: ret (ud_t * ud_obj, uint64_t address)
{
    instructions.push_back(new InstructionRet(address, ud_insn_len(ud_obj)));
}


void Translator :: sar (ud_t * ud_obj, uint64_t address)
{
    /*
     * sar family instructions are weird about operands, as a "sar <dst>, 1" instruction
     * will have an operand size of 0 and cause errors when we call operand_lval
     */
    InstructionOperand bits;
    if (ud_obj->operand[1].size == 0) bits = InstructionOperand(OPTYPE_CONSTANT, 1, 1);
    else                              bits = operand_get(ud_obj, 1, address);
    
    InstructionOperand dst        = operand_get(ud_obj, 0, address);
    InstructionOperand tmp          (OPTYPE_TMPVAR, dst.g_bits());
    InstructionOperand signPreserve (OPTYPE_CONSTANT, dst.g_bits(), 1 << (dst.g_bits() - 1));
    InstructionOperand one          (OPTYPE_CONSTANT, dst.g_bits(), 1);
    InstructionOperand zero         (OPTYPE_CONSTANT, dst.g_bits(), 0);
    
    int size = ud_insn_len(ud_obj);
    
    instructions.push_back(new InstructionShr(address, size, tmp, dst, bits));
    instructions.push_back(new InstructionAnd(address, size, tmp, signPreserve, dst));
    
    // CF takes last bit shifted out of dst
    InstructionOperand CF           (OPTYPE_TMPVAR, 1, "CF");
    InstructionOperand bitsMinusOne (OPTYPE_TMPVAR,   dst.g_bits());
    instructions.push_back(new InstructionSub(address, size, bitsMinusOne, bits, one));
    instructions.push_back(new InstructionShr(address, size, CF, dst, bitsMinusOne));
    
    // OF is cleared on 1-bit shifts, otherwise it's undefined
    InstructionOperand OF     (OPTYPE_TMPVAR, 1, "OF");
    InstructionOperand OFMask (OPTYPE_TMPVAR, 1);
    instructions.push_back(new InstructionCmpEq(address, size, OFMask, bits, one));
    instructions.push_back(new InstructionNot  (address, size, OFMask, OFMask));
    instructions.push_back(new InstructionAnd  (address, size, OF, OF, OFMask));
    
    InstructionOperand SF (OPTYPE_TMPVAR, 1, "SF");
    instructions.push_back(new InstructionCmpLts(address, size, SF, tmp, zero));
    
    InstructionOperand ZF (OPTYPE_TMPVAR, 1, "ZF");
    instructions.push_back(new InstructionCmpEq(address, size, ZF, tmp, zero));
    
    operand_set(ud_obj, 0, address, tmp);
}


void Translator :: shr (ud_t * ud_obj, uint64_t address)
{
    /*
     * sar family instructions are weird about operands, as a "sar <dst>, 1" instruction
     * will have an operand size of 0 and cause errors when we call operand_lval
     */
    InstructionOperand bits;
    if (ud_obj->operand[1].size == 0) bits = InstructionOperand(OPTYPE_CONSTANT, 1, 1);
    else                              bits = operand_get(ud_obj, 1, address);
    
    InstructionOperand dst        = operand_get(ud_obj, 0, address);
    InstructionOperand tmp          (OPTYPE_TMPVAR, dst.g_bits());
    InstructionOperand one          (OPTYPE_CONSTANT, dst.g_bits(), 1);
    InstructionOperand zero         (OPTYPE_CONSTANT, dst.g_bits(), 0);
    
    int size = ud_insn_len(ud_obj);
    
    instructions.push_back(new InstructionShr(address, size, tmp, dst, bits));
    
    // CF takes last bit shifted out of dst
    InstructionOperand CF           (OPTYPE_TMPVAR, 1, "CF");
    InstructionOperand bitsMinusOne (OPTYPE_TMPVAR,   dst.g_bits());
    instructions.push_back(new InstructionSub(address, size, bitsMinusOne, bits, one));
    instructions.push_back(new InstructionShr(address, size, CF, dst, bitsMinusOne));
    
    // OF is set to the MSB of the original operand
    InstructionOperand OF      (OPTYPE_TMPVAR, 1, "OF");
    InstructionOperand OFShift (OPTYPE_TMPVAR, 8, dst.g_bits() - 1);
    instructions.push_back(new InstructionShr  (address, size, OF, dst, OFShift));
    
    InstructionOperand SF (OPTYPE_TMPVAR, 1, "SF");
    instructions.push_back(new InstructionCmpLts(address, size, SF, tmp, zero));
    
    InstructionOperand ZF (OPTYPE_TMPVAR, 1, "ZF");
    instructions.push_back(new InstructionCmpEq(address, size, ZF, tmp, zero));
    
    operand_set(ud_obj, 0, address, tmp);
}


void Translator :: sub (ud_t * ud_obj, uint64_t address)
{

    InstructionOperand lhs = operand_get(ud_obj, 0, address);
    InstructionOperand rhs = operand_get(ud_obj, 1, address);
    InstructionOperand tmp (OPTYPE_TMPVAR, lhs.g_bits());
        
    InstructionOperand OFTmp    (OPTYPE_TMPVAR, lhs.g_bits());
    InstructionOperand OFTmpShl (OPTYPE_CONSTANT, lhs.g_bits(), lhs.g_bits() - 1);
    InstructionOperand OF       (OPTYPE_TMPVAR, 1, "OF"); // signed overflow
    InstructionOperand CF       (OPTYPE_TMPVAR, 1, "CF"); // unsigned overflow
    InstructionOperand ZF       (OPTYPE_TMPVAR, 1, "ZF");
    InstructionOperand SF       (OPTYPE_TMPVAR, 1, "SF"); // "negative" flag
    InstructionOperand zero     (OPTYPE_CONSTANT, tmp.g_bits(), 0);
    
    instructions.push_back(new InstructionSub(address, ud_insn_len(ud_obj), tmp, lhs, rhs));
    
    instructions.push_back(new InstructionXor(address, ud_insn_len(ud_obj), OFTmp, tmp,   lhs));
    instructions.push_back(new InstructionShr(address, ud_insn_len(ud_obj), OF,    OFTmp, OFTmpShl));
    
    instructions.push_back(new InstructionCmpLtu(address, ud_insn_len(ud_obj), CF, lhs, tmp));
    instructions.push_back(new InstructionCmpEq (address, ud_insn_len(ud_obj), ZF, tmp, zero));
    instructions.push_back(new InstructionCmpLts(address, ud_insn_len(ud_obj), SF, tmp, zero));
    
    operand_set(ud_obj, 0, address, tmp);
}


void Translator :: test (ud_t * ud_obj, uint64_t address)
{
    InstructionOperand lhs = operand_get(ud_obj, 0, address);
    InstructionOperand rhs = operand_get(ud_obj, 1, address);
    InstructionOperand tmp   (OPTYPE_TMPVAR, lhs.g_bits());
    
    InstructionOperand OF       (OPTYPE_TMPVAR, 1, "OF"); // signed overflow
    InstructionOperand CF       (OPTYPE_TMPVAR, 1, "CF"); // unsigned overflow
    InstructionOperand ZF       (OPTYPE_TMPVAR, 1, "ZF");
    InstructionOperand SF       (OPTYPE_TMPVAR, 1, "SF"); // "negative" flag
    InstructionOperand zero     (OPTYPE_CONSTANT, tmp.g_bits(), 0);
    
    instructions.push_back(new InstructionAnd(address, ud_insn_len(ud_obj), tmp, lhs, rhs));
    
    instructions.push_back(new InstructionCmpEq(address, ud_insn_len(ud_obj), ZF, tmp, zero));
    instructions.push_back(new InstructionCmpLts(address, ud_insn_len(ud_obj), SF, tmp, zero));
    instructions.push_back(new InstructionAssign(address, ud_insn_len(ud_obj), OF, zero));
    instructions.push_back(new InstructionAssign(address, ud_insn_len(ud_obj), CF, zero));
}


void Translator :: Xor (ud_t * ud_obj, uint64_t address)
{
    InstructionOperand dst(operand(ud_obj, 0, address));
    InstructionOperand src(operand_get(ud_obj, 1, address));
    
    if ((ud_obj->operand[0].type == UD_OP_MEM) || (ud_obj->operand[1].type == UD_OP_MEM))
        throw std::runtime_error("mem xor operations not supported");

    instructions.push_back(new InstructionXor(address, ud_insn_len(ud_obj), dst, dst, src));
}
