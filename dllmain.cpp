
#include <windows.h>
#include <webp/encode.h>
#include <webp/decode.h>
#include <memory>
#include "tp_stub.h"
#define EXPORT(hr) extern "C" __declspec(dllexport) hr __stdcall

static const tjs_int WEBP_VP8_HEADER_SIZE = 30;
static const tjs_int WEBP_IDECODE_BUFFER_SZ = (1 << 16);

void TVPLoadWEBP(void* formatdata, void *callbackdata, tTVPGraphicSizeCallback sizecallback, tTVPGraphicScanLineCallback scanlinecallback, tTVPMetaInfoPushCallback metainfopushcallback, IStream *src, tjs_int keyidx, tTVPGraphicLoadMode mode)
{
	STATSTG stg;

	src->Stat(&stg, STATFLAG_NONAME);
	tjs_int datasize = stg.cbSize.QuadPart;
	if (datasize < WEBP_VP8_HEADER_SIZE)
	{
		TVPThrowExceptionMessage(TJS_W("File size is too small"));
		return;
	}

	uint8_t headerdata[WEBP_VP8_HEADER_SIZE];
	ULONG read;
	src->Read(headerdata, WEBP_VP8_HEADER_SIZE, &read);
	if (read != WEBP_VP8_HEADER_SIZE)
	{
		TVPThrowExceptionMessage(TJS_W("Read size is too small"));
		return;
	}

	WebPDecoderConfig config;
	if (WebPInitDecoderConfig(&config) == 0)
	{
		TVPThrowExceptionMessage(TJS_W("Error initializing decoder"));
		return;
	}
	if (WebPGetFeatures(headerdata, WEBP_VP8_HEADER_SIZE, &config.input) != VP8_STATUS_OK)
	{
		TVPThrowExceptionMessage(TJS_W("Error getting features"));
		return;
	}
	int width = 0;
	int height = 0;
	if (WebPGetInfo(headerdata, WEBP_VP8_HEADER_SIZE, &width, &height) == 0)
	{
		TVPThrowExceptionMessage(TJS_W("Error getting size"));
		return;
	}
	sizecallback(callbackdata, width, height);
	// Determine the buffer pitch...
	tjs_uint8 *line0 = (tjs_uint8 *)scanlinecallback(callbackdata, 0);
	int size_pixel = (glmNormal != mode) ? sizeof(tjs_uint8) : sizeof(tjs_uint32);
	tjs_int pitch = width * size_pixel;
	// If we only have one line, we don't need pitch anyways
	if (height > 1)
	{
		tjs_uint8 *line1 = (tjs_uint8 *)scanlinecallback(callbackdata, 1);
		if (line1 != NULL)
		{
			pitch = line1 - line0;
		}
	}

	tjs_uint8 *outbuf = NULL;
	tjs_int outbufpitch = 0;
	if (0 == pitch || glmNormal != mode)
	{
		// we need a temparary buffer
	}
	else if (pitch < 0)
	{
		outbuf = (tjs_uint8 *)scanlinecallback(callbackdata, height - 1);
		outbufpitch = pitch;
	}
	else if (pitch > 0)
	{
		outbuf = line0;
		outbufpitch = pitch;
	}
	if (pitch < 0)
	{
		// negative, needs flip
		config.options.flip = 1;
		pitch = -pitch;
		outbufpitch = -outbufpitch;
	}
	if (NULL == outbuf)
	{
		// allocate the temporary buffer
		config.options.flip = 0;
		outbufpitch = width * sizeof(tjs_uint32);
		outbuf = (tjs_uint8 *)malloc(outbufpitch * height);

		if (!outbuf)
		{
			TVPThrowExceptionMessage(TJS_W("Could not allocate temporary buffer"));
			return;
		}
	}
	memset(outbuf, 0, outbufpitch * height);

	{
		config.output.colorspace = MODE_BGRA;
		config.output.u.RGBA.rgba = (uint8_t*)outbuf;
		config.output.u.RGBA.stride = outbufpitch;
		config.output.u.RGBA.size = outbufpitch * height;
		config.output.is_external_memory = 1;
	}

	WebPIDecoder* idec = WebPIDecode(NULL, 0, &config);
	if (NULL == idec)
	{
		WebPFreeDecBuffer(&(config.output));
		TVPThrowExceptionMessage(TJS_W("Could not allocate decoder"));
		return;
	}
	tjs_uint8* input = (tjs_uint8*)malloc(WEBP_IDECODE_BUFFER_SZ);
	if (NULL == input)
	{
		WebPFreeDecBuffer(&(config.output));
		TVPThrowExceptionMessage(TJS_W("Could not allocate decode buffer"));
	}
	bool success = true;
	VP8StatusCode status = VP8_STATUS_SUSPENDED;
	status = WebPIAppend(idec, headerdata, WEBP_VP8_HEADER_SIZE);
	do
	{
		src->Read(input, WEBP_IDECODE_BUFFER_SZ, &read);
		if (0 == read)
		{
			success = false;
			break;
		}
		status = WebPIAppend(idec, input, read);
		if (VP8_STATUS_OK != status && VP8_STATUS_SUSPENDED != status)
		{
			success = false;
			break;
		}
	} while (VP8_STATUS_OK != status);
	free(input);
	WebPIDelete(idec);

	if (0 == pitch || glmNormal != mode)
	{
		// Copy from the temporary buffer line by line
		for (int y = 0; y < height; y += 1)
		{
			void *scanline = scanlinecallback(callbackdata, y);
			if (NULL == scanline)
			{
				break;
			}
			if (sizeof(tjs_uint32) == size_pixel)
			{
				memcpy(scanline, (const void*)&outbuf[y * outbufpitch], width * size_pixel);
			}
			else if (sizeof(tjs_uint8) == size_pixel)
			{
				TVPBLConvert32BitTo8Bit((tjs_uint8*)scanline, (const tjs_uint32*)(tjs_uint8*)&outbuf[y * outbufpitch], width * size_pixel);
			}
			scanlinecallback(callbackdata, -1);
		}
		free(outbuf);
	}
#if 1
	else
	{
		// Do any image processing as needed
		for (int y = 0; y < height; y += 1)
		{
			void *scanline = scanlinecallback(callbackdata, y);
			if (NULL == scanline)
			{
				break;
			}
			scanlinecallback(callbackdata, -1);
		}
	}
#endif
	WebPFreeDecBuffer(&(config.output));

	if (false == success)
	{
		TVPThrowExceptionMessage(TJS_W("Unsuccessful decode"));
		return;
	}
}

void TVPLoadHeaderWEBP(void* formatdata, IStream *src, iTJSDispatch2** dic)
{
	STATSTG stg;

	src->Stat(&stg, STATFLAG_NONAME);
	if (stg.cbSize.QuadPart < WEBP_VP8_HEADER_SIZE)
	{
		TVPThrowExceptionMessage(TJS_W("File size is too small"));
		return;
	}

	uint8_t data[WEBP_VP8_HEADER_SIZE];
	ULONG read;
	src->Read(data, WEBP_VP8_HEADER_SIZE, &read);
	if (WEBP_VP8_HEADER_SIZE != read)
	{
		TVPThrowExceptionMessage(TJS_W("Read size is too small"));
		return;
	}

	WebPDecoderConfig config;
	if (WebPInitDecoderConfig(&config) == 0)
	{
		TVPThrowExceptionMessage(TJS_W("Error initializing decoder"));
		return;
	}
	if (WebPGetFeatures(data, WEBP_VP8_HEADER_SIZE, &config.input) != VP8_STATUS_OK)
	{
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
	val = tTJSVariant(config.input.has_alpha ? 1 : 0);
	(*dic)->PropSet(TJS_MEMBERENSURE, TJS_W("has_alpha"), 0, &val, (*dic));
	val = tTJSVariant(config.input.has_animation ? 1 : 0);
	(*dic)->PropSet(TJS_MEMBERENSURE, TJS_W("has_animation"), 0, &val, (*dic));
	val = tTJSVariant(config.input.format);
	(*dic)->PropSet(TJS_MEMBERENSURE, TJS_W("format"), 0, &val, (*dic));
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
