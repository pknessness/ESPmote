meta:
  id: wii_motion_plus
  file-extension: wmp
  endian: be
  bit-endian: le
  
seq:
  - id: data
    size: 6
instances:
  yaw:
    value: data[0] | ((data[3] & 0xFC) << 6)
  roll:
    value: data[1] | ((data[4] & 0xFC) << 6)
  pitch:
    value: data[2] | ((data[5] & 0xFC) << 6)
  yaw_slow:
    value: (data[3] & 0x2) >> 1
  roll_slow:
    value: (data[4] & 0x2) >> 1
  pitch_slow:
    value: (data[3] & 0x1)
  extension:
    value: (data[4] & 0x1)
  