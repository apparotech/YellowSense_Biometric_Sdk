
// import 'biometric_sdk_platform_interface.dart';

// class BiometricSdk {
//   Future<String?> getPlatformVersion() {
//     return BiometricSdkPlatform.instance.getPlatformVersion();
//   }
// }

import 'biometric_sdk_platform_interface.dart';
import 'package:flutter/services.dart';
class BiometricSdk {

  // Platform version
  Future<String?> getPlatformVersion() {
    return BiometricSdkPlatform.instance.getPlatformVersion();
  }

  // Main capture function
  static const _channel = MethodChannel('biometric_sdk');

  static Future<Map<dynamic, dynamic>> startCapture({
    required String captureMode,
  }) async {
    final result = await _channel.invokeMethod('startCapture', {
      'captureMode': captureMode,
    });
    return result as Map<dynamic, dynamic>;
  }
}
