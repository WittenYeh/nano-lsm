// Copyright 2026 Weitang Ye
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <functional>

#include <nano-lsm/options.hpp>
#include <nano-lsm/run/mutable_run/mutable_run_backend.hpp>
#include <nano-lsm/run/mutable_run/memory_backend.hpp>
#include <nano-lsm/run/mutable_run/storage_backend.hpp>
#include <nano-lsm/run/op_result.hpp>
#include <nano-lsm/run/run_entry/entry_codec.hpp>
#include <nano-lsm/run/run_entry/entry_comparator.hpp>
#include <nano-lsm/run/run_entry/key_concept.hpp>
#include <nano-lsm/run/run_entry/run_entry.hpp>

namespace nano_lsm {

/** @brief Read-only packed B+Tree run, implemented in later steps. */
template <
    PhysicalKey KeyT,
    typename KeyComparatorT = std::less<KeyT>>
requires KeyComparator<KeyComparatorT, KeyT>
class ImmutableRun;

/** @brief Batch-ingestible L0 run, implemented in later steps. */
template <
    PhysicalKey KeyT,
    typename PlacementT = Placement::MemoryResident,
    typename KeyComparatorT = std::less<KeyT>>
requires KeyComparator<KeyComparatorT, KeyT>
class MutableRun;

}  // namespace nano_lsm
