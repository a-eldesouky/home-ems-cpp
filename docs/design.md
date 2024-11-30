# Design notes

## Components

| Type | Role | Validation |
|---|---|---|
| `PvSystem` | PV output power and voltage; current = P / V | power finite and >= 0, voltage finite and > 0 |
| `House` | Household consumption, voltage, frequency; current = P / V | power finite and >= 0, voltage and frequency finite and > 0 |
| `Grid` | Running totals of energy sold and bought (Wh) | sell and buy amounts finite and >= 0 |
| `BatteryModule` | One module: temperature, voltage, max power | voltage and max power finite and > 0, -20 to 60 °C |
| `Inverter` | Power set point, limited to its rating | rating finite; set point finite and \|power\| <= rating |
| `BatteryStorage` | One inverter, at least one module, optional energy model | command within the module, inverter and state of charge limits |
| `EnergySystem` | Owns one of each of the above | at most `ModuleCount(type)` modules |

All quantities are strong types that wrap a `double`: `Power` (W), `Energy`
(Wh), `Hours`, `Voltage`, `Current`, `Frequency` and `Temperature`.
`Power * Hours` gives `Energy` and `Energy / Hours` gives `Power`; no other
mixing compiles. No component holds a pointer to another; `EnergySystem` owns
everything by value and passes references to `ControlStep` for each step.

Every check is written so that NaN fails it. A call that throws leaves the
object unchanged.

## Control rule

`ControlStep` runs once per input sample, for a step of `dt` hours:

```text
net    = pv_power - house_power
rating = min(inverter rating, sum of module ratings)

if rating <= 0 or dt is not finite and > 0:
    abort the step, report the error, change nothing

if net > 0:                                     # surplus
    battery = min(net, MaxChargePower(dt))      # charge, positive command
    export  = net - battery                     # sold if > 0
else:                                           # shortage, including 0
    battery = -min(-net, MaxDischargePower(dt)) # discharge, negative
    import  = -net - min(-net, MaxDischargePower(dt))

grid sold   += export * dt
grid bought += import * dt
```

Without an energy model, `MaxChargePower` and `MaxDischargePower` both return
`rating`, so the step length only scales the grid totals.

## State of charge model

`StateOfChargeModel` is optional. When it is set:

```text
capacity = module_capacity * number of modules
stored   = initial_soc * capacity              (before the first step)

room      = max(0, max_soc * capacity - stored)
available = max(0, stored - min_soc * capacity)

MaxChargePower(dt)    = min(rating, room / (dt * charge_efficiency))
MaxDischargePower(dt) = min(rating, available * discharge_efficiency / dt)

after a step at power P (positive charges):
    P > 0:  stored += P * dt * charge_efficiency
    P <= 0: stored += P * dt / discharge_efficiency
```

`P` is the power at the inverter (the AC side). Charging draws `P * dt` and
stores less because of losses; discharging removes more than it delivers.
After the update the stored energy is clamped to `[min_soc, max_soc]` to
remove rounding error; the limits already keep it in range.

Validation: capacity finite and > 0, `0 <= min_soc < max_soc <= 1`,
`initial_soc` in `[min_soc, max_soc]`, efficiencies in `(0, 1]`.

## Storage commands

`BatteryStorage::SetPowerCommand(command, power, dt)` checks, in order: `dt`,
that the power is finite, the module ratings, and (with an energy model) the
room or energy left for this step. Then the inverter checks its own rating.
The command and stored energy change only after every check passes, so on
`OverloadError` or `InvalidParameterError` nothing changes.

## Battery voltage

`Inverter::BatteryVoltage` returns the mean module voltage. Modules count as
matching when each one is within `max(absolute, relative * |v0|)` of the first
module's voltage `v0`. The default `VoltageTolerance` is 0.5 V or 1%. This
function is not used by the control rule.

## Errors

Constructors and setters throw `InvalidParameterError`, `HardwareFailureError`
or `OverloadError` (all derived from standard exception types). `ControlStep`
does not throw: any exception during a step is caught, the step stops, and the
message is returned in `ControlResult::error` with `battery_command` set to 0.
The CLI checks the inverter limit and the step length before it builds the
system, so this path is reached only through the library API.

## Input and output

Input is a CSV file with the header `time,pv_w,load_w`. `time` is a label.
Powers must be plain decimal numbers (no surrounding whitespace, no
hexadecimal) that are finite and `>= 0`. A value too small to represent reads
as 0. A bad row stops the run with an error that names the line number.

If the first `time` is `HH:MM`, every row must be `HH:MM` and exactly
`--step-minutes` after the previous one (wrapping at midnight). Any other
first label turns this check off.

Output is one CSV row per input row on stdout:

```text
time,pv_w,load_w,net_w,battery_w,grid_export_w,grid_import_w,soc_pct,error
```

`battery_w` is positive when charging and negative when discharging.
`soc_pct` is empty without an energy model. Energy totals for the run go to
stderr, so stdout stays a plain CSV file.

## Known limits

- The state of charge model is a simple energy balance. It has no
  temperature effects, ageing, self-discharge, or power limits that depend
  on the state of charge.
- One efficiency value applies to every power level.
- Every row has the same length; the time column is checked, not used to
  compute the length.
