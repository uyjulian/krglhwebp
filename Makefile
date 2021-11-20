
WEBP_SOURCES += external/libwebp/src/dec/alpha_dec.c external/libwebp/src/dec/buffer_dec.c external/libwebp/src/dec/frame_dec.c external/libwebp/src/dec/idec_dec.c external/libwebp/src/dec/io_dec.c external/libwebp/src/dec/quant_dec.c external/libwebp/src/dec/tree_dec.c external/libwebp/src/dec/vp8_dec.c external/libwebp/src/dec/vp8l_dec.c external/libwebp/src/dec/webp_dec.c external/libwebp/src/dsp/alpha_processing.c external/libwebp/src/dsp/cpu.c external/libwebp/src/dsp/dec.c external/libwebp/src/dsp/dec_clip_tables.c external/libwebp/src/dsp/filters.c external/libwebp/src/dsp/lossless.c external/libwebp/src/dsp/rescaler.c external/libwebp/src/dsp/upsampling.c external/libwebp/src/dsp/yuv.c external/libwebp/src/dsp/cost.c external/libwebp/src/dsp/enc.c external/libwebp/src/dsp/lossless_enc.c external/libwebp/src/dsp/ssim.c external/libwebp/src/enc/alpha_enc.c external/libwebp/src/enc/analysis_enc.c external/libwebp/src/enc/backward_references_cost_enc.c external/libwebp/src/enc/backward_references_enc.c external/libwebp/src/enc/config_enc.c external/libwebp/src/enc/cost_enc.c external/libwebp/src/enc/filter_enc.c external/libwebp/src/enc/frame_enc.c external/libwebp/src/enc/histogram_enc.c external/libwebp/src/enc/iterator_enc.c external/libwebp/src/enc/near_lossless_enc.c external/libwebp/src/enc/picture_enc.c external/libwebp/src/enc/picture_csp_enc.c external/libwebp/src/enc/picture_psnr_enc.c external/libwebp/src/enc/picture_rescale_enc.c external/libwebp/src/enc/picture_tools_enc.c external/libwebp/src/enc/predictor_enc.c external/libwebp/src/enc/quant_enc.c external/libwebp/src/enc/syntax_enc.c external/libwebp/src/enc/token_enc.c external/libwebp/src/enc/tree_enc.c external/libwebp/src/enc/vp8l_enc.c external/libwebp/src/enc/webp_enc.c external/libwebp/src/demux/anim_decode.c external/libwebp/src/demux/demux.c external/libwebp/src/mux/anim_encode.c external/libwebp/src/mux/muxedit.c external/libwebp/src/mux/muxinternal.c external/libwebp/src/mux/muxread.c external/libwebp/src/utils/bit_reader_utils.c external/libwebp/src/utils/color_cache_utils.c external/libwebp/src/utils/filters_utils.c external/libwebp/src/utils/huffman_utils.c external/libwebp/src/utils/quant_levels_dec_utils.c external/libwebp/src/utils/rescaler_utils.c external/libwebp/src/utils/random_utils.c external/libwebp/src/utils/thread_utils.c external/libwebp/src/utils/utils.c external/libwebp/src/utils/bit_writer_utils.c external/libwebp/src/utils/huffman_encode_utils.c external/libwebp/src/utils/quant_levels_utils.c
WEBP_SSE2_SOURCES += external/libwebp/src/dsp/alpha_processing_sse2.c external/libwebp/src/dsp/dec_sse2.c external/libwebp/src/dsp/filters_sse2.c external/libwebp/src/dsp/lossless_sse2.c external/libwebp/src/dsp/rescaler_sse2.c external/libwebp/src/dsp/upsampling_sse2.c external/libwebp/src/dsp/yuv_sse2.c external/libwebp/src/dsp/cost_sse2.c external/libwebp/src/dsp/enc_sse2.c external/libwebp/src/dsp/lossless_enc_sse2.c external/libwebp/src/dsp/ssim_sse2.c
WEBP_SSE41_SOURCES += external/libwebp/src/dsp/alpha_processing_sse41.c external/libwebp/src/dsp/dec_sse41.c external/libwebp/src/dsp/upsampling_sse41.c external/libwebp/src/dsp/yuv_sse41.c external/libwebp/src/dsp/enc_sse41.c external/libwebp/src/dsp/lossless_enc_sse41.c

CFLAGS += -DWEBP_HAVE_SSE2 -DWEBP_HAVE_SSE41

SOURCES += dllmain.cpp $(WEBP_SOURCES) $(WEBP_SSE2_SOURCES) $(WEBP_SSE41_SOURCES)

INCFLAGS += -Iexternal/libwebp -Iexternal/libwebp/src

PROJECT_BASENAME = krglhwebp

RC_FILEDESCRIPTION = WebP support for TVP(KIRIKIRI) Z
RC_LEGALCOPYRIGHT = Copyright (C) 2019-2019 Julian Uy; See details of license at license.txt, or the source code location.
RC_PRODUCTNAME = WebP support for TVP(KIRIKIRI) Z

include external/tp_stubz/Rules.lib.make

$(WEBP_SSE2_SOURCES:.c=.o): CFLAGS += -msse2
$(WEBP_SSE41_SOURCES:.c=.o): CFLAGS += -msse4.1
