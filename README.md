# Simple_Wire Library

A lightweight Arduino library that simplifies I²C (Wire) communication by wrapping the standard `Wire` library with an easy-to-use API. Designed to handle both simple and advanced read/write operations for a variety of microcontrollers, including AVR, ESP8266, ESP32, and Arduino RP2040 boards.

## Key Features

- **Unified I²C Interface**  
  Provides consistent methods for reading and writing to I²C devices, whether you’re manipulating a single bit or transferring multi-byte arrays.

- **Support for Multiple Data Types**  
  Read and write functions handle 8-bit, 16-bit, and 32/64-bit integers (both signed and unsigned), plus bit-level access.

- **Flexible Bit Manipulation**  
  Includes helper methods and macros for reading/writing individual bits while preserving other bits in the register.

- **Device Address Management**  
  Easily set and change the target I²C address for the device you’re communicating with.

- **Configurable Clock and Timeout**  
  Allows adjusting I²C clock speed and setting timeouts to match various hardware requirements.

- **Multi-Platform Compatibility**  
  Works on AVR, ESP8266, ESP32, and Arduino RP2040, leveraging conditional compilation to optimize I²C setup for each platform.

## Installation

1. **Download or Clone** this repository to your local machine.
2. Place the folder in your Arduino `libraries` directory (e.g., `Documents/Arduino/libraries/Simple_Wire`).
3. Restart the Arduino IDE. The library should now appear under **Sketch > Include Library > Contributed Libraries**.

Alternatively, once published to the Arduino Library Manager, you can install it directly via **Sketch > Include Library > Manage Libraries…** (search for “Simple_Wire”).

## Basic Usage

1. **Include and Initialize**  
   ```cpp
   #include <Simple_Wire.h>

   Simple_Wire I2C; // Create a Simple_Wire object

   void setup() {
     Serial.begin(115200);

     // Initialize I²C with SDA on pin 21 and SCL on pin 22 (ESP32 example)
     I2C.begin(21, 22);

     // Optionally set device address
     I2C.SetAddress(0x68); // Example: address for an MPU6050
   }

   void loop() {
     // Your code here...
   }
   ```

2. **Reading and Writing**  
   - **Single Bit**:
     ```cpp
     uint8_t bitValue;
     I2C.ReadBit(0x75, 2, 3, &bitValue); // Example: read bit #3 in register 0x75
     I2C.WriteBit(0x75, 1, 3, true);    // Example: set bit #3
     ```
   - **Single Byte**:
     ```cpp
     uint8_t byteValue;
     I2C.ReadByte(0x75, &byteValue);    // Read one byte from register 0x75
     I2C.WriteByte(0x75, 0xFF);         // Write one byte (0xFF) to register 0x75
     ```
   - **Multiple Bytes**:
     ```cpp
     uint8_t buffer[4];
     I2C.ReadBytes(0x75, 4, buffer);    // Read 4 bytes
     I2C.WriteBytes(0x75, 4, buffer);   // Write 4 bytes
     ```
   - **16-bit/32-bit/64-bit**:
     ```cpp
     uint16_t intValue;
     I2C.ReadUInt(0x75, &intValue);    // Read 16-bit unsigned integer
     I2C.WriteUInt(0x75, 0x1234);      // Write 16-bit unsigned integer 0x1234
     ```

3. **Bit Order Configuration**  
   ```cpp
   // Some devices store the MSB in a different position
   // SetIntMSBPos() can adjust how bits are interpreted.
   I2C.SetIntMSBPos(true); // or false, depending on the device
   ```

## Example Macros

You can define macros that wrap common read/write operations for specific registers on a sensor. For instance, to read linear acceleration data from a BNO055 at register 0x28:

```cpp
#define PG(Data)               WriteByte(0x07, (uint8_t)Data)       //   Go To Page ID
#define R_LIA_Data(Data)       PG(0).ReadInts(0x28, 3, (int16_t *)Data)
#define R_LIA_Data_X(Data)     PG(0).ReadInt(0x28, (int16_t *)Data)
#define R_LIA_Data_Y(Data)     PG(0).ReadInt(0x2A, (int16_t *)Data)
#define R_LIA_Data_Z(Data)     PG(0).ReadInt(0x2C, (int16_t *)Data)
```

- **Explanation**:
  - `PG(0).ReadInts(0x28, 3, (int16_t *)Data)` quickly fetches three 16-bit values from the specified registers on page zero.  
  - Other macros read individual axes.  
  - This approach reduces repeated code and makes sensor-specific readouts more readable.
  - You would use the macro like this: `BNO055.R_LIA_Data(&DataArray);` storing the array of values in DataArray.
  - For a single value, you would use the macro like this: `BNO055.R_LIA_Data_X(&Data);` storing the single interger values in Data.

## Helper Features
- **Address Scanning and Finding**:  
  The library includes an `I2C_Scanner()` method that can detect all active I²C addresses on the bus, helping you confirm which devices are present.
  You can also find a device an automatically use the address for your code to quickly start working with the component.
  
  **Checking and Troubleshooting**:
  Checking address and verifying read and write and viewing the error code can all be done easily by adding a helper function like .Success() at the end of your call which will return true if successful
  Example usage `if(!BNO055.R_LIA_Data_X(&Data).Success()) Serial.Print("We Failed to read the integer");`
  
## Advanced Usage 
- **Timeout and Error Handling**:  
  If needed, you can configure `Wire.setWireTimeout()` or check error codes from the library’s methods to handle bus errors or unexpected device responses gracefully.

## Contributing

Contributions are welcome! If you’d like to add new features, fix bugs, or improve documentation, feel free to open a pull request or file an issue on the [GitHub repository](https://github.com/YourUserName/Simple_Wire).

## License

This project is released under the [MIT License](LICENSE). Feel free to use it in both open-source and proprietary applications.
