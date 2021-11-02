/****
 * DisplayList.h
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
 ****/

#pragma once

#include <HSPI/Request.h>
#include "AddressWindow.h"
#include "Buffer.h"
#include "Blend.h"
#include <FlashString/Array.hpp>
#include <memory>

#define DEFINE_RB_COMMAND(cmd, len, ...) uint8_t(uint8_t(DisplayList::Code::command) | (len << 4)), cmd, ##__VA_ARGS__,
#define DEFINE_RB_COMMAND_LONG(cmd, len, ...)                                                                          \
	uint8_t(uint8_t(DisplayList::Code::command) | 0xf0), len, cmd, ##__VA_ARGS__,
#define DEFINE_RB_DELAY(ms) uint8_t(DisplayList::Code::delay), ms,
#define DEFINE_RB_ARRAY(name, ...) DEFINE_FSTR_ARRAY_LOCAL(name, uint8_t, ##__VA_ARGS__)

namespace Graphics
{
/**
 * @brief DisplayList command codes
 * 
 * These represent the core display operations
 *
 * Command code, maximum (fixed) arg length, description
*/
#define GRAPHICS_DL_COMMAND_LIST(XX)                                                                                   \
	XX(none, 0, "")                                                                                                    \
	XX(command, 2 + 1, "General command: arglen, cmd, args")                                                           \
	XX(repeat, 1 + 2 + 2, "Repeated data block: WRITE, len, repeat, data")                                             \
	XX(setColumn, 2 + 2, "Set column: len, start")                                                                     \
	XX(setRow, 2 + 2, "Set row: len, start")                                                                           \
	XX(writeStart, 0, "Start writing command")                                                                         \
	XX(writeData, 2, "Write data: len, data")                                                                          \
	XX(writeDataBuffer, 1 + 2 + sizeof(void*), "Write data: cmd, len, dataptr")                                        \
	XX(readStart, 2 + sizeof(void*), "Read data: len, bufptr (first packet after setting address)")                    \
	XX(read, 2 + sizeof(void*), "Read data: len, bufptr (Second and subsequent packets)")                              \
	XX(callback, 2 + sizeof(void*) + 3, "Callback: paramlen, callback, ALIGN4, params")                                \
	XX(delay, 1, "Wait n milliseconds before continuing")

/**
 * @brief Supports DisplayList blend operations
 * @see See `DisplayList::fill`
 */
struct FillInfo {
	using Callback = void (*)(FillInfo& info);

	uint8_t* dstptr;
	PackedColor color;
	uint16_t length;

	static IRAM_ATTR void callbackRGB565(FillInfo& info)
	{
		BlendAlpha::blendRGB565(__builtin_bswap16(info.color.value), info.dstptr, info.length, info.color.alpha);
	}
};

__forceinline uint16_t swapBytes(uint16_t w)
{
	return (w >> 8) | (w << 8);
}

__forceinline uint32_t makeWord(uint16_t w1, uint16_t w2)
{
	return uint32_t(swapBytes(w1)) | (swapBytes(w2) << 16);
}

/**
 * @brief Stores list of low-level display commands
 *
 * Used by hardware surfaces to efficiently buffer commands which are then executed in interrupt context.
 *
 * The ILI9341 4-wire SPI mode is awkard to use so to allow more efficient access the command and data
 * information is buffered as various types of 'chunk' using this class.
 * A single HSPI request packet is used for all requests and is re-filled in interrupt context from this list.
 * 
 * Count values (data length) are stored as either 1 or 2 bytes.
 * Values less than 0x80 bytes are stored in 1 byte, values from 0x0080 to 0x7fff are stored MSB first
 * with the top bit set, followed by the LSB. For example, 0x1234 would be stored as 0x92 0x34.
 * 
 * Commands are stored in 1 byte. To allow use of this class with other displays may require adjusting this,
 * perhaps converting this into a class template to accommodate these differences more efficiently.
 *
 *
 * - **Standard chunk**.
 * 		Contains a display-specific command byte followed by optional data.
 * 
 *		- command (1 byte)
 *		- data length (1 or 2 bytes)
 *		- data (variable length)
 *
 *
 * - **Data chunk** (no command).
 * 
 *		- COMMAND_DATA (1 byte)
 *		- data length (1 or 2 bytes)
 *		- data (variable length) 
 *
 *
 * - **Repeated data chunk** (no command).
 * 	 Used to perform colour fills.
 * 
 *		- COMMAND_DATA_REPEAT (1 byte)
 *		- data length (1 or 2 bytes)
 *		- repeat count (1 or 2 bytes)
 *		- data (varible length)
 * 
 *
 * - **Read command**.
 *   Defines a single read command packet. Reading is particularly awkward on the ILI9341 as it 'forgets' the read
 *   position after each read packet. Reading a block of display memory therefore requires the address window to be
 *   set immediately before the RAMRD instruction. The final block in the sequence is a CALLBACK command.
 *
 *		- COMMAND_READ (1 byte)
 *		- data length (1 or 2 bytes)
 *		- command (1 byte) The display-specific command to issue
 *		- buffer address (4 bytes)
 *
 * - **Callback**.
 *   Invokes a callback function (see Callback type). Note that the single parameter points to a **copy**
 *   of the original data which is stored in the list.
 *
 *		- COMMAND_CALLBACK (1 byte)
 *		- parameter data length (1 or 2 bytes)
 *		- callback function pointer (4 bytes)
 *		- callback parameters (variable)
 *
 */
class DisplayList
{
public:
	enum class Code : uint8_t {
#define XX(code, arglen, desc) code,
		GRAPHICS_DL_COMMAND_LIST(XX)
#undef XX
	};

	/**
	 * @brief Obtain maximum size for command, not including variable data which may be added
	 *
	 * Used to check space before starting a command sequence. Actual space used may be less than this.
	 */
	enum CodeArgLengths : uint8_t {
#define XX(code, arglen, desc) codelen_##code = 1 + arglen,
		GRAPHICS_DL_COMMAND_LIST(XX)
#undef XX
	};

	static String toString(Code code);

	/**
	 * @brief Each list entry starts with a header
	 * 
	 * This information is interpreted by the display driver but should translate to a single
	 * display command.
	 * 
	 * Each command has fixed content with optional variable data.
	 * Length can be packed with command (0-31), or an additional uint8_t or uint16_t.
	 * 
	 * setPixel(uint8_t x, uint8_t y, void* data, uint8_t len)
	 * 		COL(uint8_t x, uint8_t len)
	 * 		ROW(uint8_t y, 1)
	 *      DAT data, len
	 *
	 * setPixel(uint16_t x, uint8_t y, void* data, uint16_t len)
	 * 		COL(uint16_t x, uint16_t len)
	 * 		ROW(uint8_t y, 1)
	 *      DAT data, len
	 *
	 * fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, data len)
	 * 		COL16 x, w
	 * 		ROW16 y, h
	 *
	 * 
	 */
	union Header {
		// Maximum value for header len field. 31 means 'length follows header'
		static constexpr uint8_t lenMax{15};

		struct {
			Code code : 4;
			uint8_t len : 4; // Set to lenMax if length follows buffer
		};
		uint8_t u8;
	};

	static_assert(sizeof(Header) == 1, "Header wrong size");

	/**
	 * @brief Queued callback
	 * @param parameterData A copy of the original parameter data
	 * 
	 * The parameter data is copied so the caller does not need to allocate memory dynamically.
	 * The data must start on a 32-bit word boundary.
	 */
	using Callback = void (*)(void* parameterData);

	/**
	 * @brief Values returned from `readEntry`
	 */
	struct Entry {
		Code code;
		uint16_t length;
		uint16_t repeats;
		void* data;
		Callback callback;
		uint16_t value;
		uint8_t cmd;
	};

	/**
	 * @param commands Codes corresponding to specific display commands
	 * @param addrWindow Reference to current device address window, synced between lists
	 * @param bufferSize
	 */
	DisplayList(AddressWindow& addrWindow, size_t bufferSize) : addrWindow(addrWindow), capacity(bufferSize)
	{
		buffer.reset(new uint8_t[bufferSize]{});
	}

	/**
	 * @brief Create pre-defined display list from flash data
	 *
	 * Used for initialisation data
	 */
	DisplayList(AddressWindow& addrWindow, const FSTR::ObjectBase& data) : DisplayList(addrWindow, data.size())
	{
		data.read(0, buffer.get(), data.size());
		size = data.length();
	}

	/**
	 * @brief Create initialised display list from RAM data
	 *
	 * Used for initialisation data
	 */
	DisplayList(AddressWindow& addrWindow, const void* data, size_t length) : DisplayList(addrWindow, length)
	{
		memcpy(buffer.get(), data, length);
		size = length;
	}

	/**
	 * @brief Reset the display list ready for re-use
	 * List MUST NOT be in use!
	 */
	void reset();

	/**
	 * @brief Determine if any commands have been stored for execution
	 */
	bool isEmpty() const
	{
		return size == 0;
	}

	/**
	 * @brief Get number of bytes remaining in buffer
	 */
	uint16_t freeSpace() const
	{
		return capacity - size;
	}

	/**
	 * @brief Get current read position
	 */
	uint16_t readOffset() const
	{
		return offset;
	}

	/**
	 * @brief Get number of bytes stored in buffer
	 */
	uint16_t used() const
	{
		assert(size <= capacity);
		return size;
	}

	/**
	 * @brief Get read-only pointer to start of buffer
	 */
	const uint8_t* getContent() const
	{
		return buffer.get();
	}

	/**
	 * @brief Get some space in the list to write pixel data
	 * @param available (OUT) How much space is available
	 * @retval uint8_t* Where to write data
	 *
	 * Call `commit` after the data has been written
	 */
	uint8_t* getBuffer(uint16_t& available);

	/**
	 * @brief Get some space in the list to write pixel data
	 * @param minBytes Minimum bytes required
	 * @param available (OUT) How much space is available
	 * @retval uint8_t* Where to write data, nullptr if required space not available
	 *
	 * Call `commit` after the data has been written
	 */
	uint8_t* getBuffer(uint16_t minBytes, uint16_t& available)
	{
		auto buf = getBuffer(available);
		return (buf != nullptr && available >= minBytes) ? buf : nullptr;
	}

	/**
	 * @brief Commit block of data to the list
	 * @param length How many bytes have been written
	 * @note MUST NOT call with more data than returned from `available` in getBuffer call.
	 */
	void commit(uint16_t length);

	/**
	 * @brief Write command with 1-4 bytes of parameter data
	 */
	bool writeCommand(uint8_t command, uint32_t data, uint8_t length)
	{
		return writeCommand(command, &data, length);
	}

	/**
	 * @brief Write command with variable amount of parameter data
	 */
	bool writeCommand(uint8_t command, const void* data, uint16_t length);

	/**
	 * @brief Add WRITE command plus data
	 */
	bool writeData(const void* data, uint16_t length);

	/**
	 * @brief Add WRITE command plus external data
	 * @param data Will be locked until display list has been executed
	 */
	bool writeDataBuffer(SharedBuffer& data, size_t offset, uint16_t length);

	/**
	 * @brief Perform a block fill operation with repeat, e.g. multiple pixel fill or repeated pattern
	 */
	bool blockFill(const void* data, uint16_t length, uint32_t repeat);

	/**
	 * @brief Set window for read/write operations
	 */
	bool setAddrWindow(const Rect& rect);

	/**
	 * @brief Set a single pixel
	 */
	bool setPixel(PackedColor color, uint8_t bytesPerPixel, Point pt);

	/**
	 * @brief Read a block of display memory
	 */
	bool readMem(void* buffer, uint16_t length);

	/**
	 * @brief Request a callback
	 * @param callback Callback function to invoke
	 * @param params Parameter data, will be stored in list
	 * @param paramLength Size of parameter data
	 *
	 * The callback will be invoked (in interrupt context) when read from the display list
	 */
	bool writeCallback(Callback callback, void* params, uint16_t paramLength);

	/**
	 * @brief Perform a block fill operation with blending
	 * @param rect Area to fill
	 * @param color
	 * @param bytesPerPixel
	 * @param callback Invoked in interrupt context to perform blend operation
	 *
	 * Performs a read/blend/write operation.
	 * Use to perform transparent fills or other blend operations on small regions of display memory.
	 */
	bool fill(const Rect& rect, PackedColor color, uint8_t bytesPerPixel, FillInfo::Callback callback);

	/**
	 * @brief Enforce maximum number of locked buffers to conserve memory.
	 * @retval bool true if call to `lockBuffer()` will succeed
	 */
	bool canLockBuffer()
	{
		return lockCount < maxLockedBuffers;
	}

	/**
	 * @brief Lock a shared buffer by storing a reference to it. This will be released when `reset()` is called.
	 */
	bool lockBuffer(SharedBuffer& buffer);

	/**
	 * @brief Check if list has space for the given number of bytes
	 * @retval bool true if there's room
	 */
	bool require(uint16_t length)
	{
		return size + length <= capacity;
	}

	/**
	 * @brief Read next command list entry
	 * @param info
	 * @retval bool false if there are no more commands
	 *
	 * Used by virtual display and for debugging.
	 * `SpiDisplayList` uses a different mechanism for reading.
	 */
	bool readEntry(Entry& info);

	/**
	 * @brief Prepare for playback
	 * @param callback Invoked when playback has completed, in task context
	 * @param param Parameter passed to callback
	 */
	void prepare(Callback callback, void* param)
	{
		this->callback = callback;
		this->param = param;
		offset = 0;
	}

protected:
	/**
	 * @brief Write a byte into the display list buffer
	 */
	void write(uint8_t c)
	{
		buffer[size++] = c;
	}

	/**
	 * @brief Write a Header structure to the buffer
	 * @param code Command code to store
	 * @param length Length to store in header
	 */
	void writeHeader(Code code, uint16_t length)
	{
		// debug_i("%p writeHeader(%s, %u)", this, toString(code).c_str(), length);
		if(length < Header::lenMax) {
			Header hdr{code, uint8_t(length)};
			write(hdr.u8);
		} else {
			Header hdr{code, Header::lenMax};
			write(hdr.u8);
			writeVar(length);
		}
	}

	/**
	 * @brief Write a value as 2 bytes
	 */
	void write16(uint16_t c)
	{
		write((c >> 8) | 0x80);
		write(c & 0xff);
	}

	/**
	 * @brief Write a block of data into the display list buffer
	 */
	void write(const void* data, uint16_t length)
	{
		if(length != 0) {
			memcpy(&buffer[size], data, length);
			size += length;
		}
	}

	/**
	 * @brief Write a value using 1 or 2 bytes as required
	 */
	void writeVar(uint16_t count)
	{
		if(count >= 0x80) {
			write16(count);
		} else {
			write(count);
		}
	}

	/**
	 * @brief Get the appropriate command code to use for a write operation
	 * 
	 * Code depends on whether this is the first write packet or not
	 */
	Code getWriteCode()
	{
		return addrWindow.setMode(AddressWindow::Mode::write) ? Code::writeStart : Code::writeData;
	}

	/**
	 * @brief Get the appropriate command code to use for a read operation
	 * 
	 * Code depends on whether this is the first read packet or not
	 */
	Code getReadCode()
	{
		return addrWindow.setMode(AddressWindow::Mode::read) ? Code::readStart : Code::read;
	}

	void internalSetAddrWindow(const Rect& rect);

	/**
	 * @brief Read block of data from buffer
	 * 
	 * Called during playback in interrupt context
	 */
	void IRAM_ATTR read(void* data, uint16_t len)
	{
		memcpy(data, &buffer[offset], len);
		offset += len;
	}

	/**
	 * @brief Read variable-length value from buffer
	 * 
	 * Called during playback in interrupt context
	 */
	uint16_t IRAM_ATTR readVar()
	{
		uint16_t var = buffer[offset++];
		if(var & 0x80) {
			var = (var & 0x7f) << 8;
			var |= buffer[offset++];
		}
		return var;
	}

	///< Limit number of locked buffers
	static constexpr size_t maxLockedBuffers{8};

	Callback callback{nullptr};
	void* param{nullptr};
	std::unique_ptr<uint8_t[]> buffer;
	uint16_t size{0};   ///< Number of bytes stored in buffer
	uint16_t offset{0}; ///< Current read position

private:
	AddressWindow& addrWindow;
	uint16_t capacity;
	SharedBuffer lockedBuffers[maxLockedBuffers];
#ifdef ENABLE_GRAPHICS_RAM_TRACKING
	size_t maxBufferUsage{0};
#endif
	uint8_t lockCount{0};
};

} // namespace Graphics

inline String toString(Graphics::DisplayList::Code code)
{
	return Graphics::DisplayList::toString(code);
}
