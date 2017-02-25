Atheros SPI Read me for Windows Mobile/CE

Copyright 2008, Atheros Communications
 
Atheros supports SPI (Serial Peripheral Interface) interfaces for the AR6002.  The host platform must
meet the following requirements:

 - Motorola-SPI compatible protocol MOSI, MISO, CS, CLK pins.
 - Clock speeds up to 48 Mhz operation (MAX)
 - Clock polarity and phase control (modes 0-3).
 - 16 bit SPI data frames minimum, 32-bit recommended for best throughput.
 - Level sensitive interrupt for WLAN module pending interrupts (via GPIO pin).
 
The SPI support on Windows Mobile/CE uses a light-weight bus driver that uses a hardware
dependent component in a single driver DLL.  This component is provided in source and provided by the
BSP vendor.

The host controller driver encompasses the SPI bus driver core routines, AR6002 SPI protocol library
and hardware specific SPI bus control code.  The following diagram illustrates the layered components:

        -----------------------------
        | AR6K WLAN client driver   |  --------  AR6K_NDIS_SPI.DLL
        -----------------------------
                    ^
                    |
                    v
        -----------------------------
        |   SPI Bus Driver          | ---------  SPI bus driver DLL.
        |   SPI Hardware            | 
        -----------------------------
        
The source code to the bus driver is located in \host\spistack. Building this directory results
in a set of common SPI bus libraries and one or more sample SPI host controller drivers.  The
customer can copy and modify the \host\src\hcd\sample_hcd to support their specific hardware.  Refer
to the readme.txt files located in the sample_hcd directory.

The AR6K source kit automatically builds SDIO and SPI versions of the NDIS driver.  To integrate the SPI
version of the NDIS driver refer to the bib and reg files located in \host\os\wince\spi.  To integrate
the SPI hardware dependent components, refer to the reg file settings provided by the host controller
driver.  A sample registry file is provided in \host\spistack\src\hcd\sample_hcd\wince



        
 