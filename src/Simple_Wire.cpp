/* ============================================
Simple_Wire device library code is placed under the MIT license
Copyright (c) 2022 Homer Creutz

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES, OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT, OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
Version 2.0 3-3-2025 Streamlined, added features and improved performance
===============================================
*/

#include "Simple_Wire.h"
#include <Wire.h>

Simple_Wire::Simple_Wire() { // Constructor
}

// When we use Simple_Wire class
// We must call begin() Before anything else related to i2c.
void Simple_Wire::begin(int sdaPin, int sclPin) {
  _Begin = true;
#ifdef __AVR__
  Wire.begin();

  Wire.setClock(400000);           // 400kHz I2C clock.
  Wire.setWireTimeout(3000, true); // timeout value in uSec
#elif defined(ESP8266) || defined(ESP32)
  Wire.begin(sdaPin, sclPin, (uint32_t)400000); // 400kHz I2C clock.

#elif defined(ARDUINO_ARCH_RP2040)
  Wire.setSCL(sclPin);
  Wire.setSDA(sdaPin);
  Wire.begin();
  Wire.setClock(400000); // 400kHz I2C clock.
#else

  Wire.begin();
  Wire.setClock(400000); // 400kHz I2C clock.

#endif
  _sdaPin = sdaPin;
  _sclPin = sclPin;
}

// ESP32 optimized WriteThenRead with repeated start (template version)
template <typename T>
Simple_Wire &Simple_Wire::TWriteThenRead(uint8_t regAddr, T *readBuffer, uint8_t readLength) {
  return TWriteThenRead(devAddr, regAddr, readBuffer, readLength);
}

template <typename T>
Simple_Wire &Simple_Wire::TWriteThenRead(uint8_t altAddress, uint8_t regAddr, T *readBuffer, uint8_t readLength) {
  if (!_Begin) {
    ErrorMessage = 4; // Not initialized
    return *this;
  }

  I2CReadCount = 0;
  ErrorMessage = 0;
  yield();

  // Calculate byte count for the template type
  uint8_t byteCount = sizeof(T);
  uint8_t totalBytes = readLength * byteCount;

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

  return *this;
}

// Set timeout for operations
Simple_Wire &Simple_Wire::SetTimeout(uint32_t timeoutMs) {
  _timeoutMs = timeoutMs;
  return *this;
}

// Scan for i2c Devices
Simple_Wire &Simple_Wire::I2C_Scanner() {
  if (!_Begin)
    return *this;
  byte error, address;
  int nDevices;
  uint32_t scanStartTime = millis();

  Serial.println("\nScanning I2C for any device...");
  nDevices = 0;
  for (address = 1; address < 128; address++) {
    yield();

    // Check overall scan timeout
    if (millis() - scanStartTime > _timeoutMs * 2) { // Allow 2x timeout for full scan
      Serial.println("Scan timeout - stopping early");
      break;
    }

    if (Check_Address(address)) {
      Serial.print("device found at address 0x");
      Serial.println(address, HEX);
      nDevices++;
    }
  }
  if (nDevices == 0) {
    Serial.println("No I2C devices found");
  } else {
    Serial.print(nDevices);
    Serial.println(" I2C devices found");
    Serial.println("done");
  }
  return *this;
}

// Find and return first found address within range 0x40 to 0x4F for example
uint8_t Simple_Wire::Find_Address(uint8_t Address, uint8_t Limit) {
  yield();
  do {
    //  Serial.println(Address, HEX);
    if (Check_Address(Address))
      return Address;
  } while (Limit >= Address++);
  return 0;
}

// checks to see if the address is active and returns true if successful
uint8_t Simple_Wire::Check_Address(uint8_t Address, bool verbose) {
  if (!_Begin)
    return false;
  ErrorMessage = 0;
  if (Verbose || verbose) {
    Serial.print("Checking Address: 0x");
    Serial.println(Address, HEX);
  }

  uint32_t startTime = millis();
  Wire.beginTransmission(Address);

  // Check timeout before endTransmission
  if (millis() - startTime > _timeoutMs) {
    ErrorMessage = 5; // Timeout
    return false;
  }

  ErrorMessage = Wire.endTransmission();
  return (ErrorMessage == 0);
}

template <typename T>
Simple_Wire &Simple_Wire::ReadBitTemplate(uint8_t AltAddress, uint8_t regAddr, uint8_t length, uint8_t bitNum, T *Data) {
  T b = 0;
  T Mask;
  TRead<T>(AltAddress, regAddr, 1, sizeof(T), &b);
  if (I2CReadCount != 0) {
    if (length == 1)
      Data[0] = ((b & (1 << bitNum)) > 0);
    else {

      Mask = (((static_cast<T>(1) << length) - 1) << (bitNum - length + 1));
      b &= Mask;
      b >>= (bitNum - length + 1);
      Data[0] = b;
    }
  }
  Val = (uint64_t)Data[0]; // Optionally assign first value to Val.
  return *this;
}
template <typename T>
Simple_Wire &Simple_Wire::ReadBitMaskTemplate(uint8_t AltAddress, uint8_t regAddr, T Mask, T *Data) {
  T b = 0;
  TRead<T>(AltAddress, regAddr, 1, sizeof(T), &b);
  if (I2CReadCount != 0) {
    b &= Mask;
    Data[0] = b;
  }
  Val = (uint64_t)Data[0]; // Optionally assign first value to Val.
  return *this;
}
// Write Bits using Bit number and length

template <typename T>
Simple_Wire &Simple_Wire::WriteBitTemplate(uint8_t AltAddress, uint8_t regAddr, uint8_t length, uint8_t bitNum, bool SkipRead, T Val) {
  T b = 0;
  T Mask;
  Mask = (((static_cast<T>(1) << length) - 1) << (bitNum - length + 1));
  if (length == 1)
    b = (Val != 0) ? (b | (static_cast<T>(1) << bitNum)) : (b & ~(static_cast<T>(1) << bitNum));
  else {
    Val <<= (bitNum - length + 1); // shift Data into correct position
  }
  WriteBitMaskTemplate<T>(AltAddress, regAddr, SkipRead, Mask, Val);
  return *this;
}

// Write Bits using mask
template <typename T>
Simple_Wire &Simple_Wire::WriteBitMaskTemplate(uint8_t AltAddress, uint8_t regAddr, bool SkipRead, T Mask, T Val) {
  T b = 0;
  if (!SkipRead)
    TRead<T>(AltAddress, regAddr, 1, sizeof(T), &b);
  Val &= Mask;  // zero all non-important bits in Data
  b &= ~(Mask); // clear the bits in existing value
  b |= Val;     // merge the new bits
  TWrite<T>(AltAddress, regAddr, 1, sizeof(T), &b);
  return *this;
}

// These Template functions are private and that will eliminate the possibility of Unsupported data types
// Template version that works for both signed and unsigned 16-bit types.
// byteCount is the number of Bytes in the Template <typename T> so for an int32_t Data the byteCount = 4 (or 3 if you wanted to only get 24 bits)
// When Length is greater than 1 Data will be assumed to be an array of "Length"  so if length = 2 and byteCount =- 2 then 2 16 bit integers will be stored in data as an array of size 2
template <typename T>
Simple_Wire &Simple_Wire::TRead(uint8_t AltAddress, uint8_t regAddr, uint8_t length, uint8_t byteCount, T *Data) {
  if (!_Begin)
    return *this;
  I2CReadCount = 0;
  ErrorMessage = 0;
  yield();
  byteCount = constrain(byteCount, 1, 8);

  uint32_t startTime = millis();

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

  Val = (uint64_t)Data[0]; // assign the first value to Val. use .Value() to retrieve the value and dont forget to cast it back to the type you are getting.
                           // Example int16_t Data;  Serial.print((int16_t) INA.ReadInt(0x07U,&Data).value());
  return *this;
}

// Template version that works for both signed and unsigned 16-bit types.
template <typename T>
Simple_Wire &Simple_Wire::TWrite(uint8_t AltAddress, uint8_t regAddr, uint8_t length, uint8_t ByteC, T *Data) {
  if (!_Begin)
    return *this;
  I2CWriteCount = 0;
  ErrorMessage = 0;
  yield();

  uint32_t startTime = millis();

  Wire.beginTransmission(AltAddress);
  Wire.write(regAddr); // send register address

  // Write each value, sending ByteC bytes per element.
  for (uint8_t i = 0; i < length; i++) {
    // Check timeout before each element
    if (millis() - startTime > _timeoutMs) {
      ErrorMessage = 5; // Timeout
      break;
    }

    // Send MSB and LSB according to your defined shift values
    for (int8_t b = ByteC - 1; b >= 0; b--) {
      uint8_t Shift = ReverseByteShift ? (ByteC - 1 - b) * 8 : b * 8;
      Wire.write((uint8_t)(Data[i] >> Shift)); // send Byte
    }
    I2CWriteCount++;
  }

  // Check timeout before final transmission
  if (millis() - startTime > _timeoutMs) {
    ErrorMessage = 5; // Timeout
  } else {
    ErrorMessage = Wire.endTransmission();
  }

  return *this;
}

// Read
template Simple_Wire &Simple_Wire::ReadBitTemplate(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t *);
template Simple_Wire &Simple_Wire::ReadBitTemplate(uint8_t, uint8_t, uint8_t, uint8_t, uint16_t *);
template Simple_Wire &Simple_Wire::ReadBitMaskTemplate(uint8_t, uint8_t, uint8_t, uint8_t *);
template Simple_Wire &Simple_Wire::ReadBitMaskTemplate(uint8_t, uint8_t, uint16_t, uint16_t *);
template Simple_Wire &Simple_Wire::TRead<uint8_t>(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t *);
template Simple_Wire &Simple_Wire::TRead<int16_t>(uint8_t, uint8_t, uint8_t, uint8_t, int16_t *);
template Simple_Wire &Simple_Wire::TRead<uint16_t>(uint8_t, uint8_t, uint8_t, uint8_t, uint16_t *);
template Simple_Wire &Simple_Wire::TRead<int32_t>(uint8_t, uint8_t, uint8_t, uint8_t, int32_t *);
template Simple_Wire &Simple_Wire::TRead<uint32_t>(uint8_t, uint8_t, uint8_t, uint8_t, uint32_t *);
template Simple_Wire &Simple_Wire::TRead<int64_t>(uint8_t, uint8_t, uint8_t, uint8_t, int64_t *);
template Simple_Wire &Simple_Wire::TRead<uint64_t>(uint8_t, uint8_t, uint8_t, uint8_t, uint64_t *);

// Write
template Simple_Wire &Simple_Wire::WriteBitTemplate(uint8_t, uint8_t, uint8_t, uint8_t, bool, uint8_t);
template Simple_Wire &Simple_Wire::WriteBitTemplate(uint8_t, uint8_t, uint8_t, uint8_t, bool, uint16_t);
template Simple_Wire &Simple_Wire::WriteBitMaskTemplate(uint8_t, uint8_t, bool, uint8_t, uint8_t);
template Simple_Wire &Simple_Wire::WriteBitMaskTemplate(uint8_t, uint8_t, bool, uint16_t, uint16_t);
template Simple_Wire &Simple_Wire::TWrite<uint8_t>(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t *);
template Simple_Wire &Simple_Wire::TWrite<int16_t>(uint8_t, uint8_t, uint8_t, uint8_t, int16_t *);
template Simple_Wire &Simple_Wire::TWrite<uint16_t>(uint8_t, uint8_t, uint8_t, uint8_t, uint16_t *);
template Simple_Wire &Simple_Wire::TWrite<int32_t>(uint8_t, uint8_t, uint8_t, uint8_t, int32_t *);
template Simple_Wire &Simple_Wire::TWrite<uint32_t>(uint8_t, uint8_t, uint8_t, uint8_t, uint32_t *);
template Simple_Wire &Simple_Wire::TWrite<int64_t>(uint8_t, uint8_t, uint8_t, uint8_t, int64_t *);
template Simple_Wire &Simple_Wire::TWrite<uint64_t>(uint8_t, uint8_t, uint8_t, uint8_t, uint64_t *);

// WriteThenRead template instantiations
template Simple_Wire &Simple_Wire::TWriteThenRead<uint8_t>(uint8_t, uint8_t *, uint8_t);
template Simple_Wire &Simple_Wire::TWriteThenRead<uint8_t>(uint8_t, uint8_t, uint8_t *, uint8_t);
template Simple_Wire &Simple_Wire::TWriteThenRead<int8_t>(uint8_t, int8_t *, uint8_t);
template Simple_Wire &Simple_Wire::TWriteThenRead<int8_t>(uint8_t, uint8_t, int8_t *, uint8_t);
template Simple_Wire &Simple_Wire::TWriteThenRead<uint16_t>(uint8_t, uint16_t *, uint8_t);
template Simple_Wire &Simple_Wire::TWriteThenRead<uint16_t>(uint8_t, uint8_t, uint16_t *, uint8_t);
template Simple_Wire &Simple_Wire::TWriteThenRead<int16_t>(uint8_t, int16_t *, uint8_t);
template Simple_Wire &Simple_Wire::TWriteThenRead<int16_t>(uint8_t, uint8_t, int16_t *, uint8_t);
template Simple_Wire &Simple_Wire::TWriteThenRead<uint32_t>(uint8_t, uint32_t *, uint8_t);
template Simple_Wire &Simple_Wire::TWriteThenRead<uint32_t>(uint8_t, uint8_t, uint32_t *, uint8_t);
template Simple_Wire &Simple_Wire::TWriteThenRead<int32_t>(uint8_t, int32_t *, uint8_t);
template Simple_Wire &Simple_Wire::TWriteThenRead<int32_t>(uint8_t, uint8_t, int32_t *, uint8_t);
template Simple_Wire &Simple_Wire::TWriteThenRead<uint64_t>(uint8_t, uint64_t *, uint8_t);
template Simple_Wire &Simple_Wire::TWriteThenRead<uint64_t>(uint8_t, uint8_t, uint64_t *, uint8_t);
template Simple_Wire &Simple_Wire::TWriteThenRead<int64_t>(uint8_t, int64_t *, uint8_t);
template Simple_Wire &Simple_Wire::TWriteThenRead<int64_t>(uint8_t, uint8_t, int64_t *, uint8_t);
