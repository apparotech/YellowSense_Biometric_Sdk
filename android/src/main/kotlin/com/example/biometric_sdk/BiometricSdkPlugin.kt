
package com.example.biometric_sdk

import android.content.Context
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.util.Base64
import com.google.mediapipe.framework.image.BitmapImageBuilder
import com.google.mediapipe.tasks.core.BaseOptions
import com.google.mediapipe.tasks.vision.handlandmarker.HandLandmarker
import io.flutter.embedding.engine.plugins.FlutterPlugin
import io.flutter.plugin.common.MethodCall
import io.flutter.plugin.common.MethodChannel
import io.flutter.plugin.common.MethodChannel.MethodCallHandler
import io.flutter.plugin.common.MethodChannel.Result
import java.util.UUID
import android.graphics.Matrix
import java.io.FileOutputStream


class BiometricSdkPlugin : FlutterPlugin, MethodCallHandler {

    private lateinit var channel: MethodChannel
    private lateinit var context: Context
    private var handLandmarker: HandLandmarker? = null

    override fun onAttachedToEngine(
        flutterPluginBinding: FlutterPlugin.FlutterPluginBinding
    ) {
        context = flutterPluginBinding.applicationContext
        channel = MethodChannel(
            flutterPluginBinding.binaryMessenger,
            "biometric_sdk"
        )
        channel.setMethodCallHandler(this)
        setupMediaPipe()
    }

    // ── MediaPipe Setup ───────────────────────────────────────
    private fun setupMediaPipe() {
    try {
        val baseOptions = BaseOptions.builder()
            .setModelAssetPath("hand_landmarker.task")
            .build()

        val options = HandLandmarker.HandLandmarkerOptions.builder()
            .setBaseOptions(baseOptions)
            .setNumHands(2)                          // ← 1 se 2
            .setMinHandDetectionConfidence(0.1f)     // ← 0.5 se 0.1
            .setMinHandPresenceConfidence(0.1f)      // ← 0.5 se 0.1
            .setMinTrackingConfidence(0.1f)          // ← 0.5 se 0.1
            .build()

        handLandmarker = HandLandmarker.createFromOptions(context, options)
        
        android.util.Log.d("BiometricSDK", 
            "MediaPipe setup SUCCESS!")
            
    } catch (e: Exception) {
        android.util.Log.e("BiometricSDK", 
            "MediaPipe setup FAILED: ${e.message}")
    }
}

    override fun onMethodCall(call: MethodCall, result: Result) {
        when (call.method) {

            "getPlatformVersion" -> {
                result.success("Android ${android.os.Build.VERSION.RELEASE}")
            }

            // ── Process Frame — Real time ─────────────────────
            "processFrame" -> {
                val imageBytes = call.argument<ByteArray>("frame")
                if (imageBytes == null) {
                    result.error("NO_FRAME", "Frame nahi mila", null)
                    return
                }
                android.util.Log.d("BiometricSDK", 
                "Frame received: ${imageBytes.size} bytes")
            android.util.Log.d("BiometricSDK", 
                "HandLandmarker: $handLandmarker")

                val frameResult = processFrame(imageBytes)
                android.util.Log.d("BiometricSDK", 
                "Result: $frameResult")
                result.success(frameResult)
            }

            // ── Final Capture ─────────────────────────────────
            "startCapture" -> {
                val mode = call.argument<String>("captureMode") ?: "SINGLE_FINGER"
                val imageBytes = call.argument<ByteArray>("image")
                handleCapture(mode, imageBytes, result)
            }

            else -> result.notImplemented()
        }
    }

   
private fun processFrame(imageBytes: ByteArray): Map<String, Any> {
    return try {

        var bitmap = BitmapFactory.decodeByteArray(
            imageBytes, 0, imageBytes.size
        )

        if (bitmap == null) {
            return mapOf(
                "fingerDetected" to false,
                "confidence"     to 0.0,
                "guidance"       to "Image could not be loaded",
                "qualityScore"   to 0,
                "fingerCount"    to 0,
                "fingerBoxes"    to emptyList<Map<String, Any>>(),
                "readyToCapture" to false
            )
        }

        // ── Rotation Fix ──────────────────────────────────
        // Portrait 480x640 → Rotate → 640x480
        bitmap = rotateBitmap(bitmap)
        saveBitmapForDebug(bitmap)

        android.util.Log.d("BiometricSDK",
            "After rotate: ${bitmap.width}x${bitmap.height}")

        // Quality
        val brightness = getBrightness(bitmap)
        var quality = 50
        if (bitmap.width >= 480) quality += 10
        if (brightness in 60.0..220.0) quality += 20
        quality = quality.coerceIn(0, 100)

        // MediaPipe
        val argbBitmap = bitmap.copy(Bitmap.Config.ARGB_8888, false)
        val mpImage = BitmapImageBuilder(argbBitmap).build()
        val landmarkerResult = handLandmarker!!.detect(mpImage)

        android.util.Log.d("BiometricSDK",
            "Landmarks: ${landmarkerResult?.landmarks()?.size}")

        if (landmarkerResult == null ||
            landmarkerResult.landmarks().isEmpty()) {
            return mapOf(
                "fingerDetected" to false,
                "confidence"     to 0.0,
                "guidance"       to "Place your hand in front of camera",
                "qualityScore"   to quality,
                "fingerCount"    to 0,
                "fingerBoxes"    to emptyList<Map<String, Any>>(),
                "readyToCapture" to false
            )
        }

        // val landmarks   = landmarkerResult.landmarks()[0]
        // val fingerCount = countExtendedFingers(landmarks)
        // val guidance    = getGuidance(fingerCount, quality, landmarks)

        val landmarks = landmarkerResult.landmarks()[0]
        
        // 1. MediaPipe se pata karein ki haath Left hai ya Right
        var handedness = landmarkerResult.handednesses()[0][0].categoryName()

        
        // (Optional Fix: Front camera aksar mirror image deta hai, isliye Left ko Right aur Right ko Left karna padta hai. Agar ulta aaye toh ise swap kar lein)
        if (handedness == "Left") handedness = "Right" else handedness = "Left"

        // 2. Naya function call karein jo exact ungliyan batayega
        val detectedFingersList = getDetectedFingers(landmarks, handedness)
        val fingerCount = detectedFingersList.size
        
        // List ko comma-separated string bana lein (e.g. "RIGHT_INDEX, RIGHT_MIDDLE")
        val detectedFingersStr = detectedFingersList.joinToString(", ")

        val guidance = getGuidance(fingerCount, quality, landmarks)


        android.util.Log.d("BiometricSDK", "Detected Fingers: $detectedFingersStr")

        android.util.Log.d("BiometricSDK",
            "Fingers: $fingerCount")

            // ── 🚀 NAYA ADVANCED FEATURE: LIVE FINGER BOXES ─────────────────────────
        
        
        
        val fingerBoxes = mutableListOf<Map<String, Any>>()

        val fingerLandmarksIndices = listOf(
            listOf(1, 2, 3, 4) to "THUMB",
            listOf(5, 6, 7, 8) to "INDEX",
            listOf(9, 10, 11, 12) to "MIDDLE",
            listOf(13, 14, 15, 16) to "RING",
            listOf(17, 18, 19, 20) to "LITTLE"
        )

        for ((indices, fingerName) in fingerLandmarksIndices) {
            val rawPoints = indices.map { landmarks[it] }

            // 100% Fail-Proof Rotation: Portrait mode ke liye X aur Y swap aur mirror
            val xValues = rawPoints.map { it.y() } 
            val yValues = rawPoints.map { 1.0f - it.x() } 
            
            val padding = 0.04f // Bounding box thoda bada dikhe
            
            val minX = (xValues.minOrNull() ?: 0.0f) - padding
            val minY = (yValues.minOrNull() ?: 0.0f) - padding
            val maxX = (xValues.maxOrNull() ?: 1.0f) + padding
            val maxY = (yValues.maxOrNull() ?: 1.0f) + padding

            // Bina kisi condition ke list mein add karein taaki humesha draw ho
            fingerBoxes.add(mapOf(
                "x1" to minX.coerceIn(0.0f, 1.0f),
                "y1" to minY.coerceIn(0.0f, 1.0f),
                "x2" to maxX.coerceIn(0.0f, 1.0f),
                "y2" to maxY.coerceIn(0.0f, 1.0f),
                "label" to "${handedness.uppercase()}_$fingerName"
            ))
        }

        // ── 🚀 NAYA CODE YAHAN ADD HUA HAI ──────────────────────────

            // ── 🚀 NAYA CODE YAHAN ADD HUA HAI ──────────────────────────
        
        // 1. Landmarks ko FloatArray mein convert karein
        val points = FloatArray(42)// 21 landmarks * 2 (x, y)
        for(i in 0 until 21 ) {
            points[i * 2] = landmarks[i].x()
            points[i * 2 + 1] = landmarks[i].y()
        }

        // 2. C++ function call karein (Crop aur Enhance ke liye)
        val processedBytes = NativeProcessor.processFingerprint(
            imageBytes,
            bitmap.width,
            bitmap.height,
            points
        )

     
        var isLivenessPassed = true
        var processedBase64 = ""
        var finalGuidance = guidance // Purana guidance text

        if (processedBytes != null) {
            if (processedBytes.size == 1) {
                // Agar C++ ne sirf 1 byte bheja, matlab FRAUD DETECTED! 🚨
                isLivenessPassed = false
                finalGuidance = "Spoof Detected! Please use a real finger."
                android.util.Log.w("BiometricSDK", "LIVENESS FAILED: Fake finger blocked!")
            } else {
                // Asli ungli hai, Base64 banao
               
                val buffer = java.nio.ByteBuffer.wrap(processedBytes)
                val imgSize = buffer.int // Pehle 4 bytes read karein (Image ka size)
                
                val imgData = ByteArray(imgSize)
                buffer.get(imgData) // Sirf Image ka data nikalein
                
                val templateData = ByteArray(buffer.remaining())
                buffer.get(templateData) // Bacha hua data Template hai
                
                // UI Preview ke liye sirf Image ko Base64 banayein
                processedBase64 = android.util.Base64.encodeToString(imgData, android.util.Base64.NO_WRAP)
            }
        }


        mapOf(
            "fingerDetected" to (fingerCount > 0),
            "confidence"     to 0.88,
            "guidance"       to finalGuidance,
            "qualityScore"   to quality,
            "fingerCount"    to fingerCount,
            "detectedFingersName" to detectedFingersStr,
            "fingerBoxes"    to fingerBoxes,
            "processedImage" to processedBase64,
           "readyToCapture" to (quality >= 70 && fingerCount > 0 && isLivenessPassed)
        )

    } catch (e: Exception) {
        android.util.Log.e("BiometricSDK", "Error: ${e.message}")
        mapOf(
            "fingerDetected" to false,
            "confidence"     to 0.0,
            "guidance"       to "Error: ${e.message}",
            "qualityScore"   to 0,
            "fingerCount"    to 0,
            "fingerBoxes"    to emptyList<Map<String, Any>>(),
            "readyToCapture" to false
        )
    }
}

// ── Rotation Helper ───────────────────────────────────────
private fun rotateBitmap(bitmap: Bitmap): Bitmap {
    return try {
        // Try 0, 90, 180, 270 — sahi wala detect karo
        // Pehle 90 degree try karo
        val matrix = Matrix()
        matrix.postRotate(270f)
        Bitmap.createBitmap(
            bitmap, 0, 0,
            bitmap.width, bitmap.height,
            matrix, true
        )
    } catch (e: Exception) {
        bitmap // Original return karo agar fail ho
    }
}

// ── Debug Image Saver ─────────────────────────────────────
private fun saveBitmapForDebug(bitmap: Bitmap) {
    try {
        val file = java.io.File(
            context.getExternalFilesDir(null),
            "debug_frame.jpg"
        )
        val out = FileOutputStream(file)
        bitmap.compress(Bitmap.CompressFormat.JPEG, 90, out)
        out.close()
        android.util.Log.d("BiometricSDK",
            "Image saved: ${file.absolutePath}")
    } catch (e: Exception) {
        android.util.Log.e("BiometricSDK",
            "Save failed: ${e.message}")
    }
}

    // ── Finger Count ──────────────────────────────────────────
    // private fun countExtendedFingers(
    //     landmarks: List<com.google.mediapipe.tasks.components.containers.NormalizedLandmark>
    // ): Int {
    //     var count = 0

    //     // Index finger — tip(8) > pip(6)
    //     if (landmarks[8].y() < landmarks[6].y()) count++
    //     // Middle finger — tip(12) > pip(10)
    //     if (landmarks[12].y() < landmarks[10].y()) count++
    //     // Ring finger — tip(16) > pip(14)
    //     if (landmarks[16].y() < landmarks[14].y()) count++
    //     // Little finger — tip(20) > pip(18)i devloping 
    //     if (landmarks[20].y() < landmarks[18].y()) count++

    //     return count
    // }
    // ── NAYA FUNCTION: Finger aur Hand Pehchanne ke liye ────────────────
    private fun getDetectedFingers(
        landmarks: List<com.google.mediapipe.tasks.components.containers.NormalizedLandmark>,
        handedness: String
    ): List<String> {
        val fingers = mutableListOf<String>()
        val prefix = handedness.uppercase() // Result: "LEFT" ya "RIGHT"

        // Thumb (Angutha) - tip(4) > ip(3)
        if (landmarks[4].y() < landmarks[3].y()) fingers.add("${prefix}_THUMB")
        
        // Index (Pehli ungli) - tip(8) > pip(6)
        if (landmarks[8].y() < landmarks[6].y()) fingers.add("${prefix}_INDEX")
        
        // Middle (Beech ki ungli) - tip(12) > pip(10)
        if (landmarks[12].y() < landmarks[10].y()) fingers.add("${prefix}_MIDDLE")
        
        // Ring (Teesri ungli) - tip(16) > pip(14)
        if (landmarks[16].y() < landmarks[14].y()) fingers.add("${prefix}_RING")
        
        // Little (Chhoti ungli) - tip(20) > pip(18)
        if (landmarks[20].y() < landmarks[18].y()) fingers.add("${prefix}_LITTLE")

        return fingers
    }

    // ── Quality Calculate ─────────────────────────────────────
    private fun calculateQuality(
        bitmap: Bitmap,
        landmarks: List<com.google.mediapipe.tasks.components.containers.NormalizedLandmark>
    ): Int {
        var score = 60

        // Size check
        if (bitmap.width >= 640 && bitmap.height >= 480) score += 10

        // Brightness check
        val brightness = getBrightness(bitmap)
        if (brightness in 80.0..200.0) score += 15

        // Center mein hai?
        val handCenterX = landmarks[9].x()
        if (handCenterX in 0.3f..0.7f) score += 15

        return score.coerceIn(0, 100)
    }

    // ── Brightness ────────────────────────────────────────────
    private fun getBrightness(bitmap: Bitmap): Double {
        var total = 0L
        val sample = 100
        val w = bitmap.width
        val h = bitmap.height

        for (i in 0 until sample) {
            val x = (Math.random() * w).toInt()
            val y = (Math.random() * h).toInt()
            val pixel = bitmap.getPixel(x, y)
            val r = (pixel shr 16) and 0xff
            val g = (pixel shr 8) and 0xff
            val b = pixel and 0xff
            total += (r + g + b) / 3
        }
        return total.toDouble() / sample
    }

    // ── Guidance Text ─────────────────────────────────────────
    private fun getGuidance(
        fingerCount: Int,
        quality: Int,
        landmarks: List<com.google.mediapipe.tasks.components.containers.NormalizedLandmark>
    ): String {
        if (fingerCount == 0) return "Please palce your hand"
        if (quality < 50) return "More light needed"
        val cx = landmarks[9].x()
        if (cx < 0.3f) return "Move slightly to the right"
        if (cx > 0.7f) return "Move slightly to the left"
        if (quality < 70) return "Move closer to camera"
        return "Perfect — Capturing now!"
    }

private fun handleCapture(
    mode: String,
    imageBytes: ByteArray?,
    result: Result
) {
    try {
        val fingers = getFingerListForMode(mode)

        // ── C++ se liveness check karo ───────────────────
        var livenessPassed = true
        var processedImageBase64 = ""
        var finalIsoTemplateBase64 = ""

        if (imageBytes != null) {
            // Pehle MediaPipe se landmarks nikalo
            val bitmap = BitmapFactory.decodeByteArray(
                imageBytes, 0, imageBytes.size
            )
            
            if (bitmap != null) {
                val rotated = rotateBitmap(bitmap)
                val argbBitmap = rotated.copy(
                    Bitmap.Config.ARGB_8888, false
                )
                val mpImage = BitmapImageBuilder(argbBitmap).build()
                val landmarkerResult = handLandmarker?.detect(mpImage)

                // Landmarks mile toh C++ call karo
                val landmarks = landmarkerResult
                    ?.landmarks()
                    ?.firstOrNull()

                if (landmarks != null) {
                    // FloatArray banao
                    val points = FloatArray(42)
                    for (i in 0 until 21) {
                        points[i * 2]     = landmarks[i].x()
                        points[i * 2 + 1] = landmarks[i].y()
                    }

                    // C++ call karo
                    val processedBytes = NativeProcessor
                        .processFingerprint(
                            imageBytes,
                            rotated.width,
                            rotated.height,
                            points
                        )

                    when {
                        // Null = Error
                        processedBytes == null -> {
                            livenessPassed = false
                            android.util.Log.w("BiometricSDK",
                                "C++ returned null — spoof!")
                        }
                        // 1 byte = SPOOF
                        processedBytes.size == 1 -> {
                            livenessPassed = false
                            android.util.Log.w("BiometricSDK",
                                "SPOOF DETECTED by C++!")
                        }
                        // Valid = Real finger
                        else -> {
                            livenessPassed = true
                            // 🔥 NAYA CODE: Unpack Image & Template for Capture 🔥
                            val buffer = java.nio.ByteBuffer.wrap(processedBytes)
                            val imgSize = buffer.int
                            
                            val imgData = ByteArray(imgSize)
                            buffer.get(imgData)
                            
                            val templateData = ByteArray(buffer.remaining())
                            buffer.get(templateData)
                            
                            processedImageBase64 = Base64.encodeToString(imgData, Base64.DEFAULT)
                            finalIsoTemplateBase64 = Base64.encodeToString(templateData, Base64.DEFAULT)
                            
                            android.util.Log.d("BiometricSDK", "Liveness PASSED! Template Size: ${templateData.size} bytes")
                        }
                    }
                } else {
                    // Landmarks nahi mile
                    livenessPassed = false
                    android.util.Log.w("BiometricSDK",
                        "No landmarks in capture!")
                }
            }
        }

        // ── Finger Results Banao ──────────────────────────
        val fingerResults = fingers.map { fingerId ->
            mapOf(
                "fingerId"       to fingerId,
                "status"         to if (livenessPassed)
                                    "success" else "failed",
                "qualityScore"   to 85,
                "livenessPassed" to livenessPassed, // ← Real!
                "template"       to if (livenessPassed) finalIsoTemplateBase64 else "",
                "rawImage"       to (imageBytes?.let {
                    Base64.encodeToString(it, Base64.DEFAULT)
                } ?: ""),
                "processedImage" to processedImageBase64,
                "errorCode"      to if (livenessPassed)
                                    null else "SPOOF_DETECTED",
                "errorMessage"   to if (livenessPassed)
                                    null else "Liveness check failed"
            )
        }

        // ── Overall Status ────────────────────────────────
        val overallStatus = if (livenessPassed) 
                            "success" else "failed"

        result.success(mapOf(
            "transactionId" to UUID.randomUUID().toString(),
            "overallStatus" to overallStatus,
            "captureMode"   to mode,
            "results"       to fingerResults
        ))

    } catch (e: Exception) {
        android.util.Log.e("BiometricSDK", 
            "Capture error: ${e.message}")
        result.error("CAPTURE_ERROR", e.message, null)
    }
}


    // ── Mode → Fingers ────────────────────────────────────────
    private fun getFingerListForMode(mode: String): List<String> {
        return when (mode) {
            "RIGHT_FOUR"  -> listOf(
                "RIGHT_INDEX", "RIGHT_MIDDLE",
                "RIGHT_RING", "RIGHT_LITTLE"
            )
            "LEFT_FOUR"   -> listOf(
                "LEFT_INDEX", "LEFT_MIDDLE",
                "LEFT_RING", "LEFT_LITTLE"
            )
            "RIGHT_THUMB" -> listOf("RIGHT_THUMB")
            "LEFT_THUMB"  -> listOf("LEFT_THUMB")
            else          -> listOf("RIGHT_INDEX")
        }
    }



    override fun onDetachedFromEngine(
        binding: FlutterPlugin.FlutterPluginBinding
    ) {
        handLandmarker?.close()
        channel.setMethodCallHandler(null)
    }
}