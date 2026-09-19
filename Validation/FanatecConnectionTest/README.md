# V3 Fanatec cable connection regression test

Run from the repository root:

```sh
python3 Validation/FanatecConnectionTest/run.py
```

Requires a C++17 compiler with AddressSanitizer and UndefinedBehaviorSanitizer.
`CXX` can select the compiler. On macOS with Command Line Tools selected explicitly:

```sh
SDKROOT=/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk \
CXX=/Library/Developer/CommandLineTools/usr/bin/clang++ \
python3 Validation/FanatecConnectionTest/run.py
```

The test compiles the actual V3 `FanatecInterface.cpp` against simulated UART,
GPIO, clock and EEPROM. It exercises late insertion, requests received during
insertion debounce, contact bounce, unplugging in each handshake stage,
reconnection, repeated first requests, stage timeouts, fragmented handshake
packets, clock wraparound, callback state, and bounded receive bursts.

This does not validate electrical detection, UART wire timing, or a real
Fanatec wheelbase. Hardware acceptance: boot the bridge without the cable,
insert it with the wheelbase powered, disconnect during negotiation, then
reinsert after a successful session; repeat without rebooting either device.
GPIO 16 must indicate cable presence (HIGH). A permanently LOW detection pin
requires wiring/electrical diagnosis. A wheelbase restart that never changes
the detection pin is not automatically detected by this cable-only change.
