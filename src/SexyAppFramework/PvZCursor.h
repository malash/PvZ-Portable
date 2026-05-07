#pragma once

#ifdef __EMSCRIPTEN__

#include <emscripten.h>

#include "SexyAppBase.h"

EM_JS(int, PvZApplyBrowserCursorKind, (int theCursorKind), {
	var state = Module.pvzCanvasCursor;
	if (!state || !state.sources) {
		state = Object.assign(state || {}, {
			kind: -1,
			style: state ? state.style : null,
			scaledCanvas: null,
			sources: [
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
			]
		});
		Module.pvzCanvasCursor = state;
	}

	var getCanvas = function() {
		return Module.canvas;
	};

	var setCursorStyle = function(cursorStyle) {
		var canvas = getCanvas();
		if (!canvas) return;
		if (state.style === cursorStyle && canvas.style.cursor === cursorStyle) return;

		canvas.style.cursor = cursorStyle;
		state.style = cursorStyle;
	};

	var getScalePercent = function() {
		var canvas = getCanvas();
		if (!canvas) return 100;

		var rect = canvas.getBoundingClientRect();
		var scaleX = rect.width > 0 && canvas.width > 0 ? rect.width / canvas.width : 1;
		var scaleY = rect.height > 0 && canvas.height > 0 ? rect.height / canvas.height : 1;
		return Math.max(100, Math.round(Math.min(scaleX, scaleY) * 100));
	};

	var applyCursor = function(kind) {
		var source = state.sources[kind];
		if (!source) return false;

		state.kind = kind;
		var scalePercent = getScalePercent();
		if (source.cursorStyle && source.scalePercent === scalePercent) {
			setCursorStyle(source.cursorStyle);
			return true;
		}

		if (!source.image) {
			var image = new Image();
			image.onload = function() {
				source.loaded = true;
				source.cursorStyle = null;
				if (state.kind === kind) applyCursor(kind);
			};
			image.src = source.dataUrl;
			source.image = image;
			source.loaded = image.complete && image.naturalWidth > 0;
		}

		if (!source.loaded) {
			setCursorStyle('url("' + source.dataUrl + '") ' + source.hotX + ' ' + source.hotY + ', auto');
			return true;
		}

		var width = Math.max(1, Math.round(source.width * scalePercent / 100));
		var height = Math.max(1, Math.round(source.height * scalePercent / 100));
		var hotX = Math.max(0, Math.min(width - 1, Math.round(source.hotX * scalePercent / 100)));
		var hotY = Math.max(0, Math.min(height - 1, Math.round(source.hotY * scalePercent / 100)));

		if (!state.scaledCanvas) {
			state.scaledCanvas = document.createElement('canvas');
		}

		state.scaledCanvas.width = width;
		state.scaledCanvas.height = height;
		var ctx = state.scaledCanvas.getContext('2d');
		ctx.imageSmoothingEnabled = false;
		ctx.clearRect(0, 0, width, height);
		ctx.drawImage(source.image, 0, 0, width, height);

		source.scalePercent = scalePercent;
		source.cursorStyle = 'url("' + state.scaledCanvas.toDataURL('image/png') + '") ' + hotX + ' ' + hotY + ', auto';
		setCursorStyle(source.cursorStyle);
		return true;
	};

	return applyCursor(theCursorKind) ? 1 : 0;
});

EM_JS(void, PvZHideBrowserCursor, (), {
	var canvas = Module.canvas;
	if (!Module.pvzCanvasCursor) {
		Module.pvzCanvasCursor = { kind: -1, style: null };
	} else {
		Module.pvzCanvasCursor.kind = -1;
	}

	if (canvas && (Module.pvzCanvasCursor.style !== 'none' || canvas.style.cursor !== 'none')) {
		canvas.style.cursor = 'none';
		Module.pvzCanvasCursor.style = 'none';
	}
});

namespace Sexy
{

enum
{
	PVZ_BROWSER_CURSOR_POINTER,
	PVZ_BROWSER_CURSOR_HAND
};

static inline bool ApplyPvZBrowserCursor(int theCursorNum)
{
	switch (theCursorNum)
	{
	case CURSOR_POINTER:
		return PvZApplyBrowserCursorKind(PVZ_BROWSER_CURSOR_POINTER) != 0;
	case CURSOR_HAND:
		return PvZApplyBrowserCursorKind(PVZ_BROWSER_CURSOR_HAND) != 0;
	default:
		return false;
	}
}

static inline void HidePvZBrowserCursor()
{
	PvZHideBrowserCursor();
}

} // namespace Sexy

#endif
