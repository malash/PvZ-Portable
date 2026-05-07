#pragma once

#ifdef __EMSCRIPTEN__

#include <emscripten.h>

#include "SexyAppBase.h"

EM_JS(void, PvZApplyBrowserCursorKind, (int theCursorKind), {
	const CURSOR_SOURCES = [
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

	const getState = () => {
		if (!Module.pvzCanvasCursorState) {
			Module.pvzCanvasCursorState = {
				currentKind: -1
			};
		}

		return Module.pvzCanvasCursorState;
	};

	const getCache = (kind) => {
		if (!Module.pvzCanvasCursorCache) {
			Module.pvzCanvasCursorCache = CURSOR_SOURCES.map(() => ({}));
		}

		return Module.pvzCanvasCursorCache[kind];
	};

	const getScalePercent = (canvas) => {
		const rect = canvas.getBoundingClientRect();
		const scaleX = rect.width > 0 && canvas.width > 0 ? rect.width / canvas.width : 1;
		const scaleY = rect.height > 0 && canvas.height > 0 ? rect.height / canvas.height : 1;
		return Math.max(100, Math.round(Math.min(scaleX, scaleY) * 100));
	};

	const loadImage = async (source, cache) => {
		if (cache.loaded) return true;

		if (!cache.image) {
			const image = new Image();
			image.src = source.dataUrl;
			cache.image = image;
		}

		try {
			await cache.image.decode();
			cache.loaded = true;
			return true;
		} catch (error) {
			return false;
		}
	};

	const applyCursor = async (state, kind) => {
		const canvas = Module.canvas;
		const source = CURSOR_SOURCES[kind];
		if (!canvas || !source) return;

		const cache = getCache(kind);
		state.currentKind = kind;
		const scalePercent = getScalePercent(canvas);
		if (cache.cursorStyle && cache.scalePercent === scalePercent) {
			canvas.style.cursor = cache.cursorStyle;
			return;
		}

		if (!cache.loaded) {
			canvas.style.cursor = 'url("' + source.dataUrl + '") ' + source.hotX + ' ' + source.hotY + ', auto';
			if (!await loadImage(source, cache) || state.currentKind !== kind) return;
			cache.cursorStyle = null;
		}

		const width = Math.max(1, Math.round(source.width * scalePercent / 100));
		const height = Math.max(1, Math.round(source.height * scalePercent / 100));
		const hotX = Math.max(0, Math.min(width - 1, Math.round(source.hotX * scalePercent / 100)));
		const hotY = Math.max(0, Math.min(height - 1, Math.round(source.hotY * scalePercent / 100)));

		const scaledCanvas = document.createElement('canvas');
		scaledCanvas.width = width;
		scaledCanvas.height = height;
		const ctx = scaledCanvas.getContext('2d');
		ctx.imageSmoothingEnabled = false;
		ctx.clearRect(0, 0, width, height);
		ctx.drawImage(cache.image, 0, 0, width, height);

		cache.scalePercent = scalePercent;
		cache.cursorStyle = 'url("' + scaledCanvas.toDataURL('image/png') + '") ' + hotX + ' ' + hotY + ', auto';
		canvas.style.cursor = cache.cursorStyle;
	};

	applyCursor(getState(), theCursorKind);
});

EM_JS(void, PvZHideBrowserCursor, (), {
	const canvas = Module.canvas;
	const state = Module.pvzCanvasCursorState || {};
	state.currentKind = -1;
	Module.pvzCanvasCursorState = state;

	if (canvas) {
		canvas.style.cursor = 'none';
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
		PvZApplyBrowserCursorKind(PVZ_BROWSER_CURSOR_POINTER);
		return true;
	case CURSOR_HAND:
		PvZApplyBrowserCursorKind(PVZ_BROWSER_CURSOR_HAND);
		return true;
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
