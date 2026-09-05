## Smallest-Safe Patch Plan (No Code Edits Yet)

Purpose: Prepare narrowly scoped, test-first changes for the two confirmed issues without altering any source code at this stage. All behavior changes must be proven on hardware per the revised pre-patch test plan.

References:
- `I2C_LIBRARY_REVIEW.md`
- `I2C_CONFIRMED_ISSUES_EVIDENCE.md`
- `I2C_PRE_PATCH_TEST_PLAN_REVISED.md`

---

## Preconditions (must be met before any patch work)

- Reproduce both failures using the revised test plan and log template.
- Record `WIRE_BUFFER_LENGTH`, I2C clock, boards, voltage, pullups, and analyzer notes (for Test 1).
- Preferably validate Test 1 with a known-good long-register device (EEPROM/FRAM) or a verified slave fixture; validate Test 2 with a fixture that truly returns fewer bytes than requested.

---

## Issue 1 — Large reads across WIRE_BUFFER_LENGTH restart at the same register

1) Current behavior summary
- When `length*byteCount > WIRE_BUFFER_LENGTH`, `TRead` processes data in chunks. Each chunk writes the same `regAddr` and starts a new transaction, causing the second and later chunks to re-read from the same starting register instead of continuing. This duplicates data across the boundary.

2) Exact source file and function involved
- File: `src/Simple_Wire.cpp`
- Function: `template <typename T> Simple_Wire::TRead(uint8_t AltAddress, uint8_t regAddr, uint8_t length, uint8_t byteCount, T *Data)`

3) Minimal intended behavior change
- For chunked reads, continue reading from where the prior chunk ended, producing a continuous stream of `length*byteCount` bytes with no duplication across the `WIRE_BUFFER_LENGTH` boundary.

4) Why this change is needed
- Current behavior deterministically corrupts large reads, making results incorrect for sequential-register devices and any caller expecting continuous data.

5) Possible regression risks
- Devices with non-standard auto-increment rules or page boundaries could need specific handling.
- Some devices require a STOP/START between pages; altering the access pattern could affect them.
- Timing-sensitive parts may behave differently when transactions are structured differently.

6) Devices or use cases that could be affected
- EEPROM/FRAM/large register-map sensors, or drivers that attempt large bulk reads.
- Platforms where I2C buffer limits are small (e.g., AVR 32 bytes), increasing the likelihood of chunking.

7) Required tests before patching
- Test 1 from `I2C_PRE_PATCH_TEST_PLAN_REVISED.md`:
  - Reproduce duplication with N > `WIRE_BUFFER_LENGTH`.
  - Prefer logic analyzer confirmation of repeated `regAddr` per chunk.

8) Required tests after patching
- Test 1 must pass (strictly increasing sequence, no duplication).
- Normal small-read tests must still pass.
- Missing device, wrong address, and timeout behavior unchanged.
- Long-running stress and multi-platform compile tests pass.

9) Proposed implementation strategy (plain English)
- Option A (advance `regAddr` per chunk): Compute the register offset corresponding to bytes already read and write `regAddr + offset` before each subsequent chunk.
  - Pros: Minimal change; preserves current STOP/START pattern; deterministic continuation.
  - Cons: Assumes linear register space; some devices may cross page boundaries differently.
  - Risk: Medium (device-dependent).
- Option B (no chunked support for registers): If `length*byteCount > WIRE_BUFFER_LENGTH`, return an explicit error or require the caller to use a dedicated “sequential read” helper.
  - Pros: Safest for unknown devices; prevents silent corruption.
  - Cons: Behavior change for existing callers; may break current (even if incorrect) flows.
  - Risk: Medium (compat impact).
- Option C (add separate safe helper later): Leave `TRead` as-is, document limitation, and introduce a future `ReadSequentialLarge` that guarantees proper continuation.
  - Pros: Zero risk to current users; clear semantics for large reads.
  - Cons: Requires callers to migrate; does not help existing code paths until adopted.
  - Risk: Low (if documented), but delays fix.
- Option D (document-only): If production does not perform large reads, document the limitation and defer code changes.
  - Pros: No regression risk.
  - Cons: Known incorrect behavior remains; future users may be surprised.
  - Risk: Low now, but latent.

10) Public API change?
- Should remain unchanged for the smallest safe correction (Option A).
- Options B/C could introduce new behavior (error) or a new helper; require explicit approval if chosen.

---

## Issue 2 — Partial-element reads counted as successful with zero-filled bytes

1) Current behavior summary
- During element packing, if fewer than `byteCount` bytes are available, the code still increments the element count and leaves missing bytes as zeros. `TWriteThenRead` considers the read complete when the number of elements equals `readLength` even if the final element is partial.

2) Exact source file and function involved
- File: `src/Simple_Wire.cpp`
- Functions:
  - `template <typename T> Simple_Wire::TRead(...)` (element packing loop)
  - `template <typename T> Simple_Wire::TWriteThenRead(...)` (element packing loop and completeness check)

3) Minimal intended behavior change
- Only count an element as read if all `byteCount` bytes are received. If a partial element occurs, flag the operation as incomplete.

4) Why this change is needed
- Partially filled elements produce silently corrupted values and can mask real short-read or bus issues.

5) Possible regression risks
- Some callers may rely on zero-fill permissiveness; changing to strict accounting could surface new errors/warnings.
- Platforms or devices that occasionally short-read due to timing might produce more “incomplete” errors until caller logic is adjusted.

6) Devices or use cases that could be affected
- Any multi-byte register read (16/24/32/64-bit) on noisy buses, or devices that legitimately return fewer bytes in some modes.

7) Required tests before patching
- Test 2 from `I2C_PRE_PATCH_TEST_PLAN_REVISED.md`:
  - Use a fixture that truly returns fewer bytes than requested.
  - Confirm via logic analyzer or `requestFrom` byte count that the short read is real.

8) Required tests after patching
- Test 2 must pass (operation flagged as incomplete and/or final element not counted).
- Normal small reads remain unaffected.
- Missing device, wrong address, and timeout behavior unchanged.

9) Proposed implementation strategy (plain English)
- Option A (strict per-element accounting): Track the exact bytes read for the current element; increment `I2CReadCount` only if `byteCount` bytes were actually received; otherwise set “incomplete” error.
  - Pros: Minimal internal change; strong correctness; no API change.
  - Cons: May expose errors in caller flows relying on zero-fill.
  - Risk: Medium (compat perception), low technical risk.
- Option B (flag incomplete but preserve zero-filled value): Keep the partially assembled value, but set `ErrorMessage` to indicate incomplete read even if the element count equals `readLength`.
  - Pros: Preserves value observation for debugging; clearer error state.
  - Cons: Ambiguity (value present but invalid); still a behavior change vs silent success.
  - Risk: Medium (caller expectations).
- Option C (document-only): If production relies on zero-fill and never treats these as errors, document the behavior and defer changes.
  - Pros: Zero regression risk.
  - Cons: Keeps a data-integrity hazard in place.
  - Risk: Low now, but latent.

10) Public API change?
- Should remain unchanged. Behavior correction can be internal (Options A/B).

---

## Final Recommendation (no code edits yet)

- Patch order:
  - Patch Issue 1 first. It is deterministic once N exceeds `WIRE_BUFFER_LENGTH` and can be validated clearly with a logic analyzer or EEPROM/FRAM device.
  - Patch Issue 2 second, after confirming a true short-read fixture.
- Patch separately:
  - Yes. Address each issue with its own tests and review to isolate risk and simplify rollback if needed.
- Safest first code change:
  - Issue 1, Option A: Advance `regAddr` by previously read bytes (or equivalent element offset) for each chunk in `TRead`, maintaining current STOP/START pattern. This is the smallest internal correction and preserves API/flow.
- Tests that must pass before approving that first change:
  - Pre-patch Test 1 shows duplication and analyzer confirms repeated `regAddr`.
  - Post-patch Test 1 passes with strictly increasing data; small-read, missing-device, wrong-address, timeout behaviors unchanged.
  - Multi-platform builds remain green.
- Behavior that must remain unchanged:
  - Public API signature and chaining semantics.
  - Small typical reads and `WriteThenRead` behavior for devices depending on repeated-start.
  - Error code mapping and timeouts.
- What should be documented even after patching:
  - The effective `WIRE_BUFFER_LENGTH` per platform.
  - Guidance on when to use `WriteThenRead` vs default read flow.
  - Clarification that large sequential reads are supported across buffer boundaries (post-patch), and any device caveats (page boundaries, STOP/START requirements).

---

## Approval Gate

No code changes should be made until:
- The pre-patch tests reproduce the failures on hardware and logs are captured using the provided template, and
- For Issue 1, the failure is preferably confirmed by a logic analyzer or a known-good long-register device,
- And you explicitly approve one specific patch option per issue (starting with Issue 1, Option A as recommended).
