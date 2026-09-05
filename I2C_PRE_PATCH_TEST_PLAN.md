## Test-First Validation Plan (Pre-Patch)

Purpose: Prove current behaviors on real hardware or a controlled I2C slave fixture before any production code is changed. Scope: Only the two confirmed issues in `I2C_CONFIRMED_ISSUES_EVIDENCE.md`.

---

## 1) Test environment

- Arduino tooling
  - Arduino IDE 2.3.x (or newer), or PlatformIO in VSCode (platforms: atmelavr, espressif32, rp2040).
  - Set Serial monitor to 115200 baud.
- Target boards
  - AVR: Arduino Uno/Nano/Pro Mini (5V), Mega2560 optional.
  - ESP32: DevKitC or similar (3.3V).
  - RP2040: Raspberry Pi Pico or Pico W (3.3V).
- I2C slave fixture options (any one)
  - A second Arduino/ESP32/RP2040 running the “Minimal Slave” sketch provided below.
  - A programmable I2C slave (logic analyzer + pattern generator or Bus Pirate/GreatFET) scripted to emulate behavior.
  - Known-good sensor/device that auto-increments reads and can return long contiguous data (less preferred because it’s harder to control short-reads deterministically).
- Required hardware
  - 2× MCU boards (one master, one fixture slave).
  - Pull-up resistors: 4.7 kΩ on SDA/SCL to correct I/O voltage (3.3V for ESP32/RP2040; 5V for AVR unless level shifted).
  - Common GND between boards.
  - Optional logic analyzer for validation.
- Expected Wire buffer size per platform (verify at compile time by `Serial.println(WIRE_BUFFER_LENGTH)`)
  - AVR: 32 bytes (typical `BUFFER_LENGTH=32`).
  - ESP32/ESP8266: 128 bytes (`I2C_BUFFER_LENGTH` → `WIRE_BUFFER_LENGTH=128` per header).
  - RP2040: core-dependent; many define 256, some 32/128. Verify and record.
- Serial output
  - 115200 baud, newline terminated. Report inputs, received data, and pass/fail result lines.

---

## 2) Test 1 — Large read across WIRE_BUFFER_LENGTH

- Purpose
  - Demonstrate that reads larger than the platform’s `WIRE_BUFFER_LENGTH` duplicate data across the chunk boundary because each chunk restarts at the same `regAddr`.
- Exact condition
  - `length * byteCount > WIRE_BUFFER_LENGTH` for a byte-based read (e.g., 40 bytes on AVR).
- Required slave behavior
  - On receiving a starting register value X, return an incrementing sequence: X, X+1, X+2, ..., for as many bytes as the master requests (auto-increment behavior).
- Minimal master sketch (uses Simple_Wire as-is)

```cpp
#include <Simple_Wire.h>
Simple_Wire I2C;

const uint8_t SLAVE_ADDR = 0x2A;  // Match slave sketch
const uint8_t START_REG  = 0x10;
const size_t  N          = 40;    // > WIRE_BUFFER_LENGTH on AVR (32)
uint8_t buf[N];

void setup() {
  Serial.begin(115200);
  while(!Serial) {}
  I2C.begin();                    // Use library defaults
  I2C.SetAddress(SLAVE_ADDR);

  I2C.ReadBytes(START_REG, N, buf);

  // Print received block
  for (size_t i=0;i<N;i++) {
    Serial.print(i); Serial.print(",");
    Serial.println(buf[i], HEX);
  }
  // Simple verdict: check if strictly increasing by 1 from START_REG
  bool ok = true;
  for (size_t i=0;i<N;i++) {
    uint8_t expect = (uint8_t)(START_REG + i);
    if (buf[i] != expect) { ok = false; break; }
  }
  Serial.println(ok ? "PASS_STRICT_INCREASING" : "FAIL_DUPLICATION_OR_MISALIGN");
}
void loop() {}
```

- Minimal slave sketch (incrementing data source)

```cpp
#include <Wire.h>

// Acts as an I2C slave at 0x2A.
// Protocol: Master writes one byte "start register".
// On request, slave returns an incrementing stream starting at that register.

const uint8_t SLAVE_ADDR = 0x2A;
volatile uint8_t startReg = 0x00;

void onReceiveService(int howMany) {
  if (howMany > 0) {
    startReg = (uint8_t)Wire.read();
    while (Wire.available()) Wire.read(); // Drain extras if sent
  }
}

void onRequestService() {
  static uint8_t current = 0;
  current = startReg;
  // Send as many bytes as the master asks; Wire library handles the actual count
  // by repeatedly calling onRequest until the requested length is satisfied.
  // For simplicity, send up to 32 bytes per callback; master may re-enter if needed.
  for (uint8_t i=0; i<32; i++) {
    Wire.write(current++);
  }
}

void setup() {
  Wire.begin(SLAVE_ADDR);
  Wire.onReceive(onReceiveService);
  Wire.onRequest(onRequestService);
}
void loop() {}
```

- Expected current failing behavior
  - The first `WIRE_BUFFER_LENGTH` bytes are correct. The next chunk restarts from `START_REG` (duplicates), yielding a “FAIL_DUPLICATION_OR_MISALIGN” verdict.
- Expected future passing behavior
  - Entire N-byte read is strictly increasing from `START_REG` without duplication across the boundary.
- Pass/fail criteria
  - PASS: Printed “PASS_STRICT_INCREASING” and byte-by-byte matches `START_REG+i`.
  - FAIL: Any duplication or reset near the chunk boundary; or printed “FAIL_DUPLICATION_OR_MISALIGN”.
- Notes on auto-increment devices
  - Many devices auto-increment on continuous reads. This test forces chunk restarts to show that rewriting `regAddr` per chunk can reset the device’s pointer, causing duplication.

---

## 3) Test 2 — Partial-element short read

- Purpose
  - Show that the library counts a partially read element as “read” and may report success even when the final element is short (zero-filled).
- Exact condition
  - Request `readLength * byteCount` bytes; slave returns fewer bytes but at least 1 byte for the final element (e.g., 5 bytes for 3×uint16_t).
- Required slave behavior
  - For a specific register (e.g., 0x20), return a deliberately short frame (5 bytes) regardless of the requested 6 bytes.
- Minimal master sketch

```cpp
#include <Simple_Wire.h>
Simple_Wire I2C;

const uint8_t SLAVE_ADDR = 0x2A;
const uint8_t REG_SHORT  = 0x20;  // Triggers 5-byte response from slave
uint16_t vals[3];                 // Expect 3 elements (6 bytes), slave sends 5

void setup() {
  Serial.begin(115200);
  while(!Serial) {}
  I2C.begin();
  I2C.SetAddress(SLAVE_ADDR);

  I2C.WriteThenRead(REG_SHORT, vals, 3);

  Serial.print("ErrorMessage="); Serial.println(I2C.GetErrorMessage());
  Serial.print("I2CReadCount="); Serial.println(I2C.ReadCount());
  Serial.print("v0="); Serial.println(vals[0], HEX);
  Serial.print("v1="); Serial.println(vals[1], HEX);
  Serial.print("v2="); Serial.println(vals[2], HEX); // Likely partially zero-filled

  // Verdicts
  bool countedAll = (I2C.ReadCount() == 3);
  bool noError    = (I2C.GetErrorMessage() == 0);
  Serial.println((countedAll && noError) ? "FAIL_PARTIAL_UNDETECTED" : "PASS_DETECTED_SHORT_READ");
}
void loop() {}
```

- Minimal slave sketch (forced short frame on a specific register)

```cpp
#include <Wire.h>

const uint8_t SLAVE_ADDR = 0x2A;
volatile uint8_t startReg = 0x00;

void onReceiveService(int howMany) {
  if (howMany > 0) {
    startReg = (uint8_t)Wire.read();
    while (Wire.available()) Wire.read();
  }
}

void onRequestService() {
  // If startReg == 0x20, send 5 bytes only; else send complete frames.
  static uint8_t s = 0;
  s = startReg;
  uint8_t toSend = (startReg == 0x20) ? 5 : 32; // 32 is arbitrary max per callback
  for (uint8_t i=0; i<toSend; i++) {
    Wire.write(s++);
  }
}

void setup() {
  Wire.begin(SLAVE_ADDR);
  Wire.onReceive(onReceiveService);
  Wire.onRequest(onRequestService);
}
void loop() {}
```

- Expected current failing behavior
  - `I2CReadCount == 3`, `ErrorMessage == 0`, and `vals[2]` is partially zero-filled (only 1 byte received), resulting in “FAIL_PARTIAL_UNDETECTED”.
- Expected future passing behavior
  - Library flags an incomplete/partial element (error set) and/or does not count the final element as read when fewer than `byteCount` bytes were received, yielding “PASS_DETECTED_SHORT_READ”.
- Pass/fail criteria
  - PASS (current code): For evidence, the test should FAIL (prints “FAIL_PARTIAL_UNDETECTED”) proving the issue exists.
  - PASS (future code): After a fix, the test should print “PASS_DETECTED_SHORT_READ” without changing the test sketches.
- Confirming incorrect success reporting
  - The combination of `I2CReadCount == readLength` and `ErrorMessage == 0` with an obviously malformed last element confirms the issue.

---

## 4) Production impact check

- Examples/common usage in this library
  - No `examples/` directory included; `README.md` shows typical reads of small counts (e.g., 3× int16 for sensor axes) and bit/mask operations.
- Likely production usage patterns
  - Large reads > `WIRE_BUFFER_LENGTH`: unlikely in typical sensor drivers; risk mainly for bulk/EEPROM/table reads.
  - Multi-byte typed reads: very common (e.g., `ReadInts`, `ReadUInts`). Short-reads could create zero-filled partial values if the bus is noisy or device misbehaves.
  - `WriteThenRead` (repeated-start): used by devices requiring register set + repeated-start read; common in sensors.
  - STOP vs repeated-start: varies by device; current code supports both (default STOP for `TRead`, repeated-start in `WriteThenRead`).
  - Auto-increment devices: common; large-read bug only shows when transfers exceed buffer and chunking restarts.

---

## 5) Patch approval checklist (must all hold before merging any change)

- Current failing behavior reproduced:
  - Test 1 shows duplication across chunk boundary.
  - Test 2 shows partial-element counted and no error.
- Proposed patch reviewed (narrowly scoped, no behavior surprises).
- Same tests pass after patch (no duplication; partial elements detected/reported).
- Normal small-read behavior unchanged (sensor reads of a few bytes still OK).
- Missing-device behavior unchanged.
- Wrong-address behavior unchanged.
- Timeout behavior unchanged.
- Existing compilation OK for all targets.
- Public API unchanged unless explicitly approved (and versioned).

---

Notes:
- Keep voltage domains correct (3.3V vs 5V) and use level shifting if mixing boards.
- If using a logic analyzer, capture the boundary between chunks in Test 1 to confirm repeated write of the same `regAddr`.
- Record and report the actual `WIRE_BUFFER_LENGTH` printed per board in test logs. 
