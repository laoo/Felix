#pragma once

std::vector<uint8_t> decrypt(size_t blockcount, std::span<uint8_t const> encrypted);
