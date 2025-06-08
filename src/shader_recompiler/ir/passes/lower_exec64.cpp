// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/info.h"
#include "shader_recompiler/ir/basic_block.h"
#include "shader_recompiler/ir/ir_emitter.h"
#include "shader_recompiler/ir/program.h"

#include <magic_enum/magic_enum.hpp>

namespace Shader::Optimization {

constexpr s32 F64ToF32Exp = +1023 - 127;
constexpr s32 F32ToF64Exp = +127 - 1023;

// static IR::F32 PackedF64ToF32(IR::IREmitter& ir, const IR::Value& packed) {
//     const IR::U32 lo{ir.CompositeExtract(packed, 0)};
//     const IR::U32 hi{ir.CompositeExtract(packed, 1)};
//     const IR::U32 sign{ir.BitFieldExtract(hi, ir.Imm32(31), ir.Imm32(1))};
//     const IR::U32 exp{ir.BitFieldExtract(hi, ir.Imm32(20), ir.Imm32(11))};
//     const IR::U32 mantissa_hi{ir.BitFieldExtract(hi, ir.Imm32(0), ir.Imm32(20))};
//     const IR::U32 mantissa_lo{ir.BitFieldExtract(lo, ir.Imm32(29), ir.Imm32(3))};
//     const IR::U32 mantissa{
//         ir.BitwiseOr(ir.ShiftLeftLogical(mantissa_hi, ir.Imm32(3)), mantissa_lo)};
//     const IR::U32 exp_if_subnorm{
//         ir.Select(ir.IEqual(exp, ir.Imm32(0)), ir.Imm32(0), ir.IAdd(exp, ir.Imm32(F64ToF32Exp)))};
//     const IR::U32 exp_if_infnan{
//         ir.Select(ir.IEqual(exp, ir.Imm32(0x7ff)), ir.Imm32(0xff), exp_if_subnorm)};
//     const IR::U32 result{
//         ir.BitwiseOr(ir.ShiftLeftLogical(sign, ir.Imm32(31)),
//                      ir.BitwiseOr(ir.ShiftLeftLogical(exp_if_infnan, ir.Imm32(23)), mantissa))};
//     return ir.BitCast<IR::F32>(result);
// }

// IR::Value F32ToPackedF64(IR::IREmitter& ir, const IR::Value& raw) {
//     const IR::U32 value{ir.BitCast<IR::U32>(IR::F32(raw))};
//     const IR::U32 sign{ir.BitFieldExtract(value, ir.Imm32(31), ir.Imm32(1))};
//     const IR::U32 exp{ir.BitFieldExtract(value, ir.Imm32(23), ir.Imm32(8))};
//     const IR::U32 mantissa{ir.BitFieldExtract(value, ir.Imm32(0), ir.Imm32(23))};
//     const IR::U32 mantissa_hi{ir.BitFieldExtract(mantissa, ir.Imm32(3), ir.Imm32(20))};
//     const IR::U32 mantissa_lo{ir.BitFieldExtract(mantissa, ir.Imm32(0), ir.Imm32(3))};
//     const IR::U32 exp_if_subnorm{
//         ir.Select(ir.IEqual(exp, ir.Imm32(0)), ir.Imm32(0), ir.IAdd(exp, ir.Imm32(F32ToF64Exp)))};
//     const IR::U32 exp_if_infnan{
//         ir.Select(ir.IEqual(exp, ir.Imm32(0xff)), ir.Imm32(0x7ff), exp_if_subnorm)};
//     const IR::U32 lo{ir.ShiftLeftLogical(mantissa_lo, ir.Imm32(29))};
//     const IR::U32 hi{
//         ir.BitwiseOr(ir.ShiftLeftLogical(sign, ir.Imm32(31)),
//                      ir.BitwiseOr(ir.ShiftLeftLogical(exp_if_infnan, ir.Imm32(20)), mantissa_hi))};
//     return ir.CompositeConstruct(lo, hi);
// }

static IR::Opcode Replace(IR::Opcode op) {
    switch (op) {
    case IR::Opcode::CompositeConstructF64x2:
        return IR::Opcode::CompositeConstructF32x2;
    case IR::Opcode::CompositeConstructF64x3:
        return IR::Opcode::CompositeConstructF32x3;
    case IR::Opcode::CompositeConstructF64x4:
        return IR::Opcode::CompositeConstructF32x4;
    case IR::Opcode::CompositeExtractF64x2:
        return IR::Opcode::CompositeExtractF32x2;
    case IR::Opcode::CompositeExtractF64x3:
        return IR::Opcode::CompositeExtractF32x3;
    case IR::Opcode::CompositeExtractF64x4:
        return IR::Opcode::CompositeExtractF32x4;
    case IR::Opcode::CompositeInsertF64x2:
        return IR::Opcode::CompositeInsertF32x2;
    case IR::Opcode::CompositeInsertF64x3:
        return IR::Opcode::CompositeInsertF32x3;
    case IR::Opcode::CompositeInsertF64x4:
        return IR::Opcode::CompositeInsertF32x4;
    case IR::Opcode::CompositeShuffleF64x2:
        return IR::Opcode::CompositeShuffleF32x2;
    case IR::Opcode::CompositeShuffleF64x3:
        return IR::Opcode::CompositeShuffleF32x3;
    case IR::Opcode::CompositeShuffleF64x4:
        return IR::Opcode::CompositeShuffleF32x4;
    case IR::Opcode::SelectF64:
        return IR::Opcode::SelectF64;
    case IR::Opcode::FPAbs64:
        return IR::Opcode::FPAbs32;
    case IR::Opcode::FPAdd64:
        return IR::Opcode::FPAdd32;
    case IR::Opcode::FPFma64:
        return IR::Opcode::FPFma32;
    case IR::Opcode::FPMax64:
        return IR::Opcode::FPMax32;
    case IR::Opcode::FPMin64:
        return IR::Opcode::FPMin32;
    case IR::Opcode::FPMul64:
        return IR::Opcode::FPMul32;
    case IR::Opcode::FPDiv64:
        return IR::Opcode::FPDiv32;
    case IR::Opcode::FPNeg64:
        return IR::Opcode::FPNeg32;
    case IR::Opcode::FPRecip64:
        return IR::Opcode::FPRecip32;
    case IR::Opcode::FPRecipSqrt64:
        return IR::Opcode::FPRecipSqrt32;
    case IR::Opcode::FPSaturate64:
        return IR::Opcode::FPSaturate32;
    case IR::Opcode::FPClamp64:
        return IR::Opcode::FPClamp32;
    case IR::Opcode::FPRoundEven64:
        return IR::Opcode::FPRoundEven32;
    case IR::Opcode::FPFloor64:
        return IR::Opcode::FPFloor32;
    case IR::Opcode::FPCeil64:
        return IR::Opcode::FPCeil32;
    case IR::Opcode::FPTrunc64:
        return IR::Opcode::FPTrunc32;
    case IR::Opcode::FPFract64:
        return IR::Opcode::FPFract32;
    case IR::Opcode::FPFrexpSig64:
        return IR::Opcode::FPFrexpSig32;
    case IR::Opcode::FPFrexpExp64:
        return IR::Opcode::FPFrexpExp32;
    case IR::Opcode::FPOrdEqual64:
        return IR::Opcode::FPOrdEqual32;
    case IR::Opcode::FPUnordEqual64:
        return IR::Opcode::FPUnordEqual32;
    case IR::Opcode::FPOrdNotEqual64:
        return IR::Opcode::FPOrdNotEqual32;
    case IR::Opcode::FPUnordNotEqual64:
        return IR::Opcode::FPUnordNotEqual32;
    case IR::Opcode::FPOrdLessThan64:
        return IR::Opcode::FPOrdLessThan32;
    case IR::Opcode::FPUnordLessThan64:
        return IR::Opcode::FPUnordLessThan32;
    case IR::Opcode::FPOrdGreaterThan64:
        return IR::Opcode::FPOrdGreaterThan32;
    case IR::Opcode::FPUnordGreaterThan64:
        return IR::Opcode::FPUnordGreaterThan32;
    case IR::Opcode::FPOrdLessThanEqual64:
        return IR::Opcode::FPOrdLessThanEqual32;
    case IR::Opcode::FPUnordLessThanEqual64:
        return IR::Opcode::FPUnordLessThanEqual32;
    case IR::Opcode::FPOrdGreaterThanEqual64:
        return IR::Opcode::FPOrdGreaterThanEqual32;
    case IR::Opcode::FPUnordGreaterThanEqual64:
        return IR::Opcode::FPUnordGreaterThanEqual32;
    case IR::Opcode::FPIsNan64:
        return IR::Opcode::FPIsNan32;
    case IR::Opcode::FPIsInf64:
        return IR::Opcode::FPIsInf32;
    case IR::Opcode::ConvertS32F64:
        return IR::Opcode::ConvertS32F32;
    case IR::Opcode::ConvertF32F64:
        return IR::Opcode::Identity;
    case IR::Opcode::ConvertF64F32:
        return IR::Opcode::Identity;
    case IR::Opcode::ConvertF64S32:
        return IR::Opcode::ConvertF32S32;
    case IR::Opcode::ConvertF64U32:
        return IR::Opcode::ConvertF32U32;
    default:
        return op;
    }
}

static IR::Inst* GetSrc(IR::Inst& inst) {
    switch (inst.GetOpcode()) {
    case IR::Opcode::GetScalarRegister:
        return &inst;
    case IR::Opcode::GetExec64:
        return &inst;
    case IR::Opcode::GetVccLo:
        return &inst;
    default:
        break;
    }

    if (inst.NumArgs() > 0) {
        const auto arg = inst.Arg(0).InstRecursive();
        return GetSrc(*arg);
    }
    
    UNREACHABLE_MSG("unhandled argless opcode: {}", inst.GetOpcode());
}

static void Replace1Bit(IR::Inst& inst) {
    LOG_DEBUG(Render_Recompiler, "top down {}", inst.GetOpcode());
    switch (inst.GetOpcode()) {
    case IR::Opcode::PackUint2x32: {
        Replace1Bit(*inst.Arg(0).InstRecursive());
        inst.ReplaceUsesWithAndRemove(inst.Arg(0));
        break;
    }
    case IR::Opcode::UnpackUint2x32: {
        Replace1Bit(*inst.Arg(0).InstRecursive());
        inst.ReplaceUsesWithAndRemove(inst.Arg(0));
        break;
    }
    case IR::Opcode::CompositeConstructU32x2: {
        Replace1Bit(*inst.Arg(0).InstRecursive());
        inst.ReplaceUsesWithAndRemove(inst.Arg(0));
        break;
    }
    case IR::Opcode::CompositeExtractU32x2: {
        Replace1Bit(*inst.Arg(0).InstRecursive());
        inst.ReplaceUsesWithAndRemove(inst.Arg(0));
        break;
    }
    case IR::Opcode::GetScalarRegister: {
        inst.ReplaceOpcode(IR::Opcode::GetThreadBitScalarReg);
        break;
    }
    case IR::Opcode::BitwiseAnd64: {
        Replace1Bit(*inst.Arg(0).InstRecursive());
        Replace1Bit(*inst.Arg(1).InstRecursive());
        inst.ReplaceOpcode(IR::Opcode::LogicalAnd);
        break;
    }
    case IR::Opcode::BitwiseNot64: {
        Replace1Bit(*inst.Arg(0).InstRecursive());
        inst.ReplaceOpcode(IR::Opcode::LogicalNot);
        break;
    }
    case IR::Opcode::GetVccLo: {
        inst.ReplaceOpcode(IR::Opcode::GetVcc);
        break;
    }
    case IR::Opcode::GetExec64: {
        inst.ReplaceOpcode(IR::Opcode::GetExec);
        break;
    }
    default:
        LOG_DEBUG(Render_Recompiler, "replace {}", inst.GetOpcode());
        // for (auto i = 0; i < inst.NumArgs(); ++i) {
        //     LOG_DEBUG(Render_Recompiler, "arg {}", inst.Arg(0).InstRecursive().GetOpcode())
        // }
        break;
    }
}

static bool Up1Bit(IR::Inst& inst, u32 operand) {
    LOG_DEBUG(Render_Recompiler, "top up {}, op {}", inst.GetOpcode(), operand);

    switch (inst.GetOpcode()) {
    case IR::Opcode::UnpackUint2x32: {
        LOG_DEBUG(Render_Recompiler, "up {}", inst.GetOpcode());
        for (auto& use : inst.Uses()) {
            Up1Bit(*use.user, use.operand);
        }
        inst.ReplaceUsesWith(inst.Arg(0));
        break;
    }
    case IR::Opcode::CompositeExtractU32x2: {
        LOG_DEBUG(Render_Recompiler, "up {}", inst.GetOpcode());
        for (auto& use : inst.Uses()) {
            Up1Bit(*use.user, use.operand);
        }
        inst.ReplaceUsesWith(inst.Arg(0));
        break;
    }
    case IR::Opcode::BitwiseAnd64: {
        for (auto& use : inst.Uses()) {
            Up1Bit(*use.user, use.operand);
        }
        inst.ReplaceOpcode(IR::Opcode::LogicalAnd);
        break;
    }
    case IR::Opcode::BitwiseOr64: {
        for (auto& use : inst.Uses()) {
            Up1Bit(*use.user, use.operand);
        }
        inst.ReplaceOpcode(IR::Opcode::LogicalOr);
        break;
    }
    case IR::Opcode::INotEqual64: {
        LOG_DEBUG(Render_Recompiler, "up {} op {}", inst.GetOpcode(), operand);
        if (inst.Arg(operand ? 0 : 1).IsImmediate()) {
            inst.SetArg(operand ? 0 : 1, IR::Value((inst.Arg(operand ? 0 : 1).U64() & 1) == 1));
        }
        // Replace1Bit(*inst.Arg(operand ? 0 : 1).InstRecursive());
        // for (auto& use : inst.Uses()) {
        //     Up1Bit(*use.user, use.operand);
        // }
        inst.ReplaceOpcode(IR::Opcode::LogicalXor);
        break;
    }
    case IR::Opcode::SetScalarRegister: {
        inst.ReplaceOpcode(IR::Opcode::SetThreadBitScalarReg);
        break;
    }
    case IR::Opcode::SetVccLo: {
        Replace1Bit(*inst.Arg(0).InstRecursive());
        inst.ReplaceOpcode(IR::Opcode::SetVcc);
        break;
    }
    default:
        LOG_DEBUG(Render_Recompiler, "up {}", inst.GetOpcode());
        // for (auto i = 0; i < inst.NumArgs(); ++i) {
        //     LOG_DEBUG(Render_Recompiler, "arg {}", inst.Arg(0).InstRecursive().GetOpcode())
        // }
        break;
    }
    return false;
}

static void Lower(IR::Block& block, IR::Inst& inst) {
    switch (inst.GetOpcode()) {
    case IR::Opcode::SetExec64: {
        const auto arg = inst.Arg(0);
        if (arg.IsImmediate()) {
            inst.SetArg(0, IR::Value((arg.U64() & 1) == 1));
        } else {
            Replace1Bit(*inst.Arg(0).InstRecursive());
        }
        inst.ReplaceOpcode(IR::Opcode::SetExec);
        break;
    }
    default:
        break;
    }
}

static void Upper(IR::Block& block, IR::Inst& inst) {
    switch (inst.GetOpcode()) {
    case IR::Opcode::GetExec64: {
        for (auto& use : inst.Uses()) {
            Up1Bit(*use.user, use.operand);
        }
        inst.ReplaceOpcode(IR::Opcode::GetExec);
        break;
    }
    default:
        break;
    }
}

void LowerExec64(IR::Program& program) {
    for (IR::Block* const block : program.blocks) {
        for (IR::Inst& inst : block->Instructions()) {
            Lower(*block, inst);
        }
    }
    for (IR::Block* const block : program.blocks) {
        for (IR::Inst& inst : block->Instructions()) {
            Upper(*block, inst);
        }
    }
}

} // namespace Shader::Optimization
