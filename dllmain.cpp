#include <windows.h>
#include "tp_stub.h"
#include <tchar.h>
#include <string.h>
#include <vector>
#include <memory>
#include <stdio.h>
#define EXPORT(hr) extern "C" __declspec(dllexport) hr __stdcall

#include <webp/decode.h>
void TVPLoadWEBP(void* formatdata, void *callbackdata, tTVPGraphicSizeCallback sizecallback, tTVPGraphicScanLineCallback scanlinecallback, tTVPMetaInfoPushCallback metainfopushcallback, IStream *src, tjs_int keyidx, tTVPGraphicLoadMode mode)
{
	STATSTG stg;

	src->Stat(&stg, STATFLAG_NONAME);
	int datasize = stg.cbSize.QuadPart;
	std::unique_ptr<uint8_t[]> data(new uint8_t[datasize]);
	ULONG read;
	src->Read(data.get(), datasize, &read);

	if(glmNormal == mode) {
		int width, height;
		uint8_t* buffer = WebPDecodeBGRA(data.get(), datasize, &width, &height);
		if (!buffer) {
			TVPThrowExceptionMessage(TJS_W("Invalid WebP image (RGBA mode)"));
		}
		sizecallback(callbackdata, width, height);
		for( int y = 0; y < height; y++ ) {
			void *scanline = scanlinecallback(callbackdata, y);
			if(!scanline) break;
			memcpy(scanline, (const void*)&buffer[y*width*sizeof(tjs_uint32)], width*sizeof(tjs_uint32));
			scanlinecallback(callbackdata, -1);
		}
		WebPFree(buffer);
	} else if(glmGrayscale == mode) {
		int width, height, stride, uv_stride;
		uint8_t* buffer_u;
		uint8_t* buffer_v;
		uint8_t* buffer = WebPDecodeYUV(data.get(), datasize, &width, &height, &buffer_u, &buffer_v, &stride, &uv_stride);
		if (!buffer || !buffer_u || !buffer_v) {
			TVPThrowExceptionMessage(TJS_W("Invalid WebP image (Grayscale mode)"));
		}
		for( int y = 0; y < height; y++ ) {
			void *scanline = scanlinecallback(callbackdata, y);
			if(!scanline) break;
			memcpy(scanline, (const void*)&buffer[y*stride], width);
			scanlinecallback(callbackdata, -1);
		}
		WebPFree(buffer);
		WebPFree(buffer_u);
		WebPFree(buffer_v);
	} else {
		TVPThrowExceptionMessage(TJS_W("WebP does not support decoding in palette/CLUT format"));
		return;
	}
	
}

void TVPLoadHeaderWEBP(void* formatdata, IStream *src, iTJSDispatch2** dic) {
	WebPDecoderConfig config;
	if (WebPInitDecoderConfig(&config) == 0) {
		TVPThrowExceptionMessage(TJS_W("Error initializing decoder"));
		return;
	}

	STATSTG stg;

	src->Stat(&stg, STATFLAG_NONAME);
	int datasize = stg.cbSize.QuadPart;

	std::unique_ptr<uint8_t[]> data(new uint8_t[datasize]);
	ULONG read;
	src->Read(data.get(), datasize, &read);
	if (WebPGetFeatures(data.get(), datasize, &config.input) != VP8_STATUS_OK) {
		TVPThrowExceptionMessage(TJS_W("Invalid WebP image"));
		return;
	}

	*dic = TJSCreateDictionaryObject();
	tTJSVariant val((tjs_int32)config.input.width);
	(*dic)->PropSet(TJS_MEMBERENSURE, TJS_W("width"), 0, &val, (*dic));
	val = tTJSVariant((tjs_int32)config.input.height);
	(*dic)->PropSet(TJS_MEMBERENSURE, TJS_W("height"), 0, &val, (*dic));
	val = tTJSVariant(config.input.has_alpha ? 32 : 24);
	(*dic)->PropSet(TJS_MEMBERENSURE, TJS_W("bpp"), 0, &val, (*dic));
}

BOOL APIENTRY DllMain( HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved ) {
	return TRUE;
}

static tjs_int GlobalRefCountAtInit = 0;
EXPORT(HRESULT) V2Link(iTVPFunctionExporter *exporter)
{
	TVPInitImportStub(exporter);

	TVPRegisterGraphicLoadingHandler( ttstr(TJS_W(".webp")), TVPLoadWEBP, TVPLoadHeaderWEBP, NULL, NULL, NULL );

	GlobalRefCountAtInit = TVPPluginGlobalRefCount;
	return S_OK;
}

EXPORT(HRESULT) V2Unlink() {
	if(TVPPluginGlobalRefCount > GlobalRefCountAtInit) return E_FAIL;
	
	TVPRegisterGraphicLoadingHandler( ttstr(TJS_W(".webp")), TVPLoadWEBP, TVPLoadHeaderWEBP, NULL, NULL, NULL );

	TVPUninitImportStub();
	return S_OK;
}
