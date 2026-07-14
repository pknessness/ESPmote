# LSM6DS3 Driver Files

This directory should contain the STMicroelectronics LSM6DS3 driver files.

## Obtaining the Driver Files

To get the official driver files from STMicroelectronics:

1. Clone the STMicroelectronics repository:
```bash
git clone https://github.com/STMicroelectronics/STMems_Standard_C_drivers.git
```

2. Copy the driver files from the LSM6DS3 driver folder:
```bash
cp STMems_Standard_C_drivers/lsm6ds3_STdC/driver/lsm6ds3_reg.h components/lsm6ds3/driver/
cp STMems_Standard_C_drivers/lsm6ds3_STdC/driver/lsm6ds3_reg.c components/lsm6ds3/driver/
```

Alternatively, you can add the STMicroelectronics repository as a git submodule:
```bash
git submodule add https://github.com/STMicroelectronics/STMems_Standard_C_drivers.git _submodules/STMems_Standard_C_drivers
```

Then create symbolic links or copy the files to this directory.

## Required Files

- `lsm6ds3_reg.h` - Register definitions and function declarations
- `lsm6ds3_reg.c` - Register read/write implementations

These files are required for the component to compile and function properly.

