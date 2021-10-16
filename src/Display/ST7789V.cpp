/**
 * ST7789V.cpp
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
 * @author: October 2021 - Slavey Karadzhov <slav@attachix.com>
 *
 ****/

#include <Graphics/Display/ST7789V.h>
#include <Platform/System.h>

namespace Graphics
{
namespace Display
{
namespace
{
//  Manufacturer Command Set (MCS)

#define ST7789V_WRDISBV 0x51 // Write Display Brightness
#define ST7789V_WRCTRLD 0x53

#define ST7789V_RAMCTRL 0xB0
#define ST7789V_FRMCTR1 0xB1
#define ST7789V_FRMCTR2 0xB2
#define ST7789V_FRMCTR3 0xB3
#define ST7789V_INVCTR 0xB4
#define ST7789V_DISSET5 0xB6
#define ST7789V_GCTRL 0xB7
#define ST7789V_GTADJ 0xB8
#define ST7789V_VCOMS 0xBB

#define ST7789V_PWCTR1 0xC0
#define ST7789V_PWCTR2 0xC1
#define ST7789V_PWCTR3 0xC2
#define ST7789V_PWCTR4 0xC3
#define ST7789V_PWCTR5 0xC4
#define ST7789V_VMCTR1 0xC5
#define ST7789V_FRCTRL2 0xC6
#define ST7789V_CABCCTRL 0xC7

#define ST7789V_PWCTRL1 0xD0
#define ST7789V_RDID1 0xDA
#define ST7789V_RDID2 0xDB
#define ST7789V_RDID3 0xDC
#define ST7789V_RDID4 0xDD

#define ST7789V_PWCTR6 0xFC

#define ST7789V_GMCTRP1 0xE0
#define ST7789V_GMCTRN1 0xE1
#define ST7789V_PWCTRL2 0xE8
#define ST7789V_EQCTRL 0xE9

#define ST7789V_NVMEMST 0xFC

using namespace Mipi;

// Command(1), length(2) data(length)
DEFINE_RB_ARRAY(								//
	displayInitData,							//
	DEFINE_RB_COMMAND(DCS_EXIT_SLEEP_MODE, 0)   //
	DEFINE_RB_DELAY(120)						//
	DEFINE_RB_COMMAND(DCS_SET_DISPLAY_ON, 0)	//
	DEFINE_RB_COMMAND(DCS_ENTER_NORMAL_MODE, 0) // Normal display mode on
	//	DEFINE_RB_COMMAND(0xB6, 2, 0x0A, 0x82)
	DEFINE_RB_COMMAND(ST7789V_RAMCTRL, 2, 0x00, 0xE0) // 5 to 6 bit conversion: r0 = r5, b0 = b5
	DEFINE_RB_COMMAND(DCS_SET_PIXEL_FORMAT, 1, 0x55)  // 16 bits per pixel
	DEFINE_RB_DELAY(10)
	// ST7789V Frame rate setting
	DEFINE_RB_COMMAND(ST7789V_FRMCTR2, 5, 0x0c, 0x0c, 0x00, 0x33, 0x33)
	// Power Settings
	DEFINE_RB_COMMAND(ST7789V_GCTRL, 1, 0x35)		  // Voltages: VGH / VGL
	DEFINE_RB_COMMAND(ST7789V_VCOMS, 1, 0x28)		  // ST7789V Power setting
	DEFINE_RB_COMMAND(ST7789V_PWCTR1, 1, 0x0C)		  //
	DEFINE_RB_COMMAND(ST7789V_PWCTR3, 1, 0x10)		  // voltage VRHS
	DEFINE_RB_COMMAND(ST7789V_PWCTR5, 1, 0x20)		  //
	DEFINE_RB_COMMAND(ST7789V_FRCTRL2, 1, 0x0f)		  //
	DEFINE_RB_COMMAND(ST7789V_PWCTRL1, 2, 0xa4, 0xa1) //
	// ST7789V gamma setting
	DEFINE_RB_COMMAND_LONG(ST7789V_GMCTRP1, 14, 0xd0, 0x00, 0x02, 0x07, 0x0a, 0x28, 0x32, 0x44, 0x42, 0x06, 0x0e, 0x12,
						   0x14, 0x17) //
	DEFINE_RB_COMMAND_LONG(ST7789V_GMCTRN1, 14, 0xd0, 0x00, 0x02, 0x07, 0x0a, 0x28, 0x31, 0x54, 0x47, 0x0e, 0x1c, 0x17,
						   0x1b, 0x1e)			//
	DEFINE_RB_COMMAND(DCS_ENTER_INVERT_MODE, 0) //

	DEFINE_RB_COMMAND(DCS_SET_DISPLAY_ON, 0) //
	DEFINE_RB_DELAY(120)					 //
)

} // namespace

bool ST7789V::initialise()
{
	sendInitData(displayInitData);
	return true;
}

uint16_t ST7789V::readNvMemStatus()
{
	return readRegister(ST7789V_NVMEMST, 3) >> 8;
}

} // namespace Display
} // namespace Graphics
