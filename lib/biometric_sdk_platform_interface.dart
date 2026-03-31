import 'package:plugin_platform_interface/plugin_platform_interface.dart';

import 'biometric_sdk_method_channel.dart';

abstract class BiometricSdkPlatform extends PlatformInterface {
  /// Constructs a BiometricSdkPlatform.
  BiometricSdkPlatform() : super(token: _token);

  static final Object _token = Object();

  static BiometricSdkPlatform _instance = MethodChannelBiometricSdk();

  /// The default instance of [BiometricSdkPlatform] to use.
  ///
  /// Defaults to [MethodChannelBiometricSdk].
  static BiometricSdkPlatform get instance => _instance;

  /// Platform-specific implementations should set this with their own
  /// platform-specific class that extends [BiometricSdkPlatform] when
  /// they register themselves.
  static set instance(BiometricSdkPlatform instance) {
    PlatformInterface.verifyToken(instance, _token);
    _instance = instance;
  }

  Future<String?> getPlatformVersion() {
    throw UnimplementedError('platformVersion() has not been implemented.');
  }
}
