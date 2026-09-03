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

#include <nano-lsm/run/mutable_run/mutable_run_backend.hpp>

namespace nano_lsm::detail {

/** @brief Storage-resident backend implemented in the external-storage step. */
template <PhysicalKey KeyT, typename KeyComparatorT>
requires KeyComparator<KeyComparatorT, KeyT>
class MutableRunBackend<KeyT, Placement::StorageResident, KeyComparatorT>;

}  // namespace nano_lsm::detail
