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

#ifndef WIRE_BUFFER_LENGTH
    #if defined(I2C_BUFFER_LENGTH)
        // Arduino ESP32 core Wire uses this
        #define WIRE_BUFFER_LENGTH I2C_BUFFER_LENGTH
    #elif defined(BUFFER_LENGTH)
        // Arduino AVR core Wire and many others use this
        #define WIRE_BUFFER_LENGTH BUFFER_LENGTH
    #elif defined(SERIAL_BUFFER_SIZE)
        // Arduino SAMD core Wire uses this
        #define WIRE_BUFFER_LENGTH SERIAL_BUFFER_SIZE
    #else
        // should be a safe fallback, though possibly inefficient
        #define WIRE_BUFFER_LENGTH 32
    #endif
#endif



class Simple_Wire{
    public:
        uint8_t devAddr;
        int32_t Val;
        uint8_t ErrorMessage = 0;
        /*
        0 Success
        1 Data to long to fit into transmit buffer
        2 Received NACK on transmission of address
        3 Received NACK on transmission of data
        4 Other Error
        5 Timeout
        */
        Simple_Wire();
       // Simple_Wire(int sdaPin , int sclPin );
        void begin(int sdaPin = 0, int sclPin = 1);
        Simple_Wire & SetAddress(uint8_t address);
        Simple_Wire & SetIntMSBPos(bool FirstRead); // is the most Significant Bit Red First?
        // read functions
        Simple_Wire & ReadBit(uint8_t regAddr,  uint8_t length, uint8_t bitNum, uint8_t *data);
        Simple_Wire & ReadBit(uint8_t AltAddress,uint8_t regAddr,  uint8_t length, uint8_t bitNum, uint8_t *data);
        Simple_Wire & ReadByte(uint8_t regAddr,  uint8_t *Data);
        Simple_Wire & ReadByte(uint8_t AltAddress,uint8_t regAddr, uint8_t *Data);
        Simple_Wire & ReadBytes(uint8_t regAddr, uint8_t length, uint8_t *Data);
        Simple_Wire & ReadBytes(uint8_t AltAddress,uint8_t regAddr, uint8_t length, uint8_t *Data);
        //Signed Ints
        Simple_Wire & ReadInt(uint8_t regAddr, int16_t *data);
        Simple_Wire & ReadInt(uint8_t AltAddress,uint8_t regAddr, int16_t *data);
        Simple_Wire & ReadInts(uint8_t regAddr, uint8_t size, int16_t *Data);
        Simple_Wire & ReadInts(uint8_t AltAddress,uint8_t regAddr, uint8_t size, int16_t *Data);
        //Un-Signed Ints
      
        Simple_Wire & ReadUInt(uint8_t regAddr, uint16_t *data);
        Simple_Wire & ReadUInt(uint8_t AltAddress,uint8_t regAddr, uint16_t *data);
        Simple_Wire & ReadUInts(uint8_t regAddr, uint8_t size, uint16_t *Data);
        Simple_Wire & ReadUInts(uint8_t AltAddress,uint8_t regAddr, uint8_t size, uint16_t *Data);
        
        // write functions
        Simple_Wire & WriteBit(uint8_t regAddr, uint8_t length, uint8_t bitNum, uint8_t Val);
        Simple_Wire & WriteBitX(uint8_t regAddr, uint8_t length, uint8_t bitNum, uint8_t Val);
        Simple_Wire & WriteBit(uint8_t AltAddress,uint8_t regAddr, uint8_t length, uint8_t bitNum, uint8_t Val);
        Simple_Wire & WriteBitX(uint8_t AltAddress,uint8_t regAddr, uint8_t length, uint8_t bitNum, uint8_t Val);  
        Simple_Wire & WriteBitSkip(uint8_t AltAddress,uint8_t regAddr, uint8_t length, uint8_t bitNum, bool SkipRead, uint8_t Val);       
        Simple_Wire & WriteByte(uint8_t regAddr,  uint8_t Val);
        Simple_Wire & WriteByte(uint8_t AltAddress,uint8_t regAddr,  uint8_t Val);
        Simple_Wire & WriteBytes(uint8_t regAddr, uint8_t length, uint8_t *Data);
        Simple_Wire & WriteBytes(uint8_t AltAddress,uint8_t regAddr, uint8_t length, uint8_t *Data);
        //Signed Ints
        Simple_Wire & WriteInt(uint8_t regAddr,  int16_t Val);
        Simple_Wire & WriteInt(uint8_t AltAddress,uint8_t regAddr,  int16_t Val);
        Simple_Wire & WriteInts(uint8_t regAddr, uint8_t size,  int16_t *data);
        Simple_Wire & WriteInts(uint8_t AltAddress,uint8_t regAddr, uint8_t size,  int16_t *data);
        //Un-Signed Ints
        
        Simple_Wire & WriteUInt(uint8_t regAddr,  uint16_t Val);
        Simple_Wire & WriteUInt(uint8_t AltAddress,uint8_t regAddr,  uint16_t Val);
        Simple_Wire & WriteUInts(uint8_t regAddr, uint8_t size,  uint16_t *data);
        Simple_Wire & WriteUInts(uint8_t AltAddress,uint8_t regAddr, uint8_t size,  uint16_t *data);   
            
        // check fundtions
        uint8_t  CheckAddress(){return(devAddr);}; // deprecated
        uint8_t  GetAddress(){return(devAddr);};
        uint8_t  ReadCount() {return(I2CReadCount);};
        uint8_t  WriteCount() {return(I2CWriteCount);};
        bool  ReadSuccess() {return(I2CReadCount>0);};
        bool  WriteSucess() {return(I2CWriteCount>0);};
        // Helper functions
        Simple_Wire & I2C_Scanner();
        uint8_t Find_Address(uint8_t Address,uint8_t Limit );
        uint8_t Check_Address(uint8_t Address);
        uint8_t Value(uint8_t * V){return(V[0]);};
        int8_t Value(int8_t * V){return(V[0]);};
        uint16_t Value(uint16_t * V){return(V[0]);};
        int16_t Value(int16_t * V){return(V[0]);};
        int32_t Value(){return(Val);};
        uint8_t GetErrorMessage() {return ErrorMessage;};
        bool Success() {return(ErrorMessage == 0);}
        Simple_Wire & Delay( uint32_t ms ){delay(ms); return *this;};
    private:
        uint8_t FirstByteShift = 8;
        uint8_t SecondByteShift = 0;    
        uint8_t I2CReadCount;
        uint8_t I2CWriteCount;
        uint8_t _sdaPin;
        uint8_t _sclPin;

};

extern TwoWire Wire;
extern TwoWire Wire1;

#endif 