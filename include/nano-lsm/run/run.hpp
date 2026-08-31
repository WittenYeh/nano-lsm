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
#include <nano-lsm/run/op_result.hpp>
#include <nano-lsm/run/run_entry.hpp>

namespace nano_lsm {

/** @brief In-memory mutable-run storage policy, implemented in a later step. */
class MemoryRunStorage;

/** @brief File-backed mutable-run storage policy, implemented in a later step. */
class FileRunStorage;

/** @brief Read-only packed B+Tree run, implemented in later steps. */
template <
    typename KeyT,
    typename PayloadAddrT,
    typename KeyCodec,
    typename PayloadAddressCodec,
    typename Compare = std::less<KeyT>>
class ImmutableRun;

/** @brief Unordered L0 run, implemented in later steps. */
template <
    typename KeyT,
    typename PayloadAddrT,
    typename KeyCodec,
    typename PayloadAddressCodec,
    typename StoragePolicy,
    typename Compare = std::less<KeyT>>
class MutableRun;

}  // namespace nano_lsm
