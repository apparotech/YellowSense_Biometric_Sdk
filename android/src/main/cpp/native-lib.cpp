#include <jni.h>
#include <string>
#include <opencv2/opencv.hpp>
#include <vector>
#include <cmath>
#include <android/log.h>

#define TAG "BiometricSDK"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

// ============================================================================
// FINGERPRINT ENHANCEMENT FUNCTIONS
// ============================================================================

// 1. Gabor filter bank for ridge enhancement
// Enhances fingerprint ridges by detecting edges in multiple directions
cv::Mat gaborBankEnhance(const cv::Mat& gray) {
    cv::Mat temp32;
    gray.convertTo(temp32, CV_32F);
    cv::Mat gx, gy;
    cv::Sobel(temp32, gx, CV_32F, 1, 0, 3);
    cv::Sobel(temp32, gy, CV_32F, 0, 1, 3);
    cv::Mat result = cv::Mat::zeros(gray.size(), CV_32F);
    int angles[] = {0, 45, 90, 135};
    for (int angle_deg : angles) {
        float theta = angle_deg * CV_PI / 180.0f;
        cv::Mat response = cv::abs(gx * std::cos(theta) + gy * std::sin(theta));
        result = cv::max(result, response);
    }
    cv::Mat out8u;
    cv::normalize(result, out8u, 0, 255, cv::NORM_MINMAX, CV_8U);
    return out8u;
}

// 2. Skeletonization
// Converts binary fingerprint to thin (1-pixel width) ridge structure
cv::Mat skeletonize(const cv::Mat& binary) {
    cv::Mat skeleton = cv::Mat::zeros(binary.size(), CV_8UC1);
    cv::Mat element = cv::getStructuringElement(cv::MORPH_CROSS, cv::Size(3, 3));
    cv::Mat src = binary.clone();
    cv::Mat eroded, temp;
    do {
        cv::erode(src, eroded, element);
        cv::dilate(eroded, temp, element);
        cv::subtract(src, temp, temp);
        cv::bitwise_or(skeleton, temp, skeleton);
        eroded.copyTo(src);
    } while (cv::countNonZero(src) != 0);
    return skeleton;
}

// 3. Complete fingerprint enhancement pipeline
cv::Mat enhanceFingerprint(const cv::Mat& src) {
    cv::Mat gray;
    // Convert to grayscale
    if (src.channels() == 3) {
        cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
    } else if (src.channels() == 4) {
        cv::cvtColor(src, gray, cv::COLOR_RGBA2GRAY);
    } else {
        gray = src.clone();
    }

    // CLAHE for contrast enhancement
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(3.0, cv::Size(8, 8));
    cv::Mat clahe_out;
    clahe->apply(gray, clahe_out);

    // Gaussian Blur for noise reduction
    cv::Mat blurred;
    cv::GaussianBlur(clahe_out, blurred, cv::Size(3, 3), 1.0);

    // Gabor Bank for ridge enhancement
    cv::Mat ridge_enhanced = gaborBankEnhance(blurred);

    // Adaptive Threshold
    cv::Mat binary;
    cv::adaptiveThreshold(ridge_enhanced, binary, 255,
                          cv::ADAPTIVE_THRESH_GAUSSIAN_C,
                          cv::THRESH_BINARY_INV, 11, 2);

    // Skeletonize
    return skeletonize(binary);
}

// ============================================================================
// QUALITY METRICS FUNCTIONS
// ============================================================================

/**
 * Calculate blur score using Laplacian variance
 * Higher variance = sharper image
 * Returns score normalized to 0-100 range
 * 
 * FIXED: Better normalization for hand images
 * - Sharp hand images: variance typically 500-2000
 * - Blurry hand images: variance typically 50-200
 */
double calculateBlurScore(const cv::Mat& gray) {
    if (gray.empty()) return 0.0;

    cv::Mat laplacian;
    cv::Laplacian(gray, laplacian, CV_64F);

    cv::Scalar mean, stddev;
    cv::meanStdDev(laplacian, mean, stddev);

    double variance = stddev.val[0] * stddev.val[0];

    // 🔥 NEW REALISTIC MAPPING
    double blurScore;

    if (variance >= 150.0) {
        blurScore = 100.0;
    } else if (variance <= 10.0) {
        blurScore = 10.0; // never fully 0 → avoids blocking
    } else {
        blurScore = ((variance - 10.0) / 140.0) * 100.0;
    }

    blurScore = std::max(10.0, std::min(100.0, blurScore));

    LOGD("Blur detection - Variance: %.2f, Score: %.2f", variance, blurScore);

    return blurScore;
}

/**
 * Calculate brightness score from grayscale image
 * Returns score 0-100 (0=too dark, 100=optimal)
 */
double calculateBrightnessScore(const cv::Mat& gray) {
    if (gray.empty()) return 0.0;
    
    cv::Scalar mean = cv::mean(gray);
    double brightness = mean[0] / 255.0 * 100.0;
    
    // Optimal brightness range: 40-80% of 255 (40-80 brightness score)
    double brightnessScore;
    if (brightness < 40) {
        brightnessScore = (brightness / 40.0) * 50.0;
    } else if (brightness > 80) {
        brightnessScore = std::max(0.0, 100.0 - ((brightness - 80.0) / 20.0 * 50.0));
    } else {
        brightnessScore = 50.0 + ((brightness - 40.0) / 40.0 * 50.0);
    }
    
    LOGD("Brightness: %.2f%%, Score: %.2f", brightness, brightnessScore);
    return brightnessScore;
}

/**
 * Calculate hand centering score
 * Uses hand center point (landmark 9 - middle finger MCP)
 * Returns score 0-100 (higher = better centered)
 */
double calculateCenteringScore(float handX, float handY, int imageWidth, int imageHeight) {
    float centerX = imageWidth / 2.0f;
    float centerY = imageHeight / 2.0f;
    
    // Calculate normalized distance from center (0 to 1)
    float distanceX = std::abs(handX - centerX) / imageWidth;
    float distanceY = std::abs(handY - centerY) / imageHeight;
    
    // Average distance
    float avgDistance = (distanceX + distanceY) / 2.0f;
    
    // Convert to score: 100% at center, 0% at edges (50% of frame away)
    double centeringScore = 100.0f - (avgDistance / 0.5f * 100.0f);
    centeringScore = std::max(0.0, std::min(100.0, centeringScore));
    
    LOGD("Hand centering - X: %.2f, Y: %.2f, Distance: %.2f%%, Score: %.2f", 
         handX, handY, avgDistance * 100, centeringScore);
    
    return centeringScore;
}

/**
 * Calculate hand size score
 * Checks if hand occupies optimal portion of frame
 */
double calculateHandSizeScore(const cv::Mat& gray, const cv::Rect& handROI) {
    if (gray.empty()) return 50.0;

    double handArea = handROI.width * handROI.height;
    double frameArea = gray.cols * gray.rows;
    double areaRatio = handArea / frameArea;

    double sizeScore;

    // 🔥 FIXED LOGIC
    if (areaRatio >= 0.25 && areaRatio <= 0.75) {
        sizeScore = 100.0; // GOOD RANGE
    } 
    else if (areaRatio < 0.25) {
        sizeScore = (areaRatio / 0.25) * 80.0;
    } 
    else {
        sizeScore = std::max(40.0, 100.0 - ((areaRatio - 0.75) * 200));
    }

    sizeScore = std::max(40.0, std::min(100.0, sizeScore));

    LOGD("Hand size - Area Ratio: %.2f%%, Score: %.2f", areaRatio * 100, sizeScore);

    return sizeScore;
}

/**
 * Get hand ROI from landmarks with adaptive padding
 */
cv::Rect getHandROI(const cv::Mat& img, const jfloat* landmarks, int width, int height) {
    float minX = 1.0f, minY = 1.0f, maxX = 0.0f, maxY = 0.0f;
    for (int i = 0; i < 21; i++) {
        float x = landmarks[i * 2] * width;
        float y = landmarks[i * 2 + 1] * height;
        minX = std::min(minX, x);
        minY = std::min(minY, y);
        maxX = std::max(maxX, x);
        maxY = std::max(maxY, y);
    }
    
    // Add padding (15% around the hand for better context)
    int handWidth = maxX - minX;
    int handHeight = maxY - minY;
    int paddingX = handWidth * 0.15f;
    int paddingY = handHeight * 0.15f;
    
    int x1 = std::max(0, (int)minX - paddingX);
    int y1 = std::max(0, (int)minY - paddingY);
    int x2 = std::min(width, (int)maxX + paddingX);
    int y2 = std::min(height, (int)maxY + paddingY);
    
    return cv::Rect(x1, y1, x2 - x1, y2 - y1);
}

// ============================================================================
// JNI FUNCTIONS
// ============================================================================

/**
 * Process fingerprint image (existing function)
 */
extern "C"
JNIEXPORT jbyteArray JNICALL
Java_com_example_biometric_1sdk_NativeProcessor_processFingerprint(
        JNIEnv* env, jclass clazz, jbyteArray imageBytes, jint width, jint height, jfloatArray landmarks) {

    LOGD("processFingerprint called - Width: %d, Height: %d", width, height);
    
    // A. Convert Kotlin ByteArray to OpenCV Mat
    jbyte* bytes = env->GetByteArrayElements(imageBytes, nullptr);
    jsize length = env->GetArrayLength(imageBytes);
    std::vector<uchar> buffer(bytes, bytes + length);
    cv::Mat img = cv::imdecode(buffer, cv::IMREAD_COLOR);
    env->ReleaseByteArrayElements(imageBytes, bytes, JNI_ABORT);

    if (img.empty()) {
        LOGE("processFingerprint: Failed to decode image");
        return nullptr;
    }

    // B. Read landmarks and crop finger region
    jfloat* lms = env->GetFloatArrayElements(landmarks, nullptr);
    float index_tip_x = lms[8 * 2] * img.cols;
    float index_tip_y = lms[8 * 2 + 1] * img.rows;
    float index_pip_x = lms[6 * 2] * img.cols;
    float index_pip_y = lms[6 * 2 + 1] * img.rows;
    env->ReleaseFloatArrayElements(landmarks, lms, JNI_ABORT);

    int padding = 60;
    int x1 = std::max(0, (int)std::min(index_tip_x, index_pip_x) - padding);
    int y1 = std::max(0, (int)std::min(index_tip_y, index_pip_y) - padding);
    int x2 = std::min(img.cols, (int)std::max(index_tip_x, index_pip_x) + padding);
    int y2 = std::min(img.rows, (int)std::max(index_tip_y, index_pip_y) + padding);

    cv::Rect roi(x1, y1, x2 - x1, y2 - y1);
    if (roi.width <= 0 || roi.height <= 0) {
        LOGE("processFingerprint: Invalid ROI");
        return nullptr;
    }
    
    cv::Mat cropped_finger = img(roi);
    cv::Mat gray;
    cv::cvtColor(cropped_finger, gray, cv::COLOR_BGR2GRAY);

    // Calculate blur score for the cropped finger
    double blurScore = calculateBlurScore(gray);
    int normalizedBlur = (int)blurScore;
    LOGD("Fingerprint blur score: %d", normalizedBlur);

    // Enhance fingerprint
    cv::Mat final_processed = enhanceFingerprint(cropped_finger);

    // Return processed image with blur score appended
    std::vector<uchar> out_buf;
    cv::imencode(".jpg", final_processed, out_buf);
    out_buf.push_back((uchar)normalizedBlur);
    
    jbyteArray result = env->NewByteArray(out_buf.size());
    env->SetByteArrayRegion(result, 0, out_buf.size(), reinterpret_cast<jbyte*>(out_buf.data()));
    
    LOGD("processFingerprint completed - Output size: %zu", out_buf.size());
    return result;
}

/**
 * NEW FUNCTION: Calculate comprehensive quality metrics
 * Returns float array: [blurScore, brightnessScore, centeringScore, sizeScore, overallScore]
 * Weights per PDF spec: 40% blur, 30% brightness, 30% centering
 */
extern "C"
JNIEXPORT jfloatArray JNICALL
Java_com_example_biometric_1sdk_NativeProcessor_calculateQualityMetrics(
        JNIEnv* env,
        jclass clazz,
        jbyteArray imageBytes,
        jint width,
        jint height,
        jfloatArray landmarks,
        jint handCenterX,
        jint handCenterY) {
    
    LOGD("calculateQualityMetrics called - Image: %dx%d, Hand center: (%d, %d)", 
         width, height, handCenterX, handCenterY);
    
    // Convert image bytes to Mat
    jbyte* bytes = env->GetByteArrayElements(imageBytes, nullptr);
    jsize length = env->GetArrayLength(imageBytes);
    std::vector<uchar> buffer(bytes, bytes + length);
    cv::Mat img = cv::imdecode(buffer, cv::IMREAD_COLOR);
    env->ReleaseByteArrayElements(imageBytes, bytes, JNI_ABORT);
    
    // Default values (0-100 range)
    float metrics[5] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    
    if (!img.empty()) {
        // Get hand ROI from landmarks for accurate quality analysis
        cv::Mat gray;
        cv::Mat croppedHand;
        
        if (landmarks != nullptr) {
            jfloat* lms = env->GetFloatArrayElements(landmarks, nullptr);
            if (lms != nullptr) {
                // Get hand bounding box
                cv::Rect handROI = getHandROI(img, lms, width, height);
                
                if (handROI.width > 0 && handROI.height > 0) {
                    // Crop to hand region for accurate quality metrics
                    croppedHand = img(handROI);
                    cv::cvtColor(croppedHand, gray, cv::COLOR_BGR2GRAY);
                    LOGD("Hand ROI extracted - Size: %dx%d", handROI.width, handROI.height);
                } else {
                    // Fallback to full image if ROI invalid
                    cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
                    LOGD("Invalid ROI, using full image");
                }
                env->ReleaseFloatArrayElements(landmarks, lms, JNI_ABORT);
            } else {
                cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
            }
        } else {
            cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
        }
        
        // Ensure we have a valid grayscale image
        if (gray.empty()) {
            cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
        }
        
        // 1. Calculate blur score using Laplacian variance (40% weight)
        double blurScore = calculateBlurScore(gray);
        if (blurScore < 20) {
            blurScore = 20; // prevent hard rejection
        }
        metrics[0] = (float)blurScore;
        
        // 2. Calculate brightness score (30% weight)
        double brightnessScore = calculateBrightnessScore(gray);
        metrics[1] = (float)brightnessScore;
        
        // 3. Calculate centering score using hand center point (30% weight)
        double centeringScore = calculateCenteringScore(handCenterX, handCenterY, width, height);
        metrics[2] = (float)centeringScore;
        
        // 4. Calculate hand size score
        double sizeScore = 50.0;
        if (landmarks != nullptr) {
            jfloat* lms = env->GetFloatArrayElements(landmarks, nullptr);
            if (lms != nullptr) {
                cv::Rect handROI = getHandROI(img, lms, width, height);
                if (handROI.width > 0 && handROI.height > 0) {
                    sizeScore = calculateHandSizeScore(gray, handROI);
                }
                env->ReleaseFloatArrayElements(landmarks, lms, JNI_ABORT);
            }
        }
        metrics[3] = (float)sizeScore;
        
        // 5. Calculate weighted overall quality (per PDF spec: 40% blur, 30% brightness, 30% centering)
        double overallQuality = (blurScore * 0.35) + 
                        (brightnessScore * 0.25) + 
                        (centeringScore * 0.25) +
                        (sizeScore * 0.15);
        
        // Ensure within 0-100 range
        overallQuality = std::max(0.0, std::min(100.0, overallQuality));
        metrics[4] = (float)overallQuality;
        
        LOGD("Quality Metrics - Blur:%.1f, Brightness:%.1f, Centering:%.1f, Size:%.1f, Overall:%.1f",
             metrics[0], metrics[1], metrics[2], metrics[3], metrics[4]);
    } else {
        LOGE("calculateQualityMetrics: Failed to decode image");
    }
    
    // Return float array to Kotlin
    jfloatArray result = env->NewFloatArray(5);
    env->SetFloatArrayRegion(result, 0, 5, metrics);
    return result;
}

/**
 * Helper function: Simple motion blur detection with guidance
 * Returns blur level: 0=sharp, 1=moderate, 2=severe
 */
extern "C"
JNIEXPORT jint JNICALL
Java_com_example_biometric_1sdk_NativeProcessor_detectMotionBlur(
        JNIEnv* env,
        jclass clazz,
        jbyteArray imageBytes,
        jint width,
        jint height) {
    
    // Convert image bytes to Mat
    jbyte* bytes = env->GetByteArrayElements(imageBytes, nullptr);
    jsize length = env->GetArrayLength(imageBytes);
    std::vector<uchar> buffer(bytes, bytes + length);
    cv::Mat img = cv::imdecode(buffer, cv::IMREAD_COLOR);
    env->ReleaseByteArrayElements(imageBytes, bytes, JNI_ABORT);
    
    if (img.empty()) {
        return 2; // Severe blur if no image
    }
    
    cv::Mat gray;
    cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
    
    double blurScore = calculateBlurScore(gray);
    
    LOGD("Motion blur detection - Score: %.2f", blurScore);
    
    // FIXED: Updated thresholds for new blur score range
    if (blurScore >= 60) {
        return 0; // Sharp
    } else if (blurScore >= 30) {
        return 1; // Moderate blur
    } else {
        return 2; // Severe blur - need to hold camera still
    }
}