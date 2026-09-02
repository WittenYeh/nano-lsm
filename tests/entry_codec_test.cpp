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

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>
#include <stdexcept>

#include <gtest/gtest.h>

#include <nano-lsm/run/run_entry/entry_codec.hpp>

namespace nano_lsm {
namespace {

using KeyT = std::uint8_t;
using PayloadRefT = std::uint64_t;
using EntryT = RunEntry<KeyT, PayloadRefT>;
using CodecT = EntryCodec<KeyT, PayloadRefT>;

constexpr std::size_t EntryBytes = CodecT::encoded_entry_bytes;

auto expect_entry_eq(const EntryT& actual, const EntryT& expected) -> void {
    EXPECT_EQ(actual.key, expected.key);
    EXPECT_EQ(actual.version, expected.version);
    EXPECT_EQ(actual.payload_ref, expected.payload_ref);
    EXPECT_EQ(actual.kind, expected.kind);
}

auto make_entries() -> std::array<EntryT, 3> {
    return {
        EntryT::make(KeyT{0x11}, PayloadRefT{0x1111222233334444ULL}, VersionT{7}),
        EntryT{
            .key = KeyT{0x22},
            .version = VersionT{6},
            .payload_ref = PayloadRefT{0x5555666677778888ULL},
            .kind = EntryKindT::tombstone,
        },
        EntryT::make(KeyT{0x33}, PayloadRefT{0x9999AAAABBBBCCCCULL}, VersionT{5}),
    };
}

/** @brief Verifies that every field of one valid entry survives a round trip. */
TEST(EntryCodecTest, ValidEntryRoundTrips) {
    const auto original = EntryT::make(KeyT{0x12}, PayloadRefT{0x1122334455667788ULL}, VersionT{42});
    std::array<std::byte, EntryBytes> encoded{};

    CodecT::encode(original, encoded);
    const auto decoded = CodecT::decode(encoded);

    expect_entry_eq(decoded, original);
}

/**
 * @brief Verifies that a tombstone retains payload-reference bytes despite lacking value semantics.
 */
TEST(EntryCodecTest, TombstoneEntryRoundTripsPayloadBytes) {
    const EntryT original{
        .key = KeyT{0x24},
        .version = VersionT{84},
        .payload_ref = PayloadRefT{0xA1A2A3A4A5A6A7A8ULL},
        .kind = EntryKindT::tombstone,
    };
    std::array<std::byte, EntryBytes> encoded{};

    CodecT::encode(original, encoded);
    const auto decoded = CodecT::decode(encoded);

    expect_entry_eq(decoded, original);
}

/** @brief Verifies that a mixed batch round-trips without changing entry order. */
TEST(EntryCodecTest, BatchRoundTripsInOriginalOrder) {
    const auto original = make_entries();
    std::array<std::byte, original.size() * EntryBytes> encoded{};
    std::array<EntryT, original.size()> decoded{};

    CodecT::encode_batch(original, encoded);
    CodecT::decode_batch(encoded, decoded);

    for (std::size_t i = 0; i < original.size(); ++i) {
        expect_entry_eq(decoded[i], original[i]);
    }
}

/** @brief Verifies that zero-entry batches require no storage and complete successfully. */
TEST(EntryCodecTest, EmptyBatchSucceeds) {
    const std::span<const EntryT> entries{};
    const std::span<std::byte> encoded{};
    const std::span<EntryT> decoded{};

    EXPECT_EQ(CodecT::encoded_batch_bytes(0), 0U);
    EXPECT_NO_THROW(CodecT::encode_batch(entries, encoded));
    EXPECT_NO_THROW(CodecT::decode_batch(encoded, decoded));
}

/** @brief Verifies that single-entry encoding rejects both undersized and oversized output spans. */
TEST(EntryCodecTest, EncodeRejectsIncorrectOutputSize) {
    const auto entry = EntryT::make(KeyT{1}, PayloadRefT{2}, VersionT{3});
    std::array<std::byte, EntryBytes - 1> short_output{};
    std::array<std::byte, EntryBytes + 1> long_output{};

    EXPECT_THROW(CodecT::encode(entry, short_output), std::invalid_argument);
    EXPECT_THROW(CodecT::encode(entry, long_output), std::invalid_argument);
}

/** @brief Verifies that single-entry decoding rejects both undersized and oversized input views. */
TEST(EntryCodecTest, DecodeRejectsIncorrectInputSize) {
    std::array<std::byte, EntryBytes - 1> short_input{};
    std::array<std::byte, EntryBytes + 1> long_input{};

    EXPECT_THROW([[maybe_unused]] const auto decoded = CodecT::decode(short_input), std::invalid_argument);
    EXPECT_THROW([[maybe_unused]] const auto decoded = CodecT::decode(long_input), std::invalid_argument);
}

/** @brief Verifies that batch encoding requires the exact byte count for its entry count. */
TEST(EntryCodecTest, EncodeBatchRejectsIncorrectOutputSize) {
    const auto entries = make_entries();
    std::array<std::byte, entries.size() * EntryBytes - 1> short_output{};
    std::array<std::byte, entries.size() * EntryBytes + 1> long_output{};

    EXPECT_THROW(CodecT::encode_batch(entries, short_output), std::invalid_argument);
    EXPECT_THROW(CodecT::encode_batch(entries, long_output), std::invalid_argument);
}

/** @brief Verifies that batch decoding rejects input with a trailing partial entry. */
TEST(EntryCodecTest, DecodeBatchRejectsMisalignedInput) {
    std::array<std::byte, EntryBytes + 1> input{};
    std::array<EntryT, 1> output{};

    EXPECT_THROW(CodecT::decode_batch(input, output), std::invalid_argument);
}

/** @brief Verifies that batch decoding requires output count to match encoded entry count. */
TEST(EntryCodecTest, DecodeBatchRejectsWrongOutputCount) {
    const auto entry = EntryT::make(KeyT{1}, PayloadRefT{2}, VersionT{3});
    std::array<std::byte, EntryBytes> encoded{};
    std::array<EntryT, 0> short_output{};
    std::array<EntryT, 2> long_output{};
    CodecT::encode(entry, encoded);

    EXPECT_THROW(CodecT::decode_batch(encoded, short_output), std::invalid_argument);
    EXPECT_THROW(CodecT::decode_batch(encoded, long_output), std::invalid_argument);
}

/** @brief Verifies that single-entry decoding rejects an unrecognized persisted entry kind. */
TEST(EntryCodecTest, DecodeRejectsInvalidKind) {
    const auto entry = EntryT::make(KeyT{1}, PayloadRefT{2}, VersionT{3});
    std::array<std::byte, EntryBytes> encoded{};
    CodecT::encode(entry, encoded);
    encoded.back() = std::byte{0xFF};

    EXPECT_THROW([[maybe_unused]] const auto decoded = CodecT::decode(encoded), std::invalid_argument);
}

/** @brief Verifies the documented partial-output behavior of both single-pass batch operations. */
TEST(EntryCodecTest, BatchStopsAtInvalidKindAndMayLeavePartialOutput) {
    auto entries = make_entries();
    const auto original_entries = entries;
    entries[1].kind = static_cast<EntryKindT>(0xFF);

    std::array<std::byte, entries.size() * EntryBytes> encoded;
    encoded.fill(std::byte{0xA5});
    std::array<std::byte, EntryBytes> expected_first{};
    CodecT::encode(entries[0], expected_first);

    EXPECT_THROW(CodecT::encode_batch(entries, encoded), std::invalid_argument);
    EXPECT_TRUE(std::equal(expected_first.begin(), expected_first.end(), encoded.begin()));
    EXPECT_TRUE(std::all_of(encoded.begin() + EntryBytes, encoded.end(),
        [](std::byte value) { return value == std::byte{0xA5}; }));

    CodecT::encode_batch(original_entries, encoded);
    encoded[2 * EntryBytes - 1] = std::byte{0xFF};
    const EntryT untouched = EntryT::make(KeyT{0xEE}, PayloadRefT{0xEEEE}, VersionT{0xEE});
    std::array<EntryT, original_entries.size()> decoded{untouched, untouched, untouched};

    EXPECT_THROW(CodecT::decode_batch(encoded, decoded), std::invalid_argument);
    expect_entry_eq(decoded[0], original_entries[0]);
    expect_entry_eq(decoded[1], untouched);
    expect_entry_eq(decoded[2], untouched);
}

/** @brief Verifies that encoded width is the field-width sum rather than the padded struct width. */
TEST(EntryCodecTest, EncodedLayoutExcludesStructPadding) {
    constexpr auto field_bytes = sizeof(KeyT) + sizeof(VersionT) + sizeof(PayloadRefT) + sizeof(EntryKindT);

    EXPECT_EQ(EntryBytes, field_bytes);
    EXPECT_LE(EntryBytes, sizeof(EntryT));
}

/** @brief Verifies the native field order without assuming a particular byte order. */
TEST(EntryCodecTest, EncodedLayoutUsesNativeFieldOrder) {
    const EntryT entry{
        .key = KeyT{0x12},
        .version = VersionT{0x1122334455667788ULL},
        .payload_ref = PayloadRefT{0xA1A2A3A4A5A6A7A8ULL},
        .kind = EntryKindT::tombstone,
    };
    std::array<std::byte, EntryBytes> encoded{};
    CodecT::encode(entry, encoded);

    KeyT key{};
    VersionT version{};
    PayloadRefT payload_ref{};
    EntryKindT kind{};
    std::size_t offset = 0;
    std::memcpy(&key, encoded.data() + offset, sizeof(key));
    offset += sizeof(key);
    std::memcpy(&version, encoded.data() + offset, sizeof(version));
    offset += sizeof(version);
    std::memcpy(&payload_ref, encoded.data() + offset, sizeof(payload_ref));
    offset += sizeof(payload_ref);
    std::memcpy(&kind, encoded.data() + offset, sizeof(kind));

    EXPECT_EQ(key, entry.key);
    EXPECT_EQ(version, entry.version);
    EXPECT_EQ(payload_ref, entry.payload_ref);
    EXPECT_EQ(kind, entry.kind);
}

/** @brief Verifies that single-entry and batch APIs share one physical entry width. */
TEST(EntryCodecTest, SingleAndBatchApisShareEntryWidth) {
    EXPECT_EQ(CodecT::encoded_batch_bytes(3), 3 * CodecT::encoded_entry_bytes);
}

/** @brief Verifies safe batch-size calculation at and beyond the multiplication boundary. */
TEST(EntryCodecTest, EncodedBatchBytesRejectsOverflow) {
    constexpr auto max_safe_entries = std::numeric_limits<std::size_t>::max() / EntryBytes;

    EXPECT_EQ(CodecT::encoded_batch_bytes(max_safe_entries), max_safe_entries * EntryBytes);
    EXPECT_THROW([[maybe_unused]] const auto bytes = CodecT::encoded_batch_bytes(max_safe_entries + 1),
        std::invalid_argument);
}

}  // namespace
}  // namespace nano_lsm
