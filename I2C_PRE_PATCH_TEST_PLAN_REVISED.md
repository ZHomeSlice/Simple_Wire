## Test-First Validation Plan (Pre-Patch) — Revised

Scope: Validate, on real hardware or a controlled I2C slave fixture, the two confirmed issues from `I2C_CONFIRMED_ISSUES_EVIDENCE.md` before any production code change. No edits to `Simple_Wire.h/.cpp`.

---

## 1) Test environment

- Arduino tooling
  - Arduino IDE 2.3.x+ or PlatformIO (platforms: atmelavr, espressif32, rp2040).
  - Serial monitor: 115200 baud, newline-terminated.
- Master target boards (choose at least one)
  - AVR: Uno/Nano/Pro Mini (5V) — recommended for Test 1 simplicity (`WIRE_BUFFER_LENGTH` typically 32).
  - ESP32 DevKitC (3.3V) — if used as master, ensure fixture can transmit at least 128 bytes per request.
  - RP2040 (3.3V) — verify its effective `WIRE_BUFFER_LENGTH` at runtime.
- I2C slave fixture options (pick one, but verify capability first)
  1) Controlled MCU slave (Arduino/ESP32/RP2040) running a custom slave sketch.
     - IMPORTANT: Confirm the slave core can transmit up to the master’s `WIRE_BUFFER_LENGTH` bytes in one request. Many cores limit slave TX to their own buffer (e.g., ~32 bytes on AVR). If the master requests more than the slave can transmit per request, you will not get the intended long chunk.
  2) Known-good long-register device (preferred for reliability):
     - I2C EEPROM/FRAM with sequential read and auto-increment (e.g., Microchip 24LC256; Fujitsu MB85RC256, etc.). These reliably stream long reads across page boundaries.
     - For EEPROM, respect write-cycle timing if preloading data patterns.
- Required hardware
  - 2× MCUs (one master, one slave fixture) or 1× master + 1× EEPROM/FRAM breakout.
  - I2C pullups: 4.7 kΩ to correct voltage rail (3.3V or 5V). Level shift if mixing 5V AVR and 3.3V peripherals.
  - Common GND. Keep I2C lines short (≤20–30 cm) to reduce noise.
  - Optional logic analyzer with I2C decode.
- Make `WIRE_BUFFER_LENGTH` explicit on the master
  - Add a line in each master sketch: `Serial.print("WIRE_BUFFER_LENGTH="); Serial.println(WIRE_BUFFER_LENGTH);`
  - Choose N (the total requested byte count) so that N > `WIRE_BUFFER_LENGTH` on the master.
  - Record the printed value in the test log.

---

## 2) Verify the slave fixture behavior (before Test 1/2)

- Do NOT assume `Wire.onRequest()` is called multiple times per one master request. Many Arduino cores invoke `onRequest` once per I2C read transaction; the slave must provide all requested bytes from its internal TX buffer in one callback.
- Self-test to confirm slave capability (run with a simple Wire-only master first):
  - Master (temporary diagnostic) should call `Wire.requestFrom(SLAVE_ADDR, K)` for K in {8, 16, 32, 64, 128} and print how many bytes were actually received; also dump the sequence.
  - PASS if the slave reliably returns a strictly incrementing sequence of exactly K bytes for at least the master’s `WIRE_BUFFER_LENGTH` value.
  - If the slave cannot exceed its own TX buffer (e.g., AVR slave limited to 32 bytes), either:
    - Use an AVR master (32-byte `WIRE_BUFFER_LENGTH`) so chunk size ≤ slave TX, or
    - Switch the slave fixture to a platform/device capable of larger TX (ESP32 slave or external EEPROM/FRAM) when testing ESP32/RP2040 masters.

---

## 3) Test 1 — Large read across WIRE_BUFFER_LENGTH (reliable variants)

- Purpose
  - Prove that when `length*byteCount > WIRE_BUFFER_LENGTH`, `Simple_Wire` reads duplicate the first chunk because each chunk rewrites the same `regAddr` (instead of continuing).
- Exact condition under test
  - N = total bytes requested with `ReadBytes(reg, N, buf)`, where `N > WIRE_BUFFER_LENGTH` on the master.
- Fixture options
  - Option A (MCU slave, verified): Use a slave board/core that can TX at least `WIRE_BUFFER_LENGTH` bytes in one request for the master platform. See Section 2 self-test.
  - Option B (preferred): Use a known-good EEPROM/FRAM device that supports long sequential reads and auto-increments its internal address.
    - Preload a “ramp” pattern (0x10, 0x11, …) if device content is unknown, or choose a start address with known monotonic data.
- Minimal master sketch (AVR master example with N=40 for clarity)

```cpp
#include <Simple_Wire.h>
Simple_Wire I2C;

const uint8_t SLAVE_ADDR = 0x2A;  // Or EEPROM address
const uint8_t START_REG  = 0x10;
const size_t  N          = 40;    // ensure N > WIRE_BUFFER_LENGTH
uint8_t buf[N];

void setup() {
  Serial.begin(115200);
  while(!Serial) {}
  Serial.print("WIRE_BUFFER_LENGTH="); Serial.println(WIRE_BUFFER_LENGTH);
  I2C.begin();
  I2C.SetAddress(SLAVE_ADDR);

  I2C.ReadBytes(START_REG, N, buf);

  for (size_t i=0;i<N;i++) { Serial.print(i); Serial.print(","); Serial.println(buf[i], HEX); }
  bool ok = true;
  for (size_t i=0;i<N;i++) {
    uint8_t expect = (uint8_t)(START_REG + i);
    if (buf[i] != expect) { ok = false; break; }
  }
  Serial.println(ok ? "PASS_STRICT_INCREASING" : "FAIL_DUPLICATION_OR_MISALIGN");
}
void loop() {}
```

- Minimal slave sketch (MCU fixture; set TX burst to match master’s buffer)
  - For AVR master: TX ≤ 32 bytes is sufficient.
  - For ESP32 master: ensure slave platform/core can TX ≥ 128 bytes (consider ESP32 slave or EEPROM/FRAM instead).
  - Add a “fixture self-test mode” printing maximum supported TX length and showing a short sample ramp via Serial.
- Expected current behavior (fail)
  - First `WIRE_BUFFER_LENGTH` bytes correct; subsequent chunk restarts at `START_REG` and duplicates earlier bytes → “FAIL_DUPLICATION_OR_MISALIGN”.
- Expected behavior after a fix (pass)
  - Entire N-byte read strictly increasing from `START_REG` without duplication.
- Pass/fail criteria
  - PASS: “PASS_STRICT_INCREASING” and data matches `START_REG+i` for all i.
  - FAIL: Any reset/duplication around the chunk boundary or printed failure line.
- Logic analyzer validation (preferred)
  - Capture SCL/SDA during the read:
    - Confirm that each chunk begins with a write of the same `regAddr` (e.g., 0x10) before the read.
    - Confirm the second read chunk restarts from the same register byte (duplicate) instead of the next register.
    - Record START/STOP, address (RW), register byte, and requested length (if decoder supports it).

---

## 4) Test 2 — Partial-element short read (reliable variants)

- Purpose
  - Show that a partially received element is counted as “read” and no error is reported when fewer bytes than requested are returned.
- Exact condition under test
  - Request 3×uint16_t (6 bytes) but the slave returns only 5 bytes for a specific register.
- Fixture options and reliability notes
  - Some Arduino slave cores always serve up to their TX buffer size and do not provide a straightforward way to NACK mid-frame. Returning “fewer than requested” bytes must be confirmed:
    - Use a slave/platform that can limit the actual transmitted bytes (e.g., ESP32 IDF-based slave, Bus Pirate/GreatFET scripting, or a custom peripheral that ends with NACK after 5 bytes).
    - If using Arduino Wire as slave, instrument with logic analyzer and Serial on master to verify that exactly 5 bytes were clocked and received (not padded/filled by the core).
  - If your fixture cannot guarantee a true short read, document it and skip Test 2 on that setup, or switch to a capable emulator/device.
- Minimal master sketch

```cpp
#include <Simple_Wire.h>
Simple_Wire I2C;

const uint8_t SLAVE_ADDR = 0x2A;
const uint8_t REG_SHORT  = 0x20;  // Fixture: return exactly 5 bytes when asked for 6
uint16_t vals[3];

void setup() {
  Serial.begin(115200);
  while(!Serial) {}
  Serial.print("WIRE_BUFFER_LENGTH="); Serial.println(WIRE_BUFFER_LENGTH);
  I2C.begin();
  I2C.SetAddress(SLAVE_ADDR);

  I2C.WriteThenRead(REG_SHORT, vals, 3); // expects 6 bytes

  Serial.print("ErrorMessage="); Serial.println(I2C.GetErrorMessage());
  Serial.print("I2CReadCount="); Serial.println(I2C.ReadCount());
  Serial.print("v0="); Serial.println(vals[0], HEX);
  Serial.print("v1="); Serial.println(vals[1], HEX);
  Serial.print("v2="); Serial.println(vals[2], HEX);

  bool countedAll = (I2C.ReadCount() == 3);
  bool noError    = (I2C.GetErrorMessage() == 0);
  Serial.println((countedAll && noError) ? "FAIL_PARTIAL_UNDETECTED" : "PASS_DETECTED_SHORT_READ");
}
void loop() {}
```

- Minimal slave approach (examples)
  - ESP32-IDF-based I2C slave (or MCU/emulator) configured to NACK the 6th byte for `REG_SHORT`.
  - Bus Pirate/GreatFET script that serves only 5 bytes for the transaction.
  - If Arduino-based slave is used, verify via logic analyzer that only 5 bytes were clocked; otherwise this test is inconclusive on that platform.
- Expected current behavior (fail)
  - `I2CReadCount == 3`, `ErrorMessage == 0`, `vals[2]` partially zero-filled → “FAIL_PARTIAL_UNDETECTED”.
- Expected behavior after a fix (pass)
  - Partial element triggers an error and/or the final element is not counted: “PASS_DETECTED_SHORT_READ”.
- Pass/fail criteria
  - PASS (pre-patch evidence): Observe the failing line above on current code with a proven short-read fixture.
  - PASS (post-fix): Same test yields detection/flagging of partial element without changing the sketches.
- Confirming actual returned length
  - Use logic analyzer to count bytes clocked during the read.
  - Optionally, add a separate Wire-only diagnostic on the master that prints the return value of `Wire.requestFrom(...)` for the short-read register to verify fewer bytes were actually delivered by the fixture.

---

## 5) Test log template (copy/paste per run)

```
Date:
Master board (rev/clock):
Slave board/device (core/version):
Voltage level (V):
Pullups (kΩ) and rail:
WIRE_BUFFER_LENGTH (master printed):
I2C clock (printed/assumed):
Test sketch name:
Fixture variant (MCU/EEPROM/other):
Expected result:
Actual serial output (paste):
Logic analyzer notes (start/stop/addr/reg/chunk behaviors):
Pass/Fail:
Decision (proceed/adjust setup/change fixture/stop):
```

---

## 6) Production impact check (unchanged, with emphasis on reliability)

- No `examples/` directory in this repo; common usage is small multi-byte sensor reads (e.g., 3×int16). 
- Large reads > `WIRE_BUFFER_LENGTH`: uncommon in typical sensor flows; high risk only in bulk memory/table reads.
- Multi-byte typed reads: common; short-read risk exists under noisy buses or misbehaving devices/fixtures.
- Repeated-start: used when devices require it; current tests preserve existing behavior.
- Auto-increment devices: common; Test 1 uses this to reveal chunk duplication when N > `WIRE_BUFFER_LENGTH`.

---

## 7) Patch approval checklist (must all pass before merging any change)

- Failure reproduced on hardware by serial output:
  - Test 1: duplication across chunk boundary (and, preferably, logic analyzer confirmation of repeated `regAddr` writes).
  - Test 2: partial-element counted as success with zero-fill, confirmed by fixture actually returning fewer bytes (analyzer or `requestFrom` count).
- Proposed patch reviewed and narrowly scoped to the confirmed fault.
- Same tests pass after patch (no duplication; partial-element detected).
- Normal small-read behavior unchanged.
- Missing-device behavior unchanged.
- Wrong-address behavior unchanged.
- Timeout behavior unchanged.
- Existing code still compiles across supported targets.
- Public API unchanged unless explicitly approved.

---

## 8) Production decision rule

No production code change is approved unless:
- The failure is reproduced and captured via serial logs, and
- For the large-read test, it is preferably confirmed by logic analyzer traces or a known-good long-register device (EEPROM/FRAM) that auto-increments.

---

## Bottom line

- Is the current test plan safe to run as-is?
  - Yes, if you verify the slave fixture first and select platform pairings that respect the master’s `WIRE_BUFFER_LENGTH`. Use level shifting where needed and keep cables short.
- What must be corrected before testing?
  - Confirm the slave can TX at least one full master chunk size in a single request; print and log `WIRE_BUFFER_LENGTH` on the master; select N accordingly; add analyzer capture for Test 1; ensure Test 2 uses a fixture that truly shortens the read.
- Which test should be run first?
  - Test 1 (large-read) on AVR master (N=40) with a verified fixture (AVR slave or EEPROM/FRAM). It is deterministic and easy to confirm by analyzer.
- What result would justify moving to patch planning?
  - Test 1 shows duplicated data across the chunk boundary (and analyzer confirms same `regAddr` at chunk starts); and Test 2 shows partial-element counted as success with a verified short-read fixture. Both should be logged with the template above.

