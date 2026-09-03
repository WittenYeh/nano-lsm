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

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>
#include <utility>

#include <emds-toolkit/common/byte_view.hpp>
#include <emds-toolkit/common/requires.hpp>

#include <nano-lsm/run/run_entry/run_entry.hpp>

namespace nano_lsm {

/**
 * @brief Encodes and decodes the padding-free native physical representation of RunEntry objects.
 *
 * The physical layout is KeyT, packed version-and-kind, then PayloadRefT. It is bound to the current
 * architecture, ABI, and user schema and does not provide cross-endian portability.
 */
template <PhysicalKey KeyT>
class EntryCodec {
public:
    /** @brief Number of bytes occupied by every encoded value entry or tombstone. */
    static constexpr std::size_t encoded_entry_bytes =
        sizeof(KeyT) + sizeof(std::uint64_t) + sizeof(PayloadRefT);

    /**
     * @brief Returns the exact output size required to encode num_entries.
     *
     * @throws std::invalid_argument If the multiplication would overflow std::size_t.
     */
    [[nodiscard]] static auto encoded_batch_bytes(std::size_t num_entries) -> std::size_t {
        emds::common::require_argument(
            num_entries <= std::numeric_limits<std::size_t>::max() / encoded_entry_bytes,
            "EntryCodec batch byte size overflows std::size_t");
        return num_entries * encoded_entry_bytes;
    }

    /**
     * @brief Encodes one entry into an exactly sized caller-owned output span.
     *
     * @throws std::invalid_argument If the output size or entry kind is invalid.
     */
    static auto encode(const RunEntry<KeyT>& entry, std::span<std::byte> output) -> void {
        emds::common::require_argument(output.size() == encoded_entry_bytes,
            "EntryCodec output size must equal encoded_entry_bytes");
        emds::common::require_argument(is_valid_kind(entry.kind()),
            "EntryCodec cannot encode an invalid entry kind");

        encode(entry, output.data());
    }

    /**
     * @brief Encodes a batch directly into one exactly sized caller-owned output span.
     *
     * Entries are validated and encoded in one pass. If an entry kind is invalid, entries before
     * it may already have been written. No intermediate entry buffer or heap allocation is used.
     *
     * @throws std::invalid_argument If the batch size, output size, or an entry kind is invalid.
     */
    static auto encode_batch(
        std::span<const RunEntry<KeyT>> entries, std::span<std::byte> output
    ) -> void {
        const auto expected_bytes = encoded_batch_bytes(entries.size());
        emds::common::require_argument(output.size() == expected_bytes,
            "EntryCodec batch output size does not match entry count");

        for (std::size_t i = 0; i < entries.size(); ++i) {
            emds::common::require_argument(is_valid_kind(entries[i].kind()),
                "EntryCodec cannot encode an invalid entry kind");
            encode(entries[i], output.data() + i * encoded_entry_bytes);
        }
    }

    /**
     * @brief Decodes one exactly sized physical entry.
     *
     * @throws std::invalid_argument If the input size or encoded entry kind is invalid.
     */
    [[nodiscard]] static auto decode(emds::common::ByteView input) -> RunEntry<KeyT> {
        emds::common::require_argument(input.size() == encoded_entry_bytes,
            "EntryCodec input size must equal encoded_entry_bytes");

        auto entry = decode(input.data());
        emds::common::require_argument(is_valid_kind(entry.kind()),
            "EntryCodec cannot decode an invalid entry kind");
        return entry;
    }

    /**
     * @brief Decodes a contiguous batch into an exactly sized caller-owned output span.
     *
     * The input and output memory ranges must not overlap. Entries are decoded and validated in one
     * pass. On an invalid kind, preceding output entries may already be modified. No heap allocation
     * is performed.
     *
     * @throws std::invalid_argument If input is not entry-aligned, the output count differs, or an
     * encoded entry kind is invalid.
     */
    static auto decode_batch(
        emds::common::ByteView input, std::span<RunEntry<KeyT>> output
    ) -> void {
        emds::common::require_argument(input.size() % encoded_entry_bytes == 0,
            "EntryCodec batch input size must be a multiple of encoded_entry_bytes");

        const auto num_entries = input.size() / encoded_entry_bytes;
        emds::common::require_argument(output.size() == num_entries,
            "EntryCodec batch output count does not match encoded entry count");

        for (std::size_t i = 0; i < num_entries; ++i) {
            auto entry = decode(input.data() + i * encoded_entry_bytes);
            emds::common::require_argument(is_valid_kind(entry.kind()),
                "EntryCodec cannot decode an invalid entry kind");
            output[i] = std::move(entry);
        }
    }

private:
    static constexpr std::size_t KeyOffset = 0;
    static constexpr std::size_t VersionAndKindOffset = KeyOffset + sizeof(KeyT);
    static constexpr std::size_t PayloadRefOffset = VersionAndKindOffset + sizeof(std::uint64_t);

    /** @brief Encodes a previously validated entry at output. */
    static auto encode(const RunEntry<KeyT>& entry, std::byte* output) noexcept -> void {
        encode_native(entry.key, output + KeyOffset);
        encode_native(entry.version_and_kind, output + VersionAndKindOffset);
        encode_native(entry.payload_ref, output + PayloadRefOffset);
    }

    /** @brief Decodes an unvalidated physical entry. */
    [[nodiscard]] static auto decode(const std::byte* input) -> RunEntry<KeyT> {
        return RunEntry<KeyT>::from_physical(
            decode_native<KeyT>(input + KeyOffset),
            decode_native<std::uint64_t>(input + VersionAndKindOffset),
            decode_native<PayloadRefT>(input + PayloadRefOffset));
    }

    /** @brief Reports whether kind is one of the two persisted EntryKindT values. */
    [[nodiscard]] static constexpr auto is_valid_kind(EntryKindT kind) noexcept -> bool {
        return kind == EntryKindT::valid || kind == EntryKindT::tombstone;
    }

    /** @brief Copies a native physical representation into encoded storage. */
    template <typename PhysicalT>
    static auto encode_native(const PhysicalT& value, std::byte* output) noexcept -> void {
        std::memcpy(output, &value, sizeof(PhysicalT));
    }

    /** @brief Reconstructs a trivially copyable value from its physical representation. */
    template <typename PhysicalT>
    [[nodiscard]] static auto decode_native(const std::byte* input) noexcept -> PhysicalT {
        std::array<std::byte, sizeof(PhysicalT)> bytes{};
        std::memcpy(bytes.data(), input, bytes.size());
        return std::bit_cast<PhysicalT>(bytes);
    }
};

}  // namespace nano_lsm
