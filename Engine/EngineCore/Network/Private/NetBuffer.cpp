#include "NetBuffer.h"

#include <cstddef>
#include <limits>
#include <utility>

FNetBuffer::FNetBuffer(const std::vector<uint8_t>& InData) : Buffer(InData) {}

FNetBuffer::FNetBuffer(std::vector<uint8_t>&& InData) : Buffer(std::move(InData)) {}

void FNetBuffer::Clear() {
  Buffer.clear();
  ReadOffset = 0;
}

void FNetBuffer::ResetRead() { ReadOffset = 0; }

size_t FNetBuffer::GetRemainingSize() const {
  if (ReadOffset >= Buffer.size()) {
    return 0;
  }

  return Buffer.size() - ReadOffset;
}

void FNetBuffer::WriteString(const std::string& Value) {
  const size_t maxLength = static_cast<size_t>(std::numeric_limits<uint32_t>::max());
  const uint32_t length =
      static_cast<uint32_t>(Value.size() > maxLength ? maxLength : Value.size());

  Write(length);

  if (length == 0) {
    return;
  }

  const size_t offset = Buffer.size();
  Buffer.resize(offset + length);
  std::memcpy(Buffer.data() + offset, Value.data(), length);
}

bool FNetBuffer::ReadString(std::string& OutValue) {
  if (ReadOffset + sizeof(uint32_t) > Buffer.size()) {
    return false;
  }

  uint32_t length = 0;
  std::memcpy(&length, Buffer.data() + ReadOffset, sizeof(uint32_t));

  const size_t payloadOffset = ReadOffset + sizeof(uint32_t);
  if (payloadOffset + length > Buffer.size()) {
    return false;
  }

  OutValue.assign(
      reinterpret_cast<const char*>(Buffer.data() + payloadOffset), static_cast<size_t>(length)
  );
  ReadOffset = payloadOffset + length;
  return true;
}

bool FNetBuffer::WriteBufferWithSize(const FNetBuffer& Source) {
  if (Source.Size() > static_cast<size_t>((std::numeric_limits<uint32_t>::max)())) {
    return false;
  }

  const uint32_t payloadSize = static_cast<uint32_t>(Source.Size());
  Write(payloadSize);

  if (payloadSize == 0) {
    return true;
  }

  const size_t offset = Buffer.size();
  Buffer.resize(offset + payloadSize);
  std::memcpy(Buffer.data() + offset, Source.Data(), payloadSize);
  return true;
}

bool FNetBuffer::ReadBufferWithSize(FNetBuffer& OutBuffer) {
  const size_t originalReadOffset = ReadOffset;

  uint32_t payloadSize = 0;
  if (!Read(payloadSize)) {
    ReadOffset = originalReadOffset;
    return false;
  }

  if (!ReadBuffer(payloadSize, OutBuffer)) {
    ReadOffset = originalReadOffset;
    return false;
  }

  return true;
}

bool FNetBuffer::ReadBuffer(uint32_t PayloadSize, FNetBuffer& OutBuffer) {
  if (GetRemainingSize() < PayloadSize) {
    return false;
  }

  OutBuffer.Buffer.clear();
  OutBuffer.ReadOffset = 0;

  if (PayloadSize == 0) {
    return true;
  }

  OutBuffer.Buffer.resize(PayloadSize);
  std::memcpy(OutBuffer.Buffer.data(), Buffer.data() + ReadOffset, PayloadSize);
  ReadOffset += PayloadSize;
  return true;
}
