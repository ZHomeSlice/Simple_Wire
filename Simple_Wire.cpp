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
#include "Simple_Wire.h"
#include <Wire.h>

/*
Simple_Wire::Simple_Wire(int sdaPin, int sclPin) { // Constructor
#ifdef __AVR__
    Wire.begin();
    Wire.setClock(400000); // 400kHz I2C clock.
    Wire.setWireTimeout(3000, true); //timeout value in uSec
    #define yield(); 
#elif defined(ESP8266) || defined(ESP32) || defined(ARDUINO_ARCH_RP2040)
    Wire.begin(sdaPin,sclPin,(uint32_t)400000); // 400kHz I2C clock. \
    #define yield();  yield();
#endif
}
*/


Simple_Wire::Simple_Wire() {// Constructor
  /* MUST NOT CALL BEGIN IN HERE, as will occur before setup() */
}


// When we use Simple_Wire class in the Simple_Mpu6050 class,
// We must call begin() at earliest point in xxx.ino files setup(),
// before we call any other Simple_Mpu6050 methods


void Simple_Wire::begin(int sdaPin, int sclPin) {

#ifdef __AVR__
    Wire.begin();
    Wire.setClock(400000); // 400kHz I2C clock.
    Wire.setWireTimeout(3000, true); //timeout value in uSec
#elif defined(ESP8266) || defined(ESP32) || defined(ARDUINO_ARCH_RP2040)
    Wire.begin(sdaPin,sclPin,(uint32_t)400000); // 400kHz I2C clock.
#else  
    Wire.begin();
    Wire.setClock(400000); // 400kHz I2C clock.
    
#endif
    _sdaPin = sdaPin;
    _sclPin = sclPin;
}

Simple_Wire &  Simple_Wire::SetAddress(uint8_t address) {
	devAddr = address;
	return *this;
}
Simple_Wire & Simple_Wire::SetIntMSBPos(bool FirstRead){ // is the most Significant Bit Red First?
    if(FirstRead){
        FirstByteShift = 8; // Shift first read 8 bits over by 8 bits
        SecondByteShift = 0;
    } else {
        FirstByteShift = 0;
        SecondByteShift = 8; // Shift Second read 8 bits over by 8 bits
    }
    return *this;
}
Simple_Wire & Simple_Wire::ReadBit(uint8_t regAddr, uint8_t length, uint8_t bitNum, uint8_t *Data) {
	return ReadBit( devAddr,  regAddr,  length,  bitNum,  Data);
    return *this;
}
Simple_Wire & Simple_Wire::ReadBit(uint8_t AltAddress, uint8_t regAddr, uint8_t length, uint8_t bitNum, uint8_t *Data) {
    uint8_t b;
    ReadBytes(AltAddress, regAddr,1, &b);
    if ((I2CReadCount) != 0) {
        if(length == 1) Data[0] = (b & (1 << bitNum)) >0;
        else {
            uint8_t mask = ((1 << length) - 1) << (bitNum - length + 1);
            b &= mask;
            b >>= (bitNum - length + 1);
            Data[0] = b;
        }
    }
    Val = (int32_t) Data[0];
	return *this;
}

Simple_Wire & Simple_Wire::ReadByte(uint8_t regAddr,  uint8_t *Data) {
	ReadBytes(devAddr, regAddr,  1, Data);
	return *this;
}

Simple_Wire & Simple_Wire::ReadByte(uint8_t AltAddress,uint8_t regAddr,  uint8_t *Data) {
	ReadBytes(AltAddress, regAddr,  1, Data);
	return *this;
}

Simple_Wire & Simple_Wire::ReadBytes(uint8_t regAddr, uint8_t length, uint8_t *Data) {
	ReadBytes(devAddr, regAddr,  length, Data);
	return *this;
}

Simple_Wire & Simple_Wire::ReadBytes(uint8_t AltAddress,uint8_t regAddr, uint8_t length, uint8_t *Data) {
    I2CReadCount = 0;
     for (int k = 0; k < length; k += min((int)length, WIRE_BUFFER_LENGTH)) {
        Wire.beginTransmission(AltAddress);
        Wire.write(regAddr);
        ErrorMessage = Wire.endTransmission();
        Wire.requestFrom((uint8_t)AltAddress, (uint8_t)min((int)length - k, WIRE_BUFFER_LENGTH));
        for (; Wire.available(); I2CReadCount++) {
            Data[I2CReadCount] = Wire.read();
        }
    }
    Val = (int32_t) Data[0];
   	return *this;
}



// ReadInt or Word
Simple_Wire & Simple_Wire::ReadInt(uint8_t regAddr, int16_t *Data) {
	ReadInts(devAddr, regAddr, 1, Data); // reads 1 or more 16 bit integers (Word)
	return *this;
}
Simple_Wire & Simple_Wire::ReadInt(uint8_t AltAddress,uint8_t regAddr, int16_t *Data) {
	ReadInts(AltAddress, regAddr, 1, Data); // reads 1 or more 16 bit integers (Word)
	return *this;
}

// ReadInts or Words
Simple_Wire & Simple_Wire::ReadInts(uint8_t regAddr, uint8_t size, int16_t *Data) {
	ReadInts(devAddr, regAddr, size, Data); // reads 1 or more 16 bit integers (Word)
	return *this;
}

Simple_Wire & Simple_Wire::ReadInts(uint8_t AltAddress,uint8_t regAddr, uint8_t size, int16_t *Data) {
    I2CReadCount = 0;
        for (uint8_t k = 0; k < size * 2; k += min(size * 2, WIRE_BUFFER_LENGTH)) {
            Wire.beginTransmission(AltAddress);
            Wire.write(regAddr);
            ErrorMessage = Wire.endTransmission();
            Wire.requestFrom(AltAddress, (uint8_t)(size * 2)); // length=words, this wants bytes
            for (; Wire.available() && I2CReadCount < size; ) {
                Data[I2CReadCount] = Wire.read() << FirstByteShift;
                Data[I2CReadCount] |= Wire.read() << SecondByteShift ;
                I2CReadCount++;
            }
        }
    Val = (int32_t) Data[0];
    return *this;
}

Simple_Wire & Simple_Wire::ReadUInt(uint8_t regAddr, uint16_t *Data) {
	ReadUInts(devAddr, regAddr, 1, Data); // reads 1 or more 16 bit integers (Word)
	return *this;
}
Simple_Wire & Simple_Wire::ReadUInt(uint8_t AltAddress, uint8_t regAddr, uint16_t *Data) {
	ReadUInts(AltAddress, regAddr, 1, Data); // reads 1 or more 16 bit integers (Word)
	return *this;
}

// ReadInts or Words
Simple_Wire & Simple_Wire::ReadUInts(uint8_t regAddr, uint8_t size, uint16_t *Data) {
	ReadUInts(devAddr, regAddr, size, Data); // reads 1 or more 16 bit integers (Word)
	return *this;
}

Simple_Wire & Simple_Wire::ReadUInts(uint8_t AltAddress, uint8_t regAddr, uint8_t size, uint16_t *Data) {
    I2CReadCount = 0;
    yield();
        for (uint8_t k = 0; k < size * 2; k += min(size * 2, WIRE_BUFFER_LENGTH)) {
            Wire.beginTransmission(AltAddress);
            Wire.write(regAddr);
            ErrorMessage = Wire.endTransmission();
            Wire.requestFrom(AltAddress, (uint8_t)(size * 2)); // length=words, this wants bytes
            for (; Wire.available() && I2CReadCount < size; ) {
                Data[I2CReadCount] = Wire.read() << FirstByteShift ;
                Data[I2CReadCount] |= Wire.read() << SecondByteShift ;
                I2CReadCount++;
            }
        }
    yield();
    Val = (int32_t) Data[0];
    return *this;
}




// Write Bits
Simple_Wire & Simple_Wire::WriteBitX(uint8_t regAddr, uint8_t length, uint8_t bitNum, uint8_t Val) {
	return WriteBitSkip(devAddr, regAddr,  length,  bitNum, true,  Val);
}
// Write Bits Skip Read
Simple_Wire & Simple_Wire::WriteBit(uint8_t regAddr, uint8_t length, uint8_t bitNum, uint8_t Val) {
	return WriteBitSkip(devAddr, regAddr,  length,  bitNum, false,  Val);
}

Simple_Wire & Simple_Wire::WriteBitX(uint8_t AltAddress,uint8_t regAddr, uint8_t length, uint8_t bitNum,  uint8_t Val) {
    return WriteBitSkip(AltAddress, regAddr,  length,  bitNum, true,  Val);
}
Simple_Wire & Simple_Wire::WriteBit(uint8_t AltAddress,uint8_t regAddr, uint8_t length, uint8_t bitNum,  uint8_t Val) {
    return WriteBitSkip(AltAddress, regAddr,  length,  bitNum, false,  Val);
}
Simple_Wire & Simple_Wire::WriteBitSkip(uint8_t AltAddress,uint8_t regAddr, uint8_t length, uint8_t bitNum, bool SkipRead, uint8_t Val) {
    uint8_t b = 0x00;
    if (!SkipRead) ReadBytes(AltAddress, regAddr,1, &b);
    if(I2CReadCount){
        if (length == 1)  b = (Val != 0) ? (b | (1 << bitNum)) : (b & ~(1 << bitNum));
        else {
                uint8_t mask = ((1 << length) - 1) << (bitNum - length + 1);
                Val <<= (bitNum - length + 1); // shift Data into correct position
                Val &= mask; // zero all non-important bits in Data
                b &= ~(mask); // zero all important bits in existing byte
                b |= Val; // combine Data with existing byte
        }
        WriteBytes(AltAddress, regAddr,1, &b);
    }
	return *this;
}

// WriteByte
Simple_Wire & Simple_Wire::WriteByte(uint8_t regAddr,  uint8_t Val) {
	WriteBytes(devAddr, regAddr,  1, &Val); //Writes 1 or more 8 bit Bytes
	return *this;
}
Simple_Wire & Simple_Wire::WriteByte(uint8_t AltAddress,uint8_t regAddr,  uint8_t Val) {
	WriteBytes(AltAddress, regAddr,  1, &Val); //Writes 1 or more 8 bit Bytes
	return *this;
}

// WriteBytes
Simple_Wire & Simple_Wire::WriteBytes(uint8_t regAddr, uint8_t length, uint8_t *Data) {
	WriteBytes(devAddr, regAddr,  length, Data); //Writes 1 or more 8 bit Bytes
	return *this;
}

Simple_Wire & Simple_Wire::WriteBytes(uint8_t AltAddress,uint8_t regAddr, uint8_t length, uint8_t *Data) {
    I2CWriteCount = 0;
    Wire.beginTransmission(AltAddress);
    Wire.write((uint8_t) regAddr); // send address
    for (; I2CWriteCount < length; I2CWriteCount++) {
            Wire.write((uint8_t) Data[I2CWriteCount]);
    }
    ErrorMessage = Wire.endTransmission();
    return *this;
}

// WriteInt or word
Simple_Wire & Simple_Wire::WriteInt(uint8_t regAddr,  int16_t Val) {
	WriteInts(devAddr, regAddr, 1,  &Val);// Writes 1 or more 16 bit integers (Word)
	return *this;
}
Simple_Wire & Simple_Wire::WriteInt(uint8_t AltAddress, uint8_t regAddr,  int16_t Val) {
	WriteInts(AltAddress, regAddr, 1,  &Val);// Writes 1 or more 16 bit integers (Word)
	return *this;
}

// WriteInts or words
Simple_Wire & Simple_Wire::WriteInts(uint8_t regAddr, uint8_t size, int16_t *Data) {
	WriteInts(devAddr, regAddr, size ,  Data);
	return *this;
}
Simple_Wire & Simple_Wire::WriteInts(uint8_t AltAddress,uint8_t regAddr, uint8_t size, int16_t *Data) {
    I2CWriteCount = 0;
    yield();    
    Wire.beginTransmission(AltAddress);
    Wire.write(regAddr); // send address
    for (; I2CWriteCount < size; I2CWriteCount++) { 
        Wire.write((int8_t)(Data[I2CWriteCount] >> FirstByteShift));    // send MSB
        Wire.write((int8_t)(Data[I2CWriteCount] >> SecondByteShift)) ;  // send LSB
    }
    ErrorMessage = Wire.endTransmission();
    yield();    
	return *this;
}

Simple_Wire & Simple_Wire::WriteUInt(uint8_t regAddr,  uint16_t Val) {
	WriteUInts(devAddr, regAddr, 1,  &Val);// Writes 1 or more 16 bit integers (Word)
	return *this;
}
Simple_Wire & Simple_Wire::WriteUInt(uint8_t AltAddress, uint8_t regAddr,  uint16_t Val) {
	WriteUInts(AltAddress, regAddr, 1,  &Val);// Writes 1 or more 16 bit integers (Word)
	return *this;
}

// WriteInts or words
Simple_Wire & Simple_Wire::WriteUInts(uint8_t regAddr, uint8_t size, uint16_t *Data) {
	WriteUInts(devAddr, regAddr, size ,  Data);
	return *this;
}
Simple_Wire & Simple_Wire::WriteUInts(uint8_t AltAddress, uint8_t regAddr, uint8_t size, uint16_t *Data) {
    I2CWriteCount = 0;
    yield();
    Wire.beginTransmission(AltAddress);
    Wire.write(regAddr); // send address
    for (; I2CWriteCount < size; I2CWriteCount++) { 
        Wire.write((uint8_t)(Data[I2CWriteCount] >> FirstByteShift));    // send MSB
        Wire.write((uint8_t)(Data[I2CWriteCount] >> SecondByteShift)) ;  // send LSB
    }
    ErrorMessage = Wire.endTransmission();
    yield();
	return *this;
}

#define printHex(Num) print(Num>>4,HEX);Serial.print(Num&0X0F,HEX);

Simple_Wire & Simple_Wire::I2C_Scanner(){
	Serial.println(F("Scanning for Addresses on the i2c Buss:"));
    yield();
	for (int x = 0;x < 128;x++){
        if((x%8) != 0)Serial.print(",");
        if(Check_Address(x)) {
            Serial.print("<0X");
            Serial.printHex(x);
            Serial.print(">");
        }else{
            Serial.print(" 0X");
            Serial.printHex(x);
            Serial.print(" ");
        }
		if((x%8) == 7)Serial.println();
	}
	return *this;
}
uint8_t Simple_Wire::Find_Address(uint8_t Address,uint8_t Limit){
	do {
		if (Check_Address(Address))
		return Address;
	} while (Limit != Address++);// using rollover ate 255 to allow for any number on Limit
	return 0;
}

uint8_t Simple_Wire::Check_Address(uint8_t Address){
		Wire.beginTransmission(Address);
		return(ErrorMessage = Wire.endTransmission() == 0);
}