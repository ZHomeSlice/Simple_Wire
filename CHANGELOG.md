# Simple_Wire changelog

## 2.1.0

### New functions

- `Read40` / `ReadU40`, `Read48` / `ReadU48`, `Read56` / `ReadU56` and matching writes (`Write40` / `WriteU40`, etc.), including alternate-address overloads.
- Generic 1–8 byte integer access: `ReadRaw`, `ReadRawSigned`, `WriteRaw`, `WriteRawSigned`.
- Raw bus-order buffers: `ReadRegisterBytes`, `WriteRegisterBytes`.
- `WriteThenRead(..., readLength, byteCount)` so a 40-bit register can be read into a `uint64_t` without transferring eight bytes. Existing three-argument `WriteThenRead` calls still use `sizeof(T)`.
- Public `Simple_Wire::SignExtend(value, bitWidth)` for partial-width signed values.

### Sign-extension correction

Signed non-native widths are assembled as unsigned, then sign-extended.

`Read24()` of `0xFFFFFF` now yields `-1` (`0xFFFFFFFF`), not `16777215`. Positive 24-bit values are unchanged. The same helper is used for signed 40/48/56-bit reads.

### 40/48/56-bit support

These widths use the existing `TRead` / `TWrite` path with byte counts 5, 6, and 7. Signed values use `int64_t`; unsigned values use `uint64_t`.

### Generic N-byte support

`ReadRaw` / `WriteRaw` take an explicit `byteCount` of 1 through 8. Invalid counts set `ErrorMessage` to 4 and do not start an I2C transaction. `TRead` no longer silently clamps `byteCount` with `constrain()`.

### Backward compatibility

Existing public method names and signatures are unchanged. Default integer packing remains most-significant byte first (`SetIntMSBPos(false)`), matching INA228/INA238. `WriteSucess` spelling is unchanged. Error codes 0–3 and 5 keep their previous meanings; code 4 now also covers invalid `byteCount`.

### Bugs discovered (not all changed)

- Missing explicit `TRead<int8_t>` / `TWrite<int8_t>` instantiations (fixed). `ReadSByte` / `WriteSByte` could fail to link.
- `WriteBitX` / `WriteBit` comments were inverted vs `SkipRead`. Comments now match behavior; runtime is unchanged. `X` helpers skip the current-register read; non-`X` helpers read-modify-write.
- `WriteBitTemplate` with `length == 1` computes a local value that is never written; `Val` is not shifted into `bitNum`. Callers that pass `Val == 1` for a bit other than 0 may clear the bit. Left unchanged for compatibility.
- Large `TRead` chunking still restarts at the same `regAddr` (see `I2C_PATCH_PLAN_NO_EDITS.md`). Not changed in this release.
- Partial-element short reads can still be counted as complete. Not changed in this release.

### Byte order

Integer reads pack incoming bytes using `SetIntMSBPos()`. Default (`false`) places the first received byte in the highest position (MSB first). `ReadRegisterBytes` / `WriteRegisterBytes` copy bytes as received or sent and do not pack them as integers.
