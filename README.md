| Supported Targets | ESP32 |
| ----------------- | ----- |

[![ESPbote_PCB](https://img.shields.io/badge/PCB-ESPmote_PCB-green.svg)](https://github.com/pknessness/ESPmote_PCB)

# ESPmote

This code is for an ESP Wiimote emulator, the ESPmote. This is intended to be a full replacement for a Wiimote, with an accompanying drop-in replacement PCB, which you can find [here](https://github.com/pknessness/ESPmote_PCB), and this code is written for said board.

The original Wiimote is built around a Broadcom BCM2042, and my ESPmote is build around ESP32 (Only the original chip, any of the C or S variants, as none of them support Bluetooth Classic which is necessary to communicate with the Wii). Wiimotes are known to use an ADXL330 for accelerometer data and the Wii Motion Plus (WM+ or WMP) uses an InvenSense IDG-600 for pitch and roll and an EPSON TOYOCOM X3500 for yaw. ESPmote supports WM+ [TODO: Implement billiard26 WM+ handshake math] and uses an LSM6DS3 for both accelerometer and gyro. 

I attempted to replace the IR camera with a HM01B0 and pico, but due to size constraints and my lack of optics knowledge, that avenue fell through. The ESPmote currently uses an existing Wiimote's IR Camera. 

# Memory ([Wiibrew](https://wiibrew.org/wiki/Wiimote#EEPROM_Memory))

## EEPROM

### Calibration (Accelerometer, Motor, Volume)

<table><thead>
  <tr>
    <th colspan="2">Byte</th>
    <th>7</th>
    <th>6</th>
    <th>5</th>
    <th>4</th>
    <th>3</th>
    <th>2</th>
    <th>1</th>
    <th>0</th>
  </tr>
</thead>
<tbody>
  <tr>
    <td>0x16</td>
    <td>0x20</td>
    <td colspan="8">0G X Acceleration&lt;9:2&gt;</td>
  </tr>
  <tr>
    <td>0x17</td>
    <td>0x21</td>
    <td colspan="8">0G Y Acceleration&lt;9:2&gt;</td>
  </tr>
  <tr>
    <td>0x18</td>
    <td>0x22</td>
    <td colspan="8">0G Z Acceleration&lt;9:2&gt;</td>
  </tr>
  <tr>
    <td>0x19</td>
    <td>0x23</td>
    <td colspan="2">0</td>
    <td colspan="2">0G X Acc&lt;1:0&gt;</td>
    <td colspan="2">0G Y Acc&lt;1:0&gt;</td>
    <td colspan="2">0G Z Acc&lt;1:0&gt;</td>
  </tr>
  <tr>
    <td>0x1A</td>
    <td>0x24</td>
    <td colspan="8">1G X Acceleration&lt;9:2&gt;</td>
  </tr>
  <tr>
    <td>0x1B</td>
    <td>0x25</td>
    <td colspan="8">1G Y Acceleration&lt;9:2&gt;</td>
  </tr>
  <tr>
    <td>0x1C</td>
    <td>0x26</td>
    <td colspan="8">1G Z Acceleration&lt;9:2&gt;</td>
  </tr>
  <tr>
    <td>0x1D</td>
    <td>0x27</td>
    <td colspan="2">0</td>
    <td colspan="2">1G X Acc&lt;1:0&gt;</td>
    <td colspan="2">1G Y Acc&lt;1:0&gt;</td>
    <td colspan="2">1G Z Acc&lt;1:0&gt;</td>
  </tr>
  <tr>
    <td>0x1E</td>
    <td>0x28</td>
    <td>Motor</td>
    <td colspan="7">Volume</td>
  </tr>
  <tr>
    <td>0x1F</td>
    <td>0x29</td>
    <td colspan="8">Checksum</td>
  </tr>
</tbody></table>

### Mii Data

## Registers

### Speaker Settings

### Extension Controller Settings/Data

### Wii Motion Plus Settings/Data

<table><thead>
  <tr>
    <th>&nbsp;&nbsp;&nbsp;Offset</th>
    <th>0x00</th>
    <th>0x01</th>
    <th>0x02</th>
    <th>0x03</th>
    <th>0x04</th>
    <th>0x05</th>
    <th>0x06</th>
    <th>0x07</th>
  </tr>
</thead>
<tbody>
  <tr>
    <td>0x00</td>
    <td colspan="8">Controller Data&lt;0:7&gt;</td>
  </tr>
  <tr>
    <td>0x08</td>
    <td colspan="8">Controller Data&lt;8:15&gt;</td>
  </tr>
  <tr>
    <td>0x10</td>
    <td colspan="5">Controller Data&lt;16:20&gt;</td>
    <td colspan="3">Unknown [0xFF]</td>
  </tr>
  <tr>
    <td>0x18</td>
    <td colspan="8">Unknown [0xFF]</td>
  </tr>
  <tr>
    <td>0x20</td>
    <td colspan="2">ZERO YAW FAST</td>
    <td colspan="2">ZERO ROLL FAST</td>
    <td colspan="2">ZERO PITCH FAST</td>
    <td colspan="2">SCALE YAW FAST</td>
  </tr>
  <tr>
    <td>0x28</td>
    <td colspan="2">SCALE ROLL FAST</td>
    <td colspan="2">SCALE PITCH FAST</td>
    <td>FAST SCALE DEG/6</td>
    <td>UID 1 [0x01]</td>
    <td colspan="2">CRC32 MSB of WM+ Calib</td>
  </tr>
  <tr>
    <td>0x30</td>
    <td colspan="2">ZERO YAW SLOW</td>
    <td colspan="2">ZERO ROLL SLOW</td>
    <td colspan="2">ZERO PITCH SLOW</td>
    <td colspan="2">SCALE YAW SLOW</td>
  </tr>
  <tr>
    <td>0x38</td>
    <td colspan="2">SCALE ROLL SLOW</td>
    <td colspan="2">SCALE PITCH SLOW</td>
    <td>SLOW SCALE DEG/6</td>
    <td>UID 2 [0x6E]</td>
    <td colspan="2">CRC32 LSB of WM+ Calib</td>
  </tr>
  <tr>
    <td>0x40</td>
    <td colspan="8">Passthrough Extension Calibration &lt;0:7&gt;</td>
  </tr>
  <tr>
    <td>0x48</td>
    <td colspan="8">Passthrough Extension Calibration &lt;8:15&gt;</td>
  </tr>
  <tr>
    <td>0x50</td>
    <td colspan="8" rowspan="8">Challenge Data</td>
  </tr>
  <tr>
    <td>0x58</td>
  </tr>
  <tr>
    <td>0x60</td>
  </tr>
  <tr>
    <td>0x68</td>
  </tr>
  <tr>
    <td>0x70</td>
  </tr>
  <tr>
    <td>0x78</td>
  </tr>
  <tr>
    <td>0x80</td>
  </tr>
  <tr>
    <td>0x88</td>
  </tr>
  <tr>
    <td>0x90</td>
    <td colspan="8" rowspan="12">Unknown</td>
  </tr>
  <tr>
    <td>0x98</td>
  </tr>
  <tr>
    <td>0xA0</td>
  </tr>
  <tr>
    <td>0xA8</td>
  </tr>
  <tr>
    <td>0xB0</td>
  </tr>
  <tr>
    <td>0xB8</td>
  </tr>
  <tr>
    <td>0xC0</td>
  </tr>
  <tr>
    <td>0xC8</td>
  </tr>
  <tr>
    <td>0xD0</td>
  </tr>
  <tr>
    <td>0xD8</td>
  </tr>
  <tr>
    <td>0xE0</td>
  </tr>
  <tr>
    <td>0xE8</td>
  </tr>
  <tr>
    <td>0xF0</td>
    <td>Init Trig</td>
    <td>Chal Type</td>
    <td>Calib Trig</td>
    <td colspan="3">Unknown</td>
    <td>Pass ID&lt;4&gt;</td>
    <td>Chal State</td>
  </tr>
  <tr>
    <td>0xF8</td>
    <td>Pass ID&lt;0&gt;</td>
    <td>Pass ID&lt;5&gt;</td>
    <td colspan="6">Wii Motion Plus Identifier [?? 00 A6 20 0? 05]</td>
  </tr>
</tbody></table>

### IR Camera Settings

This is currently believed to relate directly to the registers on the Pixart IR Camera

# Button Data ([Wiibrew](https://wiibrew.org/wiki/Wiimote#Buttons))

Buttons are two bytes of data, this is how those bits are organized

| Bit | Mask | First Byte   | Second Byte  |
| --- | ---- | ------------ | ------------ |
| 0   | 0x01 | DPAD LEFT    | TWO          |
| 1   | 0x02 | DPAD RIGHT   | ONE          |
| 2   | 0x04 | DPAD DOWN    | B            |
| 3   | 0x08 | DPAD UP      | A            |
| 4   | 0x10 | PLUS         | MINUS        |
| 5   | 0x20 | <Accel LSBs> | <Accel LSBs> |
| 6   | 0x40 | <Accel LSBs> | <Accel LSBs> |
| 7   | 0x80 | unknown      | HOME         |

# Accelerometer Data ([Wiibrew](https://wiibrew.org/wiki/Wiimote#Accelerometer))

Accelerometer data is three unsigned values of 10-bits each for X Y and Z. There are three designated bytes for accelerometer data, which is in the format XX YY and ZZ and stores the high 8 bits of each accelerometer value, and then the remaining 2 bits are stored in the excess of the buttons data, but it's key to note that X gets both bits represented, while Y and Z only store the higher of the two remaining bits, while the last bit is considered always 0.

Because accelerometer data is unsigned, our zero value for the accelerometer is 512. The value of +- 1G depends on the accelerometer values in the calibration section of memory, but is generally 100.

<table><thead>
  <tr>
    <th>Byte</th>
    <th>7</th>
    <th>6</th>
    <th>5</th>
    <th>4</th>
    <th>3</th>
    <th>2</th>
    <th>1</th>
    <th>0</th>
    <th></th>
  </tr>
</thead>
<tbody>
  <tr>
    <td>0</td>
    <td>&nbsp;&nbsp;</td>
    <td colspan="2">X&lt;1:0&gt;</td>
    <td colspan="5">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</td>
    <td></td>
  </tr>
  <tr>
    <td>1</td>
    <td>&nbsp;&nbsp;</td>
    <td>Z&lt;1&gt;</td>
    <td>Y&lt;1&gt;</td>
    <td colspan="5">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</td>
    <td></td>
  </tr>
</tbody>
</table>

This image shows the sign of the accelerometer values.

![](assets/Wiimote_axis2.png)

For example, if the wiimote is in the shown configuration, we would get a + value on Z.

# Wii Motion Data ([Wiibrew](https://wiibrew.org/wiki/Wiimote/Extension_Controllers/Wii_Motion_Plus#Data_Format))

## Wii Motion Plus Basic Packet (GYRO)

Gyro data is in 6 bits, as shown below. Slow mode vs fast mode changes the multiplication factor of the data being encoded. The way this data works requires knowledge of the calibration parameters in the table above. The measurement at CALIB_ZERO + CALIB_SCALE_VALUE is a measurement of SCALE_DEGREES, and this applies for both FAST and SLOW mode. For example, if CALIB_ZERO is 0x8000, CALIB_SCALE_VALUE is 0x4400, and SCALE_DEGREES is 270 (Remember this is stored in calibration as SCALE_DEGREES/6, so this would be stored as 45), then a measurement of 0x8000 + 0x2200 would be 135 degrees per second. 

The slow mode bit determines whether the calibration constants being used are slow mode calibrations or fast mode calibrations.

<table><thead>
  <tr>
    <th>Byte</th>
    <th>7</th>
    <th>6</th>
    <th>5</th>
    <th>4</th>
    <th>3</th>
    <th>2</th>
    <th>1</th>
    <th>0</th>
  </tr>
</thead>
<tbody>
  <tr>
    <td>0</td>
    <td colspan="8">Yaw Down Speed&lt;7:0&gt;</td>
  </tr>
  <tr>
    <td>1</td>
    <td colspan="8">Roll Left Speed&lt;7:0&gt;</td>
  </tr>
  <tr>
    <td>2</td>
    <td colspan="8">Pitch Left Speed&lt;7:0&gt;</td>
  </tr>
  <tr>
    <td>3</td>
    <td colspan="6">Yaw Down Speed&lt;13:8&gt;</td>
    <td>Yaw slow mode</td>
    <td>Pitch slow mode</td>
  </tr>
  <tr>
    <td>4</td>
    <td colspan="6">Roll Left Speed&lt;13:8&gt;</td>
    <td>Roll slow mode</td>
    <td>Extension connected</td>
  </tr>
  <tr>
    <td>5</td>
    <td colspan="6">Pitch Left Speed&lt;13:8&gt;</td>
    <td>1</td>
    <td>0</td>
  </tr>
</tbody>
</table>

Something to note is that bit 1 of byte 5 determines whether the data in the extension section is WiiMotion+ data or passthrough extension data.

## Wii Motion Plus Nunchuk Passthrough Packet

The buttons here are 1 if unpressed and 0 if pressed

<table><thead>
  <tr>
    <th>Byte</th>
    <th>7</th>
    <th>6</th>
    <th>5</th>
    <th>4</th>
    <th>3</th>
    <th>2</th>
    <th>1</th>
    <th>0</th>
  </tr>
</thead>
<tbody>
  <tr>
    <td>0</td>
    <td colspan="8">SX&lt;7:0&gt;</td>
  </tr>
  <tr>
    <td>1</td>
    <td colspan="8">SY&lt;7:0&gt;</td>
  </tr>
  <tr>
    <td>2</td>
    <td colspan="8">AX&lt;9:2&gt;</td>
  </tr>
  <tr>
    <td>3</td>
    <td colspan="8">AY&lt;9:2&gt;</td>
  </tr>
  <tr>
    <td>4</td>
    <td colspan="7">AZ&lt;9:3&gt;</td>
    <td>Extension connected</td>
  </tr>
  <tr>
    <td>5</td>
    <td colspan="2">AZ&lt;2:1&gt;</td>
    <td>AY&lt;1&gt;</td>
    <td>AX&lt;1&gt;</td>
    <td>BC</td>
    <td>BZ</td>
    <td>0</td>
    <td>0</td>
  </tr>
</tbody>
</table>

## Wii Motion Plus Classic Controller Passthrough Packet

<table><thead>
  <tr>
    <th>Byte</th>
    <th>7</th>
    <th>6</th>
    <th>5</th>
    <th>4</th>
    <th>3</th>
    <th>2</th>
    <th>1</th>
    <th>0</th>
  </tr>
</thead>
<tbody>
  <tr>
    <td>0</td>
    <td colspan="2">RX&lt;4:3&gt;</td>
    <td colspan="5">LX&lt;5:1&gt;</td>
    <td>BDU</td>
  </tr>
  <tr>
    <td>1</td>
    <td colspan="2">RX&lt;2:1&gt;</td>
    <td colspan="5">LY&lt;5:1&gt;</td>
    <td>BDL</td>
  </tr>
  <tr>
    <td>2</td>
    <td>RX&lt;0&gt;</td>
    <td colspan="2">LT&lt;4:3&gt;</td>
    <td colspan="5">RY&lt;4:0&gt;</td>
  </tr>
  <tr>
    <td>3</td>
    <td colspan="3">LT&lt;2:0&gt;</td>
    <td colspan="5">RT&lt;4:0&gt;</td>
  </tr>
  <tr>
    <td>4</td>
    <td>BDR</td>
    <td>BDD</td>
    <td>BLT</td>
    <td>B-</td>
    <td>BH</td>
    <td>B+</td>
    <td>BRT</td>
    <td>Extension connected</td>
  </tr>
  <tr>
    <td>5</td>
    <td>BZL</td>
    <td>BB</td>
    <td>BY</td>
    <td>BA</td>
    <td>BX</td>
    <td>BZR</td>
    <td>0</td>
    <td>0</td>
  </tr>
</tbody></table>

# IR Camera Data ([Wiibrew](https://wiibrew.org/wiki/Wiimote#IR_Camera))

## IR Camera Control Registers (0xB00000 - 0xB00033)

<table><thead>
  <tr>
    <th></th>
    <th>0x0</th>
    <th>0x1</th>
    <th>0x2</th>
    <th>0x3</th>
    <th>0x4</th>
    <th>0x5</th>
    <th>0x6</th>
    <th>0x7</th>
    <th>0x8</th>
    <th>0x9</th>
    <th>0xA</th>
    <th>0xB</th>
    <th>0xC</th>
    <th>0xD</th>
    <th>0xE</th>
    <th>0xF</th>
  </tr></thead>
<tbody>
  <tr>
    <td>0xB00000</td>
    <td colspan="9">Sensitivity 1</td>
    <td></td>
    <td></td>
    <td></td>
    <td></td>
    <td></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>0xB00010</td>
    <td></td>
    <td></td>
    <td></td>
    <td></td>
    <td></td>
    <td></td>
    <td></td>
    <td></td>
    <td></td>
    <td></td>
    <td colspan="2">Sensitivity 2</td>
    <td></td>
    <td></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>0xB00020</td>
    <td></td>
    <td></td>
    <td></td>
    <td></td>
    <td></td>
    <td></td>
    <td></td>
    <td></td>
    <td></td>
    <td></td>
    <td></td>
    <td></td>
    <td></td>
    <td></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>0xB00030</td>
    <td>STATUS</td>
    <td></td>
    <td></td>
    <td>MODE</td>
    <td></td>
    <td></td>
    <td></td>
    <td></td>
    <td></td>
    <td></td>
    <td></td>
    <td></td>
    <td></td>
    <td></td>
    <td></td>
    <td></td>
  </tr>
</tbody></table>

This is the IR Camera internal registers, believed to match up to the Wiimote's control registers 0xB00000 to 0xB00033. Potential IR Calibration blocks can be found on the wiibrew page. STATUS is set to 0x01 when setting up, then 0x08 when setup is complete. MODE is one of the three data reporting modes, 0x01, 0x03, or 0x05, but can also be set to 0x33, and what this means is unknown.

## Input Data Format

### Basic Data (0x01) [(10 bytes)](https://wiibrew.org/wiki/Wiimote#Basic_Mode)

<table><thead>
  <tr>
    <th>Byte</th>
    <th>7</th>
    <th>6</th>
    <th>5</th>
    <th>4</th>
    <th>3</th>
    <th>2</th>
    <th>1</th>
    <th>0</th>
  </tr>
</thead>
<tbody>
  <tr>
    <td>0</td>
    <td colspan="8">X1&lt;7:0&gt;</td>
  </tr>
  <tr>
    <td>1</td>
    <td colspan="8">Y1&lt;7:0&gt;</td>
  </tr>
  <tr>
    <td>2</td>
    <td colspan="2">Y1&lt;9:8&gt;</td>
    <td colspan="2">X1&lt;9:8&gt;</td>
    <td colspan="2">Y2&lt;9:8&gt;</td>
    <td colspan="2">X2&lt;9:8&gt;</td>
  </tr>
  <tr>
    <td>3</td>
    <td colspan="8">X2&lt;7:0&gt;</td>
  </tr>
  <tr>
    <td>4</td>
    <td colspan="8">Y2&lt;7:0&gt;</td>
  </tr>
</tbody>
</table>

x2

### Extended Data (0x03) [(12 bytes)](https://wiibrew.org/wiki/Wiimote#Extended_Mode)

<table><thead>
  <tr>
    <th>Byte</th>
    <th>7</th>
    <th>6</th>
    <th>5</th>
    <th>4</th>
    <th>3</th>
    <th>2</th>
    <th>1</th>
    <th>0</th>
  </tr>
</thead>
<tbody>
  <tr>
    <td>0</td>
    <td colspan="8">X&lt;7:0&gt;</td>
  </tr>
  <tr>
    <td>1</td>
    <td colspan="8">Y&lt;7:0&gt;</td>
  </tr>
  <tr>
    <td>2</td>
    <td colspan="2">Y&lt;9:8&gt;</td>
    <td colspan="2">X&lt;9:8&gt;</td>
    <td colspan="4">S&lt;3:0&gt;</td>
  </tr>
</tbody>
</table>

x4

### Full Data (0x05) [(36 bytes)](https://wiibrew.org/wiki/Wiimote#Full_Mode)

<table><thead>
  <tr>
    <th>Byte</th>
    <th>7</th>
    <th>6</th>
    <th>5</th>
    <th>4</th>
    <th>3</th>
    <th>2</th>
    <th>1</th>
    <th>0</th>
  </tr>
</thead>
<tbody>
  <tr>
    <td>0</td>
    <td colspan="8">X&lt;7:0&gt;</td>
  </tr>
  <tr>
    <td>1</td>
    <td colspan="8">Y&lt;7:0&gt;</td>
  </tr>
  <tr>
    <td>2</td>
    <td colspan="2">Y&lt;9:8&gt;</td>
    <td colspan="2">X&lt;9:8&gt;</td>
    <td colspan="4">S&lt;3:0&gt;</td>
  </tr>
  <tr>
    <td>3</td>
    <td>0</td>
    <td colspan="7">X min&lt;6:0&gt;</td>
  </tr>
  <tr>
    <td>4</td>
    <td>0</td>
    <td colspan="7">Y min&lt;6:0&gt;</td>
  </tr>
  <tr>
    <td>5</td>
    <td>0</td>
    <td colspan="7">X max&lt;6:0&gt;</td>
  </tr>
  <tr>
    <td>6</td>
    <td>0</td>
    <td colspan="7">Y max&lt;6:0&gt;</td>
  </tr>
  <tr>
    <td>7</td>
    <td colspan="8">0</td>
  </tr>
  <tr>
    <td>8</td>
    <td colspan="8">Intensity&lt;7:0&gt;</td>
  </tr>
</tbody>
</table>

x4

# Notes

When looking through code and my notes above, I am sticking with the HID convention that I observed when writing the code, which is that output and input are relative to the HID Host, which in this case is the Wii. Reports leaving the wiimote are input reports, and reports leaving the wii are output reports.

# Credits

Thanks to

- [Dolphin Emulator](https://dolphin-emu.org/) devs (especially @billiard26)

- [Kako's blog](http://www.kako.com/neta/index.html)

- Contributors of [Wiibrew's wiimote page](https://wiibrew.org/wiki/Wiimote)

- 

If you have questions about my process or about the code, email me at pk.ness.ness@gmail.com.
