# chavrprog

AVR programmer using  Chinese ch341a chip

There is a lot of strange things from China, and ch341a is one of the them. Cheap USB-Serial, USB-SPI, USB-I2C, USB-Parralel, etc makes it seems to be the real thing. However, lack of documentation make that device almost useless, except for couple of open-source projects. This project adds support for AVR microcotrollers programming using CH341a chip/

Project is based on ch341prog by Setarcos (https://github.com/setarcos/ch341prog), and technically is a brief implementation of ATMEL
serial programming protocol using low-level functions from ch341prog project.
Also, to parse Intel HEX source files from libcintelhex (https://github.com/martin-helmich/libcintelhex) are used.



Usage
---------
Connect ch341 programmer by scheme:
AVR    ch341a  
MISO-----MISO  
MOSI-----MOSI  
SCK------SCK  
Reset----CS  
GND------GND

Or the same using popular chineese programmer, connect to socket for spi flash:  
socket  AVR  
1-------Reset  
2-------MISO  
4-------GND  
5-------MOSI  
6-------SCK

### Commads

-d DEVICE - specifies AVR chip  
-r ADDR - read flash memory from start to ADDR in bytes, or  
-r f FILE read whole flash memory to specified file  
-w HEX - write flash memory from intel HEX file  
-e - erase device  
-c HEX - read and compare HEX with flash memory (strictly)  
-a HEX - automatically erase, write and check data in chip, or  
-a s HEX - same with strict verification instead of fast default  
use if you're flashing complex HEX, like Arduino bootloader or if default fails  
-f - read fuse bits  
-l BYTE - write low fuse  
-H BYTE - write high fuse  
-x BYTE - write extended fuse  
-L read lock bits  

There is 2 verification functions implemented - fast and strict. By default with -a command fast one is used. This is good for writing homogenous HEX files - like most of programms, and works much faster because verifies only written part. Another one verifies whole memory, what makes it applicable for complex HEX with gaps, but works slower due to whole chip reading.


Disklaimer
-----------


Project is under development and may contain mistakes.

To add another MCU's it's necessary to add it discription (name, pagesize, number of pages, eeprom size) to file config.c Most popular AVRs (which is used in Arduino) are already added. Theoretically, current version should work with any Atmega chip, if Atmel commad set is same for all chips, but was tested only with on Atmega32u4. Devices with more then 64K memory not supported now. Implementation for Attiny devices need to change SPI programming commands and bit shifting in functions.

###TODO:
Read/write EEPROM,  
Writing from HEX with gaps (current version can write only continous HEX), | DONE
Attiny device support, 
device choose in command line | DONE
UI | done (I am quite satisfacted)



License
------------
This is free software: you can redistribute it and/or modify it under
the terms of the latest GNU General Public License as published by the
Free Software Foundation.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <http://www.gnu.org/licenses/>.
