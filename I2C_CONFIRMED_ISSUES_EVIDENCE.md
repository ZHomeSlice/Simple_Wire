## I2C Confirmed Issues — Evidence Report

This document verifies the two reported issues against the current source without modifying any code. Each section includes exact locations, code excerpts, explanations, trigger conditions, and minimal test sketches.

---

### Issue 1: Large reads across WIRE_BUFFER_LENGTH restart at the same register

1) Exact file and function
- File: `src/Simple_Wire.cpp`
- Function: `template <typename T> Simple_Wire::TRead(uint8_t AltAddress, uint8_t regAddr, uint8_t length, uint8_t byteCount, T *Data)`

2) Exact code block involved

```271:309:C:\Arduino\libraries\Simple_Wire\src\Simple_Wire.cpp
  // for (uint8_t k = 0; k < length * byteCount; k += min(length * byteCount, WIRE_BUFFER_LENGTH / byteCount)) // Process data in chunks based on the Wire buffer length.

  uint16_t totalBytes = (uint16_t)length * byteCount;
  for (uint16_t k = 0; k < totalBytes; k += min<uint16_t>(totalBytes - k, WIRE_BUFFER_LENGTH)) {
    // Check timeout before each chunk
    if (millis() - startTime > _timeoutMs) {
      ErrorMessage = 5; // Timeout
      break;
    }

    Wire.beginTransmission(AltAddress);
    Wire.write(regAddr);
    ErrorMessage = Wire.endTransmission();
    if (Success()) {
      // uint8_t readSize = min(length * byteCount - k, WIRE_BUFFER_LENGTH / byteCount);
      uint8_t readSize = min<uint16_t>(totalBytes - k, WIRE_BUFFER_LENGTH);
      Wire.requestFrom(static_cast<uint8_t>(AltAddress), static_cast<size_t>(readSize)); //

      uint32_t readStartTime = millis();
      while (Wire.available() && I2CReadCount < length) {
        // Check timeout during read
        if (millis() - readStartTime > _timeoutMs) {
          ErrorMessage = 5; // Timeout
          break;
        }

        Data[I2CReadCount] = 0; // Clear the destination value for this word
        uint8_t ByteVal = 0;
        for (int8_t b = byteCount - 1; b >= 0; b--) {
          uint8_t Shift = ReverseByteShift ? (byteCount - 1 - b) * 8 : b * 8;
          if (Wire.available()) {
            ByteVal = Wire.read();
            Data[I2CReadCount] |= ((uint64_t)ByteVal << Shift);
          }
        }
        I2CReadCount++;
      }
    }
  }
```

3) Plain-English explanation
- The code attempts to read large transfers in chunks limited by `WIRE_BUFFER_LENGTH`. For each chunk it:
  - Writes the same `regAddr` again.
  - Ends the write with a STOP (`endTransmission()`).
  - Requests up to `WIRE_BUFFER_LENGTH` bytes.
- Critically, `regAddr` is not advanced by `k` or by previous reads. Each chunk begins at the same register.

4) Why this is wrong or risky
- On devices that require the register pointer to be set each time (common), re-writing the same `regAddr` for each chunk restarts reading from the same place. The second and later chunks therefore duplicate the beginning of the data instead of continuing.
- This corrupts reads when `totalBytes > WIRE_BUFFER_LENGTH`.

5) Specific conditions to trigger
- `length * byteCount > WIRE_BUFFER_LENGTH` (e.g., 40 bytes on AVR with 32-byte buffer).
- Device supports multi-byte reads with auto-increment within a single transaction but resets its internal address when a new transaction writes the same `regAddr` again.

6) Likelihood in normal production use
- Low to Medium. Many sensors read ≤16 bytes at a time. Libraries that attempt to bulk-read large tables or firmware pages would be affected.

7) Minimal test sketch to expose

```cpp
#include <Simple_Wire.h>
Simple_Wire I2C;

// Requires an I2C slave fixture that, given a starting reg X,
// returns incrementing bytes X, X+1, X+2, ... across > 64 bytes.
// (E.g., a second MCU acting as I2C slave at 0x2A.)

const uint8_t ADDR = 0x2A;
const uint8_t START_REG = 0x10;
uint8_t buf[40]; // >32 to exceed typical AVR buffer

void setup() {
  Serial.begin(115200);
  I2C.begin();
  I2C.SetAddress(ADDR);

  I2C.ReadBytes(START_REG, sizeof(buf), buf); // current code path

  // Expect strictly increasing sequence: 0x10, 0x11, ... 0x37
  // With current bug, bytes 32..39 will duplicate 0x10..0x17
  for (int i = 0; i < 40; i++) {
    Serial.print(i); Serial.print(": 0x"); Serial.println(buf[i], HEX);
  }
}
void loop() {}
```

8) Expected result with current code
- First 32 bytes: correct (0x10..0x2F). Bytes 33–40: repeat 0x10..0x17 instead of continuing to 0x30..0x37.

9) Expected result after a correct future fix
- All 40 bytes strictly increasing from 0x10 to 0x37 (or device-specific sequence), with no duplication across the buffer boundary.

10) Risk of changing this behavior
- Medium. Fixing chunking requires either advancing the starting register per chunk or using a single transaction strategy. Device-specific behaviors around internal address auto-increment and STOP/START semantics must be verified to avoid regressions.

Patch Readiness Decision: Needs hardware test first

---

### Issue 2: Partial-element reads can be counted as successful with zero-filled bytes

1) Exact file and function
- Files/Functions:
  - `src/Simple_Wire.cpp` — `template <typename T> Simple_Wire::TRead(...)` (element packing loop)
  - `src/Simple_Wire.cpp` — `template <typename T> Simple_Wire::TWriteThenRead(...)` (element packing loop)

2) Exact code blocks involved

TRead element packing (partial-element risk):

```289:307:C:\Arduino\libraries\Simple_Wire\src\Simple_Wire.cpp
      uint32_t readStartTime = millis();
      while (Wire.available() && I2CReadCount < length) {
        // Check timeout during read
        if (millis() - readStartTime > _timeoutMs) {
          ErrorMessage = 5; // Timeout
          break;
        }

        Data[I2CReadCount] = 0; // Clear the destination value for this word
        uint8_t ByteVal = 0;
        for (int8_t b = byteCount - 1; b >= 0; b--) {
          uint8_t Shift = ReverseByteShift ? (byteCount - 1 - b) * 8 : b * 8;
          if (Wire.available()) {
            ByteVal = Wire.read();
            Data[I2CReadCount] |= ((uint64_t)ByteVal << Shift);
          }
        }
        I2CReadCount++;
      }
```

TWriteThenRead element packing and completeness check:

```80:119:C:\Arduino\libraries\Simple_Wire\src\Simple_Wire.cpp
  // Write register address with repeated start
  Wire.beginTransmission(altAddress);
  Wire.write(regAddr);
  ErrorMessage = Wire.endTransmission(false); // false = repeated start, no STOP

  if (Success()) {
    // Request data with timeout
    Wire.requestFrom(static_cast<uint8_t>(altAddress), static_cast<size_t>(totalBytes), static_cast<bool>(true)); // send STOP after read

    uint32_t startTime = millis();
    uint8_t index = 0;

    while (Wire.available() && index < readLength) {
      readBuffer[index] = 0; // Clear the destination value
      uint8_t byteVal = 0;

      // Read bytes for this element
      for (int8_t b = byteCount - 1; b >= 0; b--) {
        uint8_t shift = ReverseByteShift ? (byteCount - 1 - b) * 8 : b * 8;
        if (Wire.available()) {
          byteVal = Wire.read();
          readBuffer[index] |= ((uint64_t)byteVal << shift);
        }
      }
      index++;

      // Fail fast on timeout
      if (millis() - startTime > _timeoutMs) {
        ErrorMessage = 5; // Timeout
        break;
      }
    }

    I2CReadCount = index;
    if (I2CReadCount != readLength) {
      ErrorMessage = 4; // Incomplete read
    }
  }
```

3) Plain-English explanation
- Both paths build each element by reading `byteCount` bytes in a loop. If bytes run out mid-element, the remaining bytes are left at zero because the inner loop only assigns when `Wire.available()` is true.
- After the inner loop, the element counter (`I2CReadCount` or `index`) is incremented regardless of whether a full `byteCount` bytes were actually read.
- In `TWriteThenRead`, an “incomplete read” error is only set when the number of elements read is less than `readLength`. It does not detect a partially formed last element if at least one byte was read for it (i.e., `index == readLength` but the last element is short).

4) Why this is wrong or risky
- A partially filled element is indistinguishable from a valid value that happens to have zero bits in the missing positions. The operation reports “complete” even though fewer bytes than requested were received.
- This silently corrupts data and hides short-read conditions.

5) Specific conditions to trigger
- Device returns fewer total bytes than requested (e.g., bus glitch, early STOP from slave, or slave designed to return a short frame).
- For `TWriteThenRead`: total bytes returned are ≥ 1 per element but < `readLength * byteCount` (e.g., 5 bytes for `readLength=3` and `byteCount=2`).
- For `TRead`: a chunk boundary or device behavior results in fewer bytes available than expected for the current element.

6) Likelihood in normal production use
- Low to Medium. Most well-behaved devices return full frames. However, noise/bus contention or tight request sizes (e.g., 24-bit values) can expose this, especially near buffer boundaries.

7) Minimal test sketch to expose (controlled short-read)

```cpp
#include <Simple_Wire.h>
Simple_Wire I2C;

// Requires an I2C slave fixture at 0x2A that returns exactly 5 bytes
// when asked for 6, after writing reg 0x20, simulating a short frame.

const uint8_t ADDR = 0x2A;
const uint8_t REG  = 0x20;
uint16_t vals[3]; // 3 elements, byteCount=2 -> expect 6 bytes

void setup() {
  Serial.begin(115200);
  I2C.begin();
  I2C.SetAddress(ADDR);

  // Use WriteThenRead to request 6 bytes total
  I2C.WriteThenRead(REG, vals, 3);

  Serial.print("ErrorMessage="); Serial.println(I2C.GetErrorMessage()); // Likely 0 with current code if bytes split 2,2,1
  Serial.print("I2CReadCount="); Serial.println(I2C.ReadCount());       // 3 elements counted
  Serial.print("vals[0]="); Serial.println(vals[0], HEX);
  Serial.print("vals[1]="); Serial.println(vals[1], HEX);
  Serial.print("vals[2]="); Serial.println(vals[2], HEX); // Partially zero-filled
}
void loop() {}
```

8) Expected result with current code
- `I2CReadCount == 3` (all elements “read”).
- `GetErrorMessage() == 0` (no incomplete-read error).
- `vals[2]` contains only the first received byte in the correct position; missing byte(s) zero-filled.

9) Expected result after a correct future fix
- Either:
  - Do not increment element count when fewer than `byteCount` bytes were read; set an error (e.g., incomplete read).
  - Or explicitly flag a partial-element error even if the element count equals `readLength`.

10) Risk of changing this behavior
- Medium. Some callers may (implicitly) rely on the current permissive behavior (zero-fill). Tightening correctness can surface new errors and require caller-side handling.

Patch Readiness Decision: Needs hardware test first

---

## Bottom line

- Are both findings truly confirmed by source code? Yes. The code clearly restarts at the same `regAddr` for each chunk in large reads, and it clearly counts partially formed elements as complete under certain short-read conditions.
- Which one should be investigated first? Large-read chunking, because it deterministically corrupts data once transfers exceed `WIRE_BUFFER_LENGTH`.
- Is either one dangerous enough to justify changing production code now? Not without tests. Typical production reads are small; fix only after targeted tests to avoid regressions.
- What exact test should be run before approving a code change?
  - Large-read test with an incrementing-data I2C slave (or known device) reading > `WIRE_BUFFER_LENGTH` bytes to verify boundary correctness and absence of duplication.
  - Short-read test where the slave returns fewer bytes than requested (e.g., 5 of 6) to verify partial-element detection and error signaling.

