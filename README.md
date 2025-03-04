  <h1>Simple_Wire Library</h1>
    <p>This code implements a simple wrapper library for the Wire library in Arduino, which provides a set of convenient methods for communicating with I2C devices. The Simple_Wire library is designed to simplify I2C communication by providing a set of methods for reading and writing data to I2C devices in a simple and consistent manner.</p>
    <p>The library provides methods for reading and writing individual bits, bytes, and integers, as well as methods for reading and writing arrays of data. The library also provides a method for setting the I2C address of the device to communicate with.</p>
    <p>The library is designed to work with a variety of microcontrollers, including the AVR, ESP8266, ESP32, and Arduino RP2040. The library also includes support for setting the I2C clock speed and timeout values.</p>
    <p>Using the Simple_Wire library can simplify the process of communicating with I2C devices in your Arduino project, allowing you to focus on your application logic rather than the details of the I2C protocol.</p>
<p>This code is the implementation of the <strong>Simple_Wire</strong> class, which provides an easy-to-use interface for I2C communication using the Wire library.</p>
<p>The class provides a number of methods for reading and writing data over I2C, with support for 8-bit, 16-bit, and bit-level access.</p>
<p>The class has a constructor that takes the SDA and SCL pins as arguments and initializes the Wire library accordingly. There is also a default constructor that doesn't initialize the Wire library, but this should not be called until after <code>setup()</code>, as the Wire library requires initialization before use.</p>
<p>The <code>begin()</code> method is used to initialize the Wire library with the given SDA and SCL pins. This method must be called at the earliest point in the <code>setup()</code> function of the main sketch, before any other methods of the <strong>Simple_Wire</strong> class are used.</p>
<p>The <code>SetAddress()</code> method sets the I2C address of the device that the <strong>Simple_Wire</strong> object will communicate with. This method returns a reference to the object, allowing for chaining of methods.</p>
<p>The <code>SetIntMSBPos()</code> method is used to set the position of the most significant bit (MSB) in a 16-bit integer. This is used to support devices that have different byte orderings for their data.</p>
<p>The <code>ReadBit()</code> method is used to read a single bit from a device register. The <code>ReadByte()</code> method is used to read a single byte. The <code>ReadBytes()</code> method is used to read multiple bytes.</p>
<p>The <code>ReadInt()</code> method is used to read a 16-bit integer from a device register. The <code>ReadInts()</code> method is used to read multiple 16-bit integers. The <code>ReadUInt()</code> and <code>ReadUInts()</code> methods are similar, but are used for unsigned integers.</p>
<p>The <code>WriteBit()</code> method is used to write a single bit to a device register. The <code>WriteByte()</code> method is used to write a single byte. The <code>WriteBytes()</code> method is used to write multiple bytes.</p>
<p>The <code>WriteInt()</code> method is used to write a 16-bit integer to a device register. The <code>WriteInts()</code> method is used to write multiple 16-bit integers. The <code>WriteUInt()</code> and <code>WriteUInts()</code> methods are similar, but are used for unsigned integers.</p>

<p>Macros can be used to quickly gather data from the desired registers for example:
<code>
#define R_LIA_Data(Data) PG(0).ReadInts(0x28, 3, (int16_t *)Data)//   Linear Acceleration Data.
#define R_LIA_Data_X(Data) PG(0).ReadInt(0x28, (int16_t *)Data)  //   Linear Acceleration Data X <15:0>
#define R_LIA_Data_Y(Data) PG(0).ReadInt(0x2A, (int16_t *)Data)  //   Linear Acceleration Data Y <15:0>
#define R_LIA_Data_Z(Data) PG(0).ReadInt(0x2C, (int16_t *)Data)  //   Linear Acceleration Data Z <15:0></code></p>
<p>Here is an explanation of the code:</p>
<ul>
  <li>The code defines a set of macros for reading data from a device register using the Simple_Wire library.</li>
  <li>The macros use the ReadInt() and ReadInts() methods of the Simple_Wire class to read data from the specified register address.</li>
  <li>The macro R_LIA_Data() reads all 3 INTs of data in one go and stores the result in the specified array.</li>
  <li>The macros R_LIA_Data_X(), R_LIA_Data_Y(), and R_LIA_Data_Z() read individual INTs of data for each axis.</li>
  <li>Using these macros simplifies the process of reading data from the device register, reducing the amount of code required.</li>
</ul>
<p>Overall, this approach allows for easy and efficient access to the device register with minimal code.</p>
