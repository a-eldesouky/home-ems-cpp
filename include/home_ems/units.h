#ifndef HOME_EMS_UNITS_H_
#define HOME_EMS_UNITS_H_

#include <cstdint>
#include <ostream>

namespace home_ems {

// Zero as a double, used for sign checks on quantities.
inline constexpr double kZero = 0.0;

// Base for strongly typed electrical quantities. Each derived type wraps one
// double, so a Voltage cannot be passed where a Power is expected. Arithmetic
// is only defined between values of the same derived type.
template <typename Derived>
struct Quantity {
  double value = kZero;

  constexpr Derived operator+(const Derived& other) const {
    return Derived(value + other.value);
  }
  constexpr Derived operator-(const Derived& other) const {
    return Derived(value - other.value);
  }
  constexpr Derived& operator+=(const Derived& other) {
    value += other.value;
    return static_cast<Derived&>(*this);
  }
  constexpr Derived& operator-=(const Derived& other) {
    value -= other.value;
    return static_cast<Derived&>(*this);
  }

  constexpr bool operator<(const Derived& other) const {
    return value < other.value;
  }
  constexpr bool operator>(const Derived& other) const {
    return value > other.value;
  }
  constexpr bool operator<=(const Derived& other) const {
    return value <= other.value;
  }
  constexpr bool operator>=(const Derived& other) const {
    return value >= other.value;
  }

  friend std::ostream& operator<<(std::ostream& os, const Quantity& q) {
    return os << q.value;
  }

 private:
  // Only Derived can construct its base, so Quantity<Power> cannot be used on
  // its own or as the base of another type.
  constexpr Quantity() = default;
  explicit constexpr Quantity(double v) : value(v) {}
  friend Derived;
};

// Electrical power in watts. Positive means power flowing into the battery
// (charging) when used as a storage command.
struct Power : Quantity<Power> {
  constexpr Power() = default;
  explicit constexpr Power(double v) : Quantity(v) {}

  // True when this net power is a surplus (PV produces more than the load).
  [[nodiscard]] constexpr bool IsSurplus() const { return value > kZero; }
  // True when some power is left over after the battery took its share.
  [[nodiscard]] constexpr bool IsRemaining() const { return value > kZero; }
};

// Electrical voltage in volts.
struct Voltage : Quantity<Voltage> {
  constexpr Voltage() = default;
  explicit constexpr Voltage(double v) : Quantity(v) {}
};

// Electrical current in amperes.
struct Current : Quantity<Current> {
  constexpr Current() = default;
  explicit constexpr Current(double v) : Quantity(v) {}
};

// AC frequency in hertz.
struct Frequency : Quantity<Frequency> {
  constexpr Frequency() = default;
  explicit constexpr Frequency(double v) : Quantity(v) {}
};

// Temperature in degrees Celsius.
struct Temperature : Quantity<Temperature> {
  constexpr Temperature() = default;
  explicit constexpr Temperature(double v) : Quantity(v) {}
};

// Energy in watt-hours.
struct Energy : Quantity<Energy> {
  constexpr Energy() = default;
  explicit constexpr Energy(double v) : Quantity(v) {}
};

// Length of a time span in hours.
struct Hours : Quantity<Hours> {
  constexpr Hours() = default;
  explicit constexpr Hours(double v) : Quantity(v) {}
};

// Energy delivered at `power` for `duration`.
[[nodiscard]] constexpr Energy operator*(Power power, Hours duration) {
  return Energy(power.value * duration.value);
}
[[nodiscard]] constexpr Energy operator*(Hours duration, Power power) {
  return power * duration;
}
// Average power that delivers `energy` in `duration`.
[[nodiscard]] constexpr Power operator/(Energy energy, Hours duration) {
  return Power(energy.value / duration.value);
}

// Storage size. The enumerator value is the number of battery modules.
enum class SystemType : std::uint8_t {
  kBasic = 2,
  kStandard = 3,
  kPro = 5,
};

// Number of battery modules for a system type.
[[nodiscard]] constexpr int ModuleCount(SystemType type) {
  return static_cast<int>(type);
}

// Direction of a battery power command.
enum class PowerCommand : std::uint8_t {
  kDischarge,
  kCharge,
};

}  // namespace home_ems

#endif  // HOME_EMS_UNITS_H_
