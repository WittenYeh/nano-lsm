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

#include <nano-lsm/run/run_entry/key_concept.hpp>

namespace nano_lsm {

/** @brief Groups the supported locations for a MutableRun's authoritative representation. */
struct Placement final {
    Placement() = delete;

    /** @brief Keeps the authoritative MutableRun representation in process memory. */
    struct MemoryResident final {};

    /**
     * @brief Keeps the authoritative MutableRun representation in external storage.
     *
     * Temporary in-memory I/O buffers may still be used.
     */
    struct StorageResident final {};
};

namespace detail {

/** @brief Primary template specialized in placement-specific backend headers. */
template <PhysicalKey KeyT, typename PlacementT, typename KeyComparatorT>
requires KeyComparator<KeyComparatorT, KeyT>
class MutableRunBackend;

}  // namespace detail
}  // namespace nano_lsm
