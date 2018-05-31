#include <stdio.h>
#include <time.h> 
#include <unistd.h>

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/log.h"

#include "mediacodec/mediacodec.h"

#ifdef ANDROID
#include <jni.h>
#include <android/log.h>
#define LOGE(format, ...)  __android_log_print(ANDROID_LOG_ERROR, "(>_<)", format, ##__VA_ARGS__)
#define LOGI(format, ...)  __android_log_print(ANDROID_LOG_INFO,  "(^_^)", format, ##__VA_ARGS__)
#else
#define LOGE(format, ...)  printf("(>_<) " format "\n", ##__VA_ARGS__)
#define LOGI(format, ...)  printf("(^_^) " format "\n", ##__VA_ARGS__)
#endif

int decode_cancel = 0;

JNIEXPORT void JNICALL Java_com_example_ffmpegdecoder_activity_MainActivity_decodeCancel
(JNIEnv *env, jobject obj)
{
	decode_cancel = 1;
}

//Output FFmpeg's av_log()
void custom_log(void *ptr, int level, const char* fmt, va_list vl){
	FILE *fp=fopen("/storage/emulated/0/av_log.txt","a+");
	if(fp){
		vfprintf(fp,fmt,vl);
		fflush(fp);
		fclose(fp);
	}
}

JNIEXPORT jint JNICALL Java_com_example_ffmpegdecoder_activity_MainActivity_decodeInfo
		(JNIEnv *env, jobject obj, jobject videoInfo, jstring input_jstr)
{
	jclass objcetClass = (*env)->FindClass(env,"com/example/ffmpegdecoder/bean/VideoInfo");

	jfieldID duration = (*env)->GetFieldID(env, objcetClass, "duration", "D");
	jfieldID bit_rate = (*env)->GetFieldID(env, objcetClass, "bit_rate", "I");
	jfieldID frame_rate = (*env)->GetFieldID(env, objcetClass, "frame_rate", "D");
	jfieldID format_name = (*env)->GetFieldID(env, objcetClass, "format_name", "Ljava/lang/String;");
	jfieldID width = (*env)->GetFieldID(env, objcetClass, "width", "I");
	jfieldID height = (*env)->GetFieldID(env, objcetClass, "height", "I");
	jfieldID codec_name = (*env)->GetFieldID(env, objcetClass, "codec_name", "Ljava/lang/String;");
	jfieldID pixel_format = (*env)->GetFieldID(env, objcetClass, "pixel_format", "Ljava/lang/String;");

	AVFormatContext	*pFormatCtx;
	int				i, videoindex;
	AVCodecContext	*pCodecCtx;
	AVCodec			*pCodec;

	char input_str[256]={0};
	sprintf(input_str,"%s",(*env)->GetStringUTFChars(env,input_jstr, NULL));
	LOGI("Cpp input:%s",input_str);

	av_register_all();
	avformat_network_init();
	pFormatCtx = avformat_alloc_context();
	av_log_set_callback(custom_log);

	if(avformat_open_input(&pFormatCtx,input_str,NULL,NULL)!=0){
		LOGE("Couldn't open input stream.\n");
		return -1;
	}
	if(avformat_find_stream_info(pFormatCtx,NULL)<0){
		LOGE("Couldn't find stream information.\n");
		return -2;
	}
	videoindex=-1;
	for(i=0; i<pFormatCtx->nb_streams; i++){
		if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO){
			videoindex=i;
			break;
		}
	}
	if(videoindex==-1){
		LOGE("Couldn't find a video stream.\n");
		return -3;
	}
	pCodecCtx=pFormatCtx->streams[videoindex]->codec;
	pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
	if(pCodec==NULL){
		LOGE("Couldn't find Codec.\n");
		return -4;
	}

	if(pFormatCtx ->streams[videoindex]->duration<0){
		(*env)->SetDoubleField(env, videoInfo, duration, (double)pFormatCtx ->duration/1000000);
	}
	else{
		(*env)->SetDoubleField(env, videoInfo, duration, (double)pFormatCtx ->streams[videoindex]->duration
														* av_q2d(pFormatCtx ->streams[videoindex]->time_base));
	}
	(*env)->SetIntField(env, videoInfo, bit_rate, pFormatCtx->bit_rate);
	(*env)->SetDoubleField(env, videoInfo, frame_rate, av_q2d(pFormatCtx->streams[videoindex]->r_frame_rate));
	(*env)->SetObjectField(env, videoInfo, format_name, (*env)->NewStringUTF(env,pFormatCtx->iformat->name));
	(*env)->SetIntField(env, videoInfo, width, pCodecCtx->width);
	(*env)->SetIntField(env, videoInfo, height, pCodecCtx->height);
	(*env)->SetObjectField(env, videoInfo, codec_name, (*env)->NewStringUTF(env,pCodec->name));
	switch(pCodecCtx->pix_fmt){
		case AV_PIX_FMT_NONE     : (*env)->SetObjectField(env, videoInfo, pixel_format, (*env)->NewStringUTF(env,"AV_PIX_FMT_NONE")); break;
		case AV_PIX_FMT_YUV420P  : (*env)->SetObjectField(env, videoInfo, pixel_format, (*env)->NewStringUTF(env,"AV_PIX_FMT_YUV420P")); break;
		case AV_PIX_FMT_YUYV422  : (*env)->SetObjectField(env, videoInfo, pixel_format, (*env)->NewStringUTF(env,"AV_PIX_FMT_YUYV422")); break;
		case AV_PIX_FMT_RGB24    : (*env)->SetObjectField(env, videoInfo, pixel_format, (*env)->NewStringUTF(env,"AV_PIX_FMT_RGB24")); break;
		case AV_PIX_FMT_BGR24    : (*env)->SetObjectField(env, videoInfo, pixel_format, (*env)->NewStringUTF(env,"AV_PIX_FMT_BGR24")); break;
		case AV_PIX_FMT_YUV422P  : (*env)->SetObjectField(env, videoInfo, pixel_format, (*env)->NewStringUTF(env,"AV_PIX_FMT_YUV422P")); break;
		case AV_PIX_FMT_YUV444P  : (*env)->SetObjectField(env, videoInfo, pixel_format, (*env)->NewStringUTF(env,"AV_PIX_FMT_YUV444P")); break;
		case AV_PIX_FMT_YUV410P  : (*env)->SetObjectField(env, videoInfo, pixel_format, (*env)->NewStringUTF(env,"AV_PIX_FMT_YUV410P")); break;
		case AV_PIX_FMT_YUV411P  : (*env)->SetObjectField(env, videoInfo, pixel_format, (*env)->NewStringUTF(env,"AV_PIX_FMT_YUV411P")); break;
		case AV_PIX_FMT_GRAY8    : (*env)->SetObjectField(env, videoInfo, pixel_format, (*env)->NewStringUTF(env,"AV_PIX_FMT_GRAY8")); break;
		case AV_PIX_FMT_MONOWHITE: (*env)->SetObjectField(env, videoInfo, pixel_format, (*env)->NewStringUTF(env,"AV_PIX_FMT_MONOWHITE")); break;
		case AV_PIX_FMT_MONOBLACK: (*env)->SetObjectField(env, videoInfo, pixel_format, (*env)->NewStringUTF(env,"AV_PIX_FMT_MONOBLACK")); break;
		case AV_PIX_FMT_PAL8     : (*env)->SetObjectField(env, videoInfo, pixel_format, (*env)->NewStringUTF(env,"AV_PIX_FMT_PAL8")); break;
		case AV_PIX_FMT_YUVJ420P : (*env)->SetObjectField(env, videoInfo, pixel_format, (*env)->NewStringUTF(env,"AV_PIX_FMT_YUVJ420P")); break;
		case AV_PIX_FMT_YUVJ422P : (*env)->SetObjectField(env, videoInfo, pixel_format, (*env)->NewStringUTF(env,"AV_PIX_FMT_YUVJ422P")); break;
		case AV_PIX_FMT_YUVJ444P : (*env)->SetObjectField(env, videoInfo, pixel_format, (*env)->NewStringUTF(env,"AV_PIX_FMT_YUVJ444P")); break;
		case AV_PIX_FMT_UYVY422  : (*env)->SetObjectField(env, videoInfo, pixel_format, (*env)->NewStringUTF(env,"AV_PIX_FMT_UYVY422")); break;
		case AV_PIX_FMT_UYYVYY411: (*env)->SetObjectField(env, videoInfo, pixel_format, (*env)->NewStringUTF(env,"AV_PIX_FMT_UYYVYY411")); break;
		case AV_PIX_FMT_BGR8     : (*env)->SetObjectField(env, videoInfo, pixel_format, (*env)->NewStringUTF(env,"AV_PIX_FMT_BGR8")); break;
		case AV_PIX_FMT_BGR4     : (*env)->SetObjectField(env, videoInfo, pixel_format, (*env)->NewStringUTF(env,"AV_PIX_FMT_BGR4")); break;
		case AV_PIX_FMT_BGR4_BYTE: (*env)->SetObjectField(env, videoInfo, pixel_format, (*env)->NewStringUTF(env,"AV_PIX_FMT_BGR4_BYTE")); break;
		case AV_PIX_FMT_RGB8     : (*env)->SetObjectField(env, videoInfo, pixel_format, (*env)->NewStringUTF(env,"AV_PIX_FMT_RGB8")); break;
		case AV_PIX_FMT_RGB4     : (*env)->SetObjectField(env, videoInfo, pixel_format, (*env)->NewStringUTF(env,"AV_PIX_FMT_RGB4")); break;
		case AV_PIX_FMT_RGB4_BYTE: (*env)->SetObjectField(env, videoInfo, pixel_format, (*env)->NewStringUTF(env,"AV_PIX_FMT_RGB4_BYTE")); break;
		case AV_PIX_FMT_NV12     : (*env)->SetObjectField(env, videoInfo, pixel_format, (*env)->NewStringUTF(env,"AV_PIX_FMT_NV12")); break;
		case AV_PIX_FMT_NV21     : (*env)->SetObjectField(env, videoInfo, pixel_format, (*env)->NewStringUTF(env,"AV_PIX_FMT_NV21")); break;
		case AV_PIX_FMT_ARGB     : (*env)->SetObjectField(env, videoInfo, pixel_format, (*env)->NewStringUTF(env,"AV_PIX_FMT_ARGB")); break;
		case AV_PIX_FMT_RGBA     : (*env)->SetObjectField(env, videoInfo, pixel_format, (*env)->NewStringUTF(env,"AV_PIX_FMT_RGBA")); break;
		case AV_PIX_FMT_ABGR     : (*env)->SetObjectField(env, videoInfo, pixel_format, (*env)->NewStringUTF(env,"AV_PIX_FMT_ABGR")); break;
		case AV_PIX_FMT_BGRA     : (*env)->SetObjectField(env, videoInfo, pixel_format, (*env)->NewStringUTF(env,"AV_PIX_FMT_BGRA")); break;
		default                  : (*env)->SetObjectField(env, videoInfo, pixel_format, (*env)->NewStringUTF(env,"AV_PIX_FMT_NONE")); break;
	}   
//	LOGE("long_name:%s",pFormatCtx->iformat->long_name);
//	LOGE("name:%s",pFormatCtx->iformat->name);

	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);
	return 0;
}

typedef struct MediacodecContext{
	jbyteArray dec_buffer_array;
    jbyteArray dec_yuv_array;
	
	jclass class_Codec;
	jclass class_H264Decoder;
	jclass class_HH264Decoder;
	jclass class_H264Utils;
	jclass class_Integer;
	jclass class_CodecCapabilities;
	
	jmethodID methodID_HH264Decoder_constructor;
	jmethodID methodID_HH264Decoder_config;
	jmethodID methodID_HH264Decoder_getConfig;
	jmethodID methodID_HH264Decoder_open;
	jmethodID methodID_HH264Decoder_decode;
	jmethodID methodID_HH264Decoder_getErrorCode;
	jmethodID methodID_HH264Decoder_close;
	jmethodID methodID_H264Utils_ffAvcFindStartcode;
	jmethodID methodID_Integer_intValue;
	jfieldID fieldID_Codec_ERROR_CODE_INPUT_BUFFER_FAILURE;
	jfieldID fieldID_H264Decoder_KEY_CONFIG_WIDTH;
	jfieldID fieldID_H264Decoder_KEY_CONFIG_HEIGHT;
	jfieldID fieldID_HH264Decoder_KEY_COLOR_FORMAT;
	jfieldID fieldID_HH264Decoder_KEY_MIME;
	jfieldID fieldID_CodecCapabilities_COLOR_FormatYUV420Planar;
	jfieldID fieldID_CodecCapabilities_COLOR_FormatYUV420SemiPlanar;
	
	jint ERROR_CODE_INPUT_BUFFER_FAILURE;
	jobject KEY_CONFIG_WIDTH;
	jobject KEY_CONFIG_HEIGHT;
	jobject KEY_COLOR_FORMAT;
	jint COLOR_FormatYUV420Planar;
	jint COLOR_FormatYUV420SemiPlanar;
	
	jobject object_decoder;
}MediacodecContext;
void mediacodec_decode_video(JNIEnv* env, MediacodecContext* mediacodecContext, AVPacket *packet, AVFrame *pFrame, int *got_picture);
void mediacodec_decode_video2(MediaCodecDecoder* decoder, AVPacket *packet, AVFrame *pFrame, int *got_picture);

JNIEXPORT jint JNICALL Java_com_example_ffmpegdecoder_activity_MainActivity_decode
		(JNIEnv *env, jobject obj, jstring input_jstr, jstring output_jstr, jint codec_type)
{
	jclass objcetClass = (*env)->FindClass(env,"com/example/ffmpegdecoder/activity/MainActivity");
	jmethodID methodID_setProgressRate = (*env)->GetMethodID(env, objcetClass, "setProgressRate", "(I)V");
	jmethodID methodID_setProgressRateFull = (*env)->GetMethodID(env, objcetClass, "setProgressRateFull", "()V");
	jmethodID methodID_setProgressRateEmpty = (*env)->GetMethodID(env, objcetClass, "setProgressRateEmpty", "()V");
	decode_cancel = 0;
	
	AVFormatContext	*pFormatCtx;
	int				i, videoindex;
	AVCodecContext	*pCodecCtx;
	AVCodec			*pCodec;
	AVFrame	*pFrame,*pFrameYUV;
	uint8_t *out_buffer;
	AVPacket *packet;
	AVBitStreamFilterContext* bsfc;
	int y_size;
	int ret, got_picture;
	struct SwsContext *img_convert_ctx;
	FILE *fp_yuv;
	int frame_cnt;
	clock_t time_start, time_finish;
	double  time_duration = 0.0;

	char input_str[256]={0};
	char output_str[256]={0};
	char info[1024]={0};
	sprintf(input_str,"%s",(*env)->GetStringUTFChars(env,input_jstr, NULL));
	sprintf(output_str,"%s",(*env)->GetStringUTFChars(env,output_jstr, NULL));
	LOGI("Cpp input:%s",input_str);
	LOGI("Cpp output:%s",output_str);

	//FFmpeg av_log() callback
	av_log_set_callback(custom_log);

	av_register_all();
	avcodec_register_all();
	avformat_network_init();
	pFormatCtx = avformat_alloc_context();

	if(avformat_open_input(&pFormatCtx,input_str,NULL,NULL)!=0){
		LOGE("Couldn't open input stream.\n");
		return -1;
	}
	if(avformat_find_stream_info(pFormatCtx,NULL)<0){
		LOGE("Couldn't find stream information.\n");
		return -2;
	}
	videoindex=-1;
	for(i=0; i<pFormatCtx->nb_streams; i++){
		if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO){
			videoindex=i;
			break;
		}
	}
	if(videoindex==-1){
		LOGE("Couldn't find a video stream.\n");
		return -3;
	}
	pCodecCtx=pFormatCtx->streams[videoindex]->codec;
	pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
	if(pCodec==NULL){
		LOGE("Couldn't find Codec.\n");
		return -4;
	}
	if(avcodec_open2(pCodecCtx, pCodec,NULL)<0){
		LOGE("Couldn't open codec.\n");
		return -5;
	}

	pFrame=av_frame_alloc();
	pFrameYUV=av_frame_alloc();
	out_buffer=(uint8_t *)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width+16, pCodecCtx->height+16));
	avpicture_fill((AVPicture *)pFrameYUV, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);

	packet=(AVPacket *)av_malloc(sizeof(AVPacket));

	img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
									 pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);


	sprintf(info,   "[Input     ]%s\n", input_str);
	sprintf(info, "%s[Output    ]%s\n",info,output_str);
	sprintf(info, "%s[Format    ]%s\n",info, pFormatCtx->iformat->name);
	sprintf(info, "%s[Codec     ]%s\n",info, pCodecCtx->codec->name);
	sprintf(info, "%s[Resolution]%dx%d\n",info, pCodecCtx->width,pCodecCtx->height);


	fp_yuv=fopen(output_str,"wb+");
	if(fp_yuv==NULL){
		printf("Cannot open output file.\n");
		return -6;
	}
	
	bsfc = av_bitstream_filter_init("h264_mp4toannexb");
	
//------------Mediacodec init----------------		
	// MediacodecContext mediacodecContext;
	
	// mediacodecContext.dec_buffer_array = (*env)->NewByteArray(env, MEDIACODEC_DEC_BUFFER_ARRAY_SIZE);	
	// mediacodecContext.dec_yuv_array = (*env)->NewByteArray(env, MEDIACODEC_DEC_YUV_ARRAY_SIZE);	
	
	// mediacodecContext.class_Codec = (*env)->FindClass(env, "com/example/ffmpegsdlplayer/mediacodec/Codec");
	// mediacodecContext.class_H264Decoder = (*env)->FindClass(env, "com/example/ffmpegsdlplayer/mediacodec/H264Decoder");
	// mediacodecContext.class_HH264Decoder = (*env)->FindClass(env, "com/example/ffmpegsdlplayer/mediacodec/HH264Decoder");
	// mediacodecContext.class_H264Utils = (*env)->FindClass(env, "com/example/ffmpegsdlplayer/mediacodec/H264Utils");
	// mediacodecContext.class_Integer = (*env)->FindClass(env, "java/lang/Integer");
	// mediacodecContext.class_CodecCapabilities = (*env)->FindClass(env,"android/media/MediaCodecInfo$CodecCapabilities");
	
	// mediacodecContext.methodID_HH264Decoder_constructor = (*env)->GetMethodID(env,mediacodecContext.class_HH264Decoder,"<init>","()V");
	// mediacodecContext.methodID_HH264Decoder_config = (*env)->GetMethodID(env,mediacodecContext.class_HH264Decoder,"config","(Ljava/lang/String;Ljava/lang/Object;)V");
	// mediacodecContext.methodID_HH264Decoder_getConfig = (*env)->GetMethodID(env,mediacodecContext.class_HH264Decoder,"getConfig","(Ljava/lang/String;)Ljava/lang/Object;");
	// mediacodecContext.methodID_HH264Decoder_open = (*env)->GetMethodID(env,mediacodecContext.class_HH264Decoder,"open","()V");
	// mediacodecContext.methodID_HH264Decoder_decode = (*env)->GetMethodID(env,mediacodecContext.class_HH264Decoder,"decode","([BI[BI)I");
	// mediacodecContext.methodID_HH264Decoder_getErrorCode = (*env)->GetMethodID(env,mediacodecContext.class_HH264Decoder,"getErrorCode","()I");
	// mediacodecContext.methodID_HH264Decoder_close = (*env)->GetMethodID(env,mediacodecContext.class_HH264Decoder,"close","()V");
	// mediacodecContext.methodID_H264Utils_ffAvcFindStartcode = (*env)->GetStaticMethodID(env,mediacodecContext.class_H264Utils,"ffAvcFindStartcode","([BII)I");
	// mediacodecContext.methodID_Integer_intValue = (*env)->GetMethodID(env,mediacodecContext.class_Integer,"intValue","()I");
	// mediacodecContext.fieldID_Codec_ERROR_CODE_INPUT_BUFFER_FAILURE = (*env)->GetStaticFieldID(env, mediacodecContext.class_Codec, "ERROR_CODE_INPUT_BUFFER_FAILURE", "I");
	// mediacodecContext.fieldID_H264Decoder_KEY_CONFIG_WIDTH = (*env)->GetStaticFieldID(env, mediacodecContext.class_H264Decoder, "KEY_CONFIG_WIDTH", "Ljava/lang/String;");
	// mediacodecContext.fieldID_H264Decoder_KEY_CONFIG_HEIGHT = (*env)->GetStaticFieldID(env, mediacodecContext.class_H264Decoder, "KEY_CONFIG_HEIGHT", "Ljava/lang/String;");
	// mediacodecContext.fieldID_HH264Decoder_KEY_COLOR_FORMAT = (*env)->GetStaticFieldID(env, mediacodecContext.class_HH264Decoder, "KEY_COLOR_FORMAT", "Ljava/lang/String;");
	// mediacodecContext.fieldID_HH264Decoder_KEY_MIME = (*env)->GetStaticFieldID(env, mediacodecContext.class_HH264Decoder, "KEY_MIME", "Ljava/lang/String;");
	// mediacodecContext.fieldID_CodecCapabilities_COLOR_FormatYUV420Planar = (*env)->GetStaticFieldID(env,  mediacodecContext.class_CodecCapabilities, "COLOR_FormatYUV420Planar", "I");
	// mediacodecContext.fieldID_CodecCapabilities_COLOR_FormatYUV420SemiPlanar = (*env)->GetStaticFieldID(env,  mediacodecContext.class_CodecCapabilities, "COLOR_FormatYUV420SemiPlanar", "I");
	
	// mediacodecContext.ERROR_CODE_INPUT_BUFFER_FAILURE = (*env)->GetStaticIntField(env, mediacodecContext.class_Codec, mediacodecContext.fieldID_Codec_ERROR_CODE_INPUT_BUFFER_FAILURE);
	// mediacodecContext.KEY_CONFIG_WIDTH = (*env)->GetStaticObjectField(env, mediacodecContext.class_H264Decoder, mediacodecContext.fieldID_H264Decoder_KEY_CONFIG_WIDTH);
	// mediacodecContext.KEY_CONFIG_HEIGHT = (*env)->GetStaticObjectField(env, mediacodecContext.class_H264Decoder, mediacodecContext.fieldID_H264Decoder_KEY_CONFIG_HEIGHT);
	// mediacodecContext.KEY_COLOR_FORMAT = (*env)->GetStaticObjectField(env, mediacodecContext.class_HH264Decoder, mediacodecContext.fieldID_HH264Decoder_KEY_COLOR_FORMAT);
	// mediacodecContext.COLOR_FormatYUV420Planar = (*env)->GetStaticIntField(env, mediacodecContext.class_CodecCapabilities, mediacodecContext.fieldID_CodecCapabilities_COLOR_FormatYUV420Planar);
	// mediacodecContext.COLOR_FormatYUV420SemiPlanar = (*env)->GetStaticIntField(env, mediacodecContext.class_CodecCapabilities, mediacodecContext.fieldID_CodecCapabilities_COLOR_FormatYUV420SemiPlanar);
	
	// mediacodecContext.object_decoder = (*env)->NewObject(env, mediacodecContext.class_HH264Decoder, mediacodecContext.methodID_HH264Decoder_constructor);
	// (*env)->CallVoidMethod(env, mediacodecContext.object_decoder, mediacodecContext.methodID_HH264Decoder_open);
	
	
	MediaCodecDecoder* mediacodec_decoder = mediacodec_decoder_alloc1(1,0,NV12);
	mediacodec_decoder_open(mediacodec_decoder);

//------------Mediacadec----------------------

	frame_cnt=0;
	while(av_read_frame(pFormatCtx, packet)>=0 && decode_cancel == 0){
		if(packet->stream_index==videoindex){
			if(codec_type == 0){
				ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
				if(ret < 0){
					LOGE("Decode Error.\n");
					continue;
					// return -1;
				}
				if(got_picture){
					sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
							  pFrameYUV->data, pFrameYUV->linesize);

					y_size=pCodecCtx->width*pCodecCtx->height;
					fwrite(pFrameYUV->data[0],1,y_size,fp_yuv);    //Y 
					fwrite(pFrameYUV->data[1],1,y_size/4,fp_yuv);  //U
					fwrite(pFrameYUV->data[2],1,y_size/4,fp_yuv);  //V
					//Output info
					char pictype_str[10]={0};
					switch(pFrame->pict_type){
						case AV_PICTURE_TYPE_I:sprintf(pictype_str,"I");break;
						case AV_PICTURE_TYPE_P:sprintf(pictype_str,"P");break;
						case AV_PICTURE_TYPE_B:sprintf(pictype_str,"B");break;
						default:sprintf(pictype_str,"Other");break;
					}

					LOGI("Frame Index: %5d. Type:%s",frame_cnt,pictype_str);
					(*env)->CallVoidMethod(env, obj, methodID_setProgressRate, frame_cnt);

					frame_cnt++;
				}
			}
			else if(codec_type == 1){
				if(packet->size > 4){
					int nalu_type = *(packet->data + 4) & 0x1F;
					LOGI("before[DTS:%lld]:nalu_first=%0X %0X %0X %0X %0X\t nalu_type=%0X", packet->dts, *(packet->data), *(packet->data+1), *(packet->data+2), *(packet->data+3), *(packet->data+4), nalu_type);
				}
				
				av_bitstream_filter_filter(bsfc, pCodecCtx, NULL, &packet->data, &packet->size, packet->data, packet->size, 0);
					
				if(packet->size > 4){
					int nalu_type = *(packet->data + 4) & 0x1F;
					LOGI("after[DTS:%lld]:nalu_first=%0X %0X %0X %0X %0X\t nalu_type=%0X", packet->dts, *(packet->data), *(packet->data+1), *(packet->data+2), *(packet->data+3), *(packet->data+4), nalu_type);
				}
				
				// mediacodec_decode_video(env, &mediacodecContext, packet, pFrameYUV, &got_picture);
				mediacodec_decode_video2(mediacodec_decoder, packet, pFrameYUV, &got_picture);
				if(got_picture)
				{
					y_size=pCodecCtx->width*pCodecCtx->height;
					fwrite(pFrameYUV->data[0],1,y_size,fp_yuv);    //Y 
					fwrite(pFrameYUV->data[1],1,y_size/4,fp_yuv);  //U
					fwrite(pFrameYUV->data[2],1,y_size/4,fp_yuv);  //V

					LOGI("Frame Index: %5d",frame_cnt);
					(*env)->CallVoidMethod(env, obj, methodID_setProgressRate, frame_cnt);

					frame_cnt++;
				}
			}
		}
		av_free_packet(packet);
	}
	//flush decoder
	//FIX: Flush Frames remained in Codec
	while (decode_cancel == 0) {
		if(codec_type == 0){
			ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
			if (ret < 0)
				break;
			if (!got_picture)
				break;
			sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
					  pFrameYUV->data, pFrameYUV->linesize);
			int y_size=pCodecCtx->width*pCodecCtx->height;
			fwrite(pFrameYUV->data[0],1,y_size,fp_yuv);    //Y 
			fwrite(pFrameYUV->data[1],1,y_size/4,fp_yuv);  //U
			fwrite(pFrameYUV->data[2],1,y_size/4,fp_yuv);  //V
			//Output info
			char pictype_str[10]={0};
			switch(pFrame->pict_type){
				case AV_PICTURE_TYPE_I:sprintf(pictype_str,"I");break;
				case AV_PICTURE_TYPE_P:sprintf(pictype_str,"P");break;
				case AV_PICTURE_TYPE_B:sprintf(pictype_str,"B");break;
				default:sprintf(pictype_str,"Other");break;
			}
			LOGI("[flush]Frame Index: %5d. Type:%s",frame_cnt,pictype_str);

			(*env)->CallVoidMethod(env, obj, methodID_setProgressRate, frame_cnt);

			frame_cnt++;
		}
		else if(codec_type == 1){
			if(packet->size > 4){
				int nalu_type = *(packet->data + 4) & 0x1F;
				LOGI("before[DTS:%lld]:nalu_first=%0X %0X %0X %0X %0X\t nalu_type=%0X", packet->dts, *(packet->data), *(packet->data+1), *(packet->data+2), *(packet->data+3), *(packet->data+4), nalu_type);
			}
			
			av_bitstream_filter_filter(bsfc, pCodecCtx, NULL, &packet->data, &packet->size, packet->data, packet->size, 0);
				
			if(packet->size > 4){
				int nalu_type = *(packet->data + 4) & 0x1F;
				LOGI("after[DTS:%lld]:nalu_first=%0X %0X %0X %0X %0X\t nalu_type=%0X", packet->dts, *(packet->data), *(packet->data+1), *(packet->data+2), *(packet->data+3), *(packet->data+4), nalu_type);
			}
			
			// mediacodec_decode_video(env, &mediacodecContext, packet, pFrameYUV, &got_picture);
			mediacodec_decode_video2(mediacodec_decoder, packet, pFrameYUV, &got_picture);
			
			if (!got_picture)
				break;
			
			y_size=pCodecCtx->width*pCodecCtx->height;
			fwrite(pFrameYUV->data[0],1,y_size,fp_yuv);    //Y 
			fwrite(pFrameYUV->data[1],1,y_size/4,fp_yuv);  //U
			fwrite(pFrameYUV->data[2],1,y_size/4,fp_yuv);  //V

			LOGI("Frame Index: %5d",frame_cnt);
			(*env)->CallVoidMethod(env, obj, methodID_setProgressRate, frame_cnt);

			frame_cnt++;
		}
	}
	time_finish = clock();
	time_duration=(double)(time_finish - time_start);

	sprintf(info, "%s[Time      ]%fms\n",info,time_duration);
	sprintf(info, "%s[Count     ]%d\n",info,frame_cnt);

	if(decode_cancel == 0){
		LOGI("Decoder Finish!!!");
		(*env)->CallVoidMethod(env, obj, methodID_setProgressRateFull);
	}
	else{
		LOGI("Decoder Cancel!!!");
		(*env)->CallVoidMethod(env, obj, methodID_setProgressRateEmpty);
	}

	sws_freeContext(img_convert_ctx);

	fclose(fp_yuv);

	av_frame_free(&pFrameYUV);
	av_frame_free(&pFrame);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);
	av_bitstream_filter_close(bsfc);
	decode_cancel = 0;
	
	// (*env)->DeleteLocalRef(env, mediacodecContext.dec_yuv_array);
	// (*env)->DeleteLocalRef(env, mediacodecContext.dec_buffer_array);
	
	// (*env)->DeleteLocalRef(env, mediacodecContext.class_Codec);
	// (*env)->DeleteLocalRef(env, mediacodecContext.class_H264Decoder);
	// (*env)->DeleteLocalRef(env, mediacodecContext.class_HH264Decoder);
	// (*env)->DeleteLocalRef(env, mediacodecContext.class_H264Utils);
	// (*env)->DeleteLocalRef(env, mediacodecContext.class_Integer);
	// (*env)->DeleteLocalRef(env, mediacodecContext.class_CodecCapabilities);
	
	// (*env)->CallVoidMethod(env, mediacodecContext.object_decoder, mediacodecContext.methodID_HH264Decoder_close);
	// (*env)->DeleteLocalRef(env, mediacodecContext.object_decoder);
	// (*env)->DeleteLocalRef(env, mediacodecContext.KEY_CONFIG_WIDTH);
	// (*env)->DeleteLocalRef(env, mediacodecContext.KEY_CONFIG_HEIGHT);
	// (*env)->DeleteLocalRef(env, mediacodecContext.KEY_COLOR_FORMAT);
	
	mediacodec_decoder_close(mediacodec_decoder);
	mediacodec_decoder_free(mediacodec_decoder);
	
	return 0;
}

void mediacodec_decode_video(JNIEnv* env, MediacodecContext* mediacodecContext, AVPacket *packet, AVFrame *pFrame, int *got_picture){
	uint8_t *in,*out;
	int in_len = packet->size;
	int out_len = 0;
	in = packet->data;
	out = pFrame->data[0];
	int repeat_count = 0;
	jint jyuv_len,
		 jerror_code,
		 jyuv_wdith,
		 jyuv_height,
		 jyuv_pixel;
		 
	(*env)->SetByteArrayRegion(env, mediacodecContext->dec_buffer_array, 0, in_len, (jbyte*)in);
		 
	while(1){
		jyuv_len = (*env)->CallIntMethod(env,mediacodecContext->object_decoder,mediacodecContext->methodID_HH264Decoder_decode,mediacodecContext->dec_buffer_array,0,mediacodecContext->dec_yuv_array,in_len);
					
		jerror_code = (*env)->CallIntMethod(env,mediacodecContext->object_decoder,mediacodecContext->methodID_HH264Decoder_getErrorCode);
		LOGI("yuv_len:%6d\t error_code:%6d", jyuv_len,jerror_code);
		
		if(jerror_code == mediacodecContext->ERROR_CODE_INPUT_BUFFER_FAILURE){
			if(repeat_count < 5){
				repeat_count++;
				usleep(1000);
				continue;
			}
			else{
				repeat_count = 0;
			}
		}
		
		if(jyuv_len > 0){
			jobject yuv_wdith = (*env)->CallObjectMethod(env, mediacodecContext->object_decoder, mediacodecContext->methodID_HH264Decoder_getConfig, mediacodecContext->KEY_CONFIG_WIDTH);
			jobject yuv_height = (*env)->CallObjectMethod(env, mediacodecContext->object_decoder, mediacodecContext->methodID_HH264Decoder_getConfig, mediacodecContext->KEY_CONFIG_HEIGHT);
			jobject yuv_pixel = (*env)->CallObjectMethod(env, mediacodecContext->object_decoder, mediacodecContext->methodID_HH264Decoder_getConfig, mediacodecContext->KEY_COLOR_FORMAT);
			jyuv_wdith = (*env)->CallIntMethod(env, yuv_wdith, mediacodecContext->methodID_Integer_intValue);
			jyuv_height = (*env)->CallIntMethod(env, yuv_height, mediacodecContext->methodID_Integer_intValue);
			jyuv_pixel = (*env)->CallIntMethod(env, yuv_pixel, mediacodecContext->methodID_Integer_intValue);
			LOGI("W x H : %d x %d\t yuv_pixel:%6d", jyuv_wdith,jyuv_height,jyuv_pixel);
			
			(*env)->DeleteLocalRef(env,yuv_wdith);
			(*env)->DeleteLocalRef(env,yuv_height);
			(*env)->DeleteLocalRef(env,yuv_pixel);
			
			(*env)->GetByteArrayRegion(env, mediacodecContext->dec_yuv_array, 0, jyuv_len, (jbyte*)out);
			out_len = jyuv_len;
		}
		break;
	}
	
	if(out_len > 0){
		*got_picture = 1;
	}
	else{
		*got_picture = 0;
	}
}

void mediacodec_decode_video2(MediaCodecDecoder* decoder, AVPacket *packet, AVFrame *pFrame, int *got_picture){
	uint8_t *in,*out;
	int in_len = packet->size;
	int out_len = 0;
	in = packet->data;
	out = pFrame->data[0];
	int repeat_count = 0;
	int jyuv_len,
		jerror_code,
		jyuv_wdith,
		jyuv_height,
		jyuv_pixel;
		 
	while(1){
		jyuv_len = mediacodec_decoder_decode(decoder, in, 0, out, in_len, &jerror_code);
		LOGI("yuv_len:%6d\t error_code:%6d", jyuv_len,jerror_code);
		
		if(jerror_code){
			LOGE("error_code:%d TIME_OUT:%d repeat_count:%d",jerror_code,mediacodec_decoder_getConfig_int(decoder, "timeout"), repeat_count);
			if(repeat_count < 5){
				repeat_count++;
				usleep(100);
				if(mediacodec_decoder_getConfig_int(decoder, "timeout") < mediacodec_decoder_getConfig_int(decoder, "max-timeout")){
					mediacodec_decoder_setConfig_int(decoder, "timeout", mediacodec_decoder_getConfig_int(decoder, "timeout")+200);
				}
				else{
					mediacodec_decoder_setConfig_int(decoder, "timeout", mediacodec_decoder_getConfig_int(decoder, "max-timeout"));
				}
				continue;
			}
			else{
				repeat_count = 0;
			}
		}
		
		if(jerror_code <= -10000){
			LOGE("硬件编解码器损坏，请更换编解码器");
			// thread_codec_type = 0;
		}
		
		if(jyuv_len > 0){
			jyuv_wdith = mediacodec_decoder_getConfig_int(decoder, "width");
			jyuv_height = mediacodec_decoder_getConfig_int(decoder, "height");;
			jyuv_pixel = mediacodec_decoder_getConfig_int(decoder, "color-format");
			LOGI("W x H : %d x %d\t yuv_pixel:%6d", jyuv_wdith,jyuv_height,jyuv_pixel);
			
			pFrame->width = jyuv_wdith;
			pFrame->height = jyuv_height;
			out_len = jyuv_len;
		}
		break;
	}
	
	if(out_len > 0){
		*got_picture = 1;
	}
	else{
		*got_picture = 0;
	}
}