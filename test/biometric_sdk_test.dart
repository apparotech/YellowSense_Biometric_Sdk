import 'package:flutter_test/flutter_test.dart';
import 'package:biometric_sdk/biometric_sdk.dart';
import 'package:biometric_sdk/biometric_sdk_platform_interface.dart';
import 'package:biometric_sdk/biometric_sdk_method_channel.dart';
import 'package:plugin_platform_interface/plugin_platform_interface.dart';

class MockBiometricSdkPlatform
    with MockPlatformInterfaceMixin
    implements BiometricSdkPlatform {

  @override
  Future<String?> getPlatformVersion() => Future.value('42');
}

void main() {
  final BiometricSdkPlatform initialPlatform = BiometricSdkPlatform.instance;

  test('$MethodChannelBiometricSdk is the default instance', () {
    expect(initialPlatform, isInstanceOf<MethodChannelBiometricSdk>());
  });

  test('getPlatformVersion', () async {
    BiometricSdk biometricSdkPlugin = BiometricSdk();
    MockBiometricSdkPlatform fakePlatform = MockBiometricSdkPlatform();
    BiometricSdkPlatform.instance = fakePlatform;

    expect(await biometricSdkPlugin.getPlatformVersion(), '42');
  });
}
