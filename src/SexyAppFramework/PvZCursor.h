#pragma once

#ifdef __EMSCRIPTEN__

#include <emscripten.h>

#include "SexyAppBase.h"

EM_JS(void, PvZSetBrowserCursorKind, (int theCursorKind), {
	if (theCursorKind < 0) {
		Module.pvzCanvasCursorKind = -1;
		if (Module.canvas) Module.canvas.style.cursor = 'none';
		return;
	}

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

	const loadImage = async (source, cache) => {
		if (cache.loaded) return true;

		if (!cache.image) {
			const image = new Image(source.width, source.height);
			image.src = source.dataUrl;
			cache.image = image;
		}

		try {
			await cache.image.decode();
			return true;
		} catch (error) {
			return false;
		}
	};

	const getScalePercent = (canvas) => {
		const rect = canvas.getBoundingClientRect();
		const scaleX = rect.width > 0 && canvas.width > 0 ? rect.width / canvas.width : 1;
		const scaleY = rect.height > 0 && canvas.height > 0 ? rect.height / canvas.height : 1;
		return Math.max(100, Math.round(Math.min(scaleX, scaleY) * 100));
	};

	const applyCursor = async (kind) => {
		const canvas = Module.canvas;
		const source = CURSOR_SOURCES[kind];
		if (!canvas || !source) return;

		if (!Module.pvzCanvasCursorCache) {
			Module.pvzCanvasCursorCache = CURSOR_SOURCES.map(() => ({}));
		}

		const cache = Module.pvzCanvasCursorCache[kind];
		Module.pvzCanvasCursorKind = kind;
		let scalePercent = getScalePercent(canvas);
		if (cache.cursorStyle && cache.scalePercent === scalePercent) {
			canvas.style.cursor = cache.cursorStyle;
			return;
		}

		if (!cache.loaded) {
			const loaded = await loadImage(source, cache);
			if (!loaded || Module.pvzCanvasCursorKind !== kind) return;
			cache.loaded = true;
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
		ctx.drawImage(cache.image, 0, 0, width, height);

		cache.scalePercent = scalePercent;
		const scaledDataUrl = scaledCanvas.toDataURL('image/png');
		cache.cursorStyle = 'url("' + scaledDataUrl + '") ' + hotX + ' ' + hotY + ', auto';
		canvas.style.cursor = cache.cursorStyle;
	};

	applyCursor(theCursorKind);
});

namespace Sexy
{

enum
{
	PVZ_BROWSER_CURSOR_NONE = -1,
	PVZ_BROWSER_CURSOR_POINTER,
	PVZ_BROWSER_CURSOR_HAND
};

static inline bool ApplyPvZBrowserCursor(int theCursorNum)
{
	switch (theCursorNum)
	{
	case CURSOR_POINTER:
		PvZSetBrowserCursorKind(PVZ_BROWSER_CURSOR_POINTER);
		return true;
	case CURSOR_HAND:
		PvZSetBrowserCursorKind(PVZ_BROWSER_CURSOR_HAND);
		return true;
	default:
		PvZSetBrowserCursorKind(PVZ_BROWSER_CURSOR_NONE);
		return false;
	}
}

static inline void HidePvZBrowserCursor()
{
	PvZSetBrowserCursorKind(PVZ_BROWSER_CURSOR_NONE);
}

} // namespace Sexy

#endif
