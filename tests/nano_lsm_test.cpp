#include <type_traits>

#include <emds-toolkit/emds_toolkit.hpp>
#include <nano-lsm/nano_lsm.hpp>

auto main() -> int {
    static_assert(emds::io::DirectIOBuffer::AlignBytes == 4096);
    static_assert(!std::is_copy_constructible_v<emds::io::DirectIOBuffer>);

    emds::common::require_argument(true, "satisfied requirement");

    return 0;
}
