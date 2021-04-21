/* $Id: config.h 5546 2017-01-25 04:15:11Z nanang $ */
/* 
 * Copyright (C) 2008-2011 Teluu Inc. (http://www.teluu.com)
 * Copyright (C) 2003-2008 Benny Prijono <benny@prijono.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */
#ifndef __PJMEDIA_AUDIODEV_CONFIG_H__
#define __PJMEDIA_AUDIODEV_CONFIG_H__

/**
 * @file config.h
 * @brief 音频配置
 */
#include <pjmedia/types.h>
#include <pj/pool.h>


PJ_BEGIN_DECL

/**
 * @defgroup 音频设备API
 * @brief PJMEDIA 音频设备抽象API
 */

/**
 * @defgroup 编译时配置
 * @ingroup 音频设备API
 * @brief 编译时配置
 * @{
 */

/**
 * 此设置控制音频设备名称长度
 * Default: wins 128，其他 64
 */
#ifndef PJMEDIA_AUD_DEV_INFO_NAME_LEN
#   if (defined(PJ_WIN32) && PJ_WIN32!=0) || \
       (defined(PJ_WIN64) && PJ_WIN64!=0)
#	define PJMEDIA_AUD_DEV_INFO_NAME_LEN 128
#   else
#	define PJMEDIA_AUD_DEV_INFO_NAME_LEN 64
#   endif
#endif

/**
 * 此设置标识是否支持端口音频
 * 默认不支持
 */
#ifndef PJMEDIA_AUDIO_DEV_HAS_PORTAUDIO
#   define PJMEDIA_AUDIO_DEV_HAS_PORTAUDIO	0
#endif

/**
 * 此设置标识是否支持Opensl ES
 */
#ifndef PJMEDIA_AUDIO_DEV_HAS_OPENSL
#   define PJMEDIA_AUDIO_DEV_HAS_OPENSL		0
#endif

/**
 * 此设置标识是否支持 Android JNI
 */
#ifndef PJMEDIA_AUDIO_DEV_HAS_ANDROID_JNI
#   define PJMEDIA_AUDIO_DEV_HAS_ANDROID_JNI    PJ_ANDROID
#endif

/**
 * 此设置标识是否支持黑莓音频(BlackBerry 10---BB10)
 */
#ifndef PJMEDIA_AUDIO_DEV_HAS_BB10
#   define PJMEDIA_AUDIO_DEV_HAS_BB10		0
#endif

/**
 * 标识是否支持原生 ALSA 音频
 */
#ifndef PJMEDIA_AUDIO_DEV_HAS_ALSA
#   define PJMEDIA_AUDIO_DEV_HAS_ALSA		0
#endif


/**
 * 标识是否支持无音频
 */
#ifndef PJMEDIA_AUDIO_DEV_HAS_NULL_AUDIO
#   define PJMEDIA_AUDIO_DEV_HAS_NULL_AUDIO	0
#endif


/**
 * 此标识是否支持 coreaudio （mac or iphone）
 */
#ifndef PJMEDIA_AUDIO_DEV_HAS_COREAUDIO
#   define PJMEDIA_AUDIO_DEV_HAS_COREAUDIO	0
#endif


 /**
  * 标识是否支持 WMME （Windows Mobile）
  */
#ifndef PJMEDIA_AUDIO_DEV_HAS_WMME
#  if (defined(PJ_WIN32_UWP) && PJ_WIN32_UWP!=0) || \
      (defined(PJ_WIN32_WINPHONE8) && PJ_WIN32_WINPHONE8!=0)
#    define PJMEDIA_AUDIO_DEV_HAS_WMME		0
#  else
#    define PJMEDIA_AUDIO_DEV_HAS_WMME		1
#  endif
#endif


 /**
  * 此标识为是否支持 Wins 音频会话API（Windows Audio Session API）
  */
#ifndef PJMEDIA_AUDIO_DEV_HAS_WASAPI
#  if (defined(PJ_WIN32_UWP) && PJ_WIN32_UWP!=0) || \
      (defined(PJ_WIN32_WINPHONE8) && PJ_WIN32_WINPHONE8!=0)
#    define PJMEDIA_AUDIO_DEV_HAS_WASAPI	1
#  else
#    define PJMEDIA_AUDIO_DEV_HAS_WASAPI	0
#  endif
#endif


 /**
  * 此标识为是否支持 BDIMAD
 */
#ifndef PJMEDIA_AUDIO_DEV_HAS_BDIMAD
#	define PJMEDIA_AUDIO_DEV_HAS_BDIMAD	0
#endif


/**
 * This setting controls whether Symbian APS
 * 此标识为是否支持 Symbian APS
 */
#ifndef PJMEDIA_AUDIO_DEV_HAS_SYMB_APS
#   define PJMEDIA_AUDIO_DEV_HAS_SYMB_APS	0
#endif


/**
 * 
 * Default: 1 (minimal codec detection)
 * 此设置控制 Symbian APS是否应在其工厂初始化中执行编解码器检测。请注意，检测编解码器可能需要几秒钟，
 * 而检测更多编解码器则需要更多时间。可能的值为：
 *  0 - 不检测编解码器，所有APS编解码器（AMR-NB、G.711、G.729和iLBC）将假定为受支持。
 *  1 - 最小编解码检测，即：仅检测AMR-NB和G.711（当G.711支持/不支持时，G.729和iLBC被认为是支持/不支持的）
 *  2 - 全编解码器检测，即：检测AMR-NB、G.711、G.729和iLBC
 *
 */
#ifndef PJMEDIA_AUDIO_DEV_SYMB_APS_DETECTS_CODEC
#   define PJMEDIA_AUDIO_DEV_SYMB_APS_DETECTS_CODEC 1
#endif


/**
 * 此设置标识是否支持 Symbian VAS
 */
#ifndef PJMEDIA_AUDIO_DEV_HAS_SYMB_VAS
#   define PJMEDIA_AUDIO_DEV_HAS_SYMB_VAS	0
#endif

/**
 * 此标识为 Symbian VAS 使用版本，仅限于 1（VAS 1.0）和 2（VAS 2.0）
 */
#ifndef PJMEDIA_AUDIO_DEV_SYMB_VAS_VERSION
#   define PJMEDIA_AUDIO_DEV_SYMB_VAS_VERSION	1
#endif


/**
 * 此标识为是否支持 Symbian 音频(内建多媒体框架)
 */
#ifndef PJMEDIA_AUDIO_DEV_HAS_SYMB_MDA
#   define PJMEDIA_AUDIO_DEV_HAS_SYMB_MDA	PJ_SYMBIAN
#endif


/**
 *
 * 此设置控制是否同步启动带有内置多媒体框架后端的 Symbian音频。
 * 同步启动将阻止应用程序/UI，例如：N95上每个方向约40毫秒。
 * 而异步启动可能会导致输入/输出卷查询中返回无效值（始终为零），如果在内部启动过程未完全完成时执行查询。
 *
 * 默认值为 1，标识同步启动
 *
 */
#ifndef PJMEDIA_AUDIO_DEV_MDA_USE_SYNC_START
#   define PJMEDIA_AUDIO_DEV_MDA_USE_SYNC_START	1
#endif


/**
 *
 * 此设置控制音频设备API是否应支持基于旧声音设备API（sound.h）的设备实现。
 * 在以下情况下启用此API：
 *  -您已经使用旧的声音设备API（sound.h）实现了自己的声音设备，并且
 *  -您希望能够使用新的音频设备API使用您的声音设备实现。
 *
 */
#ifndef PJMEDIA_AUDIO_DEV_HAS_LEGACY_DEVICE
#   define PJMEDIA_AUDIO_DEV_HAS_LEGACY_DEVICE	0
#endif

PJ_END_DECL


#endif	/* __PJMEDIA_AUDIODEV_CONFIG_H__ */

/*
 --------------------- 更多文档 ---------------------------
 */

/**
 * @addtogroup 音频设备API
 * @{

 PJMedia 是一个跨平台的音频API，适用于 VoIP 应用程序和许多其他类型的音频流应用程序。

 API在各种平台上抽象了许多不同的音频API，例如：
    - 用于Win32、Windows Mobile、Linux、Unix和MacOS X的PortAudio后端。
    - Win32和Windows Mobile设备的本机WMME音频
    - 原生Symbian音频流/多媒体框架（MMF）实现
    - 本机诺基亚音频代理服务器（APS）实现
    - 空音频实现

 该音频API/库是从 PJMEDIA @ref PJMED_SND 演变而来，并且包含了许多增强功能：

 - 正向兼容
 新的API被设计为可扩展的，它将支持新的API以及将来可能引入的新特性，而不会破坏与使用该API
 的应用程序的兼容性以及与现有设备实现的兼容性。

 - 设备能力
    API的核心是设备能力管理，其中音频设备的所有可能的音频能力都应该能够以通用的方式处理。
    有了这个框架，将来可能发现的新功能就可以在不破坏现有应用程序的情况下进行处理。

 - 内置功能
   设备功能框架使应用程序能够使用和控制设备内置的音频功能，例如：
    -回声消除
    -内置编解码器
    -音频路由（例如到听筒或扬声器）
    -音量控制
    -等等

 - 编解码支持
   一些音频设备，如Nokia/Symbian audio Proxy Server（APS）和Nokia VoIP audio Services（VAS），
   支持内置硬件音频编解码器（如g.729、iLBC和AMR），应用程序可以在编码模式下使用声音设备来使用这些硬件编解码器。

 - 多后端
    新的API支持同时激活多个音频后端（在代码中称为工厂或驱动程序），并且可以在运行时添加或删除音频后端。


@section API使用概述

@subsection 开始

 - 配置应用程序设置
    添加如下，包括：
    - 代码
        #include <pjmedia_audiodev.h>
        指定链接库文件 pjmedia-audiodev

    - 编译时设置
        通过编译时设置来开启或关闭指定的音频设备，更多的信息，请看@ref s1_audio_device_config

    - API 初始化和销毁
        初始化 API:
            pjmedia_aud_subsys_init(pf)
        销毁 API:
            pjmedia_aud_subsys_shutdown()

@subsection 使用设备

 -  打印出当前系统检测到的音频设备
    代码：
        int dev_count;
        pjmedia_aud_dev_index dev_idx;
        pj_status_t status;
        dev_count = pjmedia_aud_dev_count();
        printf("Got %d audio devices\n",dev_count);
        for(dev_idx = 0;dev_idx < dev_count;++i)
        {
            pjmedia_aud_dev_info info;
            status = pjmedia_aud_dev_get_info(dev_idx,&info);
            printf("%d. %s (in=%d,out=%d)\n",dev_idx,info.name,info.input_count,info.output_count);
        }

 -  信息：
        PJMEDIA_AUD_DEFAULT_CAPTURE_DEV 和 PJMEDIA_AUD_DEFAULT_PLAYBACK_DEV 常量用于表示默认的捕获和回放设备

 -  信息：
        可以在应用程序设置中保存设备和驱动程序的名称，例如指定应用程序要使用的首选设备。然后可以通过调用以下命令检索设备的设备索引：

        const char *drv_name = "WMME";
        const char *dev_name = "Wave mapper";
        pjmedia_aud_dev_index dev_idx;
        status = pjmedia_aud_dev_lookup(drv_name.dev_name.&dev_idx);
        if(status == PJ_SUCCESS)
        {
            printf("Device index is %d\n",dev_idx);
        }

@subsection 设备能力捕获
    能力通过 pjmedia_aud_dev_cap 枚举进行编码。更多信息详见 pjmedia_aud_dev_cap 枚举
    - 打印设备支持的功能：
        pjmedia_aud_dev_info info;
        pj_status_t status;

        status = pjmedia_aud_dev_get_info(PJMEDIA_AUD_DEFAULT_CAPTURE_DEV,&info);
        if(status == PJ_SUCCESS)
        {
            unsigned i;
            //枚举能力位
            printf("Device capabilities: ");
            for(i=0;i<32;i++)
            {
                if(info.caps & (1<<i))
                    printf("%s ",pjmedia_aud_dev_cap_name(1 << i,NULL));
            }
        }

    - 当打开音频流，即调用 pjmedia_aud_stream_create(),可以设置音频参数，
    根据标识和 pjmedia_aud_param 的相关设置
    - 运行中的音频可通过 pjmedia_aud_stream_get_cap() 函数检索当前音频设置和通过 pjmedia_aud_stream_set_cap() 来改变音频设置


@subsection 创建音频流
    音频流使音频流能够捕获设备、回放设备或两者。
    - 建议在使用 pjmedia_aud_param 之前用其默认值初始化它：
        pjmedia_aud_param param;
        pjmedia_aud_dev_index dev_idx;
        pj_status_t status;

        dev_idx = PJMEDIA_AUD_DEFAULT_CAPTURE_DEV;
        status = pjmedia_aud_dev_default_param(dev_idx,&param);

    - 配置参数
        param.dir = PJMEDIA_DIR_CAPTURE_PLAYBACK;
        param.rec_id = PJMEDIA_AUD_DEFAULT_CAPTURE_DEV;
        param.play_id = PJMEDIA_AUD_DEFAULT_PLAYBACK_DEV;
        param.clock_rate = 8000;
        param.channel_count = 1;
        param.samples_per_frame = 160;
        param.bits_per_sample = 16;
    - 使用 pjmedia_aud_param 指定编解码器，通过 pjmedia_aud_dev_info 查看支持的格式列表，确保设备支持编解码器，下面的代码将设置为使用 G.711 ULAW 编码：
        //确保支持 Ulaw
        if((info.caps & PJMEDIA_AUD_DEV_CAP_EXT_FORMAT) == 0)
            error("Device does not support extended formats");
        for(i = 0;i<info.ext_fmt_cnt;++i)
        {
            if(info.ext_fmt[i].id == PJMEDIA_FORMAT_ULAW)
                break;
        }
        if(i == info.ext_fmt_cnt)
        {
            error("Device does not support Ulaw format");
        }

        //设置 Ulaw 格式
        param.flags |= PJMEDIA_AUD_DEV_CAP_EXT_FORMAT;
        param.ext_fmt.id = PJMEDIA_FORMAT_ULAW;
        param.ext_fmt.bitrate = 64000;
        param.ext_fmt.vad = PJ_FALSE;

    - 注意：
        音频配置为非 PCM 格式，则捕获和回放函数（分别为 pjmedia_aud_rec_cb 和 pjmedia_aud_play_cb） 的音频帧格式为 pjmedia_frame_ext 结构，而非 pjmedia_frame

    - 可选功能，下面代码显示如何在设备上启用回显取消（请注意，此代码段可能不是必需的，因为在调用上面的#pjmedia_audŧdev_default_param（）时，可能已启用该设置）：
        if(info.caps & PJMEDIA_AUD_DEV_CAP_EC)
        {
            param.flags |= PJMEDIA_AUD_DEV_CAP_EC;
            param.ec_enabled = PJ_TRUE;
        }

    - 打开音频流，指定捕获和播放回调函数
        pjmedia_aud_stream *stream;
        status = pjmedia_aud_stream_create(&param,&rec_cb,&play_cb,user_data,&stream);

@subsection 音频流运行
    -   打开音频流
        status = pjmedia_aud_stream_start(stream);
    -   关闭音频流
        status = pjmedia_aud_stream_stop(stream);
    -   销毁音频流
        status = pjmedia_aud_stream_destroy(stream);

    - 信息：
        检索音频的能力值
        //音量，百分比
        unsigned vol;
        status = pjmedia_aud_stream_get_cap(stream,PJMEDIA_AUD_DEV_CAP_OUTPUT_VOLUME_SETTING,&vol);

    - 信息：
        设置音频的能力值
        //音量，百分比
        unsigned vol = 50;
        status = pjmedia_aud_stream_set_cap(stream,PJMEDIA_AUD_DEV_CAP_OUTPUT_VOLUME_SETTING,&vol);

*/


/**
 * @}
 */

