#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <type_traits>
#include <vector>

class FNetBuffer {
 public:
  FNetBuffer() = default;
  explicit FNetBuffer(const std::vector<uint8_t>& InData);
  explicit FNetBuffer(std::vector<uint8_t>&& InData);

  void Clear();
  void ResetRead();

  const std::vector<uint8_t>& GetData() const { return Buffer; }
  const uint8_t* Data() const { return Buffer.data(); }
  size_t Size() const { return Buffer.size(); }
  size_t GetReadOffset() const { return ReadOffset; }
  size_t GetRemainingSize() const;

  template <class T>
  void Write(const T& Value) {
    static_assert(
        std::is_trivially_copyable_v<T>, "FNetBuffer::Write requires trivially copyable types."
    );

    const size_t offset = Buffer.size();
    Buffer.resize(offset + sizeof(T));
    std::memcpy(Buffer.data() + offset, &Value, sizeof(T));
  }

  template <class T>
  bool Read(T& OutValue) {
    static_assert(
        std::is_trivially_copyable_v<T>, "FNetBuffer::Read requires trivially copyable types."
    );

    if (ReadOffset + sizeof(T) > Buffer.size()) {
      return false;
    }

    std::memcpy(&OutValue, Buffer.data() + ReadOffset, sizeof(T));
    ReadOffset += sizeof(T);
    return true;
  }

  void WriteString(const std::string& Value);
  bool ReadString(std::string& OutValue);
  bool WriteBufferWithSize(const FNetBuffer& Source);
  bool ReadBufferWithSize(FNetBuffer& OutBuffer);
  bool ReadBuffer(uint32_t PayloadSize, FNetBuffer& OutBuffer);

 private:
  std::vector<uint8_t> Buffer;
  size_t ReadOffset = 0;
};
