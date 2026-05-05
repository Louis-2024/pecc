/*
 * Copyright (c) 1999-2008 Mark D. Hill and David A. Wood
 * Copyright (c) 2009 Advanced Micro Devices, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "cpu/testers/rubytest/CheckTable.hh"

#include "base/intmath.hh"
#include "base/random.hh"
#include "base/trace.hh"
#include "cpu/testers/rubytest/Check.hh"
#include "debug/RubyTest.hh"

#define DATA_SIZE (1048576 * CHECK_SIZE)

constexpr uint32_t ChecksPerCacheLine = 16;
constexpr uint32_t ChecksUnit = 4096;

constexpr uint32_t XShortReuseMinDistance = 2 * ChecksUnit;
constexpr uint32_t XShortReuseMaxDistance = 4 * ChecksUnit;

constexpr uint32_t ShortReuseMinDistance = 4 * ChecksUnit + 1;
constexpr uint32_t ShortReuseMaxDistance = 8 * ChecksUnit;

constexpr uint32_t MidReuseMinDistance = 8 * ChecksUnit + 1;
constexpr uint32_t MidReuseMaxDistance = 16 * ChecksUnit;

constexpr uint32_t FarReuseMinDistance = 16 * ChecksUnit + 1;
constexpr uint32_t FarReuseMaxDistance = 32 * ChecksUnit;

constexpr uint32_t XFarReuseMinDistance = 32 * ChecksUnit + 1;
constexpr uint32_t XFarReuseMaxDistance = 64 * ChecksUnit;

namespace gem5
{

uint32_t pickPastIndex(Random &rng, uint32_t history_size, uint32_t min_distance, uint32_t max_distance)
{
    uint32_t capped_max = max_distance;
    if (capped_max >= history_size) {
        capped_max = history_size;
    }

    uint32_t capped_min = min_distance;
    if (capped_min > capped_max) {
        capped_min = capped_max;
    }

    uint32_t distance = rng.random<unsigned>(capped_min, capped_max);
    return (history_size - distance);
}

CheckTable::CheckTable(int _num_writers, int _num_readers, RubyTester* _tester, uint32_t _random_seed)
    : m_num_writers(_num_writers), m_num_readers(_num_readers),
      m_tester_ptr(_tester), m_rng(_random_seed)
{
    constexpr Addr BasePhysical = 0x100000;
    constexpr Addr BlockSize = 64;

    const uint32_t index = 16;

    const uint32_t numberOfRows = index * BlockSize / CHECK_SIZE;
    const uint32_t rowSize = CHECK_SIZE;
    const uint32_t numberOfCols = DATA_SIZE / (BlockSize * index);
    const uint32_t colSize = index * BlockSize;

    const uint32_t target_checks = DATA_SIZE / CHECK_SIZE;

    m_check_vector.reserve(target_checks);
    m_lookup_map.reserve(DATA_SIZE);
    m_access_history_by_core.resize(m_num_readers);

    auto addUniqueCheck = [this](Addr address) {
        const size_t old_size = m_check_vector.size();
        addCheck(address);
        if (m_check_vector.size() != old_size + 1) {
            panic("Failed to add unique check at address %#x", address);
        }
    };

    for (uint32_t row = 0; row < numberOfRows; row++) {
        for (uint32_t col = 0; col < numberOfCols; col++) {
            const Addr address = BasePhysical + static_cast<Addr>(col) * colSize + static_cast<Addr>(row) * rowSize;
            addUniqueCheck(address);
        }
    }

    if (m_check_vector.size() != target_checks) {
        panic("Expected %u checks, built %zu", target_checks, m_check_vector.size());
    }

    if (m_lookup_map.size() != DATA_SIZE) {
        panic("Expected %u lookup-map entries, built %zu", DATA_SIZE, m_lookup_map.size());
    }
}

CheckTable::~CheckTable()
{
    int size = m_check_vector.size();
    for (int i = 0; i < size; i++)
        delete m_check_vector[i];
}

void
CheckTable::addCheck(Addr address)
{
    if (floorLog2(CHECK_SIZE) != 0) {
        if (ruby::bitSelect(address, 0, CHECK_SIZE_BITS - 1) != 0) {
            panic("Check not aligned");
        }
    }

    for (int i = 0; i < CHECK_SIZE; i++) {
        if (m_lookup_map.count(address+i)) {
            // A mapping for this byte already existed, discard the
            // entire check
            return;
        }
    }

    DPRINTF(RubyTest, "Adding check for address: %s\n", address);

    Check* check_ptr = new Check(address, 100 + m_check_vector.size(),
                                 m_num_writers, m_num_readers, m_tester_ptr);
    for (int i = 0; i < CHECK_SIZE; i++) {
        // Insert it once per byte
        m_lookup_map[address + i] = check_ptr;
    }
    m_check_vector.push_back(check_ptr);
}

Check*
CheckTable::getRandomCheck()
{
    const uint32_t coreCycleSize = 16384;
    
    uint32_t history_size = m_access_history.size();
    uint32_t out_index = history_size % m_check_vector.size();

    // random1: pick core
    float random1 = m_rng.random<float>();
    int core_index = (random1 < 0.8f)? (history_size / coreCycleSize) % m_num_readers : m_rng.random(0, m_num_readers - 1);

    // random2: pick reuse distance
    if (m_access_history.size() >= MidReuseMinDistance) {
        float random2 = m_rng.random<float>();
        if (random2 < 0.1f) {
            out_index = pickPastIndex(m_rng, history_size, XShortReuseMinDistance, XShortReuseMaxDistance);
        } else if (random2 < 0.2f) {
            out_index = pickPastIndex(m_rng, history_size, ShortReuseMinDistance, ShortReuseMaxDistance);
        } else if (random2 < 0.4f) {
            out_index = pickPastIndex(m_rng, history_size, MidReuseMinDistance, MidReuseMaxDistance);
        } else if (random2 < 0.8f) {
            out_index = pickPastIndex(m_rng, history_size, FarReuseMinDistance, FarReuseMaxDistance);
        } else if (random2 < 0.95f) {
            out_index = pickPastIndex(m_rng, history_size, XFarReuseMinDistance, XFarReuseMaxDistance);
        }

        if (random2 < 0.95f) {
            // random3: pick from global reuse history or per-core reuse history
            int per_core_history_size = m_access_history_by_core[core_index].size();
            int per_core_reuse_distance = (history_size - out_index) / 2;

            float random3 = m_rng.random<float>();
            out_index = m_access_history[out_index];
            if (random3 < 0.9f) {
                if ((per_core_reuse_distance <= per_core_history_size) && (per_core_reuse_distance > 0)) {
                    out_index = m_access_history_by_core[core_index][per_core_history_size - per_core_reuse_distance];
                }
            }
        }
    }
        
    m_access_history.push_back(out_index);
    m_access_history_by_core[core_index].push_back(out_index);
    m_check_vector[out_index]->setCoreIndex(core_index);

    return m_check_vector[out_index];
}

Check*
CheckTable::getCheck(const Addr address)
{
    DPRINTF(RubyTest, "Looking for check by address: %s\n", address);

    auto i = m_lookup_map.find(address);

    if (i == m_lookup_map.end())
        return NULL;

    Check* check = i->second;
    assert(check != NULL);
    return check;
}

void
CheckTable::print(std::ostream& out) const
{
}

} // namespace gem5
