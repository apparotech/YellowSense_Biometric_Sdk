package com.example.biometric_sdk

object NativeProcessor  {
    init {
        System.loadLibrary("native-lib")
    }

    // Ye external function C++ wale JNI function se connect hoga
    @JvmStatic
    external fun processFingerprint(
        imageBytes: ByteArray,
        width: Int,
        height: Int,
        landmarks: FloatArray
    ): ByteArray?


}