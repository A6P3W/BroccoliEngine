#include "InputDevice.h"

struct InputDevice::Impl {};

InputDevice::InputDevice() : ImplPtr(new Impl()) {}

InputDevice::~InputDevice() {
  delete ImplPtr;
}
