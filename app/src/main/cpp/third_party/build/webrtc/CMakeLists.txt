cmake_minimum_required(VERSION 3.4.1)

message(WARNING "########make webrtc-lib ################")

enable_language(ASM)

add_definitions(
        -DWEBRTC_ANDROID
        -DWEBRTC_POSIX
        -DWEBRTC_HAS_NEON
        -D__UCLIBC__
        -DWEBRTC_NS_FIXED
)

include_directories(
        ../../webrtc/src
        ../../webrtc/src/webrtc/modules/audio_coding/codecs/isac/main/include
        ../../webrtc/src/webrtc/common_audio/signal_processing/include
)

add_library(webrtc-lib STATIC

        #        base
        ../../webrtc/src/webrtc/base/checks.cc
        ../../webrtc/src/webrtc/base/criticalsection.cc
        ../../webrtc/src/webrtc/base/event.cc
        ../../webrtc/src/webrtc/base/event_tracer.cc
        ../../webrtc/src/webrtc/base/platform_file.cc
        ../../webrtc/src/webrtc/base/platform_thread.cc
        ../../webrtc/src/webrtc/base/timeutils.cc
        ../../webrtc/src/webrtc/base/logging.cc
        ../../webrtc/src/webrtc/base/stringencode.cc

        #        resampler
        ../../webrtc/src/webrtc/common_audio/resampler/push_resampler.cc
        ../../webrtc/src/webrtc/common_audio/resampler/push_sinc_resampler.cc
        ../../webrtc/src/webrtc/common_audio/resampler/resampler.cc
        ../../webrtc/src/webrtc/common_audio/resampler/sinusoidal_linear_chirp_source.cc
        ../../webrtc/src/webrtc/common_audio/resampler/sinc_resampler.cc
        ../../webrtc/src/webrtc/common_audio/resampler/sinc_resampler_neon.cc

        #        signal_processing
        ../../webrtc/src/webrtc/common_audio/signal_processing/auto_corr_to_refl_coef.c
        ../../webrtc/src/webrtc/common_audio/signal_processing/auto_correlation.c
        ../../webrtc/src/webrtc/common_audio/signal_processing/complex_fft.c
        ../../webrtc/src/webrtc/common_audio/signal_processing/copy_set_operations.c
        ../../webrtc/src/webrtc/common_audio/signal_processing/division_operations.c
        ../../webrtc/src/webrtc/common_audio/signal_processing/dot_product_with_scale.c
        ../../webrtc/src/webrtc/common_audio/signal_processing/energy.c
        ../../webrtc/src/webrtc/common_audio/signal_processing/filter_ar.c
        ../../webrtc/src/webrtc/common_audio/signal_processing/filter_ma_fast_q12.c
        ../../webrtc/src/webrtc/common_audio/signal_processing/get_hanning_window.c
        ../../webrtc/src/webrtc/common_audio/signal_processing/get_scaling_square.c
        ../../webrtc/src/webrtc/common_audio/signal_processing/ilbc_specific_functions.c
        ../../webrtc/src/webrtc/common_audio/signal_processing/levinson_durbin.c
        ../../webrtc/src/webrtc/common_audio/signal_processing/randomization_functions.c
        ../../webrtc/src/webrtc/common_audio/signal_processing/real_fft.c
        ../../webrtc/src/webrtc/common_audio/signal_processing/refl_coef_to_lpc.c
        ../../webrtc/src/webrtc/common_audio/signal_processing/resample.c
        ../../webrtc/src/webrtc/common_audio/signal_processing/resample_48khz.c
        ../../webrtc/src/webrtc/common_audio/signal_processing/resample_by_2.c
        ../../webrtc/src/webrtc/common_audio/signal_processing/resample_by_2_internal.c
        ../../webrtc/src/webrtc/common_audio/signal_processing/resample_fractional.c
        ../../webrtc/src/webrtc/common_audio/signal_processing/spl_init.c
        ../../webrtc/src/webrtc/common_audio/signal_processing/spl_sqrt.c
        ../../webrtc/src/webrtc/common_audio/signal_processing/splitting_filter.c
        ../../webrtc/src/webrtc/common_audio/signal_processing/sqrt_of_one_minus_x_squared.c
        ../../webrtc/src/webrtc/common_audio/signal_processing/vector_scaling_operations.c

        #        vad
        ../../webrtc/src/webrtc/common_audio/vad/vad.cc
        ../../webrtc/src/webrtc/common_audio/vad/vad_core.c
        ../../webrtc/src/webrtc/common_audio/vad/vad_filterbank.c
        ../../webrtc/src/webrtc/common_audio/vad/vad_gmm.c
        ../../webrtc/src/webrtc/common_audio/vad/vad_sp.c
        ../../webrtc/src/webrtc/common_audio/vad/webrtc_vad.c

        #        common_audio
        ../../webrtc/src/webrtc/common_audio/audio_converter.cc
        ../../webrtc/src/webrtc/common_audio/audio_ring_buffer.cc
        ../../webrtc/src/webrtc/common_audio/audio_util.cc
        ../../webrtc/src/webrtc/common_audio/blocker.cc
        ../../webrtc/src/webrtc/common_audio/channel_buffer.cc
        ../../webrtc/src/webrtc/common_audio/fft4g.c
        ../../webrtc/src/webrtc/common_audio/lapped_transform.cc
        ../../webrtc/src/webrtc/common_audio/real_fourier.cc
        ../../webrtc/src/webrtc/common_audio/real_fourier_ooura.cc
        ../../webrtc/src/webrtc/common_audio/ring_buffer.c
        ../../webrtc/src/webrtc/common_audio/sparse_fir_filter.cc
        ../../webrtc/src/webrtc/common_audio/window_generator.cc
        ../../webrtc/src/webrtc/common_audio/fir_filter.cc
        ../../webrtc/src/webrtc/common_audio/fir_filter_neon.cc

        #        audio_coding
        ../../webrtc/src/webrtc/modules/audio_coding/codecs/audio_decoder.cc
        ../../webrtc/src/webrtc/modules/audio_coding/codecs/audio_encoder.cc
        ../../webrtc/src/webrtc/modules/audio_coding/codecs/isac/main/source/arith_routines.c
        ../../webrtc/src/webrtc/modules/audio_coding/codecs/isac/main/source/arith_routines_hist.c
        ../../webrtc/src/webrtc/modules/audio_coding/codecs/isac/main/source/arith_routines_logist.c
        ../../webrtc/src/webrtc/modules/audio_coding/codecs/isac/main/source/audio_decoder_isac.cc
        ../../webrtc/src/webrtc/modules/audio_coding/codecs/isac/main/source/audio_encoder_isac.cc
        ../../webrtc/src/webrtc/modules/audio_coding/codecs/isac/main/source/bandwidth_estimator.c
        ../../webrtc/src/webrtc/modules/audio_coding/codecs/isac/main/source/crc.c
        ../../webrtc/src/webrtc/modules/audio_coding/codecs/isac/main/source/decode.c
        ../../webrtc/src/webrtc/modules/audio_coding/codecs/isac/main/source/decode_bwe.c
        ../../webrtc/src/webrtc/modules/audio_coding/codecs/isac/main/source/encode.c
        ../../webrtc/src/webrtc/modules/audio_coding/codecs/isac/main/source/encode_lpc_swb.c
        ../../webrtc/src/webrtc/modules/audio_coding/codecs/isac/main/source/entropy_coding.c
        ../../webrtc/src/webrtc/modules/audio_coding/codecs/isac/main/source/fft.c
        ../../webrtc/src/webrtc/modules/audio_coding/codecs/isac/main/source/filter_functions.c
        ../../webrtc/src/webrtc/modules/audio_coding/codecs/isac/main/source/filterbank_tables.c
        ../../webrtc/src/webrtc/modules/audio_coding/codecs/isac/main/source/filterbanks.c
        ../../webrtc/src/webrtc/modules/audio_coding/codecs/isac/main/source/intialize.c
        ../../webrtc/src/webrtc/modules/audio_coding/codecs/isac/main/source/isac.c
        ../../webrtc/src/webrtc/modules/audio_coding/codecs/isac/main/source/lattice.c
        ../../webrtc/src/webrtc/modules/audio_coding/codecs/isac/main/source/lpc_analysis.c
        ../../webrtc/src/webrtc/modules/audio_coding/codecs/isac/main/source/lpc_gain_swb_tables.c
        ../../webrtc/src/webrtc/modules/audio_coding/codecs/isac/main/source/lpc_shape_swb12_tables.c
        ../../webrtc/src/webrtc/modules/audio_coding/codecs/isac/main/source/lpc_shape_swb16_tables.c
        ../../webrtc/src/webrtc/modules/audio_coding/codecs/isac/main/source/lpc_tables.c
        ../../webrtc/src/webrtc/modules/audio_coding/codecs/isac/main/source/pitch_estimator.c
        ../../webrtc/src/webrtc/modules/audio_coding/codecs/isac/main/source/pitch_filter.c
        ../../webrtc/src/webrtc/modules/audio_coding/codecs/isac/main/source/pitch_gain_tables.c
        ../../webrtc/src/webrtc/modules/audio_coding/codecs/isac/main/source/pitch_lag_tables.c
        ../../webrtc/src/webrtc/modules/audio_coding/codecs/isac/main/source/spectrum_ar_model_tables.c
        ../../webrtc/src/webrtc/modules/audio_coding/codecs/isac/main/source/transform.c

        #        audio_processing
        ../../webrtc/src/webrtc/modules/audio_processing/audio_buffer.cc
        ../../webrtc/src/webrtc/modules/audio_processing/audio_processing_impl.cc
        ../../webrtc/src/webrtc/modules/audio_processing/echo_cancellation_impl.cc
        ../../webrtc/src/webrtc/modules/audio_processing/echo_control_mobile_impl.cc
        ../../webrtc/src/webrtc/modules/audio_processing/gain_control_for_experimental_agc.cc
        ../../webrtc/src/webrtc/modules/audio_processing/gain_control_impl.cc
        ../../webrtc/src/webrtc/modules/audio_processing/high_pass_filter_impl.cc
        ../../webrtc/src/webrtc/modules/audio_processing/level_estimator_impl.cc
        ../../webrtc/src/webrtc/modules/audio_processing/noise_suppression_impl.cc
        ../../webrtc/src/webrtc/modules/audio_processing/processing_component.cc
        ../../webrtc/src/webrtc/modules/audio_processing/rms_level.cc
        ../../webrtc/src/webrtc/modules/audio_processing/splitting_filter.cc
        ../../webrtc/src/webrtc/modules/audio_processing/three_band_filter_bank.cc
        ../../webrtc/src/webrtc/modules/audio_processing/typing_detection.cc
        ../../webrtc/src/webrtc/modules/audio_processing/voice_detection_impl.cc

        #        audio_processing aec
        ../../webrtc/src/webrtc/modules/audio_processing/aec/aec_resampler.c
        ../../webrtc/src/webrtc/modules/audio_processing/aec/echo_cancellation.c
        ../../webrtc/src/webrtc/modules/audio_processing/aec/aec_core.c
        ../../webrtc/src/webrtc/modules/audio_processing/aec/aec_rdft.c
        ../../webrtc/src/webrtc/modules/audio_processing/aec/aec_core_neon.c
        ../../webrtc/src/webrtc/modules/audio_processing/aec/aec_rdft_neon.c

        #        audio_processing aecm
        ../../webrtc/src/webrtc/modules/audio_processing/aecm/aecm_core.c
        ../../webrtc/src/webrtc/modules/audio_processing/aecm/echo_control_mobile.c
        ../../webrtc/src/webrtc/modules/audio_processing/aecm/aecm_core_c.c
        ../../webrtc/src/webrtc/modules/audio_processing/aecm/aecm_core_neon.c

        #        audio_processing agc
        ../../webrtc/src/webrtc/modules/audio_processing/agc/agc.cc
        ../../webrtc/src/webrtc/modules/audio_processing/agc/agc_manager_direct.cc
        ../../webrtc/src/webrtc/modules/audio_processing/agc/histogram.cc
        ../../webrtc/src/webrtc/modules/audio_processing/agc/utility.cc
        ../../webrtc/src/webrtc/modules/audio_processing/agc/legacy/analog_agc.c
        ../../webrtc/src/webrtc/modules/audio_processing/agc/legacy/digital_agc.c

        #        audio_processing beamformer
        ../../webrtc/src/webrtc/modules/audio_processing/beamformer/array_util.cc
        ../../webrtc/src/webrtc/modules/audio_processing/beamformer/covariance_matrix_generator.cc
        ../../webrtc/src/webrtc/modules/audio_processing/beamformer/nonlinear_beamformer.cc

        #        audio_processing ns
        ../../webrtc/src/webrtc/modules/audio_processing/ns/noise_suppression.c
        ../../webrtc/src/webrtc/modules/audio_processing/ns/noise_suppression_x.c
        ../../webrtc/src/webrtc/modules/audio_processing/ns/ns_core.c
        ../../webrtc/src/webrtc/modules/audio_processing/ns/nsx_core.c
        ../../webrtc/src/webrtc/modules/audio_processing/ns/nsx_core_c.c
        ../../webrtc/src/webrtc/modules/audio_processing/ns/nsx_core_neon.c

        #        audio_processing transient
        ../../webrtc/src/webrtc/modules/audio_processing/transient/click_annotate.cc
        ../../webrtc/src/webrtc/modules/audio_processing/transient/file_utils.cc
        ../../webrtc/src/webrtc/modules/audio_processing/transient/moving_moments.cc
        ../../webrtc/src/webrtc/modules/audio_processing/transient/transient_detector.cc
        ../../webrtc/src/webrtc/modules/audio_processing/transient/transient_suppressor.cc
        ../../webrtc/src/webrtc/modules/audio_processing/transient/wpd_node.cc
        ../../webrtc/src/webrtc/modules/audio_processing/transient/wpd_tree.cc

        #        audio_processing intelligibility
        ../../webrtc/src/webrtc/modules/audio_processing/intelligibility/intelligibility_enhancer.cc
        ../../webrtc/src/webrtc/modules/audio_processing/intelligibility/intelligibility_utils.cc

        #        audio_processing utility
        ../../webrtc/src/webrtc/modules/audio_processing/utility/delay_estimator.c
        ../../webrtc/src/webrtc/modules/audio_processing/utility/delay_estimator_wrapper.c

        #        audio_processing vad
        ../../webrtc/src/webrtc/modules/audio_processing/vad/gmm.cc
        ../../webrtc/src/webrtc/modules/audio_processing/vad/pitch_based_vad.cc
        ../../webrtc/src/webrtc/modules/audio_processing/vad/pitch_internal.cc
        ../../webrtc/src/webrtc/modules/audio_processing/vad/pole_zero_filter.cc
        ../../webrtc/src/webrtc/modules/audio_processing/vad/standalone_vad.cc
        ../../webrtc/src/webrtc/modules/audio_processing/vad/vad_audio_proc.cc
        ../../webrtc/src/webrtc/modules/audio_processing/vad/vad_circular_buffer.cc
        ../../webrtc/src/webrtc/modules/audio_processing/vad/voice_activity_detector.cc

        #        system_wrappers
        ../../webrtc/src/webrtc/system_wrappers/source/aligned_malloc.cc
        ../../webrtc/src/webrtc/system_wrappers/source/file_impl.cc
        ../../webrtc/src/webrtc/system_wrappers/source/logging.cc
        ../../webrtc/src/webrtc/system_wrappers/source/metrics_default.cc
        ../../webrtc/src/webrtc/system_wrappers/source/rw_lock.cc
        ../../webrtc/src/webrtc/system_wrappers/source/rw_lock_posix.cc
        ../../webrtc/src/webrtc/system_wrappers/source/trace_impl.cc
        ../../webrtc/src/webrtc/system_wrappers/source/trace_posix.cc

        ../../webrtc/src/webrtc/webrtc_apm.cpp

        ../../webrtc/src/webrtc/howling/biquad.c
        ../../webrtc/src/webrtc/howling/howling_core.c
        ../../webrtc/src/webrtc/howling/tone_detect.c
        )

if (ANDROID_ABI STREQUAL armeabi-v7a)
    set_target_properties(webrtc-lib PROPERTIES COMPILE_FLAGS -mfpu=neon)
    target_sources(webrtc-lib PRIVATE
            #        audio_processing
            ../../webrtc/src/webrtc/common_audio/signal_processing/cross_correlation_neon.c
            ../../webrtc/src/webrtc/common_audio/signal_processing/downsample_fast_neon.c
            ../../webrtc/src/webrtc/common_audio/signal_processing/min_max_operations_neon.c
            ../../webrtc/src/webrtc/common_audio/signal_processing/filter_ar_fast_q12_armv7.S
            ../../webrtc/src/webrtc/common_audio/signal_processing/spl_sqrt_floor_arm.S
            ../../webrtc/src/webrtc/common_audio/signal_processing/complex_bit_reverse_arm.S
            )
else (ANDROID_ABI STREQUAL "arm64-v8a")
    add_definitions(-DWEBRTC_ARCH_ARM64)
    target_sources(webrtc-lib PRIVATE
            #        audio_processing
            ../../webrtc/src/webrtc/common_audio/signal_processing/cross_correlation_neon.c
            ../../webrtc/src/webrtc/common_audio/signal_processing/downsample_fast_neon.c
            ../../webrtc/src/webrtc/common_audio/signal_processing/min_max_operations_neon.c
            ../../webrtc/src/webrtc/common_audio/signal_processing/filter_ar_fast_q12.c
            ../../webrtc/src/webrtc/common_audio/signal_processing/spl_sqrt_floor.c
            ../../webrtc/src/webrtc/common_audio/signal_processing/complex_bit_reverse.c
            )
endif ()

find_library(log-lib log)

target_link_libraries(webrtc-lib log)