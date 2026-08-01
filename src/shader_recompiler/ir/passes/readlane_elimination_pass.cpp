// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <unordered_map>
#include <queue>
#include "shader_recompiler/ir/program.h"

namespace Shader::Optimization {

static IR::Inst* SearchChain(IR::Inst* inst, u32 lane) {
    while (inst->GetOpcode() == IR::Opcode::WriteLane) {
        if (inst->Arg(2).U32() == lane) {
            // We found a possible write lane source, return it.
            return inst;
        }
        inst = inst->Arg(0).InstRecursive();
    }
    return inst;
}

static bool IsPossibleToEliminate(IR::Inst* inst, u32 lane) {
    // Breadth-first search visiting the right most arguments first
    boost::container::small_vector<IR::Inst*, 16> visited;
    std::queue<IR::Inst*> queue;
    queue.push(inst);

    while (!queue.empty()) {
        // Pop one instruction from the queue
        IR::Inst* inst{queue.front()};
        queue.pop();

        // If it's a WriteLane search for possible candidates
        if (inst = SearchChain(inst, lane); inst->GetOpcode() == IR::Opcode::WriteLane) {
            // We found a possible write lane source, stop looking here.
            continue;
        }
        // If there are other instructions in-between that use the value we can't eliminate.
        if (inst->GetOpcode() != IR::Opcode::ReadLane && inst->GetOpcode() != IR::Opcode::Phi) {
            return false;
        }
        // Visit the right most arguments first
        for (size_t arg = inst->NumArgs(); arg--;) {
            auto arg_value{inst->Arg(arg)};
            if (arg_value.IsImmediate()) {
                continue;
            }
            // Queue instruction if it hasn't been visited
            IR::Inst* arg_inst{arg_value.InstRecursive()};
            if (std::ranges::find(visited, arg_inst) == visited.end()) {
                visited.push_back(arg_inst);
                queue.push(arg_inst);
            }
        }
    }
    return true;
}

using PhiMap = std::unordered_map<IR::Inst*, IR::Inst*>;
using NewPhiList = boost::container::small_vector<IR::Inst*, 16>;

// Replaces a phi that merges a single real value with that value and erases it. Real
// arguments are the unique arguments of the phi, excluding the ones pointing back at itself,
// which the cycle breaking in GetRealValue introduces. Must only run once the phi tree is
// fully built, so that every reference to the phi is a registered use.
static void TryRemoveTrivialPhi(IR::Inst* phi) {
    if (phi->GetOpcode() != IR::Opcode::Phi) {
        // Already removed by an earlier cascade.
        return;
    }
    const IR::Value self{phi};
    IR::Value same{};
    for (size_t arg_index = 0; arg_index < phi->NumArgs(); arg_index++) {
        const IR::Value arg = phi->Arg(arg_index).Resolve();
        if (arg == self || arg == same) {
            // Self reference or a repeat of an argument we already counted.
            continue;
        }
        if (!same.IsEmpty()) {
            // The phi merges more than one real value, keep it.
            return;
        }
        same = arg;
    }
    if (same.IsEmpty()) {
        // Degenerate phi without any real argument, leave it alone.
        return;
    }
    // Copy since ReplaceUsesWithAndRemove will rewrite the users.
    const auto users = phi->Uses();
    phi->ReplaceUsesWithAndRemove(same);
    phi->GetParent()->Instructions().erase(IR::Block::InstructionList::s_iterator_to(*phi));
    // Rewriting a user may have turned one of its arguments into a self reference.
    for (const auto& [user, arg_index] : users) {
        if (user != phi) {
            TryRemoveTrivialPhi(user);
        }
    }
}

static IR::Value GetRealValue(PhiMap& phi_map, NewPhiList& new_phis, IR::Inst* inst, u32 lane) {
    // If this is a WriteLane op search the chain for a possible candidate. The value is
    // resolved as Use/UndoUse key on the direct instruction while being gated on
    // IsImmediate, which sees through identities, so an identity must never be stored.
    if (inst = SearchChain(inst, lane); inst->GetOpcode() == IR::Opcode::WriteLane) {
        return inst->Arg(1).Resolve();
    }

    // If this is a phi, duplicate it and populate its arguments with real values.
    if (inst->GetOpcode() == IR::Opcode::Phi) {
        // We are in a phi cycle, use the already duplicated phi.
        const auto [it, is_new_phi] = phi_map.try_emplace(inst);
        if (!is_new_phi) {
            return IR::Value{it->second};
        }

        // Create new phi and insert it right before the old one.
        const auto insert_point = IR::Block::InstructionList::s_iterator_to(*inst);
        IR::Block* block = inst->GetParent();
        IR::Inst* new_phi{&*block->PrependNewInst(insert_point, IR::Opcode::Phi)};
        new_phi->SetFlags(IR::Type::U32);
        it->second = new_phi;
        new_phis.push_back(new_phi);

        // Gather all arguments. Trivial phis are only removed once the whole tree is built,
        // as until then the arguments gathered here are not registered as uses yet and would
        // be left dangling by a removal.
        boost::container::static_vector<IR::Value, 5> phi_args;
        for (size_t arg_index = 0; arg_index < inst->NumArgs(); arg_index++) {
            const IR::Value arg_value = inst->Arg(arg_index);
            // An immediate incoming value is the same on every lane, forward it as-is.
            phi_args.push_back(arg_value.IsImmediate()
                                   ? arg_value.Resolve()
                                   : GetRealValue(phi_map, new_phis,
                                                  arg_value.InstRecursive(), lane));
        }
        for (size_t arg_index = 0; arg_index < inst->NumArgs(); arg_index++) {
            new_phi->AddPhiOperand(inst->PhiBlock(arg_index), phi_args[arg_index]);
        }
        return IR::Value{new_phi};
    }
    UNREACHABLE();
}

void ReadLaneEliminationPass(IR::Program& program) {
    PhiMap phi_map;
    NewPhiList new_phis;
    for (IR::Block* const block : program.blocks) {
        for (auto it = block->begin(); it != block->end();) {
            IR::Inst& inst{*it};
            if (inst.GetOpcode() != IR::Opcode::ReadLane || !inst.Arg(1).IsImmediate()) {
                ++it;
                continue;
            }

            const u32 lane = inst.Arg(1).U32();
            IR::Inst* prod = inst.Arg(0).InstRecursive();

            // Check simple case of no control flow and phis
            if (prod = SearchChain(prod, lane); prod->GetOpcode() == IR::Opcode::WriteLane) {
                inst.ReplaceUsesWithAndRemove(prod->Arg(1).Resolve());
                it = block->Instructions().erase(it);
                continue;
            }

            // Traverse the phi tree to see if it's possible to eliminate
            if (prod->GetOpcode() == IR::Opcode::Phi && IsPossibleToEliminate(prod, lane)) {
                inst.ReplaceUsesWithAndRemove(GetRealValue(phi_map, new_phis, prod, lane));
                // Now that the tree is complete and every reference to it is a registered
                // use, the phis that turned out to merge a single value can be removed.
                for (IR::Inst* const new_phi : new_phis) {
                    TryRemoveTrivialPhi(new_phi);
                }
                phi_map.clear();
                new_phis.clear();
                // Erased last, as the loop above may erase instructions of this block too.
                it = block->Instructions().erase(it);
                continue;
            }
            ++it;
        }
    }
}

} // namespace Shader::Optimization
