#pragma once

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <vector>

#include <SDL.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "SexyAppBase.h"
#include "CursorPointer.cur.inc"
#include "CursorHand.cur.inc"

#ifdef __EMSCRIPTEN__
EM_JS(double, PvZGetBrowserCanvasScale, (), {
	var canvas = Module.canvas || document.getElementById('canvas');
	if (!canvas) return 1;

	var rect = canvas.getBoundingClientRect();
	var scaleX = rect.width > 0 && canvas.width > 0 ? rect.width / canvas.width : 1;
	var scaleY = rect.height > 0 && canvas.height > 0 ? rect.height / canvas.height : 1;
	return Math.max(1, Math.min(scaleX, scaleY));
});

EM_JS(void, PvZEnsureCanvasBrowserCursorEvents, (), {
	if (Module.pvzCanvasCursorEventsReady) return;

	var canvas = Module.canvas || document.getElementById('canvas');
	if (!canvas) return;

	Module.pvzRestoreCanvasCursor = function() {
		var canvas = Module.canvas || document.getElementById('canvas');
		var cursorStyle = Module.pvzCanvasCursorStyle;
		if (!canvas || !cursorStyle || canvas.style.cursor === cursorStyle) return;
		canvas.style.cursor = cursorStyle;
	};

	var restoreSoon = function() {
		if (Module.pvzCanvasCursorRestorePending) return;
		Module.pvzCanvasCursorRestorePending = true;
		setTimeout(function() {
			Module.pvzCanvasCursorRestorePending = false;
			Module.pvzRestoreCanvasCursor();
		}, 0);
	};

	canvas.addEventListener('mouseenter', restoreSoon);
	canvas.addEventListener('mousemove', function() {
		if (Module.pvzCanvasCursorStyle && canvas.style.cursor !== Module.pvzCanvasCursorStyle) {
			restoreSoon();
		}
	});

	Module.pvzCanvasCursorEventsReady = true;
});

EM_JS(void, PvZSetCanvasBrowserCursorStyle, (const char* theCursorStyle), {
	var canvas = Module.canvas || document.getElementById('canvas');
	if (!canvas) return;

	var cursorStyle = UTF8ToString(theCursorStyle);
	if (Module.pvzCanvasCursorStyle === cursorStyle && canvas.style.cursor === cursorStyle) return;

	canvas.style.cursor = cursorStyle;
	Module.pvzCanvasCursorStyle = cursorStyle;
});

EM_JS(void, PvZSetCanvasBrowserCursorKind, (int theKind), {
	var cursorStyles = Module.pvzCursorStyles;
	if (!cursorStyles || !cursorStyles[theKind]) return;

	var canvas = Module.canvas || document.getElementById('canvas');
	if (!canvas) return;

	var cursorStyle = cursorStyles[theKind];
	if (Module.pvzCanvasCursorStyle === cursorStyle && canvas.style.cursor === cursorStyle) return;

	canvas.style.cursor = cursorStyle;
	Module.pvzCanvasCursorStyle = cursorStyle;
});

EM_JS(void, PvZSetCanvasBrowserCursorPixels, (int theKind, const uint32_t* thePixels, int theWidth, int theHeight, int theHotX, int theHotY), {
	var canvas = Module.canvas || document.getElementById('canvas');
	if (!canvas) return;

	var scratch = Module.pvzCursorCanvas;
	if (!scratch) {
		scratch = document.createElement('canvas');
		Module.pvzCursorCanvas = scratch;
	}

	scratch.width = theWidth;
	scratch.height = theHeight;
	var ctx = scratch.getContext('2d');
	var imageData = ctx.createImageData(theWidth, theHeight);
	var src = thePixels >>> 0;
	for (var i = 0, j = 0; i < theWidth * theHeight; i++, j += 4) {
		var p = src + i * 4;
		imageData.data[j + 0] = HEAPU8[p + 2];
		imageData.data[j + 1] = HEAPU8[p + 1];
		imageData.data[j + 2] = HEAPU8[p + 0];
		imageData.data[j + 3] = HEAPU8[p + 3];
	}

	ctx.putImageData(imageData, 0, 0);
	var cursorStyle = 'url("' + scratch.toDataURL('image/png') + '") ' + theHotX + ' ' + theHotY + ', auto';
	if (!Module.pvzCursorStyles) {
		Module.pvzCursorStyles = [];
	}

	Module.pvzCursorStyles[theKind] = cursorStyle;
	if (Module.pvzCanvasCursorStyle === cursorStyle && canvas.style.cursor === cursorStyle) return;

	canvas.style.cursor = cursorStyle;
	Module.pvzCanvasCursorStyle = cursorStyle;
});
#endif

namespace Sexy
{

enum class PvZCursorKind
{
	Pointer,
	Hand
};

struct PvZCursorBitmap
{
	std::vector<uint32_t> mPixels;
	int mWidth = 0;
	int mHeight = 0;
	int mHotX = 0;
	int mHotY = 0;
};

static inline uint16_t PvZReadLE16(const unsigned char* thePtr)
{
	return static_cast<uint16_t>(thePtr[0]) | (static_cast<uint16_t>(thePtr[1]) << 8);
}

static inline uint32_t PvZReadLE32(const unsigned char* thePtr)
{
	return static_cast<uint32_t>(thePtr[0]) |
		(static_cast<uint32_t>(thePtr[1]) << 8) |
		(static_cast<uint32_t>(thePtr[2]) << 16) |
		(static_cast<uint32_t>(thePtr[3]) << 24);
}

static inline int PvZCursorStride(int theWidth, int theBitsPerPixel)
{
	return ((theWidth * theBitsPerPixel + 31) / 32) * 4;
}

static inline bool PvZDecodeCursorDib(const unsigned char* theDib, size_t theDibSize, int theHotX, int theHotY, PvZCursorBitmap& theBitmap)
{
	if (theDibSize < 40)
		return false;

	const uint32_t aHeaderSize = PvZReadLE32(theDib);
	const int aDibWidth = static_cast<int32_t>(PvZReadLE32(theDib + 4));
	const int aDibHeightRaw = static_cast<int32_t>(PvZReadLE32(theDib + 8));
	const uint16_t aBitsPerPixel = PvZReadLE16(theDib + 14);
	if (aHeaderSize < 40 || aHeaderSize > theDibSize || aDibWidth <= 0 || aDibHeightRaw == 0 ||
		PvZReadLE16(theDib + 12) != 1 || PvZReadLE32(theDib + 16) != 0)
		return false;

	const bool aTopDown = aDibHeightRaw < 0;
	const int aWidth = aDibWidth;
	const int aHeight = std::abs(aDibHeightRaw) / 2;
	const uint32_t aColorsUsed = PvZReadLE32(theDib + 32);
	const int aPaletteSize = aBitsPerPixel <= 8 ? (aColorsUsed != 0 ? static_cast<int>(aColorsUsed) : (1 << aBitsPerPixel)) : 0;
	const size_t aPaletteOffset = aHeaderSize;
	const size_t anXorOffset = aPaletteOffset + static_cast<size_t>(aPaletteSize) * 4;
	const int anXorStride = PvZCursorStride(aWidth, aBitsPerPixel);
	const int anAndStride = PvZCursorStride(aWidth, 1);
	const size_t anAndOffset = anXorOffset + static_cast<size_t>(anXorStride) * aHeight;
	if (aWidth <= 0 || aHeight <= 0 || anAndOffset + static_cast<size_t>(anAndStride) * aHeight > theDibSize)
		return false;

	theBitmap.mWidth = aWidth;
	theBitmap.mHeight = aHeight;
	theBitmap.mHotX = theHotX;
	theBitmap.mHotY = theHotY;
	theBitmap.mPixels.assign(static_cast<size_t>(aWidth) * aHeight, 0);

	for (int y = 0; y < aHeight; y++)
	{
		const int aSourceY = aTopDown ? y : (aHeight - 1 - y);
		const unsigned char* anXorRow = theDib + anXorOffset + static_cast<size_t>(aSourceY) * anXorStride;
		const unsigned char* anAndRow = theDib + anAndOffset + static_cast<size_t>(aSourceY) * anAndStride;
		for (int x = 0; x < aWidth; x++)
		{
			uint8_t aRed = 0, aGreen = 0, aBlue = 0, anAlpha = 255;
			if (aBitsPerPixel <= 8)
			{
				int aPaletteIndex = 0;
				if (aBitsPerPixel == 1)
					aPaletteIndex = (anXorRow[x / 8] >> (7 - (x % 8))) & 1;
				else if (aBitsPerPixel == 4)
					aPaletteIndex = (x % 2 == 0) ? ((anXorRow[x / 2] >> 4) & 0xF) : (anXorRow[x / 2] & 0xF);
				else if (aBitsPerPixel == 8)
					aPaletteIndex = anXorRow[x];
				else
					return false;

				if (aPaletteIndex >= aPaletteSize)
					return false;
				const unsigned char* aColor = theDib + aPaletteOffset + static_cast<size_t>(aPaletteIndex) * 4;
				aBlue = aColor[0];
				aGreen = aColor[1];
				aRed = aColor[2];
			}
			else if (aBitsPerPixel == 24)
			{
				const unsigned char* aColor = anXorRow + x * 3;
				aBlue = aColor[0];
				aGreen = aColor[1];
				aRed = aColor[2];
			}
			else if (aBitsPerPixel == 32)
			{
				const unsigned char* aColor = anXorRow + x * 4;
				aBlue = aColor[0];
				aGreen = aColor[1];
				aRed = aColor[2];
				anAlpha = aColor[3];
			}
			else
			{
				return false;
			}

			if (((anAndRow[x / 8] >> (7 - (x % 8))) & 1) != 0 && aBitsPerPixel != 32)
				anAlpha = 0;

			theBitmap.mPixels[static_cast<size_t>(y) * aWidth + x] =
				(static_cast<uint32_t>(anAlpha) << 24) |
				(static_cast<uint32_t>(aRed) << 16) |
				(static_cast<uint32_t>(aGreen) << 8) |
				static_cast<uint32_t>(aBlue);
		}
	}

	return true;
}

static inline bool PvZDecodeCursor(const unsigned char* theData, size_t theDataSize, int theTargetWidth, PvZCursorBitmap& theBitmap)
{
	if (theDataSize < 22 || PvZReadLE16(theData) != 0 || PvZReadLE16(theData + 2) != 2)
		return false;

	const int aCount = PvZReadLE16(theData + 4);
	if (aCount < 1 || theDataSize < 6 + static_cast<size_t>(aCount) * 16)
		return false;

	int aBestIndex = 0;
	int aBestWidth = 0;
	int aBestScore = 0x7FFFFFFF;
	for (int i = 0; i < aCount; i++)
	{
		const unsigned char* anEntry = theData + 6 + static_cast<size_t>(i) * 16;
		const int aWidth = anEntry[0] == 0 ? 256 : anEntry[0];
		const int aScore = std::abs(aWidth - theTargetWidth);
		if (aScore < aBestScore || (aScore == aBestScore && aWidth > aBestWidth))
		{
			aBestIndex = i;
			aBestWidth = aWidth;
			aBestScore = aScore;
		}
	}

	const unsigned char* anEntry = theData + 6 + static_cast<size_t>(aBestIndex) * 16;
	const uint32_t anImageSize = PvZReadLE32(anEntry + 8);
	const uint32_t anImageOffset = PvZReadLE32(anEntry + 12);
	if (anImageOffset >= theDataSize || anImageSize > theDataSize - anImageOffset)
		return false;

	return PvZDecodeCursorDib(
		theData + anImageOffset,
		anImageSize,
		PvZReadLE16(anEntry + 4),
		PvZReadLE16(anEntry + 6),
		theBitmap);
}

static inline bool PvZScaleCursorBitmap(const PvZCursorBitmap& theSource, int theScalePercent, PvZCursorBitmap& theDest)
{
	if (theSource.mPixels.empty() || theSource.mWidth <= 0 || theSource.mHeight <= 0)
		return false;

	theDest.mWidth = std::max(1, (theSource.mWidth * theScalePercent + 50) / 100);
	theDest.mHeight = std::max(1, (theSource.mHeight * theScalePercent + 50) / 100);
	theDest.mHotX = std::max(0, std::min(theDest.mWidth - 1, (theSource.mHotX * theScalePercent + 50) / 100));
	theDest.mHotY = std::max(0, std::min(theDest.mHeight - 1, (theSource.mHotY * theScalePercent + 50) / 100));
	theDest.mPixels.resize(static_cast<size_t>(theDest.mWidth) * theDest.mHeight);

	for (int y = 0; y < theDest.mHeight; y++)
	{
		const int aSourceY = std::min(theSource.mHeight - 1, y * theSource.mHeight / theDest.mHeight);
		for (int x = 0; x < theDest.mWidth; x++)
		{
			const int aSourceX = std::min(theSource.mWidth - 1, x * theSource.mWidth / theDest.mWidth);
			theDest.mPixels[static_cast<size_t>(y) * theDest.mWidth + x] =
				theSource.mPixels[static_cast<size_t>(aSourceY) * theSource.mWidth + aSourceX];
		}
	}

	return true;
}

static inline bool PvZBuildCursorBitmap(PvZCursorKind theKind, int theScalePercent, PvZCursorBitmap& theBitmap)
{
	PvZCursorBitmap aSource;
	const unsigned char* aData = theKind == PvZCursorKind::Pointer ? kPvZPointerCursorCur : kPvZHandCursorCur;
	const size_t aDataSize = theKind == PvZCursorKind::Pointer ? kPvZPointerCursorCurLen : kPvZHandCursorCurLen;
	if (!PvZDecodeCursor(aData, aDataSize, 32, aSource))
		return false;

	return PvZScaleCursorBitmap(aSource, theScalePercent, theBitmap);
}

static inline SDL_Cursor* CreatePvZColorCursor(PvZCursorKind theKind, int theScalePercent)
{
	PvZCursorBitmap aBitmap;
	if (!PvZBuildCursorBitmap(theKind, theScalePercent, aBitmap))
		return nullptr;

	SDL_Surface* aSurface = SDL_CreateRGBSurfaceWithFormatFrom(
		aBitmap.mPixels.data(),
		aBitmap.mWidth,
		aBitmap.mHeight,
		32,
		aBitmap.mWidth * static_cast<int>(sizeof(uint32_t)),
		SDL_PIXELFORMAT_ARGB8888);
	if (aSurface == nullptr)
		return nullptr;

	SDL_Cursor* aCursor = SDL_CreateColorCursor(aSurface, aBitmap.mHotX, aBitmap.mHotY);
	SDL_FreeSurface(aSurface);
	return aCursor;
}

#ifdef __EMSCRIPTEN__
static inline bool ApplyPvZBrowserCursor(PvZCursorKind theKind, int theScalePercent)
{
	PvZCursorBitmap aBitmap;
	if (!PvZBuildCursorBitmap(theKind, theScalePercent, aBitmap))
		return false;

	PvZEnsureCanvasBrowserCursorEvents();
	PvZSetCanvasBrowserCursorPixels(static_cast<int>(theKind), aBitmap.mPixels.data(), aBitmap.mWidth, aBitmap.mHeight, aBitmap.mHotX, aBitmap.mHotY);
	return true;
}

static inline void ApplyPvZBrowserCursorKind(PvZCursorKind theKind)
{
	PvZEnsureCanvasBrowserCursorEvents();
	PvZSetCanvasBrowserCursorKind(static_cast<int>(theKind));
}

static inline void HidePvZBrowserCursor()
{
	PvZEnsureCanvasBrowserCursorEvents();
	PvZSetCanvasBrowserCursorStyle("none");
}

static inline void ApplyPvZBrowserSystemCursor(int theCursorNum)
{
	PvZEnsureCanvasBrowserCursorEvents();
	switch (theCursorNum)
	{
	case CURSOR_TEXT:
		PvZSetCanvasBrowserCursorStyle("text");
		break;
	case CURSOR_CIRCLE_SLASH:
		PvZSetCanvasBrowserCursorStyle("not-allowed");
		break;
	case CURSOR_SIZEALL:
		PvZSetCanvasBrowserCursorStyle("move");
		break;
	case CURSOR_SIZENESW:
		PvZSetCanvasBrowserCursorStyle("nesw-resize");
		break;
	case CURSOR_SIZENS:
		PvZSetCanvasBrowserCursorStyle("ns-resize");
		break;
	case CURSOR_SIZENWSE:
		PvZSetCanvasBrowserCursorStyle("nwse-resize");
		break;
	case CURSOR_SIZEWE:
		PvZSetCanvasBrowserCursorStyle("ew-resize");
		break;
	case CURSOR_WAIT:
		PvZSetCanvasBrowserCursorStyle("wait");
		break;
	default:
		PvZSetCanvasBrowserCursorStyle("default");
		break;
	}
}
#endif

static inline int GetPvZCursorScalePercent(SexyAppBase* theApp)
{
#ifdef __EMSCRIPTEN__
	const double aScale = PvZGetBrowserCanvasScale();
	return std::max(100, static_cast<int>(aScale * 100.0 + 0.5));
#else
	int aDrawableWidth = theApp->mWidth;
	int aDrawableHeight = theApp->mHeight;
	if (theApp->mWindow != nullptr)
	{
		SDL_GL_GetDrawableSize(static_cast<SDL_Window*>(theApp->mWindow), &aDrawableWidth, &aDrawableHeight);
		if (aDrawableWidth <= 0 || aDrawableHeight <= 0)
			SDL_GetWindowSize(static_cast<SDL_Window*>(theApp->mWindow), &aDrawableWidth, &aDrawableHeight);
	}

	if (theApp->mWidth <= 0 || theApp->mHeight <= 0 || aDrawableWidth <= 0 || aDrawableHeight <= 0)
		return 100;

	const double aScaleX = static_cast<double>(aDrawableWidth) / theApp->mWidth;
	const double aScaleY = static_cast<double>(aDrawableHeight) / theApp->mHeight;
	const double aScale = std::max(1.0, std::min(aScaleX, aScaleY));
	return std::max(100, static_cast<int>(aScale * 100.0 + 0.5));
#endif
}

} // namespace Sexy
