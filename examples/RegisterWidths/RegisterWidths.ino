/*
  RegisterWidths

  Compile-checks existing Simple_Wire integer APIs and verifies
  sign-extension / width handling for 24, 40, 48, 56, and 64-bit values.

  Register addresses used below do not require connected hardware.
  SignExtend tests run entirely on the MCU. Invalid byteCount tests
  need begin() but do not issue an I2C transaction.
*/

#include <Simple_Wire.h>

Simple_Wire I2C;

static uint32_t failCount = 0;

static void printInt64(int64_t value) {
  uint64_t magnitude;
  if (value < 0) {
    Serial.print('-');
    magnitude = (uint64_t)(-(value + 1)) + 1;
  } else {
    magnitude = (uint64_t)value;
  }

  char digits[21];
  uint8_t index = sizeof(digits);
  digits[--index] = '\0';
  do {
    digits[--index] = (char)('0' + (magnitude % 10));
    magnitude /= 10;
  } while (magnitude != 0);
  Serial.print(&digits[index]);
}

static void expectEqual(const char *name, int64_t actual, int64_t expected) {
  Serial.print(name);
  Serial.print(": ");
  if (actual == expected) {
    Serial.println("PASS");
  } else {
    failCount++;
    Serial.print("FAIL got ");
    printInt64(actual);
    Serial.print(" expected ");
    printInt64(expected);
    Serial.println();
  }
}

static void expectError(const char *name, uint8_t actual, uint8_t expected) {
  Serial.print(name);
  Serial.print(": ");
  if (actual == expected) {
    Serial.println("PASS");
  } else {
    failCount++;
    Serial.print("FAIL got error ");
    Serial.print(actual);
    Serial.print(" expected ");
    Serial.println(expected);
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println(F("Simple_Wire register width checks"));

  // Unsigned 24-bit: high bit clear, so SignExtend is a no-op (library skips it for unsigned T)
  expectEqual("unsigned 24-bit 0x123456", Simple_Wire::SignExtend(0x123456ULL, 24), 0x123456LL);

  // Signed 24-bit positive
  expectEqual("signed 24-bit positive 0x7FFFFF", Simple_Wire::SignExtend(0x7FFFFFULL, 24), 0x7FFFFFLL);

  // Signed 24-bit negative: 0xFFFFFF must become -1, not 16777215
  expectEqual("signed 24-bit negative 0xFFFFFF", Simple_Wire::SignExtend(0xFFFFFFULL, 24), -1LL);

  // Unsigned 40-bit (bit 39 clear)
  expectEqual("unsigned 40-bit 0x123456789A", Simple_Wire::SignExtend(0x123456789AULL, 40), 0x123456789ALL);

  // Signed 40-bit positive
  expectEqual("signed 40-bit positive 0x7FFFFFFFFF", Simple_Wire::SignExtend(0x7FFFFFFFFFULL, 40), 0x7FFFFFFFFFLL);

  // Signed 40-bit negative (sign bit 39 set)
  expectEqual("signed 40-bit negative 0x8000000000", Simple_Wire::SignExtend(0x8000000000ULL, 40), (int64_t)0xFFFFFF8000000000LL);

  // Unsigned 48-bit (bit 47 clear)
  expectEqual("unsigned 48-bit 0x123456789ABC", Simple_Wire::SignExtend(0x123456789ABCULL, 48), 0x123456789ABCLL);

  // Unsigned 56-bit (bit 55 clear)
  expectEqual("unsigned 56-bit 0x123456789ABCDE", Simple_Wire::SignExtend(0x123456789ABCDEULL, 56), 0x123456789ABCDELL);

  // Unsigned 64-bit (no shift by 64)
  expectEqual("unsigned 64-bit high bit", Simple_Wire::SignExtend(0x8000000000000000ULL, 64), (int64_t)0x8000000000000000ULL);

  I2C.begin();

  uint64_t raw = 0;
  I2C.ReadRaw(0x00, 0, &raw);
  expectError("ReadRaw byteCount 0", I2C.GetErrorMessage(), 4);

  I2C.ReadRaw(0x00, 9, &raw);
  expectError("ReadRaw byteCount 9", I2C.GetErrorMessage(), 4);

  // Existing callers must still compile.
  uint16_t u16 = 0;
  int16_t s16 = 0;
  uint32_t u24 = 0;
  uint32_t u32 = 0;
  uint64_t u64 = 0;
  I2C.ReadUInt(0x00, &u16);
  I2C.ReadInt(0x00, &s16);
  I2C.ReadU24(0x00, &u24);
  I2C.ReadU32(0x00, &u32);
  I2C.ReadU64(0x00, &u64);

  // INA228-style 40-bit ENERGY / CHARGE widths.
  uint64_t energy = 0;
  int64_t charge = 0;
  I2C.ReadU40(0x09, &energy);
  I2C.Read40(0x0A, &charge);

  // New generic and convenience APIs (compile coverage).
  int64_t s48 = 0;
  uint64_t u48 = 0;
  int64_t s56 = 0;
  uint64_t u56 = 0;
  I2C.Read48(0x00, &s48);
  I2C.ReadU48(0x00, &u48);
  I2C.Read56(0x00, &s56);
  I2C.ReadU56(0x00, &u56);
  I2C.ReadRaw(0x00, 5, &energy);
  I2C.ReadRawSigned(0x00, 5, &charge);
  I2C.WriteThenRead(0x09, &energy, 1, 5);

  uint8_t buf[5] = {0};
  I2C.ReadRegisterBytes(0x09, buf, 5);
  I2C.WriteRegisterBytes(0x09, buf, 5);
  I2C.WriteU40(0x09, energy);
  I2C.Write40(0x0A, charge);

  (void)u16;
  (void)s16;
  (void)u24;
  (void)u32;
  (void)u64;
  (void)s48;
  (void)u48;
  (void)s56;
  (void)u56;

  Serial.print(F("Failures: "));
  Serial.println(failCount);
}

void loop() {
}
