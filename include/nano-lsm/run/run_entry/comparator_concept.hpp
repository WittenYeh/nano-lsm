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

#include <nano-lsm/run/run_entry/key_concept.hpp>

namespace nano_lsm {

/** @brief Comparator contract used to establish a strict weak ordering over physical keys. */
template <typename ComparatorT, typename KeyT>
concept KeyComparator =
    PhysicalKey<KeyT> &&
    std::copy_constructible<ComparatorT> &&
    std::strict_weak_order<ComparatorT, const KeyT&, const KeyT&>;

}  // namespace nano_lsm
