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

#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>

#include <nano-lsm/run/run_entry/key_concept.hpp>

namespace nano_lsm {

/** @brief Globally ordered logical version assigned to an LSM entry. */
using VersionT = std::uint64_t;

/** @brief Eight-byte reference to a value payload stored outside the Run. */
using PayloadRefT = std::uint64_t;

/** @brief Describes whether a run entry refers to a value or deletes an older value. */
enum class EntryKindT : std::uint8_t {
    valid,
    tombstone,
};

/**
 * @brief A compact versioned key and its separated value-payload reference.
 *
 * Version and kind share one 64-bit word. The high 56 bits store version and the low 8 bits store
 * kind. A MemoryResident backend can use the complete object directly as its ordered index item.
 */
template <PhysicalKey KeyT>
struct RunEntry {
    /** @brief Number of low bits reserved for EntryKindT. */
    static constexpr std::size_t KindBits = 8;

    /** @brief Largest logical version representable by the packed 56-bit version field. */
    static constexpr VersionT MaxVersion = (VersionT{1} << (64 - KindBits)) - 1;

    /** @brief Creates an entry that refers to a value payload. */
    [[nodiscard]] static auto make(KeyT key, PayloadRefT payload_ref, VersionT version) -> RunEntry {
        return RunEntry{
            .key = std::move(key),
            .version_and_kind = pack(version, EntryKindT::valid),
            .payload_ref = std::move(payload_ref),
        };
    }

    /** @brief Creates an entry that hides older values for the supplied key. */
    [[nodiscard]] static auto tombstone(KeyT key, VersionT version) -> RunEntry {
        return RunEntry{
            .key = std::move(key),
            .version_and_kind = pack(version, EntryKindT::tombstone),
            .payload_ref = PayloadRefT{},
        };
    }

    /** @brief Reconstructs an entry from its unvalidated physical fields. */
    [[nodiscard]] static auto from_physical(
        KeyT key, std::uint64_t version_and_kind, PayloadRefT payload_ref
    ) -> RunEntry {
        return RunEntry{
            .key = std::move(key),
            .version_and_kind = version_and_kind,
            .payload_ref = std::move(payload_ref),
        };
    }

    /** @brief Returns the unpacked 56-bit logical version. */
    [[nodiscard]] constexpr auto version() const noexcept -> VersionT {
        return version_and_kind >> KindBits;
    }

    /** @brief Returns the EntryKindT stored in the low eight bits. */
    [[nodiscard]] constexpr auto kind() const noexcept -> EntryKindT {
        return static_cast<EntryKindT>(version_and_kind & KindMask);
    }

    /** @brief Inline physical key representation supplied by the user. */
    KeyT key;

    /** @brief Packed 56-bit version and 8-bit entry kind. */
    std::uint64_t version_and_kind;

    /** @brief External value reference; meaningful only when kind() is EntryKindT::valid. */
    PayloadRefT payload_ref;

private:
    static constexpr std::uint64_t KindMask = std::numeric_limits<std::uint8_t>::max();

    [[nodiscard]] static constexpr auto pack(VersionT version, EntryKindT kind) -> std::uint64_t {
        if (version > MaxVersion) {
            throw std::invalid_argument("RunEntry version exceeds its 56-bit physical field");
        }
        return (version << KindBits) | static_cast<std::uint8_t>(kind);
    }
};

}  // namespace nano_lsm
