#pragma once

#include <cstdint>
#include <string>

namespace hashing
{
namespace md5
{

std::string hash_as_hex(const void* input_bs, uint64_t input_size);

} // namespace md5
} // namespace hashing
