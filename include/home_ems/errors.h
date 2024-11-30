#ifndef HOME_EMS_ERRORS_H_
#define HOME_EMS_ERRORS_H_

#include <stdexcept>

namespace home_ems {

// A value passed in is outside its valid range or cannot be parsed.
class InvalidParameterError : public std::invalid_argument {
 public:
  using std::invalid_argument::invalid_argument;
};

// The hardware setup cannot do what was asked, for example a storage unit
// with no battery modules or an inverter with no power capacity.
class HardwareFailureError : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

// A power request exceeds a component's rated capacity.
class OverloadError : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

}  // namespace home_ems

#endif  // HOME_EMS_ERRORS_H_
