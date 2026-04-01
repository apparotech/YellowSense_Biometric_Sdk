package com.example.biometric_sdk

import android.graphics.Bitmap
import android.util.Log

object NativeProcessor {
    
    init {
        try {
            System.loadLibrary("native-lib")
            Log.d("BiometricSDK", "Native library loaded successfully")
        } catch (e: UnsatisfiedLinkError) {
            Log.e("BiometricSDK", "Failed to load native library: ${e.message}")
        }
    }

    /**
     * Process fingerprint image with hand landmarks
     * @param imageBytes Raw JPEG/bitmap bytes
     * @param width Image width
     * @param height Image height
     * @param landmarks FloatArray of 42 values (21 landmarks × x,y)
     * @return Processed image bytes or null on failure
     */
    @JvmStatic
    external fun processFingerprint(
        imageBytes: ByteArray,
        width: Int,
        height: Int,
        landmarks: FloatArray
    ): ByteArray?

    /**
     * Calculate advanced quality metrics including blur detection using Laplacian variance
     * 
     * @param imageBytes Raw JPEG/bitmap bytes
     * @param width Image width
     * @param height Image height
     * @param landmarks FloatArray of 42 values (21 landmarks × x,y)
     * @param handCenterX X coordinate of hand center in pixels
     * @param handCenterY Y coordinate of hand center in pixels
     * @return FloatArray of 5 metrics:
     *         [0] = blurScore (Laplacian variance) - higher = sharper, lower = blurry (0-100)
     *         [1] = brightnessScore (0-100)
     *         [2] = centeringScore (0-100)
     *         [3] = sizeScore (0-100)
     *         [4] = overallScore (weighted combination, 0-100)
     *         Returns null if calculation fails
     */
    @JvmStatic
    external fun calculateQualityMetrics(
        imageBytes: ByteArray,
        width: Int,
        height: Int,
        landmarks: FloatArray,
        handCenterX: Int,
        handCenterY: Int
    ): FloatArray?

    /**
     * Alternative: Single method that returns both processed image and quality metrics
     * in a single JNI call for efficiency
     * 
     * @param imageBytes Raw JPEG/bitmap bytes
     * @param width Image width
     * @param height Image height
     * @param landmarks FloatArray of 42 values (21 landmarks × x,y)
     * @param handCenterX X coordinate of hand center in pixels
     * @param handCenterY Y coordinate of hand center in pixels
     * @return ByteArray where:
     *         - First 20 bytes: 5 quality metrics as floats (20 bytes)
     *         - Remaining bytes: processed image data
     *         Returns null on failure
     */
    @JvmStatic
    external fun processWithQuality(
        imageBytes: ByteArray,
        width: Int,
        height: Int,
        landmarks: FloatArray,
        handCenterX: Int,
        handCenterY: Int
    ): ByteArray?

    /**
     * Utility method to extract quality metrics from combined result
     * @param combinedResult Result from processWithQuality()
     * @return Pair of (FloatArray metrics, ByteArray processedImage) or null
     */
    fun extractQualityMetrics(combinedResult: ByteArray?): Pair<FloatArray, ByteArray>? {
        if (combinedResult == null || combinedResult.size < 20) return null
        
        val metrics = FloatArray(5)
        val metricsBytes = combinedResult.copyOfRange(0, 20)
        
        // Convert 20 bytes to 5 floats (little-endian)
        for (i in 0 until 5) {
            val start = i * 4
            metrics[i] = java.nio.ByteBuffer.wrap(metricsBytes, start, 4)
                .order(java.nio.ByteOrder.LITTLE_ENDIAN)
                .float
        }
        
        val processedImage = combinedResult.copyOfRange(20, combinedResult.size)
        return Pair(metrics, processedImage)
    }

    /**
     * Simple blur detection using Kotlin fallback when C++ is unavailable
     * This matches the Laplacian variance approach used in C++
     */
    fun calculateBlurScoreFallback(bitmap: Bitmap): Int {
        // Resize for performance
        val scaled = Bitmap.createScaledBitmap(bitmap, 200, 200, true)
        val width = scaled.width
        val height = scaled.height
        
        // Convert to grayscale and apply Laplacian manually
        var sumSquared = 0.0
        var count = 0
        
        for (y in 1 until height - 1) {
            for (x in 1 until width - 1) {
                // Get grayscale values for 3x3 neighborhood
                val center = grayScale(scaled.getPixel(x, y))
                val top = grayScale(scaled.getPixel(x, y - 1))
                val bottom = grayScale(scaled.getPixel(x, y + 1))
                val left = grayScale(scaled.getPixel(x - 1, y))
                val right = grayScale(scaled.getPixel(x + 1, y))
                
                // Laplacian operator (4-neighbor)
                val laplacian = (4 * center - top - bottom - left - right).toDouble()
                sumSquared += laplacian * laplacian
                count++
            }
        }
        
        // Variance of Laplacian
        val variance = if (count > 0) sumSquared / count else 0.0
        
        // Convert to 0-100 scale (empirically: variance 0-500 maps to 0-100)
        // Higher variance = sharper image
        val blurScore = (variance / 500.0 * 100).toInt().coerceIn(0, 100)
        
        // Invert: we want blurScore where higher = sharper
        // So sharpness score = blurScore (higher is better)
        return blurScore
    }
    
    private fun grayScale(pixel: Int): Int {
        val r = (pixel shr 16) and 0xFF
        val g = (pixel shr 8) and 0xFF
        val b = pixel and 0xFF
        return (0.299 * r + 0.587 * g + 0.114 * b).toInt()
    }
}