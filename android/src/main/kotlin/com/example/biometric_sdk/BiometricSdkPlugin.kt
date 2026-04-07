
package com.example.biometric_sdk
import android.content.Context
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.graphics.Color
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
import kotlin.math.abs
import kotlin.math.sqrt
import kotlin.math.pow
import java.io.ByteArrayOutputStream
import com.google.mediapipe.tasks.components.containers.NormalizedLandmark
import kotlin.math.acos

class BiometricSdkPlugin : FlutterPlugin, MethodCallHandler {

    private lateinit var channel: MethodChannel
    private lateinit var context: Context
    private var handLandmarker: HandLandmarker? = null
    private var lastFrameBitmap: Bitmap? = null
    private var stableFrameCount = 0

    companion object {
        private const val QUALITY_THRESHOLD_CAPTURE = 35
        private const val QUALITY_THRESHOLD_GOOD    = 70
        private const val BLUR_THRESHOLD_MOTION     = 15
        private const val BRIGHTNESS_MIN            = 40
        private const val CENTERING_MIN             = 50

        private const val BLUR_WEIGHT       = 0.40
        private const val BRIGHTNESS_WEIGHT = 0.30
        private const val CENTERING_WEIGHT  = 0.30
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Engine lifecycle
    // ─────────────────────────────────────────────────────────────────────────

    override fun onAttachedToEngine(binding: FlutterPlugin.FlutterPluginBinding) {
        context = binding.applicationContext
        channel = MethodChannel(binding.binaryMessenger, "biometric_sdk")
        channel.setMethodCallHandler(this)
        setupMediaPipe()
    }

    override fun onDetachedFromEngine(binding: FlutterPlugin.FlutterPluginBinding) {
        handLandmarker?.close()
        channel.setMethodCallHandler(null)
    }

    // ─────────────────────────────────────────────────────────────────────────
    // MediaPipe setup
    // ─────────────────────────────────────────────────────────────────────────

    private fun setupMediaPipe() {
        try {
            val baseOptions = BaseOptions.builder()
                .setModelAssetPath("hand_landmarker.task")
                .build()
            val options = HandLandmarker.HandLandmarkerOptions.builder()
                .setBaseOptions(baseOptions)
                .setNumHands(2)
                .setMinHandDetectionConfidence(0.1f)
                .setMinHandPresenceConfidence(0.1f)
                .setMinTrackingConfidence(0.1f)
                .build()
            handLandmarker = HandLandmarker.createFromOptions(context, options)
            android.util.Log.d("BiometricSDK", "MediaPipe setup SUCCESS!")
        } catch (e: Exception) {
            android.util.Log.e("BiometricSDK", "MediaPipe setup FAILED: ${e.message}")
        }
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Method channel router
    // ─────────────────────────────────────────────────────────────────────────

    override fun onMethodCall(call: MethodCall, result: Result) {
        when (call.method) {

            "getPlatformVersion" ->
                result.success("Android ${android.os.Build.VERSION.RELEASE}")

            "processFrame" -> {
                val imageBytes       = call.argument<ByteArray>("frame")
                val captureMode      = call.argument<String>("captureMode") ?: "SINGLE_FINGER"
                val fingersRequested = call.argument<List<String>>("fingersRequested") ?: emptyList()
                val missingFingers   = call.argument<List<String>>("missingFingers")   ?: emptyList()

                if (imageBytes == null) {
                    result.error("NO_FRAME", "Frame data not received", null)
                    return
                }

                android.util.Log.d("BiometricSDK",
                    "processFrame: ${imageBytes.size} bytes, mode=$captureMode")

                val frameResult = processFrame(
                    imageBytes, captureMode, fingersRequested, missingFingers
                )

                android.util.Log.d("BiometricSDK_CAPTURE",
                    "readyToCapture=${frameResult["readyToCapture"]}, " +
                    "quality=${frameResult["qualityScore"]}, " +
                    "guidance=${frameResult["guidance"]}")

                result.success(frameResult)
            }

            "startCapture" -> {
                val mode           = call.argument<String>("captureMode")          ?: "SINGLE_FINGER"
                val imageBytes     = call.argument<ByteArray>("image")
                val missingFingers = call.argument<List<String>>("missingFingers") ?: emptyList()
                val detectedFinger = call.argument<String>("detectedFinger")       ?: ""
                val actualQuality  = call.argument<Int>("qualityScore")            ?: 0

                android.util.Log.d("BiometricSDK_CAPTURE",
                    "startCapture — mode=$mode, quality=$actualQuality, finger=$detectedFinger")

                handleCapture(mode, imageBytes, missingFingers, detectedFinger, actualQuality, result)
            }

            else -> result.notImplemented()
        }
    }

    // ─────────────────────────────────────────────────────────────────────────
    // processFrame — real-time feedback per camera frame
    // ─────────────────────────────────────────────────────────────────────────

    private fun processFrame(
        imageBytes: ByteArray,
        captureMode: String,
        fingersRequested: List<String>,
        missingFingers: List<String>
    ): Map<String, Any> {
        return try {
             // 1. Decode + rotate
        val rawBitmap = BitmapFactory.decodeByteArray(imageBytes, 0, imageBytes.size)
    ?: return noHandResult("Image could not be loaded", "Decode failed")
val bitmap = rotateBitmap(rawBitmap)
saveBitmapForDebug(bitmap)
android.util.Log.d("BiometricSDK", "After rotate: ${bitmap.width}x${bitmap.height}")

// 2. Quality assessment (landmarks ke bina)
val qualityReport   = assessAdvancedQualityNoLandmarks(bitmap)
val overallQuality  = qualityReport["overallScore"]   as? Int ?: 0
val blurScore       = qualityReport["blurScore"]       as? Int ?: 0
val brightnessScore = qualityReport["brightnessScore"] as? Int ?: 0
val centeringScore  = qualityReport["centeringScore"]  as? Int ?: 50
val isBlurry        = qualityReport["isTooBlurry"]     as? Boolean ?: false

// 3. MediaPipe detection — PEHLE karo
val argbBitmap       = bitmap.copy(Bitmap.Config.ARGB_8888, false)
val mpImage          = BitmapImageBuilder(argbBitmap).build()
val landmarkerResult = handLandmarker?.detect(mpImage)

android.util.Log.d("BiometricSDK",
    "Landmarks detected: ${landmarkerResult?.landmarks()?.size ?: 0}")

// 4. Stability check — landmarks mile ke BAAD
//val isStable = isHandStable(landmarkerResult?.landmarks()?.getOrNull(0))
val isStable = true  // TEMP: motion check bypassed for testing

// 5. No hand check
if (landmarkerResult == null || landmarkerResult.landmarks().isEmpty()) {
    return noHandResult(
        guidance      = "Place your hand in front of the camera",
        error         = "No hand detected",
        qualityReport = qualityReport
    )
}
            // 5. Extract landmarks & handedness ──────────────────────────────
            val landmarks = landmarkerResult.landmarks()[0]
            var handedness = landmarkerResult.handednesses()[0][0].categoryName()
            val isCentered = checkHandCentering(landmarks,captureMode )
            val isTooFar = checkHandTooFar(landmarks, captureMode)

            // 6. Finger detection ─────────────────────────────────────────────
            val allDetectedFingers = getDetectedFingers(landmarks, handedness,bitmap, captureMode).toMutableList()

       if (captureMode == "LEFT_THUMB") {
                val thumbOnly = allDetectedFingers.filter { it.contains("THUMB") }

                // 👇 FIX: Agar Thumb nahi mila, YA Thumb ke sath koi aur ungli bhi khuli hai (size > 1) -> REJECT
                if (thumbOnly.size != 1 || allDetectedFingers.size > 1) {
                    return mapOf(
                        "fingerDetected" to false,
                        "confidence" to 0.0,
                        "guidance" to "Show only your left thumb — fold other fingers",
                        "qualityScore" to overallQuality,
                        "readyToCapture" to false,
                        "validationPassed" to false,
                        "livenessPassed" to false
                    )
                }

                // 👉 override detected fingers
                allDetectedFingers.clear()
                allDetectedFingers.add(thumbOnly[0])
            }

            if (captureMode == "RIGHT_THUMB") {
                val thumbOnly = allDetectedFingers.filter { it.contains("THUMB") }

                // 👇 FIX: Same strict check for right thumb
                if (thumbOnly.size != 1 || allDetectedFingers.size > 1) {
                    return mapOf(
                        "fingerDetected" to false,
                        "confidence" to 0.0,
                        "guidance" to "Show only your right thumb — fold other fingers",
                        "qualityScore" to overallQuality,
                        "readyToCapture" to false,
                        "validationPassed" to false,
                        "livenessPassed" to false
                    )
                }

                // 👉 override detected fingers
                allDetectedFingers.clear()
                allDetectedFingers.add(thumbOnly[0])
            }
  if (captureMode == "SINGLE_FINGER") {
                // 👉 Call the strict function
                val primaryFinger = getPrimaryFinger(landmarks, handedness,bitmap)

                // ❌ Agar koi finger nahi mili ya 2 ungliyan mil gayi (confusion)
                if (primaryFinger.isEmpty()) {
                    return mapOf(
                        "fingerDetected" to false,
                        "confidence" to 0.0,
                        "guidance" to "Show exactly ONE finger clearly", // 👈 This will guide the user properly
                        "qualityScore" to overallQuality,
                        "readyToCapture" to false,
                        "validationPassed" to false,
                        "livenessPassed" to false
                    )
                }

                // ✅ Ekdum clear single finger mil gayi
                allDetectedFingers.clear()
                allDetectedFingers.add(primaryFinger)
            }

            val detectedFingersStr = allDetectedFingers.joinToString(", ")
            android.util.Log.d("BiometricSDK", "Detected fingers: $detectedFingersStr")

            // 7. Capture-mode strict validation ──────────────────────────────
            val (isValid, validationMsg) = validateCaptureMode(
                captureMode, allDetectedFingers, handedness, fingersRequested, missingFingers
            )

            // 8. Liveness check (rule-based) ──────────────────────────────────
            val (livenessPassed, livenessScore) = checkLiveness(landmarks, bitmap)

            // 9. Finger bounding boxes ────────────────────────────────────────
            val fingerBoxes = buildFingerBoxes(landmarks, handedness)

            // 10. Guidance message ────────────────────────────────────────────
            // val guidance = buildGuidance(
            //     isValid, validationMsg, livenessPassed,
            //     overallQuality, isBlurry, brightnessScore, centeringScore,
            //     isCentered, isTooFar,
            //     captureMode, allDetectedFingers.size, landmarks, qualityReport,
            //     isStable,

            // )
            // 10. Guidance message ────────────────────────────────────────────
            val guidance = buildGuidance(
                isValid = isValid,
                validationMsg = validationMsg,
                livenessPassed = livenessPassed,
                overallQuality = overallQuality,
                isBlurry = isBlurry,
                brightnessScore = brightnessScore,
                isCentered = isCentered,
                isTooFar = isTooFar,
                captureMode = captureMode,
                isStable = isStable
            )

            // 11. readyToCapture gate ─────────────────────────────────────────
            val readyToCapture = isValid &&
                overallQuality >= QUALITY_THRESHOLD_CAPTURE &&
                livenessPassed &&
                !isBlurry&&
                isCentered && // 👈 TC_12: Center mein nahi hai toh capture mat karo
                !isTooFar&&
                 isStable

            android.util.Log.d("BiometricSDK_CAPTURE", """
                === READINESS ===
                captureMode    = $captureMode
                isValid        = $isValid  [$validationMsg]
                overallQuality = $overallQuality (need >= $QUALITY_THRESHOLD_CAPTURE)
                blurScore      = $blurScore (blurry if < $BLUR_THRESHOLD_MOTION)
                liveness       = $livenessPassed
                detectedFings  = $allDetectedFingers
                readyToCapture = $readyToCapture
                guidance       = $guidance
                =================
            """.trimIndent())

            // 12. Native C++ processing ───────────────────────────────────────
            val points = FloatArray(42)
            for (i in 0 until 21) {
                points[i * 2]     = landmarks[i].x()
                points[i * 2 + 1] = landmarks[i].y()
            }

            // val processedBytes = NativeProcessor.processFingerprint(
            //     imageBytes, bitmap.width, bitmap.height, points
            // )

            val processedBytes = when (captureMode) {

    // ✅ THUMB
    "LEFT_THUMB", "RIGHT_THUMB" -> {
        val thumbBitmap = cropThumbRegion(bitmap, landmarks)

        val stream = ByteArrayOutputStream()
        thumbBitmap.compress(Bitmap.CompressFormat.JPEG, 90, stream)

        NativeProcessor.processFingerprint(
            stream.toByteArray(),
            thumbBitmap.width,
            thumbBitmap.height,
            points
        )
    }

    // ✅ SINGLE FINGER (NEW 🔥)
    "SINGLE_FINGER" -> {
        val fingerBitmap = cropSingleFinger(bitmap, landmarks)

        val stream = ByteArrayOutputStream()
        fingerBitmap.compress(Bitmap.CompressFormat.JPEG, 90, stream)

        NativeProcessor.processFingerprint(
            stream.toByteArray(),
            fingerBitmap.width,
            fingerBitmap.height,
            points
        )
    }

    // ✅ FOUR FINGERS
    "LEFT_FOUR", "RIGHT_FOUR" -> {
        NativeProcessor.processFingerprint(
            imageBytes,
            bitmap.width,
            bitmap.height,
            points
        )
    }

    else -> {
        NativeProcessor.processFingerprint(
            imageBytes,
            bitmap.width,
            bitmap.height,
            points
        )
    }
}

            val processedBase64 = if (processedBytes != null && processedBytes.size > 1) {
                val buffer  = java.nio.ByteBuffer.wrap(processedBytes)
                val imgSize = buffer.int
                val imgData = ByteArray(imgSize)
                buffer.get(imgData)
                Base64.encodeToString(imgData, Base64.NO_WRAP)
            } else ""

            val confidence = (overallQuality / 100.0 * 0.7 + livenessScore * 0.3)
                .coerceIn(0.0, 1.0)

            mapOf(
                "fingerDetected"      to allDetectedFingers.isNotEmpty(),
                "confidence"          to confidence,
                "guidance"            to guidance,
                "qualityScore"        to overallQuality,
                "qualityDetails"      to qualityReport,
                "fingerCount"         to allDetectedFingers.size,
                "detectedFingersName" to detectedFingersStr,
                "firstDetectedFinger" to (allDetectedFingers.firstOrNull() ?: ""),
                "fingerBoxes"         to fingerBoxes,
                "processedImage"      to processedBase64,
                "readyToCapture"      to readyToCapture,
                "validationPassed"    to isValid,
                "livenessPassed"      to livenessPassed,
                "validationError"     to (if (isValid) "" else validationMsg)
            )

        } catch (e: Exception) {
            android.util.Log.e("BiometricSDK", "processFrame error: ${e.message}", e)
            noHandResult("Error: ${e.message}", e.message ?: "Unknown error")
        }
    }

    // ─────────────────────────────────────────────────────────────────────────
    // FINGER BOUNDING BOXES
    // ─────────────────────────────────────────────────────────────────────────

    private fun buildFingerBoxes(
        landmarks: List<NormalizedLandmark>,
        handedness: String
    ): List<Map<String, Any>> {
        val fingerLandmarksIndices = listOf(
            listOf(1, 2, 3, 4)    to "THUMB",
            listOf(5, 6, 7, 8)    to "INDEX",
            listOf(9, 10, 11, 12) to "MIDDLE",
            listOf(13, 14, 15, 16) to "RING",
            listOf(17, 18, 19, 20) to "LITTLE"
        )
        val padding = 0.04f
        return fingerLandmarksIndices.map { (indices, fingerName) ->
            val rawPoints = indices.map { landmarks[it] }
            // Portrait mode fix: swap X/Y and mirror
            val xValues = rawPoints.map { it.y() }
            val yValues = rawPoints.map { 1.0f - it.x() }
            mapOf(
                "x1"    to ((xValues.minOrNull() ?: 0f) - padding).coerceIn(0f, 1f),
                "y1"    to ((yValues.minOrNull() ?: 0f) - padding).coerceIn(0f, 1f),
                "x2"    to ((xValues.maxOrNull() ?: 1f) + padding).coerceIn(0f, 1f),
                "y2"    to ((yValues.maxOrNull() ?: 1f) + padding).coerceIn(0f, 1f),
                "label" to "${handedness.uppercase()}_$fingerName"
            )
        }
    }

    // ─────────────────────────────────────────────────────────────────────────
    // STRICT CAPTURE MODE VALIDATION
    // ─────────────────────────────────────────────────────────────────────────

    private fun validateCaptureMode(
        captureMode: String,
        detectedFingers: List<String>,
        correctedHandedness: String,
        fingersRequested: List<String>,
        missingFingers: List<String>
    ): Pair<Boolean, String> {
        return when (captureMode) {

            "LEFT_FOUR" -> {
                val required = listOf("LEFT_INDEX", "LEFT_MIDDLE", "LEFT_RING", "LEFT_LITTLE")
                when {
                    correctedHandedness == "Right"               -> false to "Please use your LEFT hand"
                    detectedFingers.any { it.startsWith("RIGHT_") } -> false to "Please use your LEFT hand"
                    "LEFT_THUMB" in detectedFingers              -> false to "Please hide your thumb — show 4 fingers only"
                    detectedFingers.size < 4                     -> false to "Show all 4 fingers: Index, Middle, Ring and Little"
                    detectedFingers.size > 4                     -> false to "Too many fingers — show exactly 4 fingers"
                    !detectedFingers.containsAll(required)       -> false to "Ensure Index, Middle, Ring and Little are all visible"
                    else                                         -> true  to "Valid"
                }
            }

            "RIGHT_FOUR" -> {
                val required = listOf("RIGHT_INDEX", "RIGHT_MIDDLE", "RIGHT_RING", "RIGHT_LITTLE")
                when {
                    correctedHandedness == "Left"                -> false to "Please use your RIGHT hand"
                    detectedFingers.any { it.startsWith("LEFT_") }  -> false to "Please use your RIGHT hand"
                    "RIGHT_THUMB" in detectedFingers             -> false to "Please hide your thumb — show 4 fingers only"
                    detectedFingers.size < 4                     -> false to "Show all 4 fingers: Index, Middle, Ring and Little"
                    detectedFingers.size > 4                     -> false to "Too many fingers — show exactly 4 fingers"
                    !detectedFingers.containsAll(required)       -> false to "Ensure Index, Middle, Ring and Little are all visible"
                    else                                         -> true  to "Valid"
                }
            }

            "LEFT_THUMB" -> when {
                correctedHandedness == "Right"               -> false to "Please use your LEFT hand"
                detectedFingers.any { it.startsWith("RIGHT_") } -> false to "Please use your LEFT hand"
                detectedFingers.isEmpty()                    -> false to "Show only your left thumb"
                "LEFT_THUMB" !in detectedFingers             -> false to "Left thumb not detected — extend your left thumb"
                detectedFingers.size > 1                     -> false to "Show only your left thumb — fold all other fingers"
                else                                         -> true  to "Valid"
            }

            "RIGHT_THUMB" -> when {
                correctedHandedness == "Left"                -> false to "Please use your RIGHT hand"
                detectedFingers.any { it.startsWith("LEFT_") }  -> false to "Please use your RIGHT hand"
                detectedFingers.isEmpty()                    -> false to "Show only your right thumb"
                "RIGHT_THUMB" !in detectedFingers            -> false to "Right thumb not detected — extend your right thumb"
                detectedFingers.size > 1                     -> false to "Show only your right thumb — fold all other fingers"
                else                                         -> true  to "Valid"
            }

            "SINGLE_FINGER" -> when {
                detectedFingers.isEmpty()  -> false to "Show one finger clearly in front of the camera"
                detectedFingers.size > 1   -> false to "Too many fingers — show only one finger at a time"
                else                       -> true  to "Valid"
            }

            "CUSTOM_SEQUENCE" -> when {
                fingersRequested.isEmpty() -> false to "No fingers specified for custom sequence"
                detectedFingers.isEmpty()  -> false to "No fingers detected — place your hand in view"
                else -> {
                    val missing = fingersRequested.filter { it !in detectedFingers }
                    val extra   = detectedFingers.filter  { it !in fingersRequested }
                    when {
                        missing.isNotEmpty() -> false to "Missing: ${missing.humanReadable()}"
                        extra.isNotEmpty()   -> false to "Extra finger(s) — hide: ${extra.humanReadable()}"
                        else                 -> true  to "Valid"
                    }
                }
            }

            "PARTIAL_CAPTURE" -> {
                val expected = fingersRequested.filter { it !in missingFingers }
                when {
                    expected.isEmpty()        -> true  to "Valid (all fingers marked as missing)"
                    detectedFingers.isEmpty() -> false to "No fingers detected — place your hand in view"
                    else -> {
                        val missing = expected.filter        { it !in detectedFingers }
                        val extra   = detectedFingers.filter { it !in expected && it !in missingFingers }
                        when {
                            missing.isNotEmpty() -> false to "Missing: ${missing.humanReadable()}"
                            extra.isNotEmpty()   -> false to "Unexpected finger: ${extra.humanReadable()}"
                            else                 -> true  to "Valid"
                        }
                    }
                }
            }

            else -> false to "Unknown capture mode: $captureMode"
        }
    }

    // ─────────────────────────────────────────────────────────────────────────
    // FINGER DETECTION
    // ─────────────────────────────────────────────────────────────────────────



 private fun getDetectedFingers(
    landmarks: List<NormalizedLandmark>,
    correctedHandedness: String,
    bitmap: Bitmap,
    captureMode: String   // 👈 captureMode add karo
): List<String> {
    val fingers = mutableListOf<String>()
    val prefix = correctedHandedness.uppercase()

    when (captureMode) {

       "LEFT_FOUR" -> {
    if (isThumbExtendedForLeftThumb(landmarks, bitmap).first)
        fingers.add("${prefix}_THUMB")
    if (isFingerExtendedForLeftFour(landmarks, 5, 6, 7, 8, bitmap).first)      // 👈 bitmap
        fingers.add("${prefix}_INDEX")
    if (isFingerExtendedForLeftFour(landmarks, 9, 10, 11, 12, bitmap).first)   // 👈 bitmap
        fingers.add("${prefix}_MIDDLE")
    if (isFingerExtendedForLeftFour(landmarks, 13, 14, 15, 16, bitmap).first)  // 👈 bitmap
        fingers.add("${prefix}_RING")
    if (isFingerExtendedForLeftFour(landmarks, 17, 18, 19, 20, bitmap).first)  // 👈 bitmap
        fingers.add("${prefix}_LITTLE")
}

     "RIGHT_FOUR" -> {
    if (isThumbExtendedForRightThumb(landmarks, bitmap).first)
        fingers.add("${prefix}_THUMB")
    if (isFingerExtendedForRightFour(landmarks, 5, 6, 7, 8, bitmap).first)     // 👈 bitmap
        fingers.add("${prefix}_INDEX")
    if (isFingerExtendedForRightFour(landmarks, 9, 10, 11, 12, bitmap).first)  // 👈 bitmap
        fingers.add("${prefix}_MIDDLE")
    if (isFingerExtendedForRightFour(landmarks, 13, 14, 15, 16, bitmap).first) // 👈 bitmap
        fingers.add("${prefix}_RING")
    if (isFingerExtendedForRightFour(landmarks, 17, 18, 19, 20, bitmap).first) // 👈 bitmap
        fingers.add("${prefix}_LITTLE")
       }

        "LEFT_THUMB" -> {
          if (isThumbExtendedForLeftThumb(landmarks, bitmap).first)
                fingers.add("${prefix}_THUMB")
             if (isFingerExtendedForSingleFinger(landmarks, 5, 6, 7, 8, bitmap).first) fingers.add("${prefix}_INDEX")
            if (isFingerExtendedForSingleFinger(landmarks, 9, 10, 11, 12, bitmap).first) fingers.add("${prefix}_MIDDLE")
            if (isFingerExtendedForSingleFinger(landmarks, 13, 14, 15, 16, bitmap).first) fingers.add("${prefix}_RING")
            if (isFingerExtendedForSingleFinger(landmarks, 17, 18, 19, 20, bitmap).first) fingers.add("${prefix}_LITTLE")
        }

        "RIGHT_THUMB" -> {
            if (isThumbExtendedForRightThumb(landmarks, bitmap).first)
                fingers.add("${prefix}_THUMB")
            if (isFingerExtendedForSingleFinger(landmarks, 5, 6, 7, 8, bitmap).first) fingers.add("${prefix}_INDEX")
            if (isFingerExtendedForSingleFinger(landmarks, 9, 10, 11, 12, bitmap).first) fingers.add("${prefix}_MIDDLE")
            if (isFingerExtendedForSingleFinger(landmarks, 13, 14, 15, 16, bitmap).first) fingers.add("${prefix}_RING")
            if (isFingerExtendedForSingleFinger(landmarks, 17, 18, 19, 20, bitmap).first) fingers.add("${prefix}_LITTLE")
        }

        "SINGLE_FINGER" -> {
            if (isThumbExtendedForSingleFinger(landmarks).first)
                fingers.add("${prefix}_THUMB")
            if (isFingerExtendedForSingleFinger(landmarks, 5, 6, 7, 8, bitmap).first)
                fingers.add("${prefix}_INDEX")
            if (isFingerExtendedForSingleFinger(landmarks, 9, 10, 11, 12, bitmap).first)
                fingers.add("${prefix}_MIDDLE")
            if (isFingerExtendedForSingleFinger(landmarks, 13, 14, 15, 16, bitmap).first)
                fingers.add("${prefix}_RING")
            if (isFingerExtendedForSingleFinger(landmarks, 17, 18, 19, 20, bitmap).first)
                fingers.add("${prefix}_LITTLE")
        }
    }

    android.util.Log.d("BiometricSDK", "Detected [$prefix][$captureMode]: $fingers")
    return fingers
}

//     // ─────────────────────────────────────────────────────────────────────────
    // GUIDANCE
    // ─────────────────────────────────────────────────────────────────────────

    // private fun buildGuidance(
    //     isValid: Boolean,
    //     validationMsg: String,
    //     livenessPassed: Boolean,
    //     overallQuality: Int,
    //     isBlurry: Boolean,
    //     brightnessScore: Int,
    //     isCentered: Boolean, // 👈 Added
    //     isTooFar: Boolean,
    //     centeringScore: Int,
    //     captureMode: String,
    //     fingerCount: Int,
    //     landmarks: List<NormalizedLandmark>,
    //     qualityDetails: Map<String, Any>,
    //     isStable: Boolean
    // ): String = when {
    //     !isStable -> "Hold your hand steady — detecting motion"
    //     !livenessPassed                          -> "Liveness check failed — ensure a real hand is present"
    //     !isCentered                              -> "Center your hand in the frame"  // 👈 TC_12 Message
    //     isTooFar                                 -> "Move closer to the camera"
    //     overallQuality < QUALITY_THRESHOLD_CAPTURE -> "Improve image quality — hold steady in good light"
    //     !isValid                                 -> validationMsg
    //     isBlurry                                 -> "Hold the camera still — image is blurry"
    //     brightnessScore < BRIGHTNESS_MIN         -> "Increase lighting for better quality"
    //     centeringScore  < CENTERING_MIN          -> "Center your hand in the frame"
    //     overallQuality  < QUALITY_THRESHOLD_GOOD -> "Move closer to the camera"
    //     else -> when (captureMode) {
    //         "LEFT_FOUR"     -> "Show palm side — hold still!"
    //         "RIGHT_FOUR"    -> "Show palm side — hold still!"
    //         "LEFT_THUMB"    -> "Hold still — capturing left thumb!"
    //         "RIGHT_THUMB"   -> "Hold still — capturing right thumb!"
    //         "SINGLE_FINGER" -> "Hold still — capturing finger!"
    //         else            -> "Hold still — capturing now!"
    //     }
    // }

    // ─────────────────────────────────────────────────────────────────────────
    // GUIDANCE
    // ─────────────────────────────────────────────────────────────────────────

    private fun buildGuidance(
        isValid: Boolean,
        validationMsg: String,
        livenessPassed: Boolean,
        overallQuality: Int,
        isBlurry: Boolean,
        brightnessScore: Int,
        isCentered: Boolean, 
        isTooFar: Boolean,   
        captureMode: String,
        isStable: Boolean
    ): String = when {
        !isStable                                -> "Hold your hand steady — detecting motion"
        !livenessPassed                          -> "Liveness check failed — ensure a real hand is present"
        !isCentered                              -> "Center your hand in the frame"  // 👈 Naya TC_12
        isTooFar                                 -> "Move closer to the camera"      // 👈 Naya TC_13
        overallQuality < QUALITY_THRESHOLD_CAPTURE -> "Improve image quality — hold steady in good light"
        !isValid                                 -> validationMsg
        isBlurry                                 -> "Hold the camera still — image is blurry"
        brightnessScore < BRIGHTNESS_MIN         -> "Increase lighting for better quality"
        else -> when (captureMode) {
            "LEFT_FOUR"     -> "Show palm side — hold still!"
            "RIGHT_FOUR"    -> "Show palm side — hold still!"
            "LEFT_THUMB"    -> "Hold still — capturing left thumb!"
            "RIGHT_THUMB"   -> "Hold still — capturing right thumb!"
            "SINGLE_FINGER" -> "Hold still — capturing finger!"
            else            -> "Hold still — capturing now!"
        }
    }

    // ─────────────────────────────────────────────────────────────────────────
    // LIVENESS CHECK (rule-based)
    // ─────────────────────────────────────────────────────────────────────────

    private fun checkLiveness(
        landmarks: List<NormalizedLandmark>,
        bitmap: Bitmap
    ): Pair<Boolean, Double> {
        var score = 0.0

        // 1. Palm/finger length ratio
        val wristX = landmarks[0].x(); val wristY = landmarks[0].y()
        val mcpX   = landmarks[9].x(); val mcpY   = landmarks[9].y()
        val tipX   = landmarks[12].x(); val tipY  = landmarks[12].y()
        val palmLen   = dist(wristX, wristY, mcpX, mcpY)
        val fingerLen = dist(mcpX, mcpY, tipX, tipY)
        if (palmLen > 0.01f && fingerLen > 0.01f && palmLen / fingerLen in 0.5..2.5) score += 1.0

        // 2. Tip spread
        val tipXs     = listOf(landmarks[4].x(), landmarks[8].x(), landmarks[12].x(),
                               landmarks[16].x(), landmarks[20].x())
        val tipXRange = (tipXs.maxOrNull() ?: 0f) - (tipXs.minOrNull() ?: 0f)
        if (tipXRange > 0.05f) score += 1.0

        // 3. Z-depth variation
        val zValues = landmarks.map { it.z() }
        val zRange  = (zValues.maxOrNull() ?: 0f) - (zValues.minOrNull() ?: 0f)
        if (zRange > 0.05f) score += 1.0

        // 4. Skin-tone texture variance in palm
        val palmCx  = ((wristX + mcpX) / 2 * bitmap.width).toInt().coerceIn(0, bitmap.width - 1)
        val palmCy  = ((wristY + mcpY) / 2 * bitmap.height).toInt().coerceIn(0, bitmap.height - 1)
        val sampleW = (bitmap.width  * 0.06).toInt().coerceAtLeast(8)
        val sampleH = (bitmap.height * 0.06).toInt().coerceAtLeast(8)
        val reds    = mutableListOf<Int>()
        val greens  = mutableListOf<Int>()
        for (dy in -sampleH..sampleH step 2) {
            for (dx in -sampleW..sampleW step 2) {
                val px    = (palmCx + dx).coerceIn(0, bitmap.width  - 1)
                val py    = (palmCy + dy).coerceIn(0, bitmap.height - 1)
                val pixel = bitmap.getPixel(px, py)
                reds.add(Color.red(pixel))
                greens.add(Color.green(pixel))
            }
        }
        if (reds.isNotEmpty()) {
            val redStd   = stdDev(reds.map   { it.toDouble() })
            val greenStd = stdDev(greens.map { it.toDouble() })
            if (redStd > 4.0 && greenStd > 4.0) score += 1.0
        }

        val normalised = score / 4.0
        return Pair(normalised >= 0.5, normalised)
    }

    // ─────────────────────────────────────────────────────────────────────────
    // QUALITY ASSESSMENT — C++ first, Kotlin fallback
    // ─────────────────────────────────────────────────────────────────────────

    /**
     * Quality assessment without landmarks (used in processFrame before MediaPipe runs).
     * Falls back entirely to Kotlin if C++ is not available.
     */
    private fun assessAdvancedQualityNoLandmarks(bitmap: Bitmap): Map<String, Any> {
        return assessAdvancedQualityFallback(bitmap)
    }

    /**
     * Full quality assessment WITH landmarks — calls C++ for Laplacian blur etc.
     * Used in handleCapture for final quality score.
     */
    private fun assessAdvancedQuality(
        bitmap: Bitmap,
        landmarks:List<NormalizedLandmark>
    ): Map<String, Any> {
        try {
            val stream = java.io.ByteArrayOutputStream()
            bitmap.compress(Bitmap.CompressFormat.JPEG, 90, stream)
            val imageBytes  = stream.toByteArray()
            val handCenterX = (landmarks[9].x() * bitmap.width).toInt()
            val handCenterY = (landmarks[9].y() * bitmap.height).toInt()
            val points      = FloatArray(42)
            for (i in 0 until 21) {
                points[i * 2]     = landmarks[i].x()
                points[i * 2 + 1] = landmarks[i].y()
            }

            val metrics = NativeProcessor.calculateQualityMetrics(
                imageBytes, bitmap.width, bitmap.height, points, handCenterX, handCenterY
            )

            if (metrics != null && metrics.size >= 5) {
                val blurScore       = metrics[0].toInt()
                val brightnessScore = metrics[1].toInt()
                val centeringScore  = metrics[2].toInt()
                val sizeScore       = metrics[3].toInt()
                val overallScore    = metrics[4].toInt()

                android.util.Log.d("BiometricSDK",
                    "C++ Quality: blur=$blurScore, brightness=$brightnessScore, " +
                    "centering=$centeringScore, overall=$overallScore")

                return mapOf(
                    "overallScore"     to overallScore,
                    "blurScore"        to blurScore,
                    "brightnessScore"  to brightnessScore,
                    "centeringScore"   to centeringScore,
                    "sizeScore"        to sizeScore,
                    "isTooBlurry"      to (blurScore < BLUR_THRESHOLD_MOTION),
                    "isTooDark"        to (brightnessScore < BRIGHTNESS_MIN),
                    "isPoorlyCentered" to (centeringScore  < CENTERING_MIN),
                    "source"           to "cpp"
                )
            }
        } catch (e: Exception) {
            android.util.Log.e("BiometricSDK", "C++ quality failed: ${e.message}")
        }

        android.util.Log.d("BiometricSDK", "Using Kotlin fallback quality")
        return assessAdvancedQualityFallback(bitmap)
    }

    private fun assessAdvancedQualityFallback(bitmap: Bitmap): Map<String, Any> {
        val brightnessRaw   = getBrightness(bitmap, sampleCount = 200)
        val brightnessScore = when {
            brightnessRaw < 40  -> 0
            brightnessRaw < 60  -> 40
            brightnessRaw > 230 -> 0
            brightnessRaw > 200 -> 50
            else                -> 100
        }
        val sharpnessScore = computeSharpnessScore(bitmap)
        val coverageScore  = computeFingerCoverageScore(bitmap)
        val overallScore   = (
            sharpnessScore  * BLUR_WEIGHT +
            brightnessScore * BRIGHTNESS_WEIGHT +
            coverageScore   * CENTERING_WEIGHT
        ).toInt().coerceIn(0, 100)

        return mapOf(
            "overallScore"     to overallScore,
            "blurScore"        to sharpnessScore,
            "brightnessScore"  to brightnessScore,
            "centeringScore"   to 50,
            "sizeScore"        to coverageScore,
            "isTooBlurry"      to (sharpnessScore < BLUR_THRESHOLD_MOTION),
            "isTooDark"        to (brightnessRaw  < 60),
            "source"           to "kotlin"
        )
    }

    // ─────────────────────────────────────────────────────────────────────────
    // HANDLE CAPTURE — final biometric capture with C++ liveness + template
    // ─────────────────────────────────────────────────────────────────────────

    private fun handleCapture(
        mode: String,
        imageBytes: ByteArray?,
        missingFingers: List<String>,
        detectedFinger: String,
        actualQuality: Int,
        result: Result
    ) {
        try {
            val allFingers      = getFingerListForMode(mode, detectedFinger)
            val capturedFingers = allFingers.filter { it !in missingFingers }

            android.util.Log.d("BiometricSDK_CAPTURE",
                "Captured: $capturedFingers, Missing: $missingFingers")

            // ── C++ liveness + template extraction ───────────────────────────
            var livenessPassed        = false
            var processedImageBase64  = ""
            var finalIsoTemplateBase64 = ""

            if (imageBytes != null) {
                val rawBitmap = BitmapFactory.decodeByteArray(imageBytes, 0, imageBytes.size)
                if (rawBitmap != null) {
                    val rotated    = rotateBitmap(rawBitmap)
                    val argbBitmap = rotated.copy(Bitmap.Config.ARGB_8888, false)
                    val mpImage    = BitmapImageBuilder(argbBitmap).build()
                    val landmarks  = handLandmarker?.detect(mpImage)?.landmarks()?.firstOrNull()

                    if (landmarks != null) {
                        val points = FloatArray(42)
                        for (i in 0 until 21) {
                            points[i * 2]     = landmarks[i].x()
                            points[i * 2 + 1] = landmarks[i].y()
                        }

                        val processedBytes = NativeProcessor.processFingerprint(
                            imageBytes, rotated.width, rotated.height, points
                        )

                        when {
                            processedBytes == null -> {
                                livenessPassed = false
                                android.util.Log.w("BiometricSDK", "C++ returned null — spoof!")
                            }
                            processedBytes.size == 1 -> {
                                livenessPassed = false
                                android.util.Log.w("BiometricSDK", "SPOOF DETECTED by C++!")
                            }
                            else -> {
                                livenessPassed = true
                                val buffer       = java.nio.ByteBuffer.wrap(processedBytes)
                                val imgSize      = buffer.int
                                val imgData      = ByteArray(imgSize)
                                buffer.get(imgData)
                                val templateData = ByteArray(buffer.remaining())
                                buffer.get(templateData)

                                processedImageBase64   = Base64.encodeToString(imgData,      Base64.DEFAULT)
                                finalIsoTemplateBase64 = Base64.encodeToString(templateData, Base64.DEFAULT)

                                android.util.Log.d("BiometricSDK",
                                    "Liveness PASSED! Template size: ${templateData.size} bytes")
                            }
                        }
                    } else {
                        android.util.Log.w("BiometricSDK", "No landmarks in capture frame!")
                    }
                }
            }

            // ── Build finger results ──────────────────────────────────────────
            val fingerResults = capturedFingers.map { fingerId ->
                mapOf(
                    "fingerId"       to fingerId,
                    "status"         to if (livenessPassed) "success" else "failed",
                    "qualityScore"   to actualQuality,
                    "livenessPassed" to livenessPassed,
                    "template"       to if (livenessPassed) finalIsoTemplateBase64 else "",
                    "rawImage"       to (imageBytes?.let {
                        Base64.encodeToString(it, Base64.DEFAULT)
                    } ?: ""),
                    "processedImage" to processedImageBase64,
                    "errorCode"      to if (livenessPassed) null else "SPOOF_DETECTED",
                    "errorMessage"   to if (livenessPassed) null else "Liveness check failed"
                )
            } + missingFingers.map { fingerId ->
                mapOf(
                    "fingerId"       to fingerId,
                    "status"         to "missing",
                    "qualityScore"   to 0,
                    "livenessPassed" to false,
                    "template"       to "",
                    "rawImage"       to "",
                    "processedImage" to "",
                    "errorCode"      to "FINGER_MISSING",
                    "errorMessage"   to "Finger marked as missing by caller"
                )
            }

            val transactionId = UUID.randomUUID().toString()
            android.util.Log.d("BiometricSDK_CAPTURE", "SUCCESS — txnId=$transactionId")

            result.success(mapOf(
                "transactionId" to transactionId,
                "overallStatus" to if (livenessPassed) "success" else "failed",
                "captureMode"   to mode,
                "results"       to fingerResults
            ))

        } catch (e: Exception) {
            android.util.Log.e("BiometricSDK_CAPTURE", "Capture ERROR: ${e.message}", e)
            result.error("CAPTURE_ERROR", e.message, null)
        }
    }

    // ─────────────────────────────────────────────────────────────────────────
    // UTILITY HELPERS
    // ─────────────────────────────────────────────────────────────────────────

    private fun getRequiredHandedness(captureMode: String): String? = when (captureMode) {
        "LEFT_FOUR",  "LEFT_THUMB"  -> "Left"
        "RIGHT_FOUR", "RIGHT_THUMB" -> "Right"
        else                        -> null
    }

    private fun getFingerListForMode(mode: String, detectedFinger: String = ""): List<String> =
        when (mode) {
            "RIGHT_FOUR"    -> listOf("RIGHT_INDEX", "RIGHT_MIDDLE", "RIGHT_RING", "RIGHT_LITTLE")
            "LEFT_FOUR"     -> listOf("LEFT_INDEX",  "LEFT_MIDDLE",  "LEFT_RING",  "LEFT_LITTLE")
            "RIGHT_THUMB"   -> listOf("RIGHT_THUMB")
            "LEFT_THUMB"    -> listOf("LEFT_THUMB")
            "SINGLE_FINGER" -> listOf(if (detectedFinger.isNotEmpty()) detectedFinger else "RIGHT_INDEX")
            else            -> listOf(if (detectedFinger.isNotEmpty()) detectedFinger else "RIGHT_INDEX")
        }

    private fun noHandResult(
        guidance: String,
        error: String,
        qualityReport: Map<String, Any> = mapOf("overallScore" to 0)
    ): Map<String, Any> = mapOf(
        "fingerDetected"      to false,
        "confidence"          to 0.0,
        "guidance"            to guidance,
        "qualityScore"        to (qualityReport["overallScore"] as? Int ?: 0),
        "qualityDetails"      to qualityReport,
        "fingerCount"         to 0,
        "detectedFingersName" to "",
        "firstDetectedFinger" to "",
        "fingerBoxes"         to emptyList<Map<String, Any>>(),
        "processedImage"      to "",
        "readyToCapture"      to false,
        "validationPassed"    to false,
        "livenessPassed"      to false,
        "validationError"     to error
    )

    private fun dist(x1: Float, y1: Float, x2: Float, y2: Float): Float =
        sqrt((x2 - x1).pow(2) + (y2 - y1).pow(2))

    private fun stdDev(values: List<Double>): Double {
        if (values.isEmpty()) return 0.0
        val mean = values.average()
        return sqrt(values.map { (it - mean).pow(2) }.average())
    }

    private fun getBrightness(bitmap: Bitmap, sampleCount: Int = 100): Double {
        var total = 0L
        val w = bitmap.width; val h = bitmap.height
        repeat(sampleCount) {
            val x = (Math.random() * w).toInt().coerceIn(0, w - 1)
            val y = (Math.random() * h).toInt().coerceIn(0, h - 1)
            val p = bitmap.getPixel(x, y)
            total += ((p shr 16 and 0xff) + (p shr 8 and 0xff) + (p and 0xff)) / 3
        }
        return total.toDouble() / sampleCount
    }

    //  private fun computeSharpnessScore(bitmap: Bitmap): Int {
    //     val scaled = Bitmap.createScaledBitmap(bitmap, 200, 200, true)
    //     val w = scaled.width; val h = scaled.height
    //     var sumLap = 0.0
    //     var sumLapSq = 0.0
    //     var count = 0

    //     for (y in 1 until h - 1) {
    //         for (x in 1 until w - 1) {
    //             val c  = gray(scaled.getPixel(x,     y    ))
    //             val n  = gray(scaled.getPixel(x,     y - 1))
    //             val s  = gray(scaled.getPixel(x,     y + 1))
    //             val e  = gray(scaled.getPixel(x + 1, y    ))
    //             val ww = gray(scaled.getPixel(x - 1, y    ))

    //             // Laplacian response calculation
    //             val lap = (4 * c - n - s - e - ww).toDouble()
    //             sumLap += lap
    //             sumLapSq += lap * lap
    //             count++
    //         }
    //     }

    //     if (count == 0) return 0

    //     // True Variance Calculation (Strict Blur Check)
    //     val mean = sumLap / count
    //     val variance = (sumLapSq / count) - (mean * mean)

    //     // Strict thresholds: 
    //     // Variance <= 10 means extremely blurry -> Score 0
    //     // Variance >= 150 means extremely sharp -> Score 100
    //     val score = if (variance <= 10.0) {
    //         0.0
    //     } else if (variance >= 150.0) {
    //         100.0
    //     } else {
    //         ((variance - 10.0) / 140.0) * 100.0
    //     }

    //     android.util.Log.d("BiometricSDK_BLUR", "Kotlin Variance: $variance -> Score: $score")
    //     return score.toInt().coerceIn(0, 100)
    // }

    private fun computeSharpnessScore(bitmap: Bitmap): Int {
        // 👇 FIX 1: Image ko chota (scale) mat karo, isse asli blur chhup jata hai
        val w = bitmap.width
        val h = bitmap.height
        
        var sumLap = 0.0
        var sumLapSq = 0.0
        var count = 0

        // 👇 FIX 2: Fast calculation ke liye step 2 use karo (har alternate pixel)
        for (y in 1 until h - 1 step 2) {
            for (x in 1 until w - 1 step 2) {
                val c  = gray(bitmap.getPixel(x,     y    ))
                val n  = gray(bitmap.getPixel(x,     y - 1))
                val s  = gray(bitmap.getPixel(x,     y + 1))
                val e  = gray(bitmap.getPixel(x + 1, y    ))
                val ww = gray(bitmap.getPixel(x - 1, y    ))

                // Laplacian response (Edges detect karne ke liye)
                val lap = (4 * c - n - s - e - ww).toDouble()
                sumLap += lap
                sumLapSq += lap * lap
                count++
            }
        }

        if (count == 0) return 0

        // True Variance Calculation
        val mean = sumLap / count
        val variance = (sumLapSq / count) - (mean * mean)

        // 👇 FIX 3: Strict Thresholds lagao!
        // Camera ki raw image mein, hilte haath ka variance 100 se kam hota hai
        // Aur ekdum sharp haath ka variance 800 se upar hota hai.
        val minVar = 40.0
        val maxVar = 300.0

        val score = when {
            variance <= minVar -> 0.0
            variance >= maxVar -> 100.0
            else -> ((variance - minVar) / (maxVar - minVar)) * 100.0
        }

        android.util.Log.d("BiometricSDK_BLUR", "Kotlin Variance: $variance -> Score: $score")
        return score.toInt().coerceIn(0, 100)
    }

    private fun computeContrastScore(bitmap: Bitmap): Int {
        val w = bitmap.width; val h = bitmap.height
        val values = mutableListOf<Double>()
        repeat(300) {
            val p = bitmap.getPixel(
                (Math.random() * w).toInt().coerceIn(0, w - 1),
                (Math.random() * h).toInt().coerceIn(0, h - 1)
            )
            values.add(Color.red(p) * 0.299 + Color.green(p) * 0.587 + Color.blue(p) * 0.114)
        }
        val mean     = values.average()
        val variance = values.map { (it - mean) * (it - mean) }.average()
        return (sqrt(variance) / 60.0 * 100).toInt().coerceIn(0, 100)
    }

    private fun computeFingerCoverageScore(bitmap: Bitmap): Int {
        val scaled = Bitmap.createScaledBitmap(bitmap, 100, 100, true)
        val w = scaled.width; val h = scaled.height
        val bgBrightness = listOf(
            scaled.getPixel(0, 0), scaled.getPixel(w - 1, 0),
            scaled.getPixel(0, h - 1), scaled.getPixel(w - 1, h - 1)
        ).map { Color.red(it) * 0.299 + Color.green(it) * 0.587 + Color.blue(it) * 0.114 }.average()
        var fg = 0
        for (y in 0 until h) {
            for (x in 0 until w) {
                val px = scaled.getPixel(x, y)
                val br = Color.red(px) * 0.299 + Color.green(px) * 0.587 + Color.blue(px) * 0.114
                if (abs(br - bgBrightness) > 25) fg++
            }
        }
        return (fg.toDouble() / (w * h) / 0.5 * 100).toInt().coerceIn(0, 100)
    }

    private fun gray(pixel: Int): Int =
        (Color.red(pixel) * 0.299 + Color.green(pixel) * 0.587 + Color.blue(pixel) * 0.114).toInt()

    private fun rotateBitmap(bitmap: Bitmap): Bitmap = try {
        val matrix = Matrix()
        matrix.postRotate(270f)
        Bitmap.createBitmap(bitmap, 0, 0, bitmap.width, bitmap.height, matrix, true)
    } catch (e: Exception) { bitmap }

    private fun saveBitmapForDebug(bitmap: Bitmap) {
        try {
            val file = java.io.File(context.getExternalFilesDir(null), "debug_frame.jpg")
            FileOutputStream(file).use { bitmap.compress(Bitmap.CompressFormat.JPEG, 90, it) }
            android.util.Log.d("BiometricSDK", "Debug image saved: ${file.absolutePath}")
        } catch (e: Exception) {
            android.util.Log.e("BiometricSDK", "Debug save failed: ${e.message}")
        }
    }

    private fun generateTemplate(imageBytes: ByteArray?): String {
        if (imageBytes == null) {
            val dummy = ByteArray(512) { it.toByte() }
            return Base64.encodeToString(dummy, Base64.DEFAULT)
        }
        return Base64.encodeToString(imageBytes, Base64.DEFAULT)
    }

    private fun List<String>.humanReadable(): String =
        joinToString(", ") {
            it.replace("_", " ").lowercase().replaceFirstChar { c -> c.uppercase() }
        }
 // crop thumb
    private fun cropThumbRegion(
    bitmap: Bitmap,
    landmarks: List<NormalizedLandmark>
               ): Bitmap {
    val tip = landmarks[4]
    val ip  = landmarks[3]

    val cx = ((tip.x() + ip.x()) / 2 * bitmap.width).toInt()
    val cy = ((tip.y() + ip.y()) / 2 * bitmap.height).toInt()

    val size = (bitmap.width * 0.25).toInt()

    val x1 = (cx - size).coerceIn(0, bitmap.width - 1)
    val y1 = (cy - size).coerceIn(0, bitmap.height - 1)
    val x2 = (cx + size).coerceIn(0, bitmap.width)
    val y2 = (cy + size).coerceIn(0, bitmap.height)

    return Bitmap.createBitmap(bitmap, x1, y1, x2 - x1, y2 - y1)
}

private fun cropSingleFinger(
    bitmap: Bitmap,
    landmarks: List<NormalizedLandmark>
): Bitmap {

    // 👉 Assume index finger (default)
    val tip = landmarks[8]
    val pip = landmarks[6]

    val cx = ((tip.x() + pip.x()) / 2 * bitmap.width).toInt()
    val cy = ((tip.y() + pip.y()) / 2 * bitmap.height).toInt()

    val size = (bitmap.width * 0.20).toInt()

    val x1 = (cx - size).coerceIn(0, bitmap.width - 1)
    val y1 = (cy - size).coerceIn(0, bitmap.height - 1)
    val x2 = (cx + size).coerceIn(0, bitmap.width)
    val y2 = (cy + size).coerceIn(0, bitmap.height)

    return Bitmap.createBitmap(bitmap, x1, y1, x2 - x1, y2 - y1)
}

private fun getPrimaryFinger(
    landmarks: List<NormalizedLandmark>,
    handedness: String,
    bitmap: Bitmap
): String {

    val prefix = handedness.uppercase()
    val extendedFingers = mutableMapOf<String, Double>()

    // 👇 Check all 5 fingers
    val fingerChecks = mapOf(
        "INDEX"  to isFingerExtendedForSingleFinger(landmarks, 5, 6, 7, 8, bitmap),
        "MIDDLE" to isFingerExtendedForSingleFinger(landmarks, 9, 10, 11, 12, bitmap),
        "RING"   to isFingerExtendedForSingleFinger(landmarks, 13, 14, 15, 16, bitmap),
        "LITTLE" to isFingerExtendedForSingleFinger(landmarks, 17, 18, 19, 20, bitmap),
        "THUMB"  to isThumbExtendedForSingleFinger(landmarks)

    )

    // 👇 Only pick extended fingers
    fingerChecks.forEach { (finger, result) ->
        if (result.first) {
            extendedFingers[finger] = result.second
        }
    }

    // ❌ Reject if no finger OR multiple fingers
     if (extendedFingers.size != 1) {
        android.util.Log.d("BiometricSDK", 
            "getPrimaryFinger REJECTED — ${extendedFingers.size} fingers: ${extendedFingers.keys}")
        return ""
    }

    // ✅ Exactly ONE finger found
    val bestFinger = extendedFingers.keys.first()

    return "${prefix}_$bestFinger"
}


   private fun calculateAngle(landmarks: List<NormalizedLandmark>, a: Int, b: Int, c: Int): Double{
    val ax = landmarks[a].x()
    val ay = landmarks[a].y()

    val bx = landmarks[b].x()
    val by = landmarks[b].y()

    val cx = landmarks[c].x()
    val cy = landmarks[c].y()

    val abx = ax - bx
    val aby = ay - by
    val cbx = cx - bx
    val cby = cy - by

    val dot = (abx * cbx + aby * cby)
    val magAB = sqrt((abx * abx + aby * aby).toDouble())
    val magCB = sqrt((cbx * cbx + cby * cby).toDouble())

    //val cosAngle = dot / (magAB * magCB + 1e-6)
    val cosAngle = (dot / (magAB * magCB + 1e-6)).coerceIn(-1.0, 1.0)
    return Math.toDegrees(acos(cosAngle))
}






// private fun isFingerFrontFacing(
//     landmarks: List<NormalizedLandmark>,
//     bitmap: Bitmap,
//     tipIdx: Int,
//     mcpIdx: Int
// ): Boolean {
//     val tipX = (landmarks[tipIdx].x() * bitmap.width).toInt()
//     val tipY = (landmarks[tipIdx].y() * bitmap.height).toInt()
//     val mcpX = (landmarks[mcpIdx].x() * bitmap.width).toInt()
//     val mcpY = (landmarks[mcpIdx].y() * bitmap.height).toInt()

//     // 👇 3 points sample karo — tip, middle, mcp
//     val midX = ((tipX + mcpX) / 2).coerceIn(0, bitmap.width - 1)
//     val midY = ((tipY + mcpY) / 2).coerceIn(0, bitmap.height - 1)

//     val samplePoints = listOf(
//         Pair(tipX.coerceIn(0, bitmap.width-1), tipY.coerceIn(0, bitmap.height-1)),
//         Pair(midX, midY),
//         Pair(mcpX.coerceIn(0, bitmap.width-1), mcpY.coerceIn(0, bitmap.height-1))
//     )

//     val reds   = mutableListOf<Int>()
//     val greens = mutableListOf<Int>()
//     val blues  = mutableListOf<Int>()

//     // 👇 Har point ke around 5px tight sample
//     for ((cx, cy) in samplePoints) {
//         for (dy in -5..5 step 2) {
//             for (dx in -5..5 step 2) {
//                 val px = (cx + dx).coerceIn(0, bitmap.width - 1)
//                 val py = (cy + dy).coerceIn(0, bitmap.height - 1)
//                 val pixel = bitmap.getPixel(px, py)

//                 val r = Color.red(pixel)
//                 val g = Color.green(pixel)
//                 val b = Color.blue(pixel)

//                 // 👇 Strict skin filter
//                 // Skin: R high, G medium, B low
//                 // Background/nail: R-G-B close to each other ya B dominant
//                 if (r > 80 &&           // minimum brightness
//                     r > g &&            // red > green (skin condition)
//                     r > b &&            // red > blue (skin condition)
//                     r - b > 15 &&       // red-blue gap (key skin indicator)
//                     g > 40) {           // not too dark
//                     reds.add(r)
//                     greens.add(g)
//                     blues.add(b)
//                 }
//             }
//         }
//     }

//     android.util.Log.d("BiometricSDK_ORIENT",
//         "tip[$tipIdx] skinPixels=${reds.size}")

//     // 👇 Kam skin pixels = back side ya background
//     if (reds.size < 8) {
//         android.util.Log.d("BiometricSDK_ORIENT", "REJECTED — too few skin pixels")
//         return false  // 👈 pehle true tha — ab false kiya!
//     }

//     val avgRed   = reds.average()
//     val avgGreen = greens.average()
//     val avgBlue  = blues.average()

//     // Front (pulp) = warm, red dominant, R-B gap bada
//     val redBluGap     = avgRed - avgBlue
//     val redGreenRatio = avgRed / (avgGreen + 1.0)

//     val isFrontFacing = redBluGap > 20 &&        // 👈 R-B gap strict
//                         redGreenRatio > 1.05 &&   // 👈 red > green
//                         avgRed > 90               // 👈 bright enough

//     android.util.Log.d("BiometricSDK_ORIENT",
//         "R=${"%.0f".format(avgRed)} G=${"%.0f".format(avgGreen)} " +
//         "B=${"%.0f".format(avgBlue)} " +
//         "RBgap=${"%.1f".format(redBluGap)} " +
//         "RGratio=${"%.2f".format(redGreenRatio)} " +
//         "result=$isFrontFacing")

//     return isFrontFacing
// }
   private fun isFingerFrontFacing(
    landmarks: List<NormalizedLandmark>,
    bitmap: Bitmap,
    tipIdx: Int,
    mcpIdx: Int
): Boolean {

    // Z-depth check — MediaPipe mein
    // Front side: tip ka Z, mcp ke Z se chota (camera ke paas) hota hai
    // Back side: Z pattern ulta hota hai

    val tipZ = landmarks[tipIdx].z()
    val mcpZ = landmarks[mcpIdx].z()
    val wristZ = landmarks[0].z()

    // PIP landmark index calculate karo
    val pipIdx = when (tipIdx) {
        8  -> 6   // INDEX pip
        12 -> 10  // MIDDLE pip
        16 -> 14  // RING pip
        20 -> 18  // LITTLE pip
        else -> mcpIdx
    }
    val pipZ = landmarks[pipIdx].z()

    // Front side pe:
    // tip Z < pip Z < mcp Z (tip camera ke sabse paas)
    // Back side pe:
    // tip Z > pip Z (tip camera se door)

    val isTipCloserThanPip = tipZ < pipZ
    val isTipCloserThanMcp = tipZ < mcpZ

    android.util.Log.d("BiometricSDK_ORIENT",
        "tip[$tipIdx] tipZ=${"%.4f".format(tipZ)} " +
        "pipZ=${"%.4f".format(pipZ)} " +
        "mcpZ=${"%.4f".format(mcpZ)} " +
        "tipCloserPip=$isTipCloserThanPip " +
        "tipCloserMcp=$isTipCloserThanMcp")

    return isTipCloserThanPip && isTipCloserThanMcp
}
// ✅ 1. LEFT FOUR
private fun isFingerExtendedForLeftFour(
    landmarks: List<NormalizedLandmark>,
    mcp: Int, pip: Int, dip: Int, tip: Int,
    bitmap: Bitmap
): Pair<Boolean, Double> {
    val angle1 = calculateAngle(landmarks, mcp, pip, dip)
    val angle2 = calculateAngle(landmarks, pip, dip, tip)
    val avgAngle = (angle1 + angle2) / 2

    val tipX = landmarks[tip].x(); val tipY = landmarks[tip].y()
    val mcpX = landmarks[mcp].x(); val mcpY = landmarks[mcp].y()
    val wristX = landmarks[0].x(); val wristY = landmarks[0].y()

    val tipToMcpDist   = sqrt((tipX - mcpX).pow(2) + (tipY - mcpY).pow(2))
    val tipToWristDist = sqrt((tipX - wristX).pow(2) + (tipY - wristY).pow(2))

    // Four fingers ke liye thoda loose — saari 4 ek saath dikhani hain
    val isStraight     = avgAngle > 155
    val isFarFromWrist = tipToWristDist > 0.25
    val isFarFromMcp   = tipToMcpDist > 0.10
    

    return Pair(isStraight && isFarFromWrist && isFarFromMcp, avgAngle / 180.0)
}

// ✅ 2. RIGHT FOUR — same as LEFT FOUR (alag isliye ki future mein alag tune kar sako)
private fun isFingerExtendedForRightFour(
    landmarks: List<NormalizedLandmark>,
    mcp: Int, pip: Int, dip: Int, tip: Int,
    bitmap: Bitmap
): Pair<Boolean, Double> {
    val angle1 = calculateAngle(landmarks, mcp, pip, dip)
    val angle2 = calculateAngle(landmarks, pip, dip, tip)
    val avgAngle = (angle1 + angle2) / 2

    val tipX = landmarks[tip].x(); val tipY = landmarks[tip].y()
    val mcpX = landmarks[mcp].x(); val mcpY = landmarks[mcp].y()
    val wristX = landmarks[0].x(); val wristY = landmarks[0].y()

    val tipToMcpDist   = sqrt((tipX - mcpX).pow(2) + (tipY - mcpY).pow(2))
    val tipToWristDist = sqrt((tipX - wristX).pow(2) + (tipY - wristY).pow(2))

    val isStraight     = avgAngle > 155
    val isFarFromWrist = tipToWristDist > 0.25
    val isFarFromMcp   = tipToMcpDist > 0.10
   

   return Pair(isStraight && isFarFromWrist && isFarFromMcp, avgAngle / 180.0)
}

// ✅ 3. LEFT THUMB
private fun isThumbExtendedForLeftThumb(
    landmarks: List<NormalizedLandmark>,
    bitmap: Bitmap
): Pair<Boolean, Double> {
    val angle = calculateAngle(landmarks, 2, 3, 4)

    val tipX   = landmarks[4].x(); val tipY   = landmarks[4].y()
    val mcpX   = landmarks[2].x(); val mcpY   = landmarks[2].y()
    val wristX = landmarks[0].x(); val wristY = landmarks[0].y()

    val tipToWrist = sqrt((tipX - wristX).pow(2) + (tipY - wristY).pow(2))
    val isSideways  = abs(tipX - mcpX) > 0.12
    val isStraight  = angle > 150
    val isFarEnough = tipToWrist > 0.20
    val isFrontFacing = isFingerFrontFacing(landmarks, bitmap, 4, 2)

    android.util.Log.d("BiometricSDK_THUMB", 
        "LEFT_THUMB angle=${"%.1f".format(angle)} sideways=$isSideways far=$isFarEnough")

   return Pair(isStraight && isSideways && isFarEnough && isFrontFacing, angle / 180.0)
}

// ✅ 4. RIGHT THUMB
private fun isThumbExtendedForRightThumb(
    landmarks: List<NormalizedLandmark>,
    bitmap: Bitmap
): Pair<Boolean, Double> {
    val angle = calculateAngle(landmarks, 2, 3, 4)

    val tipX   = landmarks[4].x(); val tipY   = landmarks[4].y()
    val mcpX   = landmarks[2].x(); val mcpY   = landmarks[2].y()
    val wristX = landmarks[0].x(); val wristY = landmarks[0].y()

    val tipToWrist = sqrt((tipX - wristX).pow(2) + (tipY - wristY).pow(2))
    val isSideways  = abs(tipX - mcpX) > 0.12
    val isStraight  = angle > 150
    val isFarEnough = tipToWrist > 0.20
    val isFrontFacing = isFingerFrontFacing(landmarks, bitmap, 4, 2)

    android.util.Log.d("BiometricSDK_THUMB", 
        "RIGHT_THUMB angle=${"%.1f".format(angle)} sideways=$isSideways far=$isFarEnough")

    return Pair(isStraight && isSideways && isFarEnough && isFrontFacing, angle / 180.0)
}

// ✅ 5. SINGLE FINGER — sabse strict + front facing check
private fun isFingerExtendedForSingleFinger(
    landmarks: List<NormalizedLandmark>,
    mcp: Int, pip: Int, dip: Int, tip: Int,
    bitmap: Bitmap
): Pair<Boolean, Double> {
    val angle1 = calculateAngle(landmarks, mcp, pip, dip)
    val angle2 = calculateAngle(landmarks, pip, dip, tip)
    val avgAngle = (angle1 + angle2) / 2

    val tipX = landmarks[tip].x(); val tipY = landmarks[tip].y()
    val mcpX = landmarks[mcp].x(); val mcpY = landmarks[mcp].y()
    val wristX = landmarks[0].x(); val wristY = landmarks[0].y()

    val tipToMcpDist   = sqrt((tipX - mcpX).pow(2) + (tipY - mcpY).pow(2))
    val tipToWristDist = sqrt((tipX - wristX).pow(2) + (tipY - wristY).pow(2))

    // Single finger ke liye SABSE STRICT
    val isStraight     = avgAngle > 165
    val isFarFromWrist = tipToWristDist > 0.30
    val isFarFromMcp   = tipToMcpDist > 0.12

    // Front facing check sirf SINGLE FINGER mein
    val isFrontFacing = isFingerFrontFacing(landmarks, bitmap, tip, mcp)

    android.util.Log.d("BiometricSDK_SINGLE",
        "finger[$tip] angle=${"%.1f".format(avgAngle)} " +
        "wrist=${"%.3f".format(tipToWristDist)} " +
        "frontFacing=$isFrontFacing")

    return Pair(isStraight && isFarFromWrist && isFarFromMcp && isFrontFacing, avgAngle / 180.0)
}
// Thumb for single finger mode — alag tune kiya
private fun isThumbExtendedForSingleFinger(
    landmarks: List<NormalizedLandmark>
): Pair<Boolean, Double> {
    val angle = calculateAngle(landmarks, 2, 3, 4)
    val tipX   = landmarks[4].x()
    val mcpX   = landmarks[2].x()
    val wristX = landmarks[0].x()
    val wristY = landmarks[0].y()
    val tipY   = landmarks[4].y()

    val tipToWrist = sqrt((tipX - wristX).pow(2) + (tipY - wristY).pow(2))
    val isSideways  = abs(tipX - mcpX) > 0.14  // stricter for single
    val isStraight  = angle > 155
    val isFarEnough = tipToWrist > 0.22

    return Pair(isStraight && isSideways && isFarEnough, angle / 180.0)
}


// ── Landmark-based hand stability tracker ────────────────────
private val lastPositions = mutableMapOf<Int, Pair<Float, Float>>()
private var landmarkStableCount = 0

private fun isHandStable(
    landmarks: List<NormalizedLandmark>?
): Boolean {

    if (landmarks == null) {
        landmarkStableCount = 0
        return false
    }

    val keyPoints = listOf(0, 5, 9, 13, 17)

    var totalMovement = 0f

    for (i in keyPoints) {
        val x = landmarks[i].x()
        val y = landmarks[i].y()

        val last = lastPositions[i]
        val dx = if (last != null) abs(x - last.first) else 0f
        val dy = if (last != null) abs(y - last.second) else 0f

        totalMovement += (dx + dy)
        lastPositions[i] = Pair(x, y)
    }

    val avgMovement = totalMovement / keyPoints.size
    val moved = avgMovement > 0.025f

    if (!moved) {
        landmarkStableCount = (landmarkStableCount + 1).coerceAtMost(10)
    } else {
        landmarkStableCount = (landmarkStableCount - 1).coerceAtLeast(0)
    }

    // UX buffer (avoid flicker)
    if (landmarkStableCount < 2) return true

    return landmarkStableCount >= 6
}



    private fun checkHandTooFar(
        landmarks: List<NormalizedLandmark>,
        captureMode: String
    ): Boolean {
        return when (captureMode) {
            "LEFT_FOUR", "RIGHT_FOUR" -> {
                val dx = landmarks[12].x() - landmarks[0].x()
                val dy = landmarks[12].y() - landmarks[0].y()
                val handLength = sqrt((dx * dx + dy * dy).toDouble())

                handLength < 0.35
            }

              "SINGLE_FINGER" -> {
            val dx = landmarks[8].x() - landmarks[5].x()
            val dy = landmarks[8].y() - landmarks[5].y()
            val fingerLength = sqrt((dx * dx + dy * dy).toDouble())

            fingerLength < 0.12
        }

         // ✅ THUMB
        "LEFT_THUMB", "RIGHT_THUMB" -> {
            val dx = landmarks[4].x() - landmarks[2].x()
            val dy = landmarks[4].y() - landmarks[2].y()
            val thumbLength = sqrt((dx * dx + dy * dy).toDouble())

            thumbLength < 0.12
        }
        else -> false
        }
    }

    private fun checkHandCentering(
    landmarks: List<NormalizedLandmark>,
    captureMode: String
): Boolean {

    return when (captureMode) {


        // ✅ FULL HAND MODES
        "LEFT_FOUR", "RIGHT_FOUR" -> {
            val palmX = landmarks[9].x()
            val palmY = landmarks[9].y()

            val minX = landmarks.minOf { it.x() }
            val maxX = landmarks.maxOf { it.x() }
            val minY = landmarks.minOf { it.y() }
            val maxY = landmarks.maxOf { it.y() }

            val isPalmCentered = palmX in 0.2f..0.8f && palmY in 0.2f..0.8f
            val isInsideFrame = minX > 0.02f && maxX < 0.98f && minY > 0.02f && maxY < 0.98f

            isPalmCentered && isInsideFrame
        }

        // ✅ SINGLE FINGER
        "SINGLE_FINGER" -> {
    val tip = landmarks[8]   // finger tip
    val pip = landmarks[6]   // finger PIP joint (middle)

    val tipRotX = tip.y()
    val tipRotY = 1.0f - tip.x()
    val pipRotX = pip.y()
    val pipRotY = 1.0f - pip.x()

    // Use center of finger instead of just the tip
    val centerX = (tipRotX + pipRotX) / 2f
    val centerY = (tipRotY + pipRotY) / 2f

    val isCenteredX = centerX in 0.20f..0.80f
    val isCenteredY = centerY in 0.15f..0.85f

    isCenteredX && isCenteredY

        }

        // ✅ THUMB
        "LEFT_THUMB", "RIGHT_THUMB" -> {
             val tip  = landmarks[4]  // thumb tip
    val base = landmarks[2]  // thumb MCP (base)

    // Convert both landmarks to rotated-image coordinates
    val tipRotX  = tip.y()
    val tipRotY  = 1.0f - tip.x()
    val baseRotX = base.y()
    val baseRotY = 1.0f - base.x()

    // Use center of thumb (tip + base average) — more stable than tip alone
    val centerX = (tipRotX + baseRotX) / 2f
    val centerY = (tipRotY + baseRotY) / 2f

    // Looser bounds — thumb doesn't need to be pixel-perfect center
    val isCenteredX = centerX in 0.20f..0.80f
    val isCenteredY = centerY in 0.15f..0.85f

    isCenteredX && isCenteredY
        }

        else -> true
    }
}

}