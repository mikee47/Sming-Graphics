/**
 * ILI9341.cpp
 *
 * Copyright 2021 mikee47 <mike@sillyhouse.net>
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
 * @author: May 2021 - mikee47 <mike@sillyhouse.net>
 *
 * This code contains some reworked code from the Adafruit_ILI9341 display driver.
 * 
 * See https://github.com/adafruit/Adafruit_ILI9341
 *
 ****/

#include <Graphics/Display/ILI9341.h>
#include <Platform/System.h>

namespace Graphics
{
namespace Display
{
namespace
{
//  Manufacturer Command Set (MCS)
#define ILI9341_FRMCTR1 0xB1
#define ILI9341_FRMCTR2 0xB2
#define ILI9341_FRMCTR3 0xB3
#define ILI9341_INVCTR 0xB4
#define ILI9341_DFUNCTR 0xB6

#define ILI9341_PWCTR1 0xC0
#define ILI9341_PWCTR2 0xC1
#define ILI9341_PWCTR3 0xC2
#define ILI9341_PWCTR4 0xC3
#define ILI9341_PWCTR5 0xC4
#define ILI9341_VMCTR1 0xC5
#define ILI9341_VMCTR2 0xC7

#define ILI9341_PWCTRA 0xCB
#define ILI9341_PWCTRB 0xCF

#define ILI9341_NVMEMWR 0xD0
#define ILI9341_NVMEMPK 0xD1
#define ILI9341_NVMEMST 0xD2

#define ILI9341_RDID4 0xD3
#define ILI9341_RDID1 0xDA
#define ILI9341_RDID2 0xDB
#define ILI9341_RDID3 0xDC

#define ILI9341_GMCTRP1 0xE0
#define ILI9341_GMCTRN1 0xE1

#define ILI9341_DRVTMA 0xE8
#define ILI9341_DRVTMB 0xEA
#define ILI9341_PWRSEQ 0xED

#define ILI9341_ENA3G 0xF2
#define ILI9341_IFCTL 0xF6
#define ILI9341_PMPRC 0xF7

using namespace Mipi;

// Command(1), length(2) data(length)
DEFINE_RB_ARRAY(													   //
	displayInitData,												   //
	DEFINE_RB_COMMAND(DCS_SOFT_RESET, 0)							   //
	DEFINE_RB_DELAY(5)												   //
	DEFINE_RB_COMMAND(ILI9341_PWCTRA, 5, 0x39, 0x2C, 0x00, 0x34, 0x02) //
	DEFINE_RB_COMMAND(ILI9341_PWCTRB, 3, 0x00, 0XC1, 0X30)			   //
	DEFINE_RB_COMMAND(ILI9341_DRVTMA, 3, 0x85, 0x00, 0x78)			   //
	DEFINE_RB_COMMAND(ILI9341_DRVTMB, 2, 0x00, 0x00)				   //
	DEFINE_RB_COMMAND(ILI9341_PWRSEQ, 4, 0x64, 0x03, 0X12, 0X81)	   //
	DEFINE_RB_COMMAND(ILI9341_PMPRC, 1, 0x20)						   //
	DEFINE_RB_COMMAND(ILI9341_PWCTR1, 1, 0x23)						   // Power control: VRH[5:0]
	DEFINE_RB_COMMAND(ILI9341_PWCTR2, 1, 0x10)						   // Power control: SAP[2:0], BT[3:0]
	DEFINE_RB_COMMAND(ILI9341_VMCTR1, 2, 0x3e, 0x28)				   // VCM control: Contrast
	DEFINE_RB_COMMAND(ILI9341_VMCTR2, 1, 0x86)						   // VCM control2
	DEFINE_RB_COMMAND(DCS_SET_PIXEL_FORMAT, 1, 0x55)				   // 16 bits per pixel
	DEFINE_RB_COMMAND(ILI9341_FRMCTR1, 2, 0x00, 0x18)				   //
	DEFINE_RB_COMMAND(ILI9341_DFUNCTR, 3, 0x08, 0x82, 0x27)			   // Display Function Control
	DEFINE_RB_COMMAND(ILI9341_ENA3G, 1, 0x00)						   // 3Gamma Function Disable
	DEFINE_RB_COMMAND(DCS_SET_GAMMA_CURVE, 1, 0x01)					   // Gamma curve selected
	DEFINE_RB_COMMAND_LONG(ILI9341_GMCTRP1, 15, 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03,
						   0x0E, 0x09, 0x00) // Set Gamma
	DEFINE_RB_COMMAND_LONG(ILI9341_GMCTRN1, 15, 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C,
						   0x31, 0x36, 0x0F)  // Set Gamma
	DEFINE_RB_COMMAND(DCS_EXIT_SLEEP_MODE, 0) //
	DEFINE_RB_DELAY(120)					  //
	DEFINE_RB_COMMAND(DCS_SET_DISPLAY_ON, 0)  //
	DEFINE_RB_DELAY(5)					      //
)

} // namespace

bool ILI9341::initialise()
{
	sendInitData(displayInitData);
	return true;
}

uint16_t ILI9341::readNvMemStatus()
{
	return readRegister(ILI9341_NVMEMST, 3) >> 8;
}

} // namespace Display
} // namespace Graphics
