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
===============================================
*/

#ifndef Simple_Wire_h
#define Simple_Wire_h
#include "Arduino.h"
#include <Wire.h>

#define printHex(Num)     \
    print(Num >> 4, HEX); \
    Serial.print(Num & 0X0F, HEX);

#ifndef WIRE_BUFFER_LENGTH
#if defined(I2C_BUFFER_LENGTH)
// Arduino ESP32 core Wire uses this
#define WIRE_BUFFER_LENGTH I2C_BUFFER_LENGTH
#elif defined(BUFFER_LENGTH)
// Arduino AVR core Wire and many others use this
#define WIRE_BUFFER_LENGTH BUFFER_LENGTH
#elif defined(ESP32) || defined(ESP8266)
#define WIRE_BUFFER_LENGTH 128
#else
#define WIRE_BUFFER_LENGTH 32
#endif
#endif

class Simple_Wire
{
private:
    bool _Begin = false;
    bool ReverseByteShift = false;
    uint8_t I2CReadCount = 0;
    uint8_t I2CWriteCount = 0;
    uint8_t _sdaPin = 0;
    uint8_t _sclPin = 0;
    template <typename T>
    Simple_Wire &ReadBitTemplate(uint8_t AltAddress, uint8_t regAddr, uint8_t length, uint8_t bitNum, T *Data);
    template <typename T>
    Simple_Wire &ReadBitMaskTemplate(uint8_t AltAddress, uint8_t regAddr, T mask, T *Data);
    template <typename T>
    Simple_Wire &WriteBitTemplate(uint8_t AltAddress, uint8_t regAddr, uint8_t length, uint8_t bitNum, bool SkipRead, T Val);
    template <typename T>
    Simple_Wire &WriteBitMaskTemplate(uint8_t AltAddress, uint8_t regAddr, bool SkipRead,T Mask, T Val);
    template <typename T>
    Simple_Wire &TRead(uint8_t AltAddress, uint8_t regAddr, uint8_t length, uint8_t ByteC, T *Data);
    template <typename T>
    Simple_Wire &TWrite(uint8_t AltAddress, uint8_t regAddr, uint8_t length, uint8_t ByteC, T *Data);
    static const __FlashStringHelper * const i2cErrorMessages[5];
    bool Verbose = false;
    uint64_t Val = 0;
    uint8_t ErrorMessage = 0;
      
public:
    /*
    0 Success
    1 Data to long to fit into transmit buffer
    2 Received NACK on transmission of address
    3 Received NACK on transmission of data
    4 Other Error
    5 Timeout
    */

    uint8_t devAddr = 0;

    Simple_Wire();
    void begin(int sdaPin = 0, int sclPin = 1);
    Simple_Wire &SetAddress(uint8_t address){devAddr = address; return *this;};// Sets the default address
    Simple_Wire &SetIntMSBPos(bool FirstRead){ReverseByteShift = FirstRead; return *this;}// is the most Significant Bit Read first?

    // Read functions
    // Read a Bytes worth of Bits
    Simple_Wire &ReadBit(uint8_t regAddr, uint8_t length, uint8_t bitNum, uint8_t *Data) {return ReadBitTemplate<uint8_t>(devAddr, regAddr, length, bitNum, Data); };
    Simple_Wire &ReadBit(uint8_t AltAddress, uint8_t regAddr, uint8_t length, uint8_t bitNum, uint8_t *Data){return ReadBitTemplate<uint8_t>(AltAddress, regAddr, length, bitNum, Data);};
    Simple_Wire &ReadBitM(uint8_t regAddr, uint8_t mask, uint8_t *Data) {return ReadBitMaskTemplate<uint8_t>(devAddr, regAddr, mask, Data); };
    Simple_Wire &ReadBitM(uint8_t AltAddress, uint8_t regAddr, uint8_t mask, uint8_t *Data){return ReadBitMaskTemplate<uint8_t>(AltAddress, regAddr, mask, Data);};
    // Read an Int worth of Bits
    Simple_Wire &ReadIntBit(uint8_t regAddr, uint8_t length, uint8_t bitNum, uint16_t *Data) {return ReadBitTemplate<uint16_t>(devAddr, regAddr, length, bitNum, Data); };
    Simple_Wire &ReadIntBit(uint8_t AltAddress, uint8_t regAddr, uint8_t length, uint8_t bitNum, uint16_t *Data){return ReadBitTemplate<uint16_t>(AltAddress, regAddr, length, bitNum, Data);};
    Simple_Wire &ReadIntBitM(uint8_t regAddr,  uint16_t mask, uint16_t *Data) {return ReadBitMaskTemplate<uint16_t>(devAddr, regAddr, mask, Data); };
    Simple_Wire &ReadIntBitM(uint8_t AltAddress, uint8_t regAddr, uint16_t mask, uint16_t *Data){return ReadBitMaskTemplate<uint16_t>(AltAddress, regAddr, mask, Data);};
    
    // Read Bytes
    // Read a Single signed byte
    Simple_Wire &ReadSByte(uint8_t regAddr, int8_t *Data) {return TRead<int8_t>(devAddr, regAddr, 1, 1, Data);};
    Simple_Wire &ReadSByte(uint8_t AltAddress, uint8_t regAddr, int8_t *Data) {return TRead<int8_t>(AltAddress, regAddr, 1, 1, Data);};
    // Read multiple Signed bytes (array of values)
    Simple_Wire &ReadSBytes(uint8_t regAddr, uint8_t length, int8_t *Data) {return TRead<int8_t>(devAddr, regAddr, length, 1, Data);};
    Simple_Wire &ReadSBytes(uint8_t AltAddress, uint8_t regAddr, uint8_t length, int8_t *Data) {return TRead<int8_t>(AltAddress, regAddr, length, 1, Data);};

    // Read a Unsingle signed byte
    Simple_Wire &ReadByte(uint8_t regAddr, uint8_t *Data) { return TRead<uint8_t>(devAddr, regAddr, 1, 1, Data); };
    Simple_Wire &ReadByte(uint8_t AltAddress, uint8_t regAddr, uint8_t *Data) { return TRead<uint8_t>(AltAddress, regAddr, 1, 1, Data); };
    // Read multiple Unsigned bytes (array of values)
    Simple_Wire &ReadBytes(uint8_t regAddr, uint8_t length, uint8_t *Data) { return TRead<uint8_t>(devAddr, regAddr, length, 1, Data); };
    Simple_Wire &ReadBytes(uint8_t AltAddress, uint8_t regAddr, uint8_t length, uint8_t *Data) { return TRead<uint8_t>(AltAddress, regAddr, length, 1, Data); };

    // Read Ints
    // Read Signed Ints
    Simple_Wire &ReadInt(uint8_t regAddr, int16_t *Data) { return TRead<int16_t>(devAddr, regAddr, 1, 2, Data); };
    Simple_Wire &ReadInt(uint8_t AltAddress, uint8_t regAddr, int16_t *Data) { return TRead<int16_t>(AltAddress, regAddr, 1, 2, Data); };
    // Read multiple Unsigned Ints (array of values)
    Simple_Wire &ReadInts(uint8_t regAddr, uint8_t length, int16_t *Data) { return TRead<int16_t>(devAddr, regAddr, length, 2, Data); }
    Simple_Wire &ReadInts(uint8_t AltAddress, uint8_t regAddr, uint8_t length, int16_t *Data) { return TRead<int16_t>(AltAddress, regAddr, length, 2, Data); };

    // Read Unsigned Int
    Simple_Wire &ReadUInt(uint8_t regAddr, uint16_t *Data) { return TRead<uint16_t>(devAddr, regAddr, 1, 2, Data); };
    Simple_Wire &ReadUInt(uint8_t AltAddress, uint8_t regAddr, uint16_t *Data) { return TRead<uint16_t>(AltAddress, regAddr, 1, 2, Data); };
    // Read multiple Unsigned Ints (array of values)
    Simple_Wire &ReadUInts(uint8_t regAddr, uint8_t length, uint16_t *Data) { return TRead<uint16_t>(devAddr, regAddr, length, 2, Data); };
    Simple_Wire &ReadUInts(uint8_t AltAddress, uint8_t regAddr, uint8_t length, uint16_t *Data) { return TRead<uint16_t>(AltAddress, regAddr, length, 2, Data); };
 
    // Read 24-bit Signed Int
    Simple_Wire &Read24(uint8_t regAddr, int32_t *Data) { return TRead<int32_t>(devAddr, regAddr, 1, 3, Data); };
    Simple_Wire &Read24(uint8_t AltAddress, uint8_t regAddr, int32_t *Data) { return TRead<int32_t>(AltAddress, regAddr, 1, 3, Data); };
    // Read 24-bit Signed Int
    Simple_Wire &ReadU24(uint8_t regAddr, uint32_t *Data) { return TRead<uint32_t>(devAddr, regAddr, 1, 3, Data); };
    Simple_Wire &ReadU24(uint8_t AltAddress, uint8_t regAddr, uint32_t *Data) { return TRead<uint32_t>(AltAddress, regAddr, 1, 3, Data); };

    // Read 32-bit Signed Int
    Simple_Wire &Read32(uint8_t regAddr, int32_t *Data) { return TRead<int32_t>(devAddr, regAddr, 1, 4, Data); };
    Simple_Wire &Read32(uint8_t AltAddress, uint8_t regAddr, int32_t *Data) { return TRead<int32_t>(AltAddress, regAddr, 1, 4, Data); };
    // Read multiple 32-bit Signed Ints (array of values)
    Simple_Wire &Read32s(uint8_t regAddr, uint8_t length, int32_t *Data) { return TRead<int32_t>(devAddr, regAddr, length, 4, Data);};
    Simple_Wire &Read32s(uint8_t AltAddress, uint8_t regAddr, uint8_t length, int32_t *Data) { return TRead<int32_t>(AltAddress, regAddr, length, 4, Data);};

    // Read 32-bit Unsigned Int
    Simple_Wire &ReadU32(uint8_t regAddr, uint32_t *Data) { return TRead<uint32_t>(devAddr, regAddr, 1, 4, Data); };
    Simple_Wire &ReadU32(uint8_t AltAddress, uint8_t regAddr, uint32_t *Data) { return TRead<uint32_t>(AltAddress, regAddr, 1, 4, Data); };
    // Read multiple 32-bit Unsigned Ints (array of values)
    Simple_Wire &ReadU32s(uint8_t regAddr, uint8_t length, uint32_t *Data) {return TRead<uint32_t>(devAddr, regAddr, length, 4, Data);};
    Simple_Wire &ReadU32s(uint8_t AltAddress, uint8_t regAddr, uint8_t length, uint32_t *Data) {return TRead<uint32_t>(AltAddress, regAddr, length, 4, Data);};
    
    // Read 64-bit Signed Int
    Simple_Wire &Read64(uint8_t regAddr, int64_t *Data) { return TRead<int64_t>(devAddr, regAddr, 1, 8, Data); };
    Simple_Wire &Read64(uint8_t AltAddress, uint8_t regAddr, int64_t *Data) { return TRead<int64_t>(AltAddress, regAddr, 1, 8, Data); };
    // Read multiple 32-bit Unsigned Ints (array of values)
    Simple_Wire &Read64s(uint8_t regAddr, uint8_t length, int64_t *Data) {return TRead<int64_t>(devAddr, regAddr, length, 8, Data);};
    Simple_Wire &Read64s(uint8_t AltAddress, uint8_t regAddr, uint8_t length, int64_t *Data) {return TRead<int64_t>(AltAddress, regAddr, length, 8, Data);};

    // Read 64-bit Unsigned Int
    Simple_Wire &ReadU64(uint8_t regAddr, uint64_t *Data) { return TRead<uint64_t>(devAddr, regAddr, 1, 8, Data); };
    Simple_Wire &ReadU64(uint8_t AltAddress, uint8_t regAddr, uint64_t *Data) { return TRead<uint64_t>(AltAddress, regAddr, 1, 8, Data); };
    // Read multiple 64-bit Unsigned Ints (array of values)
    Simple_Wire &ReadU64s(uint8_t regAddr, uint8_t length, uint64_t *Data) {return TRead<uint64_t>(devAddr, regAddr, length, 8, Data);};
    Simple_Wire &ReadU64s(uint8_t AltAddress, uint8_t regAddr, uint8_t length, uint64_t *Data) {return TRead<uint64_t>(AltAddress, regAddr, length, 8, Data);};


    // write functions
    //
    Simple_Wire &WriteBitX(uint8_t regAddr, uint8_t length, uint8_t bitNum, uint8_t Val) { return WriteBitTemplate<uint8_t>(devAddr, regAddr, length, bitNum, true, Val); };                        // Alters only specific Bits by reading the byte first
    Simple_Wire &WriteBit(uint8_t regAddr, uint8_t length, uint8_t bitNum, uint8_t Val) { return WriteBitTemplate<uint8_t>(devAddr, regAddr, length, bitNum, false, Val); };                        // Sets all other bits to 0
    Simple_Wire &WriteBit(uint8_t regAddr, uint8_t length, uint8_t bitNum, bool SkipRead, uint8_t Val){return WriteBitTemplate<uint8_t>(devAddr, regAddr, length, bitNum, SkipRead, Val);};
    Simple_Wire &WriteBitX(uint8_t AltAddress, uint8_t regAddr, uint8_t length, uint8_t bitNum, uint8_t Val) { return WriteBitTemplate<uint8_t>(AltAddress, regAddr, length, bitNum, true, Val); }; // Alters only specific Bits by reading the byte first
    Simple_Wire &WriteBit(uint8_t AltAddress, uint8_t regAddr, uint8_t length, uint8_t bitNum, uint8_t Val) { return WriteBitTemplate<uint8_t>(AltAddress, regAddr, length, bitNum, false, Val); }; // Sets all other bits to 0
    Simple_Wire &WriteBit(uint8_t AltAddress, uint8_t regAddr, uint8_t length, uint8_t bitNum, bool SkipRead, uint8_t Val){return WriteBitTemplate<uint8_t>(AltAddress, regAddr, length, bitNum, SkipRead, Val);};
    
    Simple_Wire &WriteBitMX(uint8_t regAddr, uint8_t mask, uint8_t Val) { return WriteBitMaskTemplate<uint8_t>(devAddr, regAddr, mask, true, Val); };                        // Alters only specific Bits by reading the byte first
    Simple_Wire &WriteBitM(uint8_t regAddr, uint8_t mask, uint8_t Val) { return WriteBitMaskTemplate<uint8_t>(devAddr, regAddr, mask, false, Val); };                        // Sets all other bits to 0
    Simple_Wire &WriteBitM(uint8_t regAddr, bool SkipRead,uint8_t mask, uint8_t Val){return WriteBitMaskTemplate<uint8_t>(devAddr, regAddr, mask, SkipRead, Val);};
    Simple_Wire &WriteBitMX(uint8_t AltAddress, uint8_t regAddr, uint8_t mask, uint8_t Val) { return WriteBitMaskTemplate<uint8_t>(AltAddress, regAddr, mask, true, Val); }; // Alters only specific Bits by reading the byte first
    Simple_Wire &WriteBitM(uint8_t AltAddress, uint8_t regAddr, uint8_t mask, uint8_t Val) { return WriteBitMaskTemplate<uint8_t>(AltAddress, regAddr, mask, false, Val); }; // Sets all other bits to 0
    Simple_Wire &WriteBitM(uint8_t AltAddress, uint8_t regAddr, bool SkipRead, uint8_t mask, uint8_t Val){return WriteBitMaskTemplate<uint8_t>(AltAddress, regAddr, mask, SkipRead, Val);};

    Simple_Wire &WriteIntBitX(uint8_t regAddr, uint8_t length, uint8_t bitNum, uint16_t Val) { return WriteBitTemplate<uint16_t>(devAddr, regAddr, length, bitNum, true, Val); };                        // Alters only specific Bits by reading the int first
    Simple_Wire &WriteIntBit(uint8_t regAddr, uint8_t length, uint8_t bitNum, uint16_t Val) { return WriteBitTemplate<uint16_t>(devAddr, regAddr, length, bitNum, false, Val); };                        // Sets all other bits to 0
    Simple_Wire &WriteIntBitSkip(uint8_t regAddr, uint8_t length, uint8_t bitNum, bool SkipRead, uint16_t Val){return WriteBitTemplate<uint16_t>(devAddr, regAddr, length, bitNum, SkipRead, Val);};
    Simple_Wire &WriteIntBitX(uint8_t AltAddress, uint8_t regAddr, uint8_t length, uint8_t bitNum, uint16_t Val) { return WriteBitTemplate<uint16_t>(AltAddress, regAddr, length, bitNum, true, Val); }; // Alters only specific Bits by reading the int first
    Simple_Wire &WriteIntBit(uint8_t AltAddress, uint8_t regAddr, uint8_t length, uint8_t bitNum, uint16_t Val) { return WriteBitTemplate<uint16_t>(AltAddress, regAddr, length, bitNum, false, Val); }; // Sets all other bits to 0
    Simple_Wire &WriteIntBitSkip(uint8_t AltAddress, uint8_t regAddr, uint8_t length, uint8_t bitNum, bool SkipRead, uint16_t Val){return WriteBitTemplate<uint16_t>(AltAddress, regAddr, length, bitNum, SkipRead, Val);};

    Simple_Wire &WriteIntBitMX(uint8_t regAddr, uint16_t mask, uint16_t Val) { return WriteBitMaskTemplate<uint16_t>(devAddr, regAddr, mask, true, Val); };                        // Alters only specific Bits by reading the int first
    Simple_Wire &WriteIntBitM(uint8_t regAddr, uint16_t mask, uint16_t Val) { return WriteBitMaskTemplate<uint16_t>(devAddr, regAddr, mask, false, Val); };                        // Sets all other bits to 0
    Simple_Wire &WriteIntBitMSkip(uint8_t regAddr,  bool SkipRead, uint16_t mask, uint16_t Val){return WriteBitMaskTemplate<uint16_t>(devAddr, regAddr, mask, SkipRead, Val);};
    Simple_Wire &WriteIntBitMX(uint8_t AltAddress, uint8_t regAddr, uint16_t mask,  uint16_t Val) { return WriteBitMaskTemplate<uint16_t>(AltAddress, regAddr, mask, true, Val); }; // Alters only specific Bits by reading the int first
    Simple_Wire &WriteIntBitM(uint8_t AltAddress, uint8_t regAddr, uint16_t mask, uint16_t Val) { return WriteBitMaskTemplate<uint16_t>(AltAddress, regAddr, mask, false, Val); }; // Sets all other bits to 0
    Simple_Wire &WriteIntBitMSkip(uint8_t AltAddress, uint8_t regAddr, bool SkipRead, uint16_t mask, uint16_t Val){return WriteBitMaskTemplate<uint16_t>(AltAddress, regAddr, mask, SkipRead, Val);};


    // Write Signed Bytes
    Simple_Wire &WriteSByte(uint8_t regAddr, int8_t Val) { return TWrite<int8_t>(devAddr, regAddr, 1, 1, &Val); };
    Simple_Wire &WriteSByte(uint8_t AltAddress, uint8_t regAddr, int8_t Val) { return TWrite<int8_t>(AltAddress, regAddr, 1, 1, &Val); };
    // Write Multiple Signed Bytes (from an array of values)
    Simple_Wire &WriteSBytes(uint8_t regAddr, uint8_t length, int8_t *Data) { return TWrite<int8_t>(devAddr, regAddr, length, 1, Data); };
    Simple_Wire &WriteSBytes(uint8_t AltAddress, uint8_t regAddr, uint8_t length, int8_t *Data) { return TWrite<int8_t>(AltAddress, regAddr, length, 1, Data); };

    // Write Unsigned Byte
    Simple_Wire &WriteByte(uint8_t regAddr, uint8_t Val) { return TWrite<uint8_t>(devAddr, regAddr, 1, 1, &Val); };
    Simple_Wire &WriteByte(uint8_t AltAddress, uint8_t regAddr, uint8_t Val) { return TWrite<uint8_t>(AltAddress, regAddr, 1, 1, &Val); };
    // Write Multiple Unsigned Bytes (from an array of values)
    Simple_Wire &WriteBytes(uint8_t regAddr, uint8_t length, uint8_t *Data) { return TWrite<uint8_t>(devAddr, regAddr, length, 1, Data); };
    Simple_Wire &WriteBytes(uint8_t AltAddress, uint8_t regAddr, uint8_t length, uint8_t *Data) { return TWrite<uint8_t>(devAddr, regAddr, length, 1, Data); };

    // Write Signed Int or word
    Simple_Wire &WriteInt(uint8_t regAddr, int16_t Val) { return TWrite<int16_t>(devAddr, regAddr, 1, 2, &Val); };
    Simple_Wire &WriteInt(uint8_t AltAddress, uint8_t regAddr, int16_t Val) { return TWrite<int16_t>(AltAddress, regAddr, 1, 2, &Val); };
    // Write Multiple Signed Ints or words (from an array of values)
    Simple_Wire &WriteInts(uint8_t regAddr, uint8_t length, int16_t *Data) { return TWrite<int16_t>(devAddr, regAddr, length, 2, Data); };
    Simple_Wire &WriteInts(uint8_t AltAddress, uint8_t regAddr, uint8_t length, int16_t *Data) { return TWrite<int16_t>(AltAddress, regAddr, length, 2, Data); };

    // Write Unsigned Int
    Simple_Wire &WriteUInt(uint8_t regAddr, uint16_t Val) { return TWrite<uint16_t>(devAddr, regAddr, 1, 2, &Val); };
    Simple_Wire &WriteUInt(uint8_t AltAddress, uint8_t regAddr, uint16_t Val) { return TWrite<uint16_t>(AltAddress, regAddr, 1, 2, &Val); };
     // Write Multiple Unsigned Ints (from an array of values)
    Simple_Wire &WriteUInts(uint8_t regAddr, uint8_t length, uint16_t *Data) { return TWrite<uint16_t>(devAddr, regAddr, length, 2, Data); };
    Simple_Wire &WriteUInts(uint8_t AltAddress, uint8_t regAddr, uint8_t length, uint16_t *Data) { return TWrite<uint16_t>(AltAddress, regAddr, length, 2, Data); };

    // Write 24 Bit
    Simple_Wire &Write24(uint8_t regAddr, int32_t Val) { return TWrite<int32_t>(devAddr, regAddr, 1, 3, &Val); };
    Simple_Wire &Write24(uint8_t AltAddress, uint8_t regAddr, int32_t Val) { return TWrite<int32_t>(AltAddress, regAddr, 1, 3, &Val); };

     // Write unsigned 24 Bit
    Simple_Wire &WriteU24(uint8_t regAddr, uint32_t Val) { return TWrite<uint32_t>(devAddr, regAddr, 1, 3, &Val); };
    Simple_Wire &WriteU24(uint8_t AltAddress, uint8_t regAddr, uint32_t Val) { return TWrite<uint32_t>(AltAddress, regAddr, 1, 3, &Val); };

    // Write 32 Bit (single signed 32-bit value)
    Simple_Wire &Write32(uint8_t regAddr, int32_t Val) { return TWrite<int32_t>(devAddr, regAddr, 1, 4, &Val); };
    Simple_Wire &Write32(uint8_t AltAddress, uint8_t regAddr, int32_t Val) { return TWrite<int32_t>(AltAddress, regAddr, 1, 4, &Val); };
    // Write multiple 32 Bit Signed values (from an array of values)
    Simple_Wire &Write32s(uint8_t regAddr, uint8_t length, int32_t *Data) { return TWrite<int32_t>(devAddr, regAddr, length, 4, Data); };
    Simple_Wire &Write32s(uint8_t AltAddress, uint8_t regAddr, uint8_t length, int32_t *Data) { return TWrite<int32_t>(AltAddress, regAddr, length, 4, Data); };

    // Write Unsigned 32 Bit (single unsigned 32-bit value)
    Simple_Wire &WriteU32(uint8_t regAddr, uint32_t Val) { return TWrite<uint32_t>(devAddr, regAddr, 1, 4, &Val); };
    Simple_Wire &WriteU32(uint8_t AltAddress, uint8_t regAddr, uint32_t Val) { return TWrite<uint32_t>(AltAddress, regAddr, 1, 4, &Val); };
    // Write multiple Unsigned 32 Bit values (from an array of values)
    Simple_Wire &WriteU32s(uint8_t regAddr, uint8_t length, uint32_t *Data) { return TWrite<uint32_t>(devAddr, regAddr, length, 4, Data); };
    Simple_Wire &WriteU32s(uint8_t AltAddress, uint8_t regAddr, uint8_t length, uint32_t *Data) { return TWrite<uint32_t>(AltAddress, regAddr, length, 4, Data); };

    // Write 64 Bit (single signed 64-bit value)
    Simple_Wire &Write64(uint8_t regAddr, int64_t Val) { return TWrite<int64_t>(devAddr, regAddr, 1, 8, &Val); };
    Simple_Wire &Write64(uint8_t AltAddress, uint8_t regAddr, int64_t Val) { return TWrite<int64_t>(AltAddress, regAddr, 1, 8, &Val); };
    // Write multiple 64 Bit Signed values (from an array of values)
    Simple_Wire &Write64s(uint8_t regAddr, uint8_t length, int64_t *Data) { return TWrite<int64_t>(devAddr, regAddr, length, 8, Data); };
    Simple_Wire &Write64s(uint8_t AltAddress, uint8_t regAddr, uint8_t length, int64_t *Data) { return TWrite<int64_t>(AltAddress, regAddr, length, 8, Data); };

    // Write Unsigned 64 Bit (single unsigned 64-bit value)
    Simple_Wire &WriteU64(uint8_t regAddr, uint64_t Val) { return TWrite<uint64_t>(devAddr, regAddr, 1, 8, &Val); };
    Simple_Wire &WriteU64(uint8_t AltAddress, uint8_t regAddr, uint64_t Val) { return TWrite<uint64_t>(AltAddress, regAddr, 1, 8, &Val); };
    // Write multiple Unsigned 64 Bit values (from an array of values)
    Simple_Wire &WriteU64s(uint8_t regAddr, uint8_t length, uint64_t *Data) { return TWrite<uint64_t>(devAddr, regAddr, length, 8, Data); };
    Simple_Wire &WriteU64s(uint8_t AltAddress, uint8_t regAddr, uint8_t length, uint64_t *Data) { return TWrite<uint64_t>(AltAddress, regAddr, length, 8, Data); };

    // check functions
    uint8_t CheckAddress() { return (devAddr); }; // deprecated
    uint8_t GetAddress() { return (devAddr); };
    uint8_t ReadCount() { return (I2CReadCount); };
    uint8_t WriteCount() { return (I2CWriteCount); };
    bool ReadSuccess() { return (I2CReadCount > 0); };
    bool WriteSucess() { return (I2CWriteCount > 0); };

    // Helper functions
    Simple_Wire &I2C_Scanner();
    
    uint8_t Find_Address(uint8_t Limit = 128){return Find_Address(devAddr, Limit);};
    uint8_t Find_Address(uint8_t Address, uint8_t Limit);
    uint8_t Check_Address(){return Check_Address(devAddr);};
    uint8_t Check_Address(uint8_t Address);
    uint64_t Value() { return (Val); };
    uint8_t GetErrorMessage() { return ErrorMessage; };
    bool Success(bool TF = true) { return (TF)?(ErrorMessage == 0):(ErrorMessage != 0); }
    Simple_Wire &Delay(uint32_t ms){delay(ms);return *this;};
    Simple_Wire &This_Wire(){return *this;};
    Simple_Wire & SetVerbose(bool V = true){Verbose = V; return *this;};
};

extern TwoWire Wire;
extern TwoWire Wire1;

#endif