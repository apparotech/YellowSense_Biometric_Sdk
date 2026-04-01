package com.example.biometric_sdk

object NativeProcessor  {
    init {
        System.loadLibrary("native-lib")
    }

    // Existing method
    @JvmStatic
    external fun processFingerprint(
        imageBytes: ByteArray,
        width: Int,
        height: Int,
        landmarks: FloatArray
    ): ByteArray?

    // NEW: Calculate quality metrics (blur, brightness, centering, etc.)
    @JvmStatic
    external fun calculateQualityMetrics(
        imageBytes: ByteArray,
        width: Int,
        height: Int,
        landmarks: FloatArray,
        handCenterX: Int,
        handCenterY: Int
    ): FloatArray?

    // NEW: Simple motion blur detection
    @JvmStatic
    external fun detectMotionBlur(
        imageBytes: ByteArray,
        width: Int,
        height: Int
    ): Int
}