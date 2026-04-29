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

#define DATA_SIZE 1048576

constexpr uint32_t ChecksPerCacheLine = 16;
constexpr uint32_t ApproxL1Lines = 1024;
constexpr uint32_t ApproxL1Checks = ApproxL1Lines * ChecksPerCacheLine;

constexpr uint32_t ShortReuseMinDistance = 1;
constexpr uint32_t ShortReuseMaxDistance = 0.5 * ApproxL1Checks;
constexpr uint32_t MidReuseMinDistance = 0.5 * ApproxL1Checks + 1;
constexpr uint32_t MidReuseMaxDistance = 2 * ApproxL1Checks;
constexpr uint32_t FarReuseMinDistance = 2 * ApproxL1Checks + 1;
constexpr uint32_t FarReuseMaxDistance = 4 * ApproxL1Checks;
constexpr uint32_t XFarReuseMinDistance = 4 * ApproxL1Checks + 1;
constexpr uint32_t XFarReuseMaxDistance = 8 * ApproxL1Checks;
constexpr uint32_t XXFarReuseMinDistance = 8 * ApproxL1Checks + 1;
constexpr uint32_t XXFarReuseMaxDistance = 16 * ApproxL1Checks;

namespace gem5
{

uint32_t pickPastIndex(uint32_t current_index, uint32_t total_checks, uint32_t min_distance, uint32_t max_distance)
{
    if (total_checks == 1) {
        return 0;
    }

    uint32_t capped_max = max_distance;
    if (capped_max >= total_checks) {
        capped_max = total_checks - 1;
    }

    uint32_t capped_min = min_distance;
    if (capped_min > capped_max) {
        capped_min = capped_max;
    }

    uint32_t distance = random_mt.random<unsigned>(capped_min, capped_max);
    return (current_index + total_checks - distance) % total_checks;
}

CheckTable::CheckTable(int _num_writers, int _num_readers, RubyTester* _tester)
    : m_num_writers(_num_writers), m_num_readers(_num_readers),
      m_tester_ptr(_tester)
{
    constexpr Addr BasePhysical = 0x100000;
    constexpr Addr RegionGap = 0x100000;
    constexpr Addr ConflictStride = 256;

    const uint32_t target_checks = DATA_SIZE / CHECK_SIZE;
    const uint32_t conflict_checks = target_checks / 8;
    const uint32_t distributed_checks = target_checks - 2 * conflict_checks;

    m_check_vector.reserve(target_checks);
    m_lookup_map.reserve(DATA_SIZE);

    auto addUniqueCheck = [this](Addr address) {
        const size_t old_size = m_check_vector.size();
        addCheck(address);

        if (m_check_vector.size() != old_size + 1) {
            panic("Failed to add unique check at address %#x", address);
        }
    };

    Addr physical = BasePhysical;
    const Addr conflict_region_base = physical + RegionGap;
    DPRINTF(RubyTest, "Adding cache conflict checks\n");
    physical = conflict_region_base;
    for (uint32_t i = 0; i < conflict_checks; ++i) {
        addUniqueCheck(physical);
        physical += ConflictStride;
    }

    DPRINTF(RubyTest, "Adding cache conflict checks2\n");
    physical = conflict_region_base + CHECK_SIZE;
    for (uint32_t i = 0; i < conflict_checks; ++i) {
        addUniqueCheck(physical);
        physical += ConflictStride;
    }

    DPRINTF(RubyTest, "Adding interleaved capacity checks\n");
    const Addr distributed_base = conflict_region_base + (static_cast<Addr>(conflict_checks) * ConflictStride) + RegionGap;
    const uint32_t stream_count = ChecksPerCacheLine;
    const uint32_t rows = (distributed_checks + stream_count - 1) / stream_count;

    for (uint32_t row = 0; row < rows; ++row) {
        for (uint32_t stream = 0;
             stream < stream_count && m_check_vector.size() < target_checks;
             ++stream) {
            const Addr address = distributed_base + CHECK_SIZE * (static_cast<Addr>(stream) * rows + row);
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
    uint32_t total_checks = m_check_vector.size();
    uint32_t out_index = m_current_index;

    if (m_current_index >= MidReuseMinDistance) {
        float selection = random_mt.random<float>();

        if (selection < 0.1f) {
            out_index = pickPastIndex(m_current_index, total_checks, ShortReuseMinDistance, ShortReuseMaxDistance);
        } else if (selection < 0.3f) {
            out_index = pickPastIndex(m_current_index, total_checks, MidReuseMinDistance, MidReuseMaxDistance);
        } else if (selection < 0.75f) {
            out_index = pickPastIndex(m_current_index, total_checks, FarReuseMinDistance,FarReuseMaxDistance);
        } else if (selection < 0.95f) {
            out_index = pickPastIndex(m_current_index, total_checks, XFarReuseMinDistance, XFarReuseMaxDistance);
        } else if (selection < 0.97f) {
            out_index = pickPastIndex(m_current_index, total_checks, XXFarReuseMinDistance, XXFarReuseMaxDistance);
        }
    }

    m_current_index = (m_current_index + 1) % total_checks;
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
