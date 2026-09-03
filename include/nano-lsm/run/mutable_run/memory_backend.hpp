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

#include <concepts>
#include <cstddef>
#include <functional>
#include <span>
#include <utility>

#include <absl/container/btree_set.h>

#include <emds-toolkit/common/requires.hpp>

#include <nano-lsm/run/mutable_run/mutable_run_backend.hpp>
#include <nano-lsm/run/run_entry/entry_comparator.hpp>
#include <nano-lsm/run/run_entry/run_entry.hpp>

namespace nano_lsm::detail {

/** @brief Memory-resident backend whose complete entries are the ordered B-tree index items. */
template <PhysicalKey KeyT, typename KeyComparatorT>
requires KeyComparator<KeyComparatorT, KeyT>
class MutableRunBackend<KeyT, Placement::MemoryResident, KeyComparatorT> {
public:
    using EntryT = RunEntry<KeyT>;
    using EntryComparatorT = EntryComparator<KeyT, KeyComparatorT>;
    using EntryIndexT = absl::btree_set<EntryT, EntryComparatorT>;

    /** @brief Creates an empty backend ordered by the supplied key comparator. */
    explicit MutableRunBackend(KeyComparatorT key_comparator = KeyComparatorT{})
        : entry_index_(EntryComparatorT{std::move(key_comparator)}) {}

    MutableRunBackend(const MutableRunBackend&) = delete;
    auto operator=(const MutableRunBackend&) -> MutableRunBackend& = delete;
    MutableRunBackend(MutableRunBackend&&) = default;
    auto operator=(MutableRunBackend&&) -> MutableRunBackend& = default;
    ~MutableRunBackend() = default;

    /**
     * @brief Inserts complete entries as ordered index items.
     *
     * Entries are ordered by key, descending version, and descending kind. If an operation throws,
     * earlier entries from the same batch may already be present.
     *
     * @throws std::invalid_argument If the batch contains an InternalKey already in the backend.
     */
    auto append(std::span<const EntryT> entries) -> void {
        for (const auto& entry : entries) {
            // The complete entry is both the stored value and its InternalKey index item.
            const auto insert_result = entry_index_.insert(entry);
            // PayloadRefT is not ordered, so equal key/version/kind values are duplicates.
            emds::common::require_argument(insert_result.second,
                "Memory-resident MutableRun cannot contain duplicate InternalKeys");
        }
    }

    /** @brief Visits every complete entry in InternalKey order. */
    template <typename VisitorT>
    requires std::invocable<VisitorT&, const EntryT&>
    auto scan(VisitorT&& visitor) const -> void {
        // B-tree iteration already follows the order required by an ImmutableRun builder.
        for (const auto& entry : entry_index_) {
            std::invoke(visitor, entry);
        }
    }

    /** @brief Visits all versions of key from highest version to lowest version. */
    template <typename VisitorT>
    requires std::invocable<VisitorT&, const EntryT&>
    auto scan_versions(const KeyT& key, VisitorT&& visitor) const -> void {
        // Transparent comparison seeks by KeyT without constructing a placeholder RunEntry.
        auto key_pos = entry_index_.lower_bound(key);
        const auto comparator = entry_index_.key_comp();
        // Equal-key entries are contiguous and already ordered from newest to oldest.
        while (key_pos != entry_index_.end() && comparator.keys_equal(key_pos->key, key)) {
            std::invoke(visitor, *key_pos);
            ++key_pos;
        }
    }

    /** @brief Returns the number of indexed physical entries. */
    [[nodiscard]] auto size() const noexcept -> std::size_t {
        return entry_index_.size();
    }

    /** @brief Reports whether the backend contains no entries. */
    [[nodiscard]] auto empty() const noexcept -> bool {
        return entry_index_.empty();
    }

    /** @brief Removes all entries and releases the B-tree's owned node memory. */
    auto clear() noexcept -> void {
        // Abseil deletes the owned B-tree nodes instead of retaining vector-like capacity.
        entry_index_.clear();
    }

private:
    EntryIndexT entry_index_;
};

}  // namespace nano_lsm::detail
