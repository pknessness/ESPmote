meta:
  id: accelerometer_calibration
  file-extension: acc_c

seq:
  - id: data
    size: 10
instances:
  x_0g:
    value: (data[0] << 2) | (data[3] & 0x30)
  y_0g:
    value: (data[0] << 2) | (data[3] & 0x0C)
  z_0g:
    value: (data[0] << 2) | (data[3] & 0x03)
  x_1g:
    value: (data[4] << 2) | (data[7] & 0x30)
  y_1g:
    value: (data[4] << 2) | (data[7] & 0x0C)
  z_1g:
    value: (data[4] << 2) | (data[7] & 0x03)
  motor:
    value: (data[8] & 0x80)
  volume:
    value: (data[8] & 0x7F)
  checksum:
    value: (data[9])
  calculated_checksum:
    value: (data[9]) == ((data[0] + data[1] + data[2] + data[3] + data[4] + data[5] + data[6] + data[7] + data[8] + 0x55) & 0xFF)
    
    
    
    
    