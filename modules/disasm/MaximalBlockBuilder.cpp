//===------------------------------------------------------------*- C++ -*-===//
//
// This file is distributed under BSD License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// 
// Copyright (c) 2015 Technical University of Kaiserslautern.

#include "MaximalBlockBuilder.h"
#include <algorithm>
#include <cassert>

namespace disasm{

MaximalBlockBuilder::MaximalBlockBuilder() :
    m_buildable{false},
    m_bb_idx{0},
    m_frag_idx{0} {
}

std::vector<unsigned int>
MaximalBlockBuilder::appendableBasicBlocksAt(const addr_t addr) const {
    std::vector<unsigned int> result;
    for(auto& bblock:m_bblocks){
        // we know that a basic block should contain at least
        //  one fragment upon creation.
        Fragment* frag = findFragment(bblock.m_frag_ids.back());
        if(frag->isAppendableAt(addr))
            result.push_back(bblock.id());
    }
    return result;
}

void
MaximalBlockBuilder::createBasicBlockWith(const MCInstSmall &inst) {
    m_bblocks.emplace_back(BasicBlock(m_bb_idx));
    m_frags.emplace_back(Fragment(m_frag_idx, inst));
    // link basic block to fragment
    m_bblocks.back().m_frag_ids.push_back(m_frag_idx);
    m_bb_idx++;
    m_frag_idx++;
}

void
MaximalBlockBuilder::append(
    const unsigned int bb_id,
    const MCInstSmall &inst) {

    assert((bb_id < m_bblocks.size()) && "Out of bound block index!!");
    auto block = findBasicBlock(bb_id);

    assert(block != nullptr && "Error BasicBlock not found!!");
    auto last_frag_id = block->m_frag_ids.back();
    // Add instruction to fragment
    findFragment(last_frag_id)->m_insts.push_back(inst);
}

void
MaximalBlockBuilder::append(
    const unsigned int bb_id,
    const MCInstSmall &inst,
    const BranchInstType br_type,
    const addr_t br_target) {

    assert((bb_id < m_bblocks.size()) && "Out of bound block index!!");
    auto block = findBasicBlock(bb_id);
    assert(block != nullptr && "Error BasicBlock not found!!");
    block->m_br_target = br_target;
    block->m_br_type = br_type;
    // Add instruction to last fragment
    findFragment(block->m_frag_ids.back())->m_insts.push_back(inst);
    m_buildable = true;
}

void
MaximalBlockBuilder::append(
    const std::vector<unsigned int> &bb_ids,
    const MCInstSmall &inst) {

    BasicBlock* block;
    // create a new fragment with the given instruction
    m_frags.emplace_back(Fragment(m_frag_idx, inst));
    for (auto& id: bb_ids) {
        assert((id < m_bblocks.size()) && "Out of bound block index!!");
        block = findBasicBlock(id);
        assert(block != nullptr && "Error BasicBlock not found!!");
        // Append fragment to all basic blocks
        block->m_frag_ids.push_back(id);
    }
    m_frag_idx++;
}

void
MaximalBlockBuilder::append(
    const std::vector<unsigned int> &bb_ids,
    const MCInstSmall &inst,
    const BranchInstType br_type,
    const addr_t br_target) {

    BasicBlock* block;
    // create a new fragment with the given instruction
    m_frags.emplace_back(Fragment(m_frag_idx, inst));
    for (auto& id: bb_ids) {
        assert((id < m_bblocks.size()) && "Out of bound block index!!");
        block = findBasicBlock(id);
        assert(block != nullptr && "Error BasicBlock not found!!");
        // Append fragment to all basic blocks
        block->m_frag_ids.push_back(id);
        block-> m_br_type = br_type;
        block->m_br_target = br_target;
    }
    m_frag_idx++;
    m_buildable = true;
}

Fragment*
MaximalBlockBuilder::findFragment(const unsigned int frag_id) const {
    Fragment* result = nullptr;
    std::vector<Fragment>::iterator frag;
    for (frag = m_frags.begin(); frag < m_frags.end();frag++) {
        if (frag->id() == frag_id ) {
            result = &(*frag);
            break;
        }
    }
    return result;
}

BasicBlock*
MaximalBlockBuilder::findBasicBlock(const unsigned int bb_id) const {
    BasicBlock* result = nullptr;
    std::vector<BasicBlock>::iterator block;
    for (block = m_bblocks.begin(); block < m_bblocks.end(); block++) {
        if (block->id() == bb_id ) {
            result = &(*block);
            break;
        }
    }
    return result;
}

MaximalBlock MaximalBlockBuilder::build() {
    MaximalBlock result;
    Fragment *frag;
    if (!m_buildable)
        // return an invalid maximal block!
        return result;

    std::vector<unsigned int> frag_id_map;
    auto findInMap = [](unsigned int id) {
        for (unsigned int i = 0; i < frag_id_map.size(); i++) {
            if (frag_id_map[i] == id) {
                return i;
            }
        }
        return (0);
    };

    // copy valid BBs to result
    std::copy_if(m_bblocks.begin(), m_bblocks.end(),
                 result.m_bblocks.begin(),
                 [](const BasicBlock &temp) { return temp.valid(); });

    for (unsigned int i = 0; i < result.m_bblocks.size(); i++) {
        // fixed ids for faster lookup
        result.m_bblocks[i].m_id = i;
        // build id map for fragments
        for (auto &id: result.m_bblocks[i].m_frag_ids) {
            auto pos = findInMap(id);
            if (pos == frag_id_map.size()) {
                // id was not found in map
                frag = findFragment(id);
                assert((frag != nullptr) && "Fragment was not found!!");
                frag_id_map.push_back(id);
                result.m_frags.push_back(*frag);
                result.m_frags.back().m_id = pos;
            }

        }
    }
    return result;
}

bool
MaximalBlockBuilder::reset() {

    m_buildable = false;
    m_bb_idx = 0;
    m_frag_idx = 0;

    Fragment *valid_frag{nullptr};
    Fragment *current{nullptr};

    std::vector<BasicBlock> temp_bb;
    std::vector<Fragment> temp_frag;

    // detects a potential overlap between two fragments
    auto isOverlap = [](const Fragment *vfrag, const Fragment *cfrag) {
        if (vfrag->id() == cfrag->id()) return false;
//        int frame = static_cast<int>(
//        (valid_frag->startAddr() + valid_frag->memSize()) // last address of first
//        - (current->startAddr() + current->memSize())); // last address of second
//
//        if ( ) {
//    }
        return false;
    };

    // look for the last fragment in a valid basic block
    for (const BasicBlock &block : m_bblocks) {
        if (block.valid()) {
            valid_frag = findFragment(block.m_frag_ids.back());
            assert((valid_frag != nullptr) && "Fragment was not found!!");
            break;
        }
    }

    for (const BasicBlock &block : m_bblocks) {
        if (!block.valid()) {
            current = findFragment(block.m_frag_ids.back());
            if (isOverlap(valid_frag, current)) {
                temp_bb.push_back(block);
            }
        }
    }
    assert(temp_bb.size() < 2 && "More than two overlaping BBs found!!");
    if (temp_bb.size() == 0) {
        // no overlap was detected, resetting data structures.
        m_bblocks.clear();
        m_frags.clear();
        return true;
    } else {
        // For variable-length RISC it's impossible to have more than 2 overlapping
        //  BBs. For x86, it's extremely unlikely.
        m_bblocks.clear();
        m_bblocks.push_back(temp_bb.back());
        m_bblocks.back().m_id = m_bb_idx;
        for (auto id : m_bblocks.front().m_frag_ids) {
            current = findFragment(id);
            current->m_id = m_frag_idx;
            temp_frag.push_back(*current);
            m_frag_idx++;
        }
        m_frags.clear();
        m_frags.swap(temp_frag);
        return false;
    }
}

void
MaximalBlockBuilder::remove(const std::vector<unsigned int> &bb_ids) {
    // XXX erasing elements from a vector can be costly. Should be OK here
    //  since the number of elements in this context is very small.
    for(auto& id:bb_ids){
        assert((id < m_bblocks.size()) && "Out of bound access to a vector");
        m_bblocks.erase(m_bblocks.begin()+id);
    }
}
}
