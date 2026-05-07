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
EM_JS(void, PvZEnsureCanvasBrowserCursor, (), {
	if (Module.pvzCanvasCursorReady) return;

	var getCanvas = function() {
		return Module.canvas || document.getElementById('canvas');
	};

	Module.pvzGetCanvasCursorScalePercent = function() {
		var canvas = getCanvas();
		if (!canvas) return 100;

		var rect = canvas.getBoundingClientRect();
		var scaleX = rect.width > 0 && canvas.width > 0 ? rect.width / canvas.width : 1;
		var scaleY = rect.height > 0 && canvas.height > 0 ? rect.height / canvas.height : 1;
		return Math.max(100, Math.round(Math.min(scaleX, scaleY) * 100));
	};

	Module.pvzSetCanvasCursorStyle = function(cursorStyle) {
		var canvas = getCanvas();
		if (!canvas) return;
		if (Module.pvzCanvasCursorStyle === cursorStyle && canvas.style.cursor === cursorStyle) return;

		canvas.style.cursor = cursorStyle;
		Module.pvzCanvasCursorStyle = cursorStyle;
	};

	Module.pvzApplyCanvasCursorKind = function(cursorKind) {
		var sources = Module.pvzCursorSources;
		var source = sources && sources[cursorKind];
		if (!source) return false;

		var scalePercent = Module.pvzGetCanvasCursorScalePercent();
		if (source.cursorStyle && source.scalePercent === scalePercent) {
			Module.pvzSetCanvasCursorStyle(source.cursorStyle);
			return true;
		}

		var width = Math.max(1, Math.round(source.width * scalePercent / 100));
		var height = Math.max(1, Math.round(source.height * scalePercent / 100));
		var hotX = Math.max(0, Math.min(width - 1, Math.round(source.hotX * scalePercent / 100)));
		var hotY = Math.max(0, Math.min(height - 1, Math.round(source.hotY * scalePercent / 100)));

		var scaled = Module.pvzCursorScaledCanvas;
		if (!scaled) {
			scaled = document.createElement('canvas');
			Module.pvzCursorScaledCanvas = scaled;
		}

		scaled.width = width;
		scaled.height = height;
		var ctx = scaled.getContext('2d');
		ctx.imageSmoothingEnabled = false;
		ctx.clearRect(0, 0, width, height);
		ctx.drawImage(source.canvas, 0, 0, width, height);

		source.scalePercent = scalePercent;
		source.cursorStyle = 'url("' + scaled.toDataURL('image/png') + '") ' + hotX + ' ' + hotY + ', auto';
		Module.pvzSetCanvasCursorStyle(source.cursorStyle);
		return true;
	};

	Module.pvzCanvasCursorReady = true;
});

EM_JS(void, PvZSetCanvasBrowserCursorStyle, (const char* theCursorStyle), {
	Module.pvzSetCanvasCursorStyle(UTF8ToString(theCursorStyle));
});

EM_JS(int, PvZSetCanvasBrowserCursorKind, (int theKind), {
	return Module.pvzApplyCanvasCursorKind(theKind) ? 1 : 0;
});

EM_JS(void, PvZSetCanvasBrowserCursorPixels, (int theKind, const uint32_t* thePixels, int theWidth, int theHeight, int theHotX, int theHotY), {
	var sourceCanvas = document.createElement('canvas');
	sourceCanvas.width = theWidth;
	sourceCanvas.height = theHeight;

	var ctx = sourceCanvas.getContext('2d');
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
	if (!Module.pvzCursorSources) {
		Module.pvzCursorSources = [];
	}

	Module.pvzCursorSources[theKind] = {
		canvas: sourceCanvas,
		width: theWidth,
		height: theHeight,
		hotX: theHotX,
		hotY: theHotY,
		scalePercent: 0,
		cursorStyle: null
	};
	Module.pvzApplyCanvasCursorKind(theKind);
});
#endif

namespace Sexy
{

enum class PvZCursorKind
{
	Pointer = 0,
	Hand = 1
};

struct PvZCursorBitmap
{
	std::vector<uint32_t> mPixels;
	int mWidth = 0;
	int mHeight = 0;
	int mHotX = 0;
	int mHotY = 0;
};

struct PvZCursorResource
{
	const unsigned char* mData = nullptr;
	size_t mSize = 0;
};

static inline bool PvZCursorKindFromCursorNum(int theCursorNum, PvZCursorKind& theKind)
{
	switch (theCursorNum)
	{
	case CURSOR_POINTER:
		theKind = PvZCursorKind::Pointer;
		return true;
	case CURSOR_HAND:
		theKind = PvZCursorKind::Hand;
		return true;
	default:
		return false;
	}
}

static inline PvZCursorResource GetPvZCursorResource(PvZCursorKind theKind)
{
	switch (theKind)
	{
	case PvZCursorKind::Pointer:
		return { kPvZPointerCursorCur, kPvZPointerCursorCurLen };
	case PvZCursorKind::Hand:
		return { kPvZHandCursorCur, kPvZHandCursorCurLen };
	}

	return {};
}

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
			if (aBitsPerPixel == 1)
			{
				const int aPaletteIndex = (anXorRow[x / 8] >> (7 - (x % 8))) & 1;
				if (aPaletteIndex >= aPaletteSize)
					return false;

				const unsigned char* aColor = theDib + aPaletteOffset + static_cast<size_t>(aPaletteIndex) * 4;
				aBlue = aColor[0];
				aGreen = aColor[1];
				aRed = aColor[2];
			}
			else if (aBitsPerPixel == 4)
			{
				const int aPaletteIndex = (x % 2 == 0) ? ((anXorRow[x / 2] >> 4) & 0xF) : (anXorRow[x / 2] & 0xF);
				if (aPaletteIndex >= aPaletteSize)
					return false;

				const unsigned char* aColor = theDib + aPaletteOffset + static_cast<size_t>(aPaletteIndex) * 4;
				aBlue = aColor[0];
				aGreen = aColor[1];
				aRed = aColor[2];
			}
			else
			{
				return false;
			}

			if (((anAndRow[x / 8] >> (7 - (x % 8))) & 1) != 0)
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

static inline bool PvZDecodeCursor(const unsigned char* theData, size_t theDataSize, PvZCursorBitmap& theBitmap)
{
	if (theDataSize < 22 || PvZReadLE16(theData) != 0 || PvZReadLE16(theData + 2) != 2)
		return false;

	const int aCount = PvZReadLE16(theData + 4);
	if (aCount != 1 || theDataSize < 22)
		return false;

	const unsigned char* anEntry = theData + 6;
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

static inline bool PvZLoadCursorBitmap(PvZCursorKind theKind, PvZCursorBitmap& theBitmap)
{
	const PvZCursorResource aResource = GetPvZCursorResource(theKind);
	if (aResource.mData == nullptr)
		return false;

	return PvZDecodeCursor(aResource.mData, aResource.mSize, theBitmap);
}

#ifdef __EMSCRIPTEN__
static inline bool ApplyPvZBrowserCursorKind(PvZCursorKind theKind)
{
	PvZEnsureCanvasBrowserCursor();
	if (PvZSetCanvasBrowserCursorKind(static_cast<int>(theKind)) != 0)
		return true;

	PvZCursorBitmap aBitmap;
	if (!PvZLoadCursorBitmap(theKind, aBitmap))
		return false;

	PvZSetCanvasBrowserCursorPixels(static_cast<int>(theKind), aBitmap.mPixels.data(), aBitmap.mWidth, aBitmap.mHeight, aBitmap.mHotX, aBitmap.mHotY);
	return true;
}

static inline bool ApplyPvZBrowserCursor(int theCursorNum)
{
	PvZCursorKind aKind;
	return PvZCursorKindFromCursorNum(theCursorNum, aKind) && ApplyPvZBrowserCursorKind(aKind);
}

static inline void HidePvZBrowserCursor()
{
	PvZEnsureCanvasBrowserCursor();
	PvZSetCanvasBrowserCursorStyle("none");
}

#else
static inline int& PvZCursorScaleCache(PvZCursorKind theKind)
{
	static int aScalePercent[2] = {};
	return aScalePercent[static_cast<int>(theKind)];
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

static inline bool PvZBuildScaledCursorBitmap(PvZCursorKind theKind, int theScalePercent, PvZCursorBitmap& theBitmap)
{
	PvZCursorBitmap aSource;
	if (!PvZLoadCursorBitmap(theKind, aSource))
		return false;

	return PvZScaleCursorBitmap(aSource, theScalePercent, theBitmap);
}

static inline SDL_Cursor* CreatePvZColorCursor(PvZCursorKind theKind, int theScalePercent)
{
	PvZCursorBitmap aBitmap;
	if (!PvZBuildScaledCursorBitmap(theKind, theScalePercent, aBitmap))
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

static inline int GetPvZCursorScalePercent(SexyAppBase* theApp)
{
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
}

static inline SDL_Cursor* GetPvZColorCursor(SexyAppBase* theApp, int theCursorNum, SDL_Cursor*& theCachedCursor)
{
	PvZCursorKind aKind;
	if (!PvZCursorKindFromCursorNum(theCursorNum, aKind))
		return nullptr;

	const int aScalePercent = GetPvZCursorScalePercent(theApp);
	int& aCachedScalePercent = PvZCursorScaleCache(aKind);
	if (theCachedCursor != nullptr && aCachedScalePercent != aScalePercent)
	{
		SDL_FreeCursor(theCachedCursor);
		theCachedCursor = nullptr;
	}

	if (theCachedCursor == nullptr)
	{
		theCachedCursor = CreatePvZColorCursor(aKind, aScalePercent);
		if (theCachedCursor != nullptr)
			aCachedScalePercent = aScalePercent;
	}

	return theCachedCursor;
}
#endif

} // namespace Sexy
