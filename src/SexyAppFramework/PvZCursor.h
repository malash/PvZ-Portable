#pragma once

#ifdef __EMSCRIPTEN__

#include <emscripten.h>

#include "SexyAppBase.h"

EM_JS(void, PvZEnsureCanvasBrowserCursor, (), {
	if (Module.pvzCanvasCursorReady) return;

	var getCanvas = function() {
		return Module.canvas || document.getElementById('canvas');
	};

	Module.pvzCursorSources = [
		{
			dataUrl: 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAYAAABzenr0AAAAqklEQVR42u3XQQrAIAwEQP//Kn9WKSiIRBM12e2hgqeCTqOta3pqS6xWJ+chGoCGaICcMwfRAyjLMQLgCAkARcwAMMQKAEFogHCEBRCKsALCEDuAEMQuwB1xAnBFnALcEDcAF4QV8D6f9SvEDCCdDVo/QkiA9lbSCWlYkjvAUFLsj6ifHJYTVpEMUoVVKIVUQRs0vAra7qVnRthe+HRy/qsgfbLUu6Tnlb4AgaQqovkXqhYAAAAASUVORK5CYII=',
			width: 32,
			height: 32,
			hotX: 1,
			hotY: 4
		},
		{
			dataUrl: 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAYAAABzenr0AAAAaklEQVR42u3VywrAIAxE0fz/T+tGQaNihOgYmAtu2+OjVoQxxgKVysC8vIZCEPAd4DmkAyAwS8AEgwN4nhE9IzPACzEs7a5rAP1wS27bgAZIe7CggNO8P8dYgFuXEQFivYye/JBgAOOIVQaps8BcUFPTsgAAAABJRU5ErkJggg==',
			width: 32,
			height: 32,
			hotX: 10,
			hotY: 10
		}
	];

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

	Module.pvzLoadCanvasCursorSource = function(cursorKind, source) {
		if (source.image) return source.loaded;

		var image = new Image();
		image.onload = function() {
			source.loaded = true;
			source.cursorStyle = null;
			if (Module.pvzCanvasCursorKind === cursorKind) {
				Module.pvzApplyCanvasCursorKind(cursorKind);
			}
		};
		image.src = source.dataUrl;
		source.image = image;
		source.loaded = image.complete && image.naturalWidth > 0;
		return source.loaded;
	};

	Module.pvzApplyCanvasCursorKind = function(cursorKind) {
		var source = Module.pvzCursorSources && Module.pvzCursorSources[cursorKind];
		if (!source) return false;

		Module.pvzCanvasCursorKind = cursorKind;
		var scalePercent = Module.pvzGetCanvasCursorScalePercent();
		if (source.cursorStyle && source.scalePercent === scalePercent) {
			Module.pvzSetCanvasCursorStyle(source.cursorStyle);
			return true;
		}

		if (!Module.pvzLoadCanvasCursorSource(cursorKind, source)) {
			Module.pvzSetCanvasCursorStyle('url("' + source.dataUrl + '") ' + source.hotX + ' ' + source.hotY + ', auto');
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
		ctx.drawImage(source.image, 0, 0, width, height);

		source.scalePercent = scalePercent;
		source.cursorStyle = 'url("' + scaled.toDataURL('image/png') + '") ' + hotX + ' ' + hotY + ', auto';
		Module.pvzSetCanvasCursorStyle(source.cursorStyle);
		return true;
	};

	Module.pvzCanvasCursorReady = true;
});

EM_JS(void, PvZClearCanvasBrowserCursorKind, (), {
	Module.pvzCanvasCursorKind = -1;
});

EM_JS(void, PvZHideCanvasBrowserCursor, (), {
	Module.pvzCanvasCursorKind = -1;
	Module.pvzSetCanvasCursorStyle('none');
});

EM_JS(int, PvZSetCanvasBrowserCursorKind, (int theKind), {
	return Module.pvzApplyCanvasCursorKind(theKind) ? 1 : 0;
});

namespace Sexy
{

static inline bool ApplyPvZBrowserCursor(int theCursorNum)
{
	PvZEnsureCanvasBrowserCursor();
	switch (theCursorNum)
	{
	case CURSOR_POINTER:
		return PvZSetCanvasBrowserCursorKind(0) != 0;
	case CURSOR_HAND:
		return PvZSetCanvasBrowserCursorKind(1) != 0;
	default:
		PvZClearCanvasBrowserCursorKind();
		return false;
	}
}

static inline void HidePvZBrowserCursor()
{
	PvZEnsureCanvasBrowserCursor();
	PvZHideCanvasBrowserCursor();
}

} // namespace Sexy

#endif
