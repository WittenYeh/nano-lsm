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
#include <utility>

#include <nano-lsm/run/key_concept.hpp>

namespace nano_lsm {

/** @brief Globally ordered version assigned to an LSM entry. */
using VersionT = std::uint64_t;

/** @brief Describes whether a run entry refers to a value or deletes an older value. */
enum class EntryKindT : std::uint8_t {
    value,
    tombstone,
};

/**
 * @brief A versioned physical key and its separated value-payload reference.
 *
 * The key is stored inline without dynamic allocation. Storage implementations persist each field
 * separately and never copy the native RunEntry struct, whose layout may contain padding.
 */
template <PhysicalKey KeyT, typename PayloadRefT>
struct RunEntry {
    /** @brief Creates an entry that refers to a value payload. */
    [[nodiscard]] static auto make(
        KeyT key,
        PayloadRefT payload_ref,
        VersionT version
    ) -> RunEntry {
        return RunEntry{
            .key = std::move(key),
            .version = version,
            .payload_ref = std::move(payload_ref),
            .kind = EntryKindT::value,
        };
    }

    /** @brief Creates an entry that hides older values for the supplied key. */
    [[nodiscard]] static auto tombstone(
        KeyT key,
        VersionT version
    ) -> RunEntry {
        return RunEntry{
            .key = std::move(key),
            .version = version,
            .payload_ref = PayloadRefT{},
            .kind = EntryKindT::tombstone,
        };
    }

    /** @brief Inline physical key representation supplied by the user. */
    KeyT key;

    /** @brief Globally unique version assigned by the owning LSM tree. */
    VersionT version;

    /** @brief External value reference; meaningful only when @ref kind is EntryKindT::value. */
    PayloadRefT payload_ref;

    /** @brief Whether this entry contains a payload address or represents a deletion. */
    EntryKindT kind;
};

}  // namespace nano_lsm
