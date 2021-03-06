/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

package modules.audio_device.android.java.src.org.webrtc.voiceengine;

import android.content.Context;
import android.content.pm.PackageManager;
import android.media.audiofx.AcousticEchoCanceler;
import android.media.audiofx.AudioEffect;
import android.media.audiofx.AudioEffect.Descriptor;
import android.media.AudioManager;
import android.os.Build;
import android.os.Process;

import org.webrtc.Logging;

import java.lang.Thread;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public final class WebRtcAudioUtils {
  private static final String TAG = "WebRtcAudioUtils";

  // List of devices where we have seen issues (e.g. bad audio quality) using
  // the low latency output mode in combination with OpenSL ES.
  // The device name is given by Build.MODEL.
  private static final String[] BLACKLISTED_OPEN_SL_ES_MODELS = new String[] {
    // This list is currently empty ;-)
  };

  // List of devices where it has been verified that the built-in effect
  // bad and where it makes sense to avoid using it and instead rely on the
  // native WebRTC version instead. The device name is given by Build.MODEL.
  private static final String[] BLACKLISTED_AEC_MODELS = new String[] {
      "D6503",      // Sony Xperia Z2 D6503
      "ONE A2005",  // OnePlus 2
  };
  private static final String[] BLACKLISTED_AGC_MODELS = new String[] {
      "Nexus 10",
      "Nexus 9",
  };
  private static final String[] BLACKLISTED_NS_MODELS = new String[] {
      "Nexus 10",
      "Nexus 9",
      "ONE A2005",  // OnePlus 2
  };

  // Use 16kHz as the default sample rate. A higher sample rate might prevent
  // us from supporting communication mode on some older (e.g. ICS) devices.
  private static final int DEFAULT_SAMPLE_RATE_HZ = 16000;
  private static int defaultSampleRateHz = DEFAULT_SAMPLE_RATE_HZ;
  // Set to true if setDefaultSampleRateHz() has been called.
  private static boolean isDefaultSampleRateOverridden = false;

  // By default, utilize hardware based audio effects when available.
  private static boolean useWebRtcBasedAcousticEchoCanceler = false;
  private static boolean useWebRtcBasedAutomaticGainControl = false;
  private static boolean useWebRtcBasedNoiseSuppressor = false;

  // Call these methods if any hardware based effect shall be replaced by a
  // software based version provided by the WebRTC stack instead.
  public static synchronized void setWebRtcBasedAcousticEchoCanceler(
      boolean enable) {
    useWebRtcBasedAcousticEchoCanceler = enable;
  }
  public static synchronized void setWebRtcBasedAutomaticGainControl(
      boolean enable) {
    useWebRtcBasedAutomaticGainControl = enable;
  }
  public static synchronized void setWebRtcBasedNoiseSuppressor(
      boolean enable) {
    useWebRtcBasedNoiseSuppressor = enable;
  }

  public static synchronized boolean useWebRtcBasedAcousticEchoCanceler() {
    if (useWebRtcBasedAcousticEchoCanceler) {
      Logging.w(TAG, "Overriding default behavior; now using WebRTC AEC!");
    }
    return useWebRtcBasedAcousticEchoCanceler;
  }
  public static synchronized boolean useWebRtcBasedAutomaticGainControl() {
    if (useWebRtcBasedAutomaticGainControl) {
      Logging.w(TAG, "Overriding default behavior; now using WebRTC AGC!");
    }
    return useWebRtcBasedAutomaticGainControl;
  }
  public static synchronized boolean useWebRtcBasedNoiseSuppressor() {
    if (useWebRtcBasedNoiseSuppressor) {
      Logging.w(TAG, "Overriding default behavior; now using WebRTC NS!");
    }
    return useWebRtcBasedNoiseSuppressor;
  }

  // Call this method if the default handling of querying the native sample
  // rate shall be overridden. Can be useful on some devices where the
  // available Android APIs are known to return invalid results.
  public static synchronized void setDefaultSampleRateHz(int sampleRateHz) {
    isDefaultSampleRateOverridden = true;
    defaultSampleRateHz = sampleRateHz;
  }

  public static synchronized boolean isDefaultSampleRateOverridden() {
    return isDefaultSampleRateOverridden;
  }

  public static synchronized int getDefaultSampleRateHz() {
    return defaultSampleRateHz;
  }

  public static List<String> getBlackListedModelsForAecUsage() {
    return Arrays.asList(WebRtcAudioUtils.BLACKLISTED_AEC_MODELS);
  }

  public static List<String> getBlackListedModelsForAgcUsage() {
    return Arrays.asList(WebRtcAudioUtils.BLACKLISTED_AGC_MODELS);
  }

  public static List<String> getBlackListedModelsForNsUsage() {
    return Arrays.asList(WebRtcAudioUtils.BLACKLISTED_NS_MODELS);
  }

  public static boolean runningOnGingerBreadOrHigher() {
    // November 2010: Android 2.3, API Level 9.
    return Build.VERSION.SDK_INT >= Build.VERSION_CODES.GINGERBREAD;
  }

  public static boolean runningOnJellyBeanOrHigher() {
    // June 2012: Android 4.1. API Level 16.
    return Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN;
  }

  public static boolean runningOnJellyBeanMR1OrHigher() {
    // November 2012: Android 4.2. API Level 17.
    return Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1;
  }

  public static boolean runningOnJellyBeanMR2OrHigher() {
    // July 24, 2013: Android 4.3. API Level 18.
    return Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2;
  }

  public static boolean runningOnLollipopOrHigher() {
    // API Level 21.
    return Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP;
  }

  // TODO(phoglund): enable when all downstream users use M.
  // public static boolean runningOnMOrHigher() {
    // API Level 23.
    // return Build.VERSION.SDK_INT >= Build.VERSION_CODES.M;
  //}

  // Helper method for building a string of thread information.
  public static String getThreadInfo() {
    return "@[name=" + Thread.currentThread().getName()
        + ", id=" + Thread.currentThread().getId() + "]";
  }

  // Returns true if we're running on emulator.
  public static boolean runningOnEmulator() {
    return Build.HARDWARE.equals("goldfish") &&
        Build.BRAND.startsWith("generic_");
  }

  // Returns true if the device is blacklisted for OpenSL ES usage.
  public static boolean deviceIsBlacklistedForOpenSLESUsage() {
    List<String> blackListedModels =
        Arrays.asList(BLACKLISTED_OPEN_SL_ES_MODELS);
    return blackListedModels.contains(Build.MODEL);
  }

  // Information about the current build, taken from system properties.
  public static void logDeviceInfo(String tag) {
    Logging.d(tag, "Android SDK: " + Build.VERSION.SDK_INT + ", "
        + "Release: " + Build.VERSION.RELEASE + ", "
        + "Brand: " + Build.BRAND + ", "
        + "Device: " + Build.DEVICE + ", "
        + "Id: " + Build.ID + ", "
        + "Hardware: " + Build.HARDWARE + ", "
        + "Manufacturer: " + Build.MANUFACTURER + ", "
        + "Model: " + Build.MODEL + ", "
        + "Product: " + Build.PRODUCT);
  }

  // Checks if the process has as specified permission or not.
  public static boolean hasPermission(Context context, String permission) {
    return context.checkPermission(
        permission,
        Process.myPid(),
        Process.myUid()) == PackageManager.PERMISSION_GRANTED;
    }
}
