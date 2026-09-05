## Executive Summary

- The `Simple_Wire` library is a lightweight wrapper around Arduino `Wire.h` providing a fluent, type-generic API for I2C register reads/writes, bit/mask operations, and a repeated-start write-then-read helper.
- It is production-used and appears stable for common, small I2C transactions. Current behavior should be preserved for now.
- Two confirmed edge-condition bugs exist:
  - Large reads that exceed the Wire buffer are chunked incorrectly and re-start at the same register for each chunk, returning duplicated/incorrect data.
  - Partially available bytes during a single element read are silently zero-filled while still counting the element as “read”.
- Several design assumptions (default 400 kHz clock, ESP32 default SDA/SCL pins, short scan timeout) should be documented; timeouts and error propagation are partially implemented and platform-dependent.
- Recommendation: leave code unchanged for now; document behavior and add tests to protect current usage patterns. Plan targeted fixes guarded by tests to avoid regressions.

## Library Purpose

Provide a simple, chainable API over `Wire.h` to:
- Set device address and endianness.
- Read/write registers as 8/16/24/32/64-bit values and arrays.
- Read/write by bit positions and masks.
- Perform repeated-start write-then-read transactions.
- Scan and check devices on the bus.

## Current File Structure

- `src/Simple_Wire.h` — Public class, inline overloads, templates, constants/macros.
- `src/Simple_Wire.cpp` — Implementation: `begin`, `TRead`, `TWrite`, `TWriteThenRead`, scanner, helpers.
- `library.properties` — Arduino metadata (`architectures=*`).
- `README.md` — Overview of purpose and example concept.
- `.gitattributes`, `LICENSE`.

No `examples/` folder is present.

## Public API Reference

Class: `Simple_Wire`
- Construction and init
  - `Simple_Wire()`
  - `void begin(int sdaPin=0, int sclPin=1)` — initializes `Wire` and sets 400 kHz by platform. See platform branches.
  - `Simple_Wire& SetAddress(uint8_t address)`
  - `Simple_Wire& SetIntMSBPos(bool FirstRead)` — toggles byte order packing for multi-byte values.
  - `Simple_Wire& SetTimeout(uint32_t timeoutMs=100)`; `uint32_t GetTimeout()`
  - `Simple_Wire& SetVerbose(bool=true)`
  - `Simple_Wire& Delay(uint32_t ms)` — blocking delay; returns `*this`.
  - `Simple_Wire& This_Wire()` — returns `*this`.
  - `uint8_t GetAddress()`
- Status/introspection
  - `uint8_t ReadCount()`, `uint8_t WriteCount()`
  - `bool ReadSuccess()`, `bool WriteSucess()` [spelling as-is]
  - `uint8_t GetErrorMessage()` — 0 (OK), 1–4 (`Wire` endTransmission codes), 5 (timeout/other, library-defined).
  - `bool Success(bool TF=true)` — returns `ErrorMessage==0` if `TF` is true; inverted if false.
- Device discovery
  - `Simple_Wire& I2C_Scanner()`
  - `uint8_t Find_Address(uint8_t Limit=128)` and `uint8_t Find_Address(uint8_t Address, uint8_t Limit)`
  - `uint8_t Check_Address(uint8_t Address, bool verbose=false)` and convenience `Check_Address()`
  - Deprecated: `uint8_t CheckAddress()` returns `devAddr` (not a check)
- IO primitives (fluent, returning `Simple_Wire&`)
  - Bit/mask reads: `ReadBit`, `ReadBitM`, and 16-bit variants.
  - Typed reads: `ReadSByte/Bytes`, `ReadByte/Bytes`, `ReadInt/Ints`, `ReadUInt/UInts`, `Read24/U24`, `Read32/U32`, `Read64/U64`, and array variants.
  - Bit/mask writes: `WriteBit[X]`, `WriteBitM[X]`, 16-bit variants with `SkipRead` options.
  - Typed writes: `WriteSByte/Bytes`, `WriteByte/Bytes`, `WriteInt/Ints`, `WriteUInt/UInts`, `Write24/U24`, `Write32/U32s`, `Write64/U64s`.
  - Repeated-start helpers: `WriteThenRead` overloads for 8/16/32/64-bit buffers.

See signatures in:
- `src/Simple_Wire.h` lines ~85–329 (public API surface).

## Internal Flow and Design

- `begin(...)` configures `Wire` once and stores pins. 400 kHz clock is set on all platforms (with `Wire.setWireTimeout(3000, true)` on AVR only).
- Reads (`TRead<T>`) perform:
  - `Wire.beginTransmission(addr)`; `Wire.write(reg)`; `Wire.endTransmission()` (STOP).
  - `Wire.requestFrom(addr, readSize)`; pack `byteCount` bytes into each element using endianness switch `ReverseByteShift`. Increments `I2CReadCount` per element.
  - Chunking attempts to respect `WIRE_BUFFER_LENGTH`.
- Writes (`TWrite<T>`) perform:
  - `Wire.beginTransmission(addr)`; `Wire.write(reg)`; write `length*ByteC` bytes; `Wire.endTransmission()` (STOP).
- Repeated start (`TWriteThenRead<T>`) uses `endTransmission(false)` then `requestFrom(...)` with STOP after read; packs elements similarly.
- Helpers for scanning/validation (`I2C_Scanner`, `Find_Address`, `Check_Address`).
- Error handling: `ErrorMessage` mirrors `Wire.endTransmission()` results; additional codes used for timeout or incomplete results in some paths.

## I2C Communication Behavior

- Register access pattern (default): write register address with STOP, then perform read with a new START (`TRead`). Some devices require repeated start; use `WriteThenRead` family in that case.
- Repeated-start path: `endTransmission(false)` then `requestFrom(..., true)`.
- Clock speed: set to 400 kHz unconditionally in `begin()` across platforms.
- Addressing: 7-bit addresses, 1–127 range in scanner.

## Error Handling and Recovery

- `ErrorMessage` values:
  - 0: success
  - 1–3: `Wire` NACK/data-too-long conditions from `endTransmission()`
  - 4: “Other” (used for incomplete read/write in some paths)
  - 5: Library-defined timeout
- `Success()` convenience accessor; `ReadCount()/WriteCount()` show elements processed.
- AVR only: `Wire.setWireTimeout(3000, true)` in `begin()` (3 ms, reset on timeout). Other platforms rely on platform defaults.

## Timing, Blocking, and Timeout Behavior

- Default `_timeoutMs = 100`. Used for:
  - Element read/write loops and chunk loops.
  - Scanner overall-time budget (`~2×timeoutMs`) for full bus scan.
  - Note: `Wire.endTransmission()` / `requestFrom()` are synchronous and may block until bus transactions complete or core timeouts trigger. Only AVR sets a Wire-level timeout explicitly.
- `yield()` is invoked before long operations to keep cooperative schedulers healthy.
- `Delay(ms)` is blocking.

## Buffer and Memory Review

- Effective buffer macro: `WIRE_BUFFER_LENGTH` (derived from `I2C_BUFFER_LENGTH`, `BUFFER_LENGTH`, or defaults: 128 on ESP32/ESP8266, else 32).
- `TRead` tries to chunk by `WIRE_BUFFER_LENGTH` bytes. `TWrite` does not chunk and may rely on `Wire` to reject overlong TX (error 1).
- Packing uses a local accumulator per element; no dynamic allocation; stack usage is low.

## Platform and Dependency Assumptions

- Depends on `Arduino.h` and `Wire.h`.
- `architectures=*` per `library.properties`.
- Pin handling:
  - ESP32/ESP8266: `Wire.begin(sdaPin, sclPin, 400000)`. Defaults are `(0,1)` unless caller overrides.
  - RP2040: `setSDA`, `setSCL`, then `begin()`.
  - AVR/other: default `Wire.begin()`; 400 kHz clock.
- External pullups, device readiness, and bus state are assumed to be valid.

## Production Behavior Notes

- Typical device transactions (few bytes) are well covered and likely why this library is reliable in production.
- Devices that need a repeated start are supported via `WriteThenRead` helpers.
- Larger reads/writes near or above `WIRE_BUFFER_LENGTH` may be unreliable (see Findings).

## Confirmed Bugs

1) Large-read chunking restarts at same register
- Location: `src/Simple_Wire.cpp` lines ~271–309 (chunk loop in `TRead<T>`).
- Current behavior: Each chunk begins by writing the same `regAddr` and issues a new `requestFrom`. The `regAddr` is not advanced by the previous chunk’s length.
- Why it matters: If `totalBytes > WIRE_BUFFER_LENGTH`, subsequent chunks will re-read from the same starting register, corrupting results (duplicate data) and misaligning the destination array.
- Risk: High (for large reads); None for small reads below buffer.
- Confidence: High.
- Action: Do not change now; document and test first.
- Suggested test: Read a known-incrementing register block larger than buffer; compare to a reference one-shot read on a platform with larger buffer or a device simulator.
- Future change: Advance `regAddr` (or use device auto-increment within a single transaction) or compute chunking by element count and maintain register offset.

2) Partial-element reads counted as success
- Location:
  - `src/Simple_Wire.cpp` lines ~289–307 (`TRead<T>` inner packing).
  - `src/Simple_Wire.cpp` lines ~92–111 (`TWriteThenRead<T>` inner packing).
- Current behavior: When bytes run out mid-element, remaining bytes are not read (left as zero) but the element is still counted as read (`I2CReadCount++` / `index++`). `TWriteThenRead` only flags incomplete if fewer elements than requested are read; it cannot detect partial bytes within an element.
- Why it matters: Silent truncation/zero-fill can produce incorrect data without an error.
- Risk: Medium (appears only when devices return fewer bytes than expected).
- Confidence: High.
- Action: Document now; test before changing.
- Suggested test: Force device/simulator to return N-1 bytes for an N-byte element and assert error or element not counted.
- Future change: Track per-element byte completeness; only increment count if all bytes read; set error for partial elements.

## Possible Issues

1) Default ESP32 pins `(sda=0, scl=1)` if `begin()` is called without arguments
- Location: `src/Simple_Wire.h` line ~88.
- Why it matters: Many ESP32 boards default to SDA=21, SCL=22; `(0,1)` may be invalid, leading to nonfunctional I2C if caller forgets to override.
- Risk: Medium (usage dependent).
- Confidence: High.
- Action: Document; no change now.
- Suggested test: Basic device detection with/without explicit pins on ESP32.

2) `I2C_Scanner` early-stop timeout may be too aggressive
- Location: `src/Simple_Wire.cpp` lines ~141–145.
- Behavior: Stops scanning if elapsed time exceeds `2 × _timeoutMs` (default 200 ms).
- Why it matters: On slower buses or heavy stretch, a full 1–127 address scan might exceed this and stop early.
- Risk: Low–Medium.
- Confidence: Medium.
- Action: Document; test on slower devices.
- Suggested test: Scan while another device holds the bus briefly; observe if scanning completes.

3) `Check_Address` pre-check timeout cannot detect a stall inside `endTransmission`
- Location: `src/Simple_Wire.cpp` lines ~184–193.
- Behavior: Compares elapsed time before calling `endTransmission()`, not after.
- Why it matters: It won’t guard an internal stall (relies on platform Wire timeouts, only set on AVR).
- Risk: Medium (platform dependent).
- Confidence: Medium.
- Action: Document; rely on platform timeouts; test on ESP32/ESP8266/RP2040 for lockup scenarios.
- Suggested test: Hold SCL low to force a bus lock and observe return paths.

4) No retry policy
- Behavior: Single-attempt operations; on transient NACKs, caller must retry.
- Risk: Low–Medium.
- Confidence: High.
- Action: Document; test before adding optional retry hooks.

## Design Risks

- Unconditional 400 kHz clock (`begin()` across platforms).
  - Some devices specify 100 kHz max or require configuration time; faster clock may cause intermittent NACKs after reset.
- `TWrite` not chunked: Very long writes will rely on `Wire` buffer rejection (error 1). Caller may not anticipate buffer limits.
- Endianness toggle (`SetIntMSBPos`) is global to the instance; mixing devices with different endianness on the same instance is error-prone.
- Exposed `Success(bool)` inversion parameter is non-obvious; misuse could invert checks silently.

## Documentation Gaps

- Clarify when to use `WriteThenRead` vs the default `TRead` path.
- Document default 400 kHz and platform pin defaults; note device constraints.
- Explain endianness control and typical values for common devices.
- Explain `ErrorMessage` mapping and when 4 vs 5 are used.
- Note `ReadCount`/`WriteCount` count elements (not bytes).
- Deprecated `CheckAddress()` returns `devAddr` (name misleading).
- `i2cErrorMessages` declared in header but not defined/used.
- Typo in `WriteSucess` name (documented as-is, do not change).

## Modernization Opportunities (defer)

- Fix large-read chunking and partial-element accounting (with tests).
- Optionally support configurable I2C clock and keep current 400 kHz as default.
- Add optional per-operation retries and post-transaction small delays (configurable).
- Unify timeout handling around platform support; consider using `endTransmission(false)` where repeated start is safer.
- Provide `examples/` sketches covering common devices and flows.
- Remove unused macros/declarations (e.g., `printHex`, `i2cErrorMessages` extern) after confirming no external dependencies.

## Recommended Test Plan

- Compile test across AVR, ESP8266, ESP32, RP2040.
- Basic device detection test
  - Call `begin`, `SetAddress`, `Check_Address`, and `I2C_Scanner` on a board with a known device.
- Known-good device read/write test
  - Read fixed ID registers; write/read a config register; verify endianness toggle.
- Missing device behavior
  - Set an unused address; confirm `Check_Address` returns false and error codes make sense.
- Wrong address behavior
  - Read/write at a wrong address and verify error handling and no hangs.
- Bus lockup behavior (if safe)
  - Hold SCL/SDA to simulate lock; verify platform timeouts (especially on non-AVR) and library responses.
- Timeout behavior
  - Set `_timeoutMs` low and force slow responses; verify timeout=5 mapping and non-blocking loops.
- Repeated read/write stress test
  - Loop thousands of small reads/writes; track `ReadCount`/`WriteCount`, memory stability, and no drift.
- Long-running production-style test
  - 24–72 hour loop at normal rates; ensure no hangs or memory growth.
- Multiple device test (if applicable)
  - Switch `devAddr` across devices; confirm endianness and timeouts per device.
- Power-cycle recovery test
  - Power-cycle the I2C peripheral while MCU runs; verify recovery without restart when possible.
- Large-read test
  - Request reads larger than `WIRE_BUFFER_LENGTH`; confirm current behavior (duplicate/incorrect) and lock in via tests before any fix.
- Behavior comparison before/after any future change
  - Golden master tests capturing current outputs for representative devices and sizes.

## Safe Future Development Plan

1) Add tests to capture current successful behaviors and reproduce edge conditions (large reads, partial-element reads).
2) Fix large-read chunking with register offset progression or single-transaction reads, guarded behind tests.
3) Fix partial-element accounting (only count full elements; flag error otherwise).
4) Expose optional configuration for I2C clock and scanner timeout, defaulting to today’s behavior.
5) Add examples and documentation clarifying repeated-start usage and endianness controls.

## Change Approval Checklist

- Tests in place for:
  - Small typical reads/writes
  - Repeated start flows
  - Large reads across buffer boundary
  - Partial-element returns
  - Timeouts and scanner behavior
- No behavior changes without corresponding passing tests.
- Document any API surface changes (names, defaults) separately and version accordingly.

---

## Findings Catalog (with locations)

Each item includes current behavior, why it matters, risk, confidence, action, and a suggested test/change if appropriate.

1) Confirmed bug — Large-read chunking restarts at same register
- File/Location: `src/Simple_Wire.cpp` ~271–309 (`TRead<T>` chunk loop)
- Behavior: Rewrites the same `regAddr` for each chunk; does not advance register offset.
- Impact: Incorrect data for `totalBytes > WIRE_BUFFER_LENGTH`.
- Risk: High; Confidence: High.
- Action: Test first; then fix chunk offseting.

2) Confirmed bug — Partial-element reads counted as success
- File/Location: `src/Simple_Wire.cpp` ~289–307 (`TRead<T>`); ~92–111 (`TWriteThenRead<T>`)
- Behavior: If data ends mid-element, unread bytes are zero; element still counted.
- Impact: Silent data corruption.
- Risk: Medium; Confidence: High.
- Action: Test first; then only count fully formed elements.

3) Possible issue — ESP32 default pins (0,1) when not specified
- File/Location: `src/Simple_Wire.h` line ~88; `begin()` use in `src/Simple_Wire.cpp` ~41–43
- Behavior: Defaults may be invalid on common boards.
- Impact: No I2C if user forgets pins.
- Risk: Medium; Confidence: High.
- Action: Document; consider safer defaults later.

4) Possible issue — `I2C_Scanner` early-stop
- File/Location: `src/Simple_Wire.cpp` ~141–145
- Behavior: Global 2×timeoutMs budget; may abort full scan.
- Impact: Could miss devices under slow/busy bus.
- Risk: Low–Medium; Confidence: Medium.
- Action: Document; consider making configurable later.

5) Design risk — Pre-`endTransmission` timeout check in `Check_Address`
- File/Location: `src/Simple_Wire.cpp` ~184–193
- Behavior: Checks elapsed time before the blocking call; cannot catch in-call stalls.
- Impact: Depends on platform-level timeouts (only AVR set explicitly).
- Risk: Medium; Confidence: Medium.
- Action: Document; enhance after tests.

6) Design risk — Unconditional 400 kHz
- File/Location: `begin()` platform branches: `src/Simple_Wire.cpp` ~39–53
- Impact: Devices requiring 100 kHz may intermittently fail.
- Risk: Medium; Confidence: Medium.
- Action: Document; make configurable in future.

7) Documentation gap — Error codes and helpers
- File/Location: `src/Simple_Wire.h` ~68 (unused `i2cErrorMessages` declaration), ~270 (deprecated `CheckAddress`), ~286 (`Success(bool)`), ~275 (`WriteSucess`).
- Impact: Confusing semantics for new users; minor naming typo.
- Risk: Low; Confidence: High.
- Action: Document now; modernize later with care.

8) Modernization opportunity — Examples folder
- Behavior: No `examples/` to demonstrate flows.
- Impact: Onboarding friction.
- Risk: Low; Confidence: High.
- Action: Add curated examples later.

