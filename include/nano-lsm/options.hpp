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
#include <filesystem>

namespace nano_lsm {

/** @brief Runtime layout and L1 flush-threshold configuration shared by run implementations. */
struct RunOptions {
    /** @brief External-memory page size in bytes. */
    std::size_t page_bytes = 4096;

    /** @brief Entry count at which a mutable run must be sealed. */
    std::size_t max_entries = 0;
};

/** @brief Paths and memory budget used while converting a mutable run to an immutable run. */
struct SealOptions {
    /** @brief Final path of the immutable run. */
    std::filesystem::path output_path;

    /** @brief Directory used for external-sort working files. */
    std::filesystem::path scratch_directory;

    /** @brief Maximum number of pages available to the external sorter. */
    std::size_t memory_budget_pages = 256;
};

}  // namespace nano_lsm
