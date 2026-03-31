import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';

import 'biometric_sdk_platform_interface.dart';

/// An implementation of [BiometricSdkPlatform] that uses method channels.
class MethodChannelBiometricSdk extends BiometricSdkPlatform {
  /// The method channel used to interact with the native platform.
  @visibleForTesting
  final methodChannel = const MethodChannel('biometric_sdk');

  @override
  Future<String?> getPlatformVersion() async {
    final version = await methodChannel.invokeMethod<String>('getPlatformVersion');
    return version;
  }
}
