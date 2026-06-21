#include "SimpleCrypto.h"

#include <cstddef>

namespace
{
	constexpr char CryptoKey[] = "BroccoliEngine_Simple_Key_2026!";
	constexpr std::size_t CryptoKeyLength = sizeof(CryptoKey) - 1;
}

std::string SimpleCrypto::Process(const std::string& input)
{
	if (input.empty())
	{
		return {};
	}

	std::string output = input;
	std::size_t keyIndex = 0;
	for (std::size_t i = 0; i != output.size(); ++i)
	{
		output[i] = (char)(output[i] ^ CryptoKey[keyIndex]);
		++keyIndex;
		if (keyIndex == CryptoKeyLength)
		{
			keyIndex = 0;
		}
	}

	return output;
}
