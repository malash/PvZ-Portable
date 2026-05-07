#pragma once

#ifdef __EMSCRIPTEN__

#include <cstdint>

#include <emscripten.h>

#include "SexyAppBase.h"
#include "CursorPointerBitmap.inc"
#include "CursorHandBitmap.inc"

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

namespace Sexy
{

enum class PvZCursorKind
{
	Pointer = 0,
	Hand = 1
};

struct PvZCursorData
{
	const uint32_t* mPixels = nullptr;
	int mWidth = 0;
	int mHeight = 0;
	int mHotX = 0;
	int mHotY = 0;
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

static inline bool PvZGetCursorData(PvZCursorKind theKind, PvZCursorData& theData)
{
	switch (theKind)
	{
	case PvZCursorKind::Pointer:
		theData = {
			kPvZPointerCursorPixels,
			kPvZPointerCursorWidth,
			kPvZPointerCursorHeight,
			kPvZPointerCursorHotX,
			kPvZPointerCursorHotY
		};
		return true;
	case PvZCursorKind::Hand:
		theData = {
			kPvZHandCursorPixels,
			kPvZHandCursorWidth,
			kPvZHandCursorHeight,
			kPvZHandCursorHotX,
			kPvZHandCursorHotY
		};
		return true;
	}

	return false;
}

static inline bool ApplyPvZBrowserCursorKind(PvZCursorKind theKind)
{
	PvZEnsureCanvasBrowserCursor();
	if (PvZSetCanvasBrowserCursorKind(static_cast<int>(theKind)) != 0)
		return true;

	PvZCursorData aData;
	if (!PvZGetCursorData(theKind, aData))
		return false;

	PvZSetCanvasBrowserCursorPixels(static_cast<int>(theKind), aData.mPixels, aData.mWidth, aData.mHeight, aData.mHotX, aData.mHotY);
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

} // namespace Sexy

#endif
