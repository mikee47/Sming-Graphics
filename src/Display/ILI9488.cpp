/**
 * ILI9488.cpp
 *
 * Copyright 2023 mikee47 <mike@sillyhouse.net>
 *
 * This file is part of the Sming-Graphics Library
 *
 * This library is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation, version 3 or later.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this library.
 * If not, see <https://www.gnu.org/licenses/>.
 *
 ****/

#include <Graphics/Display/ILI9488.h>
#include <Platform/System.h>

namespace Graphics
{
namespace Display
{
namespace
{
//  Manufacturer Command Set (MCS)
#define ILI9488_IFMODECTL 0xB0
#define ILI9488_FRMCTR1 0xB1
#define ILI9488_FRMCTR2 0xB2
#define ILI9488_FRMCTR3 0xB3
#define ILI9488_INVCTR 0xB4
#define ILI9488_DFUNCTR 0xB6
#define ILI9488_EMSET 0xB7

#define ILI9488_PWCTR1 0xC0
#define ILI9488_PWCTR2 0xC1
#define ILI9488_PWCTR3 0xC2
#define ILI9488_PWCTR4 0xC3
#define ILI9488_PWCTR5 0xC4
#define ILI9488_VMCTR1 0xC5

#define ILI9488_NVMEMWR 0xD0
#define ILI9488_NVMEMPK 0xD1
#define ILI9488_NVMEMST 0xD2

#define ILI9488_RDID4 0xD3
#define ILI9488_RDID1 0xDA
#define ILI9488_RDID2 0xDB
#define ILI9488_RDID3 0xDC

#define ILI9488_PGAMCTRL 0xE0
#define ILI9488_NGAMCTRL 0xE1

#define ILI9488_ADJCTRL2 0xF2
#define ILI9488_ADJCTRL3 0xF7

using namespace Mipi;

// Command(1), length(2) data(length)
// Init values from https://github.com/Bodmer/TFT_eSPI/blob/master/TFT_Drivers/ILI9488_Init.h
DEFINE_RB_ARRAY(											//
	displayInitData,										//
	DEFINE_RB_COMMAND(DCS_SOFT_RESET, 0)					//
	DEFINE_RB_DELAY(5)										//
	DEFINE_RB_COMMAND(ILI9488_PWCTR1, 2, 0x17, 0x15)		//
	DEFINE_RB_COMMAND(ILI9488_PWCTR2, 1, 0x41)				//
	DEFINE_RB_COMMAND(ILI9488_VMCTR1, 3, 0x00, 0x12, 0x80)  //
	DEFINE_RB_COMMAND(DCS_SET_PIXEL_FORMAT, 1, 0x66)		// 18 bits per pixel (3 bytes)
	DEFINE_RB_COMMAND(ILI9488_FRMCTR1, 1, 0xA0)				// Frame Rate Control
	DEFINE_RB_COMMAND(ILI9488_DFUNCTR, 3, 0x02, 0x02, 0x3B) // Display Function Control
	DEFINE_RB_COMMAND_LONG(ILI9488_PGAMCTRL, 15, 0x00, 0x03, 0x09, 0x08, 0x16, 0x0A, 0x3F, 0x78, 0x4C, 0x09, 0x0A, 0x08,
						   0x16, 0x1A, 0x0F) // Positive Gamma Control
	DEFINE_RB_COMMAND_LONG(ILI9488_NGAMCTRL, 15, 0x00, 0x16, 0x19, 0x03, 0x0F, 0x05, 0x32, 0x45, 0x46, 0x04, 0x0E, 0x0D,
						   0x35, 0x37, 0x0F)					   // Negative Gamma Control
	DEFINE_RB_COMMAND(ILI9488_IFMODECTL, 1, 0x00)				   //
	DEFINE_RB_COMMAND(ILI9488_INVCTR, 1, 0x02)					   // Display Inversion Control
	DEFINE_RB_COMMAND(ILI9488_EMSET, 1, 0xC6)					   // Entry Mode Set
	DEFINE_RB_COMMAND(ILI9488_ADJCTRL3, 4, 0xA9, 0x51, 0x2C, 0x82) // Adjust Control 3
	DEFINE_RB_COMMAND(DCS_EXIT_SLEEP_MODE, 0)					   //
	DEFINE_RB_DELAY(120)										   //
	DEFINE_RB_COMMAND(DCS_SET_DISPLAY_ON, 0)					   //
)

} // namespace

bool ILI9488::initialise()
{
	sendInitData(displayInitData);
	return true;
}

uint16_t ILI9488::readNvMemStatus()
{
	return readRegister(ILI9488_NVMEMST, 3) >> 8;
}

} // namespace Display
} // namespace Graphics
