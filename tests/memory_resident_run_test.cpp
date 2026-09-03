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

#include <array>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <nano-lsm/run/run.hpp>

namespace nano_lsm {
namespace {

using KeyT = std::uint64_t;
using EntryT = RunEntry<KeyT>;
using BackendT = detail::MutableRunBackend<KeyT, Placement::MemoryResident, std::less<KeyT>>;
using StorageBackendT = detail::MutableRunBackend<KeyT, Placement::StorageResident, std::less<KeyT>>;

struct InvalidKeyComparator {
    auto operator()(std::string_view lhs, std::string_view rhs) const -> bool {
        return lhs < rhs;
    }
};

struct StatefulKeyComparator {
    bool descending = false;

    auto operator()(KeyT lhs, KeyT rhs) const -> bool {
        return descending ? lhs > rhs : lhs < rhs;
    }
};

struct DecadeKeyComparator {
    auto operator()(KeyT lhs, KeyT rhs) const -> bool {
        return lhs / 10 < rhs / 10;
    }
};

static_assert(KeyComparator<std::less<KeyT>, KeyT>);
static_assert(!KeyComparator<InvalidKeyComparator, KeyT>);
static_assert(std::is_class_v<StorageBackendT>);

auto collect_entries(const BackendT& backend) -> std::vector<EntryT> {
    std::vector<EntryT> entries;
    backend.scan([&entries](const EntryT& entry) { entries.push_back(entry); });
    return entries;
}

auto expect_entry(const EntryT& entry, KeyT key, VersionT version, EntryKindT kind) -> void {
    EXPECT_EQ(entry.key, key);
    EXPECT_EQ(entry.version(), version);
    EXPECT_EQ(entry.kind(), kind);
}

/** @brief Verifies key ascending, packed version-and-kind descending, and ignored payload ordering. */
TEST(MemoryResidentRunTest, EntryComparatorEstablishesInternalKeyOrder) {
    const EntryComparator<KeyT, std::less<KeyT>> comparator;
    const auto key_one = EntryT::make(1, 10, 4);
    const auto key_two = EntryT::make(2, 10, 4);
    const auto newest = EntryT::make(1, 10, 7);
    const auto oldest = EntryT::make(1, 10, 3);
    auto tombstone = EntryT::tombstone(1, 7);
    tombstone.payload_ref = 99;
    const auto same_internal_key = EntryT::make(1, 999, 7);

    EXPECT_TRUE(comparator(key_one, key_two));
    EXPECT_TRUE(comparator(newest, oldest));
    EXPECT_TRUE(comparator(tombstone, newest));
    EXPECT_FALSE(comparator(newest, same_internal_key));
    EXPECT_FALSE(comparator(same_internal_key, newest));
}

/** @brief Verifies that unordered entries from multiple batches scan in complete InternalKey order. */
TEST(MemoryResidentRunTest, MultipleBatchesScanInInternalKeyOrder) {
    BackendT backend;
    const std::array first_batch{
        EntryT::make(3, 30, 2),
        EntryT::make(1, 11, 4),
        EntryT::make(2, 20, 8),
    };
    const std::array second_batch{
        EntryT::make(1, 12, 9),
        EntryT::tombstone(1, 6),
    };

    backend.append(first_batch);
    backend.append(second_batch);
    const auto entries = collect_entries(backend);

    ASSERT_EQ(entries.size(), 5U);
    expect_entry(entries[0], 1, 9, EntryKindT::valid);
    expect_entry(entries[1], 1, 6, EntryKindT::tombstone);
    expect_entry(entries[2], 1, 4, EntryKindT::valid);
    expect_entry(entries[3], 2, 8, EntryKindT::valid);
    expect_entry(entries[4], 3, 2, EntryKindT::valid);
    EXPECT_EQ(backend.size(), 5U);
    EXPECT_FALSE(backend.empty());
}

/** @brief Verifies that an empty batch leaves an empty backend unchanged. */
TEST(MemoryResidentRunTest, EmptyBatchDoesNothing) {
    BackendT backend;
    const std::span<const EntryT> empty_batch;

    EXPECT_NO_THROW(backend.append(empty_batch));
    EXPECT_TRUE(backend.empty());
    EXPECT_EQ(backend.size(), 0U);
}

/** @brief Verifies that scan_versions visits only one key from newest version to oldest version. */
TEST(MemoryResidentRunTest, ScanVersionsVisitsOnlyRequestedKey) {
    BackendT backend;
    const std::array entries{
        EntryT::make(7, 71, 1),
        EntryT::make(5, 53, 3),
        EntryT::make(5, 59, 9),
        EntryT::make(6, 61, 1),
    };
    backend.append(entries);
    std::vector<VersionT> versions;

    backend.scan_versions(5, [&versions](const EntryT& entry) { versions.push_back(entry.version()); });

    EXPECT_EQ(versions, (std::vector<VersionT>{9, 3}));
}

/** @brief Verifies that scan_versions does not invoke its visitor for a missing key. */
TEST(MemoryResidentRunTest, ScanVersionsIgnoresMissingKey) {
    BackendT backend;
    const std::array entries{EntryT::make(1, 10, 1), EntryT::make(3, 30, 1)};
    backend.append(entries);
    std::size_t visits = 0;

    backend.scan_versions(2, [&visits](const EntryT&) { ++visits; });

    EXPECT_EQ(visits, 0U);
}

/** @brief Verifies scan_versions uses comparator equivalence rather than raw key equality. */
TEST(MemoryResidentRunTest, ScanVersionsUsesKeyComparatorEquivalence) {
    using DecadeBackendT = detail::MutableRunBackend<KeyT, Placement::MemoryResident, DecadeKeyComparator>;
    DecadeBackendT backend;
    const std::array entries{EntryT::make(11, 110, 3), EntryT::make(18, 180, 9)};
    backend.append(entries);
    std::vector<VersionT> versions;

    backend.scan_versions(15, [&versions](const EntryT& entry) { versions.push_back(entry.version()); });

    EXPECT_EQ(versions, (std::vector<VersionT>{9, 3}));
}

/** @brief Verifies duplicate InternalKeys are rejected while a different kind remains distinct. */
TEST(MemoryResidentRunTest, DuplicateInternalKeyIsRejected) {
    BackendT backend;
    const auto original = EntryT::make(4, 40, 7);
    const auto different_payload = EntryT::make(4, 99, 7);
    const auto different_kind = EntryT::tombstone(4, 7);
    backend.append(std::span{&original, 1U});

    EXPECT_THROW(backend.append(std::span{&different_payload, 1U}), std::invalid_argument);
    EXPECT_NO_THROW(backend.append(std::span{&different_kind, 1U}));
    EXPECT_EQ(backend.size(), 2U);
}

/** @brief Verifies clear empties the B-tree and permits subsequent insertion. */
TEST(MemoryResidentRunTest, ClearAllowsBackendReuse) {
    BackendT backend;
    const std::array entries{EntryT::make(1, 10, 1), EntryT::make(2, 20, 2)};
    backend.append(entries);

    backend.clear();

    EXPECT_TRUE(backend.empty());
    EXPECT_EQ(backend.size(), 0U);
    const auto replacement = EntryT::make(9, 90, 9);
    EXPECT_NO_THROW(backend.append(std::span{&replacement, 1U}));
    EXPECT_EQ(backend.size(), 1U);
}

/** @brief Verifies the packed 56-bit version boundary and kind round trip. */
TEST(MemoryResidentRunTest, RunEntryEnforcesPackedVersionBoundary) {
    const auto entry = EntryT::make(1, 10, EntryT::MaxVersion);

    EXPECT_EQ(entry.version(), EntryT::MaxVersion);
    EXPECT_EQ(entry.kind(), EntryKindT::valid);
    EXPECT_THROW(static_cast<void>(EntryT::make(1, 10, EntryT::MaxVersion + 1)), std::invalid_argument);
    EXPECT_THROW(static_cast<void>(EntryT::tombstone(1, EntryT::MaxVersion + 1)), std::invalid_argument);
}

/** @brief Verifies move construction preserves entries and a stateful key comparator. */
TEST(MemoryResidentRunTest, MovePreservesEntriesAndComparatorState) {
    using StatefulBackendT =
        detail::MutableRunBackend<KeyT, Placement::MemoryResident, StatefulKeyComparator>;
    StatefulBackendT source(StatefulKeyComparator{.descending = true});
    const std::array entries{
        EntryT::make(1, 10, 1),
        EntryT::make(3, 30, 1),
        EntryT::make(2, 20, 1),
    };
    source.append(entries);

    StatefulBackendT destination(std::move(source));
    std::vector<KeyT> keys;
    destination.scan([&keys](const EntryT& entry) { keys.push_back(entry.key); });

    EXPECT_EQ(keys, (std::vector<KeyT>{3, 2, 1}));
}

}  // namespace
}  // namespace nano_lsm
