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

#include <cstdint>
#include <optional>
#include <utility>

namespace nano_lsm {

/** @brief Globally ordered version assigned to an LSM entry. */
using VersionT = std::uint64_t;

/** @brief Describes whether a run entry refers to a value or deletes an older value. */
enum class EntryKindT : std::uint8_t {
    value,
    tombstone,
};

/**
 * @brief A versioned key and its separated value-payload address.
 *
 * A value entry owns a payload address. A tombstone has no payload address and hides older value
 * entries with the same key.
 *
 * @tparam KeyT Key type compared by a Run's configured comparator.
 * @tparam PayloadAddrT Address type supplied by the value-storage layer.
 */
template <typename KeyT, typename PayloadAddrT>
struct RunEntry {
    /** @brief Creates an entry that refers to a value payload. */
    [[nodiscard]] static auto put(
        KeyT key,
        PayloadAddrT payload_addr,
        VersionT version
    ) -> RunEntry {
        return RunEntry{
            .key = std::move(key),
            .version = version,
            .kind = EntryKindT::value,
            .payload_addr = std::move(payload_addr),
        };
    }

    /** @brief Creates an entry that hides older values for the supplied key. */
    [[nodiscard]] static auto tombstone(KeyT key, VersionT version) -> RunEntry {
        return RunEntry{
            .key = std::move(key),
            .version = version,
            .kind = EntryKindT::tombstone,
            .payload_addr = std::nullopt,
        };
    }

    /** @brief Entry key. */
    KeyT key;

    /** @brief Globally unique version assigned by the owning LSM tree. */
    VersionT version;

    /** @brief Whether this entry contains a payload address or represents a deletion. */
    EntryKindT kind;

    /** @brief Value payload address; present exactly when @ref kind is EntryKindT::value. */
    std::optional<PayloadAddrT> payload_addr;
};

}  // namespace nano_lsm
