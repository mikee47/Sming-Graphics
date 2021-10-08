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
#define ST7789V_RDID1   0xDA
#define ST7789V_RDID2   0xDB
#define ST7789V_RDID3   0xDC
#define ST7789V_RDID4   0xDD

#define ST7789V_PWCTR6 0xFC

#define ST7789V_GMCTRP1 0xE0
#define ST7789V_GMCTRN1 0xE1
#define ST7789V_PWCTRL2 0xE8
#define ST7789V_EQCTRL 0xE9

#define ST7789V_NVMEMST 0xFC

// MADCTL register bits
#define MADCTL_MY 0x80
#define MADCTL_MX 0x40
#define MADCTL_MV 0x20
#define MADCTL_ML 0x10
#define MADCTL_RGB 0x00
#define MADCTL_BGR 0x08
#define MADCTL_MH 0x04

// Command(1), length(2) data(length)
DEFINE_RB_ARRAY(													   //
	displayInitData,
	DEFINE_RB_COMMAND(Mipi::DCS_SOFT_RESET, 0)

	DEFINE_RB_DELAY(200)
	DEFINE_RB_COMMAND(Mipi::DCS_EXIT_SLEEP_MODE, 0)
	DEFINE_RB_DELAY(120) //Delay 120ms
	//------------------------------display and color format setting--------------------------------//
    DEFINE_RB_COMMAND(Mipi::DCS_SET_ADDRESS_MODE, 1, 0x00)
    DEFINE_RB_COMMAND(Mipi::DCS_SET_PIXEL_FORMAT, 1, Mipi::DCS_PIXEL_FMT_16BIT)
	DEFINE_RB_COMMAND(Mipi::DCS_ENTER_INVERT_MODE, 0)
	//--------------------------------ST7789V Frame rate setting------------------------------------//
	DEFINE_RB_COMMAND(ST7789V_FRMCTR2, 5, 0x05, 0x05, 0x00, 0x33, 0x33)
	DEFINE_RB_COMMAND(ST7789V_GCTRL, 1, 0x35)
	//---------------------------------ST7789V Power setting----------------------------------------//
	DEFINE_RB_COMMAND(ST7789V_GTADJ, 4, 0x2f, 0x2b, 0x2f, 0xBB)
	DEFINE_RB_COMMAND(Mipi::DCS_SET_PAGE_ADDRESS, 4, 0xc0, 0x2c, 0xc2, 0x01)
	DEFINE_RB_COMMAND(ST7789V_PWCTR4, 1, 0x0b)
	DEFINE_RB_COMMAND(ST7789V_PWCTR5, 1, 0x20)
	DEFINE_RB_COMMAND(ST7789V_FRCTRL2, 1, 0x11)
	DEFINE_RB_COMMAND(ST7789V_PWCTRL1, 2, 0xa4, 0xa1)
	DEFINE_RB_COMMAND(ST7789V_PWCTRL2, 1, 0x03)
	DEFINE_RB_COMMAND(ST7789V_EQCTRL, 3, 0x0d, 0x12, 0x00)
	//-------------------------------ST7789V gamma setting-------------------------------------------//
	DEFINE_RB_COMMAND_LONG(ST7789V_GMCTRP1, 14, 0xd0, 0x06, 0x0b, 0x0a, 0x09, 0x05, 0x2e, 0x43, 0x44, 0x09, 0x16, 0x15, 0x23, 0x27)

	DEFINE_RB_COMMAND_LONG(ST7789V_GMCTRN1, 14, 0xd0, 0x06, 0x0b, 0x09, 0x08, 0x06, 0x2e, 0x44, 0x44, 0x3a, 0x15 ,0x15, 0x23, 0x26)
	//-----------------Init RGB-Mode---------------
	DEFINE_RB_COMMAND(Mipi::DCS_SET_PIXEL_FORMAT, 1, 0x55) //RGB 65K Colors, Control interface 16bit/pixel

	DEFINE_RB_COMMAND(ST7789V_RAMCTRL, 2, 0x11, 0xE0) //RAM access control
					//RGB interface access RAM, Display operation RGB interface
					//16 Bit color format R7 -> R0, MSB first, 18 bit bus width,

	DEFINE_RB_COMMAND(ST7789V_FRMCTR1, 3, 0xEF, 0x08, 0x14) //RGB interfacecontrol
					//Direct RGB mode, RGB DE Mode, Control pins high active
					//VSYNC Back porch setting
					//HSYNC Back porch setting
	//--------------Display on-------------------
	DEFINE_RB_COMMAND(Mipi::DCS_EXIT_SLEEP_MODE, 0)
	DEFINE_RB_DELAY(120)
	DEFINE_RB_COMMAND(Mipi::DCS_SET_DISPLAY_ON, 0)
	DEFINE_RB_DELAY(100)
)

// Reading GRAM returns one byte per pixel for R/G/B (only top 6 bits are used, bottom 2 are clear)
static constexpr size_t READ_PIXEL_SIZE{3};

/**
 * @brief Manages completion of a `readDataBuffer` operation
 *
 * Performs format conversion, invokes callback (if provided) then releases shared buffer.
 *
 * @note Data is read back in RGB24 format, but written in RGB565.
 */
struct ReadPixelInfo {
	ReadBuffer buffer;
	size_t bytesToRead;
	ReadStatus* status;
	Surface::ReadCallback callback;
	void* param;

	static void IRAM_ATTR transferCallback(void* param)
	{
		System.queueCallback(taskCallback, param);
	}

	static void taskCallback(void* param)
	{
		auto info = static_cast<ReadPixelInfo*>(param);
		info->readComplete();
	}

	void readComplete()
	{
		if(buffer.format != PixelFormat::RGB24) {
			auto srcptr = &buffer.data[buffer.offset];
			auto dstptr = srcptr;
			if(buffer.format == PixelFormat::RGB565) {
				for(size_t i = 0; i < bytesToRead; i += READ_PIXEL_SIZE) {
					PixelBuffer buf;
					buf.rgb565.r = *srcptr++ >> 3;
					buf.rgb565.g = *srcptr++ >> 2;
					buf.rgb565.b = *srcptr++ >> 3;
					*dstptr++ = buf.u8[1];
					*dstptr++ = buf.u8[0];
				}
			} else {
				for(size_t i = 0; i < bytesToRead; i += READ_PIXEL_SIZE) {
					PixelBuffer buf;
					buf.rgb24.r = *srcptr++;
					buf.rgb24.g = *srcptr++;
					buf.rgb24.b = *srcptr++;
					dstptr += writeColor(dstptr, buf.color, buffer.format);
				}
			}
			bytesToRead = dstptr - &buffer.data[buffer.offset];
		}
		if(status != nullptr) {
			*status = ReadStatus{bytesToRead, buffer.format, true};
		}

		if(callback) {
			callback(buffer, bytesToRead, param);
		}

		buffer.data.release();
	}
};

} // namespace

class ST7789VSurface : public Mipi::Surface
{
public:
	using Mipi::Surface::Surface;

	/*
	 * The ST7789V is fussy when reading GRAM.
	 *
	 *  - Pixels are read in RGB24 format, but written in RGB565.
	 * 	- The RAMRD command resets the read position to the start of the address window
	 *    so is used only for the first packet
	 *  - Second and subsequent packets use the RAMRD_CONT command
	 *  - Pixels must not be split across SPI packets so each packet can be for a maximum of 63 bytes (21 pixels)
	 */
	int readDataBuffer(ReadBuffer& buffer, ReadStatus* status, ReadCallback callback, void* param) override
	{
		// ST7789V RAM read transactions must be in multiples of 3 bytes
		static constexpr size_t packetPixelBytes{63};

		auto pixelCount = (buffer.size() - buffer.offset) / READ_PIXEL_SIZE;
		if(pixelCount == 0) {
			debug_w("[readDataBuffer] pixelCount == 0");
			return 0;
		}
		auto& addrWindow = display.getAddressWindow();
		if(addrWindow.bounds.h == 0) {
			debug_w("[readDataBuffer] addrWindow.bounds.h == 0");
			return 0;
		}

		constexpr size_t hdrsize = DisplayList::codelen_readStart + DisplayList::codelen_read +
								   DisplayList::codelen_callback + sizeof(ReadPixelInfo);
		if(!displayList.require(hdrsize)) {
			debug_w("[readDataBuffer] no space");
			return -1;
		}
		if(!displayList.canLockBuffer()) {
			return -1;
		}
		if(buffer.format == PixelFormat::None) {
			buffer.format = PixelFormat::RGB24;
		}
		size_t maxPixels = (addrWindow.bounds.w * addrWindow.bounds.h) - addrWindow.column;
		pixelCount = std::min(maxPixels, pixelCount);
		ReadPixelInfo info{buffer, pixelCount * READ_PIXEL_SIZE, status, callback, param};
		if(status != nullptr) {
			*status = ReadStatus{};
		}

		auto bufptr = &buffer.data[buffer.offset];
		if(addrWindow.mode == AddressWindow::Mode::read) {
			displayList.readMem(bufptr, info.bytesToRead);
		} else {
			auto len = std::min(info.bytesToRead, packetPixelBytes);
			displayList.readMem(bufptr, len);
			if(len < info.bytesToRead) {
				displayList.readMem(bufptr + len, info.bytesToRead - len);
			}
		}
		addrWindow.seek(pixelCount);

		info.buffer.data.addRef();
		if(!displayList.writeCallback(info.transferCallback, &info, sizeof(info))) {
			debug_e("[DL] CALLBACK NO SPACE");
		}

		displayList.lockBuffer(buffer.data);
		return pixelCount;
	}

};

bool ST7789V::initialise()
{
	execute(Mipi::commands, displayInitData);
	return true;
}

bool ST7789V::setOrientation(Orientation orientation)
{
	auto setMadCtl = [&](uint8_t value) -> bool {
		SpiDisplayList list(Mipi::commands, addrWindow, 16);
		list.writeCommand(Mipi::DCS_SET_ADDRESS_MODE, value, 1);
		execute(list);
		this->orientation = orientation;
		return true;
	};

	switch(orientation) {
	case Orientation::deg0:
		return setMadCtl(MADCTL_MX | MADCTL_BGR);
	case Orientation::deg90:
		return setMadCtl(MADCTL_MV | MADCTL_BGR);
	case Orientation::deg180:
		return setMadCtl(MADCTL_MY | MADCTL_BGR);
	case Orientation::deg270:
		return setMadCtl(MADCTL_MX | MADCTL_MY | MADCTL_MV | MADCTL_BGR);
	default:
		return false;
	}
}

Surface* ST7789V::createSurface(size_t bufferSize)
{
	return new ST7789VSurface(*this, bufferSize ?: 512U);
}

uint16_t ST7789V::readNvMemStatus()
{
	return readRegister(ST7789V_NVMEMST, 3) >> 8;
}

} // namespace Display
} // namespace Graphics
