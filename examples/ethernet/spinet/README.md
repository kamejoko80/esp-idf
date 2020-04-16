# SPINET Example
(See the README.md file in the upper level 'examples' directory for more information about examples.)

## Overview

Build an ethernet network device by using software MAC layer via SPI interface.

## How to use example

### Hardware Required

ESP32(IntoRobot) + STM32F4 Discovery

#### Pin Assignment

|                    |                            |
|  Master(STM32F4)   | Slaver(ESP32)              |
| ------------------ | -------------------------- |
|  PB6 (SPI1_NSS)    | GPIO14 (A5) (SPI1_NSS)     |
|  PB3 (SPI1_SCK)    | GPIO15 (A7) (SPI_SCK)      |
|  PB4 (SPI1_MISO)   | GPIO13 (A8) (SPI_MISO)     |
|  PB5 (SPI1_MOSI)   | GPIO12 (A6) (SPI_MOSI)     |
|  PE4 (m_status_out)| GPIO16 (D4) (m_status_in)  |
|  PE5 (s_status_in) | GPIO17 (D3) (s_status_out) |
|  PB0 (m_irq_in)    | GPIO5  (D5) (s_send_rqst)  |
|  PE1 (m_send_rqst) | GPIO2  (A9) (s_irq_in)     |


### Configure the project

```
idf.py menuconfig
```

In the `Example Configuration` menu, set SPI specific configuration, such as SPI host number, GPIO used for MISO/MOSI/CS signal, GPIO for interrupt event and the SPI clock rate.

**Note:** According to ENC28J60 data sheet, SPI clock could reach up to 20MHz, but in practice, the clock speed will depend on your PCB layout (in this example, the default clock rate is set to 6MHz, just to make sure that most modules on the market can work at this speed).

### Build, Flash, and Run

Build the project and flash it to the board, then run monitor tool to view serial output:

```
idf.py -p PORT build flash monitor
```

(Replace PORT with the name of the serial port to use.)

(To exit the serial monitor, type ``Ctrl-]``.)

See the [Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/index.html) for full steps to configure and use ESP-IDF to build projects.

## Example Output

```bash
I (9) boot: ESP-IDF v4.2-dev-792-g6330b33-dirty 2nd stage bootloader
I (9) boot: compile time 22:41:12
I (9) boot: chip revision: 0
I (23) boot.esp32: SPI Speed      : 80MHz
I (23) boot.esp32: SPI Mode       : DIO
I (23) boot.esp32: SPI Flash Size : 2MB
I (26) boot: Enabling RNG early entropy source...
I (32) boot: Partition Table:
I (35) boot: ## Label            Usage          Type ST Offset   Length
I (43) boot:  0 nvs              WiFi data        01 02 00009000 00006000
I (50) boot:  1 phy_init         RF data          01 01 0000f000 00001000
I (57) boot:  2 factory          factory app      00 00 00010000 00100000
I (65) boot: End of partition table
I (69) esp_image: segment 0: paddr=0x00010020 vaddr=0x3f400020 size=0x136b0 ( 79536) map
I (104) esp_image: segment 1: paddr=0x000236d8 vaddr=0x3ffb0000 size=0x02e7c ( 11900) load
I (108) esp_image: segment 2: paddr=0x0002655c vaddr=0x40080000 size=0x00404 (  1028) load
I (111) esp_image: segment 3: paddr=0x00026968 vaddr=0x40080404 size=0x096b0 ( 38576) load
I (134) esp_image: segment 4: paddr=0x00030020 vaddr=0x400d0020 size=0x442c0 (279232) map
I (224) esp_image: segment 5: paddr=0x000742e8 vaddr=0x40089ab4 size=0x033ac ( 13228) load
I (229) esp_image: segment 6: paddr=0x0007769c vaddr=0x400c0000 size=0x00064 (   100) load
I (238) boot: Loaded app from partition at offset 0x10000
I (238) boot: Disabling RNG early entropy source...
I (242) cpu_start: Pro cpu up.
I (246) cpu_start: Application information:
I (251) cpu_start: Project name:     spinet
I (255) cpu_start: App version:      1
I (260) cpu_start: Compile time:     Apr 13 2020 23:17:11
I (266) cpu_start: ELF file SHA256:  644e73f0c9f83fd0...
I (272) cpu_start: ESP-IDF:          v4.2-dev-792-g6330b33-dirty
I (279) cpu_start: Starting app cpu, entry point is 0x40081618
I (0) cpu_start: App cpu up.
I (289) heap_init: Initializing. RAM available for dynamic allocation:
I (296) heap_init: At 3FFAE6E0 len 00001920 (6 KiB): DRAM
I (302) heap_init: At 3FFB4E28 len 0002B1D8 (172 KiB): DRAM
I (308) heap_init: At 3FFE0440 len 00003AE0 (14 KiB): D/IRAM
I (314) heap_init: At 3FFE4350 len 0001BCB0 (111 KiB): D/IRAM
I (321) heap_init: At 4008CE60 len 000131A0 (76 KiB): IRAM
I (327) cpu_start: Pro cpu start user code
I (345) spi_flash: detected chip: generic
I (345) spi_flash: flash io: dio
W (345) spi_flash: Detected size(4096k) larger than the size in the binary image header(2048k). Using the size in the binary image header.
I (356) cpu_start: Starting scheduler on PRO CPU.
I (0) cpu_start: Starting scheduler on APP CPU.

Type 'help' to get the list of commands.
Use UP/DOWN arrows to navigate through command history.
Press TAB when typing command name to auto-complete.
I (562) register_ethernet: starting...
I (562) spinet_mac: Initialize IPC...
I (562) gpio: GPIO[4]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:0 
I (572) gpio: GPIO[17]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:0 
I (572) gpio: GPIO[16]| InputEn: 1| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:0 
I (592) gpio: GPIO[5]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:0 
I (602) gpio: GPIO[2]| InputEn: 1| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:2 
I (612) esp_eth.netif.glue: a6:99:91:ad:9d:6f
I (612) esp_eth.netif.glue: ethernet attached to netif

 =======================================================
 |       Steps to Test Ethernet Bandwidth              |
 |                                                     |
 |  1. Enter 'help', check all supported commands      |
 |  2. Wait ESP32 to get IP from DHCP                  |
 |  3. Enter 'ethernet info', optional                 |
 |  4. Server: 'iperf -u -s -i 3'                      |
 |  5. Client: 'iperf -u -c SERVER_IP -t 60 -i 3'      |
 |                                                     |
 =======================================================

esp> ethernet info
HW ADDR: a6:99:91:ad:9d:6f
ETHIP: 192.168.2.1
ETHMASK: 255.255.255.0
ETHGW: 192.168.1.1

esp> ping 192.168.2.2
64 bytes from 192.168.2.2 icmp_seq=1 ttl=255 time=4 ms
esp> 64 bytes from 192.168.2.2 icmp_seq=2 ttl=255 time=3 ms
64 bytes from 192.168.2.2 icmp_seq=3 ttl=255 time=4 ms
64 bytes from 192.168.2.2 icmp_seq=4 ttl=255 time=4 ms
64 bytes from 192.168.2.2 icmp_seq=5 ttl=255 time=3 ms

--- 192.168.2.2 ping statistics ---
5 packets transmitted, 5 received, 0% packet loss, time 18ms
```
