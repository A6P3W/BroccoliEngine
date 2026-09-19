#pragma once

#include <cstdint>
#include <filesystem>

class FAutomationControlRegistration {
 public:
  bool Create(uint16_t Port);
  void Remove();

 private:
  std::filesystem::path RegistrationPath;
};
