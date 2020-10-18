
#if 0
#include <windows.h>
#endif
#include "ncbind/ncbind.hpp"
#include "istream_compat.h"
#include <webp/encode.h>
#include <webp/decode.h>
#include <memory>
#include "tp_stub.h"
#if 0
#define EXPORT(hr) extern "C" __declspec(dllexport) hr __stdcall
#endif

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
			return;
		}
		sizecallback(callbackdata, width, height);
		for(int y = 0; y < height; y++) {
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
			return;
		}
		sizecallback(callbackdata, width, height);
		for(int y = 0; y < height; y++) {
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

bool TVPAcceptSaveAsWebP(void* formatdata, const ttstr & type, class iTJSDispatch2** dic ) {
	if( type.StartsWith(TJS_W("webp")) || (type == TJS_W(".webp")) ) return true;
	return false;
}

static int WebPIStreamWrite(const uint8_t* data, size_t data_size, const WebPPicture* pic)
{
	ULONG bytes_written = 0;
	((IStream*)(pic->custom_ptr))->Write(data, data_size, &bytes_written);
	return bytes_written == data_size;
}

#define WEBP_OPTION(parameter, casttype, access_method) \
	else if (value == TJS_W(#parameter)) \
	{ \
		opt_->parameter = (casttype)param[2]->access_method(); \
	}

void TVPSaveAsWebP(void* formatdata, void* callbackdata, IStream* dst, const ttstr & mode, tjs_uint width, tjs_uint height, tTVPGraphicSaveScanLineCallback scanlinecallback, iTJSDispatch2* meta )
{
	if (height == 0 || width == 0)
	{
		TVPThrowExceptionMessage(TJS_W("Height or width is zero"));
		return;
	}
	int stride = width * sizeof(tjs_uint32);
	int datasize = stride * height;
	std::unique_ptr<uint8_t[]> data(new uint8_t[datasize]);
	for (tjs_uint y = 0; y < height; y += 1)
	{
		memcpy(&(data[y*stride]), scanlinecallback(callbackdata,y), stride);
	}
	WebPConfig config;
	if (!WebPConfigPreset(&config, WEBP_PRESET_DRAWING, 100))
	{
		TVPThrowExceptionMessage(TJS_W("Unable to set WebP config preset"));
		return;
	}
	{
		// Set properties using EnumCallback
		struct MetaDictionaryEnumCallback : public tTJSDispatch
		{
			WebPConfig *opt_;
			MetaDictionaryEnumCallback( WebPConfig *opt ) : opt_(opt) {}
			tjs_error TJS_INTF_METHOD FuncCall(tjs_uint32 flag, const tjs_char * membername, tjs_uint32 *hint, tTJSVariant *result, tjs_int numparams, tTJSVariant **param, iTJSDispatch2 *objthis)
			{ // called from tTJSCustomObject::EnumMembers
				if (numparams < 3)
				{
					return TJS_E_BADPARAMCOUNT;
				}
				tjs_uint32 flags = (tjs_int)*param[1];
				if (flags & TJS_HIDDENMEMBER)
				{
					if (result)
					{
						*result = (tjs_int)1;
					}
					return TJS_S_OK;
				}
				// push items
				ttstr value = *param[0];
				if (0) {}
				WEBP_OPTION(lossless, int, AsInteger)
				WEBP_OPTION(quality, float, AsReal)
				WEBP_OPTION(method, int, AsInteger)
				WEBP_OPTION(image_hint, WebPImageHint, AsInteger)
				WEBP_OPTION(target_size, int, AsInteger)
				WEBP_OPTION(target_PSNR, float, AsReal)
				WEBP_OPTION(segments, int, AsInteger)
				WEBP_OPTION(sns_strength, int, AsInteger)
				WEBP_OPTION(filter_strength, int, AsInteger)
				WEBP_OPTION(filter_sharpness, int, AsInteger)
				WEBP_OPTION(filter_type, int, AsInteger)
				WEBP_OPTION(autofilter, int, AsInteger)
				WEBP_OPTION(alpha_compression, int, AsInteger)
				WEBP_OPTION(alpha_filtering, int, AsInteger)
				WEBP_OPTION(alpha_quality, int, AsInteger)
				WEBP_OPTION(pass, int, AsInteger)
				WEBP_OPTION(show_compressed, int, AsInteger)
				WEBP_OPTION(preprocessing, int, AsInteger)
				WEBP_OPTION(partitions, int, AsInteger)
				WEBP_OPTION(partition_limit, int, AsInteger)
				WEBP_OPTION(emulate_jpeg_size, int, AsInteger)
				WEBP_OPTION(thread_level, int, AsInteger)
				WEBP_OPTION(low_memory, int, AsInteger)
				WEBP_OPTION(near_lossless, int, AsInteger)
				WEBP_OPTION(exact, int, AsInteger)
				WEBP_OPTION(use_delta_palette, int, AsInteger)
				WEBP_OPTION(use_sharp_yuv, int, AsInteger)

				if (result)
				{
					*result = (tjs_int)1;
				}
				return TJS_S_OK;
			}
		} callback( &config );
		tTJSVariantClosure clo(&callback, NULL);
		meta->EnumMembers(TJS_IGNOREPROP, &clo, meta);
	}
	if (!WebPValidateConfig(&config))
	{
		TVPThrowExceptionMessage(TJS_W("WebP encoding config is invalid"));
	}
	WebPPicture pic;
	if (!WebPPictureInit(&pic))
	{
		TVPThrowExceptionMessage(TJS_W("Could not initialize WebP picture"));
		return;
	}
	pic.width = width;
	pic.height = height;
	if (!WebPPictureAlloc(&pic))
	{
		TVPThrowExceptionMessage(TJS_W("Could not allocate WebP picture"));
		return;
	}
	if (!WebPPictureImportBGRA(&pic, data.get(), stride))
	{
		WebPPictureFree(&pic);
		TVPThrowExceptionMessage(TJS_W("Could not import image to WebP picture"));
		return;
	}
	pic.writer = WebPIStreamWrite;
	pic.custom_ptr = (void*)dst;
	int ok = WebPEncode(&config, &pic);
	WebPPictureFree(&pic);
	if (!ok)
	{
		TVPThrowExceptionMessage(TJS_W("WebP encoding error: %1"), ttstr(pic.error_code));
	}
}

static void krglhwebp_init()
{
	TVPRegisterGraphicLoadingHandler( ttstr(TJS_W(".webp")), &TVPLoadWEBP, &TVPLoadHeaderWEBP, &TVPSaveAsWebP, &TVPAcceptSaveAsWebP, NULL );
}

NCB_PRE_REGIST_CALLBACK(krglhwebp_init);

#if 0
BOOL APIENTRY DllMain( HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved ) {
	return TRUE;
}

static tjs_int GlobalRefCountAtInit = 0;
EXPORT(HRESULT) V2Link(iTVPFunctionExporter *exporter)
{
	TVPInitImportStub(exporter);

	TVPRegisterGraphicLoadingHandler( ttstr(TJS_W(".webp")), TVPLoadWEBP, TVPLoadHeaderWEBP, TVPSaveAsWebP, TVPAcceptSaveAsWebP, NULL );

	GlobalRefCountAtInit = TVPPluginGlobalRefCount;
	return S_OK;
}

EXPORT(HRESULT) V2Unlink() {
	if(TVPPluginGlobalRefCount > GlobalRefCountAtInit) return E_FAIL;
	
	TVPRegisterGraphicLoadingHandler( ttstr(TJS_W(".webp")), TVPLoadWEBP, TVPLoadHeaderWEBP, TVPSaveAsWebP, TVPAcceptSaveAsWebP, NULL );

	TVPUninitImportStub();
	return S_OK;
}
#endif
