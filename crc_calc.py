#!/usr/bin/env python 

# This program calculates the checksum of the device ID
# Different device families have different first byte:
# DS18B20	0x28
# DS2423	0x1D
#
# CRC calculation in 1-wire devices
# http://www.maximintegrated.com/en/app-notes/index.mvp/id/27

# Change this to your wanted 7-byte ID, 
# last byte is checksum (which will be calculated)
ID = [0x1D,0xA2,0xD9,0x84,0x00,0x00,0x01, 0x00]

crc=0
for i in range(0,7):
	inbyte = ID[i]
	for j in range(0,8):
		mix = (crc ^ inbyte) & 0x01
		crc >>= 1
		if mix:
			crc ^= 0x8C
		inbyte >>= 1

ID[7]=crc

print "{",
for i in range(0,7):
	print format(ID[i], '#04x')+",",
print format(ID[7], '#04x'),"}"
