/*
    Copyright 2012 Alex Eubanks (endeavor[at]rainbowsandpwnies.com)

    This file is part of rnp_see ( http://github.com/endeav0r/rnp_see/ )

    rnp_see is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "vm.h"

#include <iostream>
#include <sstream>
#include <stdexcept>

#include "kernel.h"

#define DEBUG

void VM :: debug_x86_registers ()
{
    std::stringstream ss;

    #define GVALUE(XX) variables[InstructionOperand::str_to_id(XX)].str()
    #define PRINTREG(XX) << XX << "=" \
                         << std::hex << GVALUE(XX) << std::endl
    
    ss PRINTREG("UD_R_RAX")  PRINTREG("UD_R_RBX")
       PRINTREG("UD_R_RCX")  PRINTREG("UD_R_RDX")
       PRINTREG("UD_R_RDI")  PRINTREG("UD_R_RSI")
       PRINTREG("UD_R_RBP")  PRINTREG("UD_R_RSP")
       PRINTREG("UD_R_R8")   PRINTREG("UD_R_R9")
       PRINTREG("UD_R_R10")  PRINTREG("UD_R_R11") 
       PRINTREG("UD_R_R12")  PRINTREG("UD_R_R13")
       PRINTREG("UD_R_R14")  PRINTREG("UD_R_R15")
       PRINTREG("UD_R_RIP")  PRINTREG("ZF")
       PRINTREG("OF")        PRINTREG("CF")
       PRINTREG("UD_R_XMM0") PRINTREG("UD_R_XMM1")
       PRINTREG("UD_R_XMM2") PRINTREG("UD_R_XMM3")
       PRINTREG("SF")        PRINTREG("UD_R_FS");
    std::cout << ss.str();
}

void VM :: debug_variables ()
{
    std::stringstream ss;

    std::map <uint64_t, SymbolicValue> :: iterator it;
    for (it = variables.begin(); it != variables.end(); it++) {
        ss << std::hex << it->first << "=" 
           << it->second.str() << std::endl;
    }

    std::cout << ss.str();
}

SymbolicValue VM :: g_variable (uint64_t identifier)
{
    return variables[identifier];
}

const SymbolicValue VM :: g_value (InstructionOperand operand)
{
    if (operand.g_type() == OPTYPE_CONSTANT)
        return SymbolicValue(operand.g_bits(), operand.g_value());

    if (variables.count(operand.g_id()) == 0) {
        std::stringstream ss;
        ss << "operand not found, id: " << std::hex << operand.g_id();
        throw std::runtime_error(ss.str());
    }
    
    return variables[operand.g_id()].extend(operand.g_bits());
}


void VM :: init ()
{
    #ifdef DEBUG
    std::cerr << "loading translator" << std::endl;
    #endif    
    translator = Translator();

    #ifdef DEBUG
    std::cerr << "getting memory" << std::endl;
    #endif
    memory     = loader->g_memory();

    #ifdef DEBUG
    std::cerr << "getting variables" << std::endl;
    #endif
    variables  = loader->g_variables();

    #ifdef DEBUG
    std::cerr << "getting instruction pointer id" << std::endl;
    #endif
    ip_id      = loader->g_ip_id();

    //std::cout << "Memory mmap: " << std::endl << memory.memmap() << std::endl;
}


VM :: VM (Loader * loader)
{
    this->engine = NULL;
    this->loader = loader;
    delete_loader = false;

    init();
}


VM :: VM (Loader * loader, Engine * engine)
{
    this->engine = engine;
    this->loader = loader;
    delete_loader = false;

    init();
}


VM :: VM (Loader * loader, bool delete_loader)
{
    this->engine = NULL;
    this->loader = loader;
    this->delete_loader = delete_loader;

    init();
}


VM :: VM (Loader * loader,
          std::list <std::pair<SymbolicValue, SymbolicValue>> assertions)
{
    this->engine = NULL;
    this->loader = loader;
    this->delete_loader = false;
    this->assertions = assertions;

    init();
}


VM :: ~VM ()
{
    if (delete_loader == true) {
        //delete loader;
    }
    memory.destroy();
}


void VM :: copy (VM & rhs)
{
    ip_id         = rhs.ip_id;
    loader        = rhs.loader;
    kernel        = rhs.kernel;
    translator    = rhs.translator;
    delete_loader = false;
    variables     = rhs.variables;
    memory        = rhs.memory.copy();
    engine        = rhs.engine;
}


VM * VM :: new_copy ()
{
    VM * child = new VM();

    child->ip_id         = ip_id;
    child->loader        = loader;
    child->kernel        = kernel;
    child->translator    = translator;
    child->delete_loader = false;
    child->variables     = variables;
    child->memory        = memory.copy();
    child->engine        = engine;

    return child;   
}


void VM :: step ()
{
    uint64_t ip_addr = variables[ip_id].g_uint64();
    std::list <Instruction *> instructions;

    // if there is a symbol name for this location, print it out
    // this code is very slow
    std::string symbol_name = loader->func_symbol(ip_addr);
    if (symbol_name != "")
        std::cout << std::hex << ip_addr 
                  << "SYMBOL: " << symbol_name << " :" << std::endl;

    instructions = translator.translate(ip_addr,
                                        memory.g_data(ip_addr),
                                        memory.g_data_size(ip_addr));

    size_t instruction_size = instructions.front()->g_size();

    #ifdef DEBUG
        std::cout << "step IP=" << std::hex << ip_addr
                 << " " << translator.native_asm((uint8_t *) memory.g_data(ip_addr), instruction_size);
        for (size_t i = 0; i < instruction_size; i++) {
            std::cout << " " << std::hex << (int) memory.g_byte(ip_addr + i);
        }
        std::cout << std::endl;
    #endif

    variables[ip_id] = variables[ip_id] + SymbolicValue(64, instruction_size);

    #define EXECUTE(XX) if (dynamic_cast<XX *>(*it)) \
                            execute(dynamic_cast<XX *>(*it));

    std::list <Instruction *> :: iterator it;
    for (it = instructions.begin(); it != instructions.end(); it++) {
        #ifdef DEBUG
            //std::cout << (*it)->str() << std::endl;
        #endif
             EXECUTE(InstructionAdd)
        else EXECUTE(InstructionAnd)
        else EXECUTE(InstructionAssign)
        else EXECUTE(InstructionBrc)
        else EXECUTE(InstructionCmpEq)
        else EXECUTE(InstructionCmpLes)
        else EXECUTE(InstructionCmpLeu)
        else EXECUTE(InstructionCmpLts)
        else EXECUTE(InstructionCmpLtu)
        else EXECUTE(InstructionDiv)
        else EXECUTE(InstructionHlt)
        else EXECUTE(InstructionLoad)
        else EXECUTE(InstructionNot)
        else EXECUTE(InstructionMod)
        else EXECUTE(InstructionMul)
        else EXECUTE(InstructionOr)
        else EXECUTE(InstructionShl)
        else EXECUTE(InstructionShr)
        else EXECUTE(InstructionSignExtend)
        else EXECUTE(InstructionStore)
        else EXECUTE(InstructionSub)
        else EXECUTE(InstructionSyscall)
        else EXECUTE(InstructionXor)
        else throw std::runtime_error("unimplemented vm instruction: " + (*it)->str());

        delete *it;
    }
}


void VM :: execute (InstructionAdd * add)
{
    variables[add->g_dst().g_id()] = (g_value(add->g_lhs())
                                      + g_value(add->g_rhs())).extend(add->g_dst().g_bits());
}


void VM :: execute (InstructionAnd * And)
{
    variables[And->g_dst().g_id()] = (g_value(And->g_lhs())
                                      & g_value(And->g_rhs())).extend(And->g_dst().g_bits());
}


void VM :: execute (InstructionAssign * assign)
{
    variables[assign->g_dst().g_id()] = g_value(assign->g_src()).extend(assign->g_dst().g_bits());
}


void VM :: execute (InstructionBrc * brc)
{
    const SymbolicValue condition = g_value(brc->g_cond());
    if (condition.g_wild()) {
        // we need engine to be set in order to handle wild conditions
        if (engine == NULL)
            throw std::runtime_error("wild condition called on VM with no Engine");

        #ifdef DEBUG
            std::cerr << "wild condition: " << condition.str() << std::endl;
        #endif

        bool condition_true  = false;
        bool condition_false = false;
        
        if (condition.sv_assert(SymbolicValue(1, 1), assertions))
            condition_true  = true;
        if (condition.sv_assert(SymbolicValue(1, 0), assertions))
            condition_false = true;


        // if both conditions possible, we'll make a copy for the false branch
        // and set the assertion for the true branch for this VM
        if (condition_true && condition_false) {
            std::cout << "condition_true && condition_false" << std::endl;
            VM * newvm = new_copy();
            std::pair <SymbolicValue, SymbolicValue>
                assert_false(condition, SymbolicValue(1, 0));
            newvm->assertions.push_back(assert_false);
            engine->push_vm(newvm);
        }
        else if (condition_false) {
            std::cout << "condition_false" << std::endl;
            std::pair <SymbolicValue, SymbolicValue>
                assert_false(condition, SymbolicValue(1, 0));
            assertions.push_back(assert_false);
        }
        else
            std::cout << "condition_true" << std::endl;
        if (condition_true) {
            std::pair <SymbolicValue, SymbolicValue>
                assert_true(condition, SymbolicValue(1, 1));
            assertions.push_back(assert_true);
            variables[ip_id] = g_value(brc->g_dst()).extend(variables[ip_id].g_bits());
        }
    }
    else if (condition.g_uint64()) {
        variables[ip_id] = g_value(brc->g_dst()).extend(variables[ip_id].g_bits());
    }
}


void VM :: execute (InstructionCmpEq * cmpeq)
{
    SymbolicValue cmp = g_value(cmpeq->g_lhs()) == g_value(cmpeq->g_rhs());
    variables[cmpeq->g_dst().g_id()] = cmp.extend(cmpeq->g_dst().g_bits());
}


void VM :: execute (InstructionCmpLes * cmples)
{
    SymbolicValue cmp = g_value(cmples->g_lhs()).cmpLes(g_value(cmples->g_rhs()));
    variables[cmples->g_dst().g_id()] = cmp.extend(cmples->g_dst().g_bits());
}


void VM :: execute (InstructionCmpLeu * cmpleu)
{
    SymbolicValue cmp = g_value(cmpleu->g_lhs()).cmpLeu(g_value(cmpleu->g_rhs()));
    variables[cmpleu->g_dst().g_id()] = cmp.extend(cmpleu->g_dst().g_bits());
}


void VM :: execute (InstructionCmpLts * cmplts)
{
    SymbolicValue cmp = g_value(cmplts->g_lhs()).cmpLts(g_value(cmplts->g_rhs()));
    variables[cmplts->g_dst().g_id()] = cmp.extend(cmplts->g_dst().g_bits());
}


void VM :: execute (InstructionCmpLtu * cmpltu)
{
    SymbolicValue cmp = g_value(cmpltu->g_lhs()).cmpLtu(g_value(cmpltu->g_rhs()));
    variables[cmpltu->g_dst().g_id()] = cmp.extend(cmpltu->g_dst().g_bits());
}


void VM :: execute (InstructionDiv * div)
{
    variables[div->g_dst().g_id()] = (g_value(div->g_lhs())
                                      / g_value(div->g_rhs())).extend(div->g_dst().g_bits());
}


void VM :: execute (InstructionHlt * hlt)
{
    engine->remove_vm(this);
}


void VM :: execute (InstructionLoad * load)
{
    InstructionOperand dst = load->g_dst();
    const SymbolicValue src = g_value(load->g_src());

    switch (load->g_bits()) {
    case 8 :
        variables[dst.g_id()] = memory.g_sym8(src.g_uint64()); break;
    case 16 :
        variables[dst.g_id()] = memory.g_sym16(src.g_uint64()); break;
    case 32 :
        variables[dst.g_id()] = memory.g_sym32(src.g_uint64()); break;
    case 64 :
        variables[dst.g_id()] = memory.g_sym64(src.g_uint64()); break;
    default :
        std::stringstream ss;
        ss << "Tried to load invalid bit size: " << load->g_bits();
        throw std::runtime_error(ss.str());
    }

    #ifdef DEBUG
    std::cout << "loadloc [" << std::hex << src.g_uint64() << "] = "
              << variables[dst.g_id()].str() << std::endl;
    #endif
}


void VM :: execute (InstructionMod * mod)
{
    variables[mod->g_dst().g_id()] = (g_value(mod->g_lhs())
                                      % g_value(mod->g_rhs())).extend(mod->g_dst().g_bits());
}


void VM :: execute (InstructionMul * mul)
{
    variables[mul->g_dst().g_id()] = (g_value(mul->g_lhs()) 
                                      * g_value(mul->g_rhs())).extend(mul->g_dst().g_bits());
}


void VM :: execute (InstructionNot * Not)
{
    variables[Not->g_dst().g_id()] = (~ g_value(Not->g_src())).extend(Not->g_dst().g_bits());
}


void VM :: execute (InstructionOr * Or)
{
    variables[Or->g_dst().g_id()] = (g_value(Or->g_lhs()) 
                                     | g_value(Or->g_rhs())).extend(Or->g_dst().g_bits());
}


void VM :: execute (InstructionShl * shl)
{
    variables[shl->g_dst().g_id()] = (g_value(shl->g_lhs())
                                      << g_value(shl->g_rhs())).extend(shl->g_dst().g_bits());
}


void VM :: execute (InstructionShr * shr)
{
    variables[shr->g_dst().g_id()] = (g_value(shr->g_lhs())
                                      >> g_value(shr->g_rhs())).extend(shr->g_dst().g_bits());
}


void VM :: execute (InstructionSignExtend * sext)
{
    // force src variable to be the src size
    SymbolicValue src = g_value(sext->g_src()).extend(sext->g_src().g_bits());
    // now sign extend this value to the dst's size
    const SymbolicValue dst = src.signExtend(sext->g_dst().g_bits());
    variables[sext->g_dst().g_id()] = dst;
}


void VM :: execute (InstructionStore * store)
{
    const SymbolicValue dst = g_value(store->g_dst());
    const SymbolicValue src = g_value(store->g_src());

    #ifdef DEBUG
    std::cout << "storeloc: " << std::hex << dst.str() << " = "
              << src.str() << std::endl;
    #endif

    switch (store->g_bits()) {
    case 8  : memory.s_sym8(dst.g_uint64(), src); break;
    case 16 : memory.s_word(dst.g_uint64(), src.g_uint64()); break;
    case 32 : memory.s_sym32(dst.g_uint64(), src); break;
    case 64 : memory.s_sym64(dst.g_uint64(), src); break;
    default :
        std::stringstream ss;
        ss << "Tried to store invalid bit size: " << store->g_bits();
        throw std::runtime_error(ss.str());
    }
}


void VM :: execute (InstructionSub * sub)
{
    variables[sub->g_dst().g_id()] = (g_value(sub->g_lhs())
                                      - g_value(sub->g_rhs())).extend(sub->g_dst().g_bits());
}


void VM :: execute (InstructionSyscall * syscall)
{
    kernel.syscall(variables, memory);
}


void VM :: execute (InstructionXor * Xor)
{
    variables[Xor->g_dst().g_id()] = (g_value(Xor->g_lhs())
                                      ^ g_value(Xor->g_rhs())).extend(Xor->g_dst().g_bits());
}

void VM :: execute (Instruction * ins) {}