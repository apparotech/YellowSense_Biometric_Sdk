// #include <jni.h>
// #include <string>
// #include <opencv2/opencv.hpp>
// #include <vector>
// #include <cmath>
// #include <android/log.h>

// #define TAG "BiometricSDK"
// #define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
// #define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

// // ============================================================================
// // FINGERPRINT ENHANCEMENT FUNCTIONS
// // ============================================================================

// // 1. Gabor filter bank for ridge enhancement
// // Enhances fingerprint ridges by detecting edges in multiple directions
// cv::Mat gaborBankEnhance(const cv::Mat& gray) {
//     cv::Mat temp32;
//     gray.convertTo(temp32, CV_32F);
//     cv::Mat gx, gy;
//     cv::Sobel(temp32, gx, CV_32F, 1, 0, 3);
//     cv::Sobel(temp32, gy, CV_32F, 0, 1, 3);
//     cv::Mat result = cv::Mat::zeros(gray.size(), CV_32F);
//     int angles[] = {0, 45, 90, 135};
//     for (int angle_deg : angles) {
//         float theta = angle_deg * CV_PI / 180.0f;
//         cv::Mat response = cv::abs(gx * std::cos(theta) + gy * std::sin(theta));
//         result = cv::max(result, response);
//     }
//     cv::Mat out8u;
//     cv::normalize(result, out8u, 0, 255, cv::NORM_MINMAX, CV_8U);
//     return out8u;
// }

// // 2. Skeletonization
// // Converts binary fingerprint to thin (1-pixel width) ridge structure
// cv::Mat skeletonize(const cv::Mat& binary) {
//     cv::Mat skeleton = cv::Mat::zeros(binary.size(), CV_8UC1);
//     cv::Mat element = cv::getStructuringElement(cv::MORPH_CROSS, cv::Size(3, 3));
//     cv::Mat src = binary.clone();
//     cv::Mat eroded, temp;
//     do {
//         cv::erode(src, eroded, element);
//         cv::dilate(eroded, temp, element);
//         cv::subtract(src, temp, temp);
//         cv::bitwise_or(skeleton, temp, skeleton);
//         eroded.copyTo(src);
//     } while (cv::countNonZero(src) != 0);
//     return skeleton;
// }

// // 3. Complete fingerprint enhancement pipeline
// cv::Mat enhanceFingerprint(const cv::Mat& src) {
//     cv::Mat gray;
//     // Convert to grayscale
//     if (src.channels() == 3) {
//         cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
//     } else if (src.channels() == 4) {
//         cv::cvtColor(src, gray, cv::COLOR_RGBA2GRAY);
//     } else {
//         gray = src.clone();
//     }

//     // CLAHE for contrast enhancement
//     cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(3.0, cv::Size(8, 8));
//     cv::Mat clahe_out;
//     clahe->apply(gray, clahe_out);

//     // Gaussian Blur for noise reduction
//     cv::Mat blurred;
//     cv::GaussianBlur(clahe_out, blurred, cv::Size(3, 3), 1.0);

//     // Gabor Bank for ridge enhancement
//     cv::Mat ridge_enhanced = gaborBankEnhance(blurred);

//     // Adaptive Threshold
//     cv::Mat binary;
//     cv::adaptiveThreshold(ridge_enhanced, binary, 255,
//                           cv::ADAPTIVE_THRESH_GAUSSIAN_C,
//                           cv::THRESH_BINARY_INV, 11, 2);

//     // Skeletonize
//     return skeletonize(binary);
// }

// // Liveness Result Structure
// struct LivenessResult {
//     bool passed;
//     std::string reason;
//     float confidence;
// };




// // LivenessResult checkAdvancedLiveness(
// //     const cv::Mat& bgr_roi,
// //     float initial_confidence) {

// //     if (bgr_roi.empty() ||
// //         bgr_roi.rows < 20 ||
// //         bgr_roi.cols < 20) {
// //         return {false, "Invalid ROI", 0.0f};
// //     }

// //     cv::Mat gray;
// //     cv::cvtColor(bgr_roi, gray, cv::COLOR_BGR2GRAY);

// //     // ── Layer 1: Glare Guard ──────────────────────────────
// //     cv::Mat bright_mask;
// //     cv::threshold(gray, bright_mask, 240, 255, cv::THRESH_BINARY);
// //     double glare_ratio = (double)cv::countNonZero(bright_mask) /
// //                          (gray.rows * gray.cols);
// //     if (glare_ratio > 0.20) {
// //         return {false, "Screen replay detected", 0.9f};
// //     }

// //     // ── Layer 2: Moire Pattern Detection ─────────────────
// //     // Screen pe repetitive pattern hota hai
// //     // Real finger mein nahi hota
// //     cv::Mat fft_input;
// //     gray.convertTo(fft_input, CV_32F);
// //     cv::Mat planes[] = {fft_input, cv::Mat::zeros(
// //         fft_input.size(), CV_32F)};
// //     cv::Mat complex_img;
// //     cv::merge(planes, 2, complex_img);
// //     cv::dft(complex_img, complex_img);
// //     cv::split(complex_img, planes);

// //     cv::Mat magnitude;
// //     cv::magnitude(planes[0], planes[1], magnitude);
// //     magnitude += cv::Scalar::all(1);
// //     cv::log(magnitude, magnitude);

// //     // High frequency peaks = Screen/Print
// //     cv::Scalar mean_mag, std_mag;
// //     cv::meanStdDev(magnitude, mean_mag, std_mag);
// //     double freq_variance = std_mag[0];

// //     // Screen mein freq_variance zyada hoti hai
// //     bool moire_detected = (freq_variance > 3.5);

// //     // ── Layer 3: Adaptive LBP Texture ────────────────────
// //     cv::Mat f, f_squared, mu, mu2;
// //     gray.convertTo(f, CV_32F);
// //     cv::multiply(f, f, f_squared);
// //     cv::blur(f, mu, cv::Size(5, 5));
// //     cv::blur(f_squared, mu2, cv::Size(5, 5));

// //     cv::Mat mu_squared, var, stddev_map;
// //     cv::multiply(mu, mu, mu_squared);
// //     cv::subtract(mu2, mu_squared, var);
// //     cv::max(var, 0.0, var);
// //     cv::sqrt(var, stddev_map);
// //     double lbp_var = cv::mean(stddev_map)[0];

// //     double mean_brightness = cv::mean(gray)[0];
// //     double adaptive_threshold;
// //     if (mean_brightness > 180)      adaptive_threshold = 18.0;
// //     else if (mean_brightness > 120) adaptive_threshold = 22.0;
// //     else                            adaptive_threshold = 15.0;

// //     bool lbp_ok = (lbp_var >= adaptive_threshold);

// //     // ── Layer 4: Color Uniformity Check ──────────────────
// //     // Screen pe color zyada uniform hota hai
// //     // Real finger mein variation hoti hai
// //     cv::Mat hsv;
// //     cv::cvtColor(bgr_roi, hsv, cv::COLOR_BGR2HSV);
// //     std::vector<cv::Mat> hsv_channels;
// //     cv::split(hsv, hsv_channels);

// //     cv::Scalar mean_s, std_s;
// //     cv::meanStdDev(hsv_channels[1], mean_s, std_s);
// //     double saturation_std = std_s[0];

// //     // Real finger: saturation variation zyada hoti hai
// //     // Screen/print: saturation uniform hoti hai
// //     bool color_variation_ok = (saturation_std > 15.0);

// //     // ── Layer 5: Skin Detection ───────────────────────────
// //     cv::Mat mask1, mask2, mask3, skin_mask;
// //     cv::inRange(hsv,
// //         cv::Scalar(0, 15, 30),
// //         cv::Scalar(25, 255, 255), mask1);
// //     cv::inRange(hsv,
// //         cv::Scalar(0, 8, 40),
// //         cv::Scalar(50, 200, 255), mask2);
// //     cv::inRange(hsv,
// //         cv::Scalar(155, 10, 30),
// //         cv::Scalar(180, 255, 255), mask3);
// //     cv::bitwise_or(mask1, mask2, skin_mask);
// //     cv::bitwise_or(skin_mask, mask3, skin_mask);

// //     double skin_ratio = (double)cv::countNonZero(skin_mask) /
// //                         (bgr_roi.rows * bgr_roi.cols);
// //     bool skin_ok = (skin_ratio >= 0.03);

// //     // ── Layer 6: Edge Density ─────────────────────────────
// //     cv::Mat edges;
// //     cv::Canny(gray, edges, 30, 100);
// //     double edge_density = (double)cv::countNonZero(edges) /
// //                           (gray.rows * gray.cols);
// //     bool edge_ok = (edge_density >= 0.03 && edge_density <= 0.65);

// //     // ── Scoring ───────────────────────────────────────────
// //     int score = 0;
// //     if (lbp_ok)              score += 30;
// //     if (skin_ok)             score += 20;
// //     if (edge_ok)             score += 20;
// //     if (color_variation_ok)  score += 20;
// //     if (!moire_detected)     score += 10;

// //     // ── Instant Reject Rules ──────────────────────────────
// //     // Screen pe moire hoga
// //     if (moire_detected && !lbp_ok) {
// //         return {false, "Screen replay detected", 0.0f};
// //     }

// //     // Print pe color variation nahi hogi
// //     if (!color_variation_ok && !lbp_ok) {
// //         return {false, "Printed photo detected", 0.0f};
// //     }

// //     // ── Confidence ────────────────────────────────────────
// //     float confidence = initial_confidence;
// //     if (!lbp_ok)             confidence *= 0.80f;
// //     if (!skin_ok)            confidence *= 0.85f;
// //     if (!edge_ok)            confidence *= 0.85f;
// //     if (!color_variation_ok) confidence *= 0.75f;
// //     if (moire_detected)      confidence *= 0.70f;
// //     confidence = std::max(0.0f, std::min(confidence, 1.0f));

// //     // ── Decision ─────────────────────────────────────────
// //     if (score >= 50) {
// //         return {true, "Real finger detected", confidence};
// //     }

// //     if (score >= 30 && skin_ok && lbp_ok) {
// //         return {true, "Real finger (ISP smoothed)", 0.70f};
// //     }

// //     return {false, "Spoof detected", 0.0f};
// // }


// LivenessResult checkAdvancedLiveness(
//     const cv::Mat& bgr_roi,
//     float initial_confidence) {

//     if (bgr_roi.empty() ||
//         bgr_roi.rows < 20 ||
//         bgr_roi.cols < 20) {
//         return {false, "Invalid ROI", 0.0f};
//     }

//     cv::Mat gray;
//     cv::cvtColor(bgr_roi, gray, cv::COLOR_BGR2GRAY);

//     // ── Layer 2: Glare — Python se match ─────────────────
//     cv::Mat bright_mask;
//     cv::threshold(gray, bright_mask, 245, 255, cv::THRESH_BINARY);
//     double glare_ratio = (double)cv::countNonZero(bright_mask) /
//                          (gray.rows * gray.cols);

//     if (glare_ratio > 0.12) {  // Python: 0.12
//         return {false, "Screen replay detected", 0.85f};
//     }

//     // ── Layer 3: LBP Texture ──────────────────────────────
//     cv::Mat f, f_squared, mu, mu2;
//     gray.convertTo(f, CV_32F);
//     cv::multiply(f, f, f_squared);
//     cv::blur(f, mu, cv::Size(5, 5));
//     cv::blur(f_squared, mu2, cv::Size(5, 5));

//     cv::Mat mu_squared, var, stddev_map;
//     cv::multiply(mu, mu, mu_squared);
//     cv::subtract(mu2, mu_squared, var);
//     cv::max(var, 0.0, var);
//     cv::sqrt(var, stddev_map);

//     double lbp_var = cv::mean(stddev_map)[0];
//     bool lbp_ok = (lbp_var >= 18.0);  // Python: 18.0

//     // ── Layer 4: Skin Detection — Python se match ─────────
//     cv::Mat hsv;
//     cv::cvtColor(bgr_roi, hsv, cv::COLOR_BGR2HSV);

//     cv::Mat mask1, mask2, skin_mask;

//     // Primary skin range — Python se same
//     cv::inRange(hsv,
//         cv::Scalar(0, 20, 50),
//         cv::Scalar(25, 170, 255), mask1);

//     // Reddish wrap — Python se same
//     cv::inRange(hsv,
//         cv::Scalar(165, 20, 50),
//         cv::Scalar(180, 170, 255), mask2);

//     cv::bitwise_or(mask1, mask2, skin_mask);

//     double skin_ratio = (double)cv::countNonZero(skin_mask) /
//                         (bgr_roi.rows * bgr_roi.cols);

//     bool skin_ok = (skin_ratio >= 0.15);  // Python: 0.15

//     // ── Confidence — Python se same logic ─────────────────
//     float confidence = initial_confidence;
//     confidence *= (1.0f - 0.08f * (float)(glare_ratio / 0.12f));

//     if (!lbp_ok) confidence *= 0.75f;
//     if (!skin_ok) confidence *= 0.80f;
//     confidence = std::max(0.0f, std::min(confidence, 1.0f));

//     // ── Hard Fail — Python se same ────────────────────────
//     if (!lbp_ok && !skin_ok) {
//         return {false,
//                 "Spoof: low texture and no skin tone",
//                 confidence};
//     }

//     return {true, "Real finger detected", confidence};
// }


// struct Minutia {
//     int x, y;
//     int type; // 1 = Ridge Ending, 3 = Bifurcation
// };

// // 1. Extract Minutiae using Crossing Number
// std::vector<Minutia> extractMinutiae(const cv::Mat& skeleton) {
//     std::vector<Minutia> minutiae;
    
//     // Scan every pixel (ignoring borders to prevent out-of-bounds)
//     for (int y = 1; y < skeleton.rows - 1; y++) {
//         for (int x = 1; x < skeleton.cols - 1; x++) {
//             if (skeleton.at<uchar>(y, x) == 255) { // If it's a ridge pixel
                
//                 // Get 8 neighbors (Clockwise starting from top)
//                 int p[8];
//                 p[0] = skeleton.at<uchar>(y-1, x)   > 0 ? 1 : 0;
//                 p[1] = skeleton.at<uchar>(y-1, x+1) > 0 ? 1 : 0;
//                 p[2] = skeleton.at<uchar>(y, x+1)   > 0 ? 1 : 0;
//                 p[3] = skeleton.at<uchar>(y+1, x+1) > 0 ? 1 : 0;
//                 p[4] = skeleton.at<uchar>(y+1, x)   > 0 ? 1 : 0;
//                 p[5] = skeleton.at<uchar>(y+1, x-1) > 0 ? 1 : 0;
//                 p[6] = skeleton.at<uchar>(y, x-1)   > 0 ? 1 : 0;
//                 p[7] = skeleton.at<uchar>(y-1, x-1) > 0 ? 1 : 0;

//                 // Calculate Crossing Number (transitions from 0 to 1)
//                 int transitions = 0;
//                 for (int i = 0; i < 8; i++) {
//                     transitions += std::abs(p[i] - p[(i + 1) % 8]);
//                 }
//                 int cn = transitions / 2;

//                 // Classify based on Crossing Number
//                 if (cn == 1) {
//                     minutiae.push_back({x, y, 1}); // Ending
//                 } else if (cn == 3) {
//                     minutiae.push_back({x, y, 3}); // Bifurcation
//                 }
//             }
//         }
//     }
//     return minutiae;
// }

// // 2. Pack into ISO/IEC 19794-2 Binary Format
// std::vector<uchar> createIsoTemplate(const std::vector<Minutia>& minutiae, int width, int height) {
//     std::vector<uchar> tmpl;
    
//     // --- ISO Magic Header (24 bytes) ---
//     tmpl.push_back('F'); tmpl.push_back('M'); tmpl.push_back('R'); tmpl.push_back(0); // Magic
//     tmpl.push_back(' '); tmpl.push_back('2'); tmpl.push_back('0'); tmpl.push_back(0); // Version
    
//     // Total Length = Header(24) + FingerHeader(4) + (MinutiaeCount * 6) + Footer(2)
//     int totalLen = 24 + 4 + (minutiae.size() * 6) + 2; 
//     tmpl.push_back((totalLen >> 24) & 0xFF); tmpl.push_back((totalLen >> 16) & 0xFF);
//     tmpl.push_back((totalLen >> 8) & 0xFF);  tmpl.push_back(totalLen & 0xFF);
    
//     tmpl.push_back(0); tmpl.push_back(0); // Capture Equipment ID
//     tmpl.push_back((width >> 8) & 0xFF);  tmpl.push_back(width & 0xFF); // Width
//     tmpl.push_back((height >> 8) & 0xFF); tmpl.push_back(height & 0xFF); // Height
//     tmpl.push_back(0); tmpl.push_back(197); // X Resolution (197 pixels/cm = ~500 dpi)
//     tmpl.push_back(0); tmpl.push_back(197); // Y Resolution (~500 dpi)
//     tmpl.push_back(1); // Finger Count (1)
//     tmpl.push_back(0); // Reserved byte

//     // --- Finger View Header (4 bytes) ---
//     tmpl.push_back(0);   // Finger position (0 = Unknown, Kotlin handles mapping)
//     tmpl.push_back(0);   // View number
//     tmpl.push_back(100); // Impression type (Contactless)
//     tmpl.push_back(100); // Quality (0-100)
//     tmpl.push_back(minutiae.size() & 0xFF); // Total Minutiae Count

//     // --- Minutiae Data (6 bytes per point) ---
//     for(const auto& m : minutiae) {
//         int type = (m.type == 1) ? 1 /*Ending*/ : 2 /*Bifurcation*/;
        
//         // Byte 1: Type (first 2 bits) + X-coord high bits
//         tmpl.push_back((type << 6) | ((m.x >> 8) & 0x3F)); 
//         // Byte 2: X-coord low bits
//         tmpl.push_back(m.x & 0xFF);
//         // Byte 3: Reserved (first 2 bits) + Y-coord high bits
//         tmpl.push_back((m.y >> 8) & 0x3F);
//         // Byte 4: Y-coord low bits
//         tmpl.push_back(m.y & 0xFF);
//         // Byte 5: Angle (Simplified to 0 for this version)
//         tmpl.push_back(0); 
//         // Byte 6: Quality (Set to 100 for all points)
//         tmpl.push_back(100); 
//     }
    
//     // --- Footer ---
//     tmpl.push_back(0); tmpl.push_back(0); // Extended data length (0)
    
//     return tmpl;
// }




// // --- 2. Main JNI Bridge (Kotlin se yahan aayega) ---
// // --- 4. Main JNI Bridge ---
// // ============================================================================
// // QUALITY METRICS FUNCTIONS
// // ============================================================================

// /**
//  * Calculate blur score using Laplacian variance
//  * Higher variance = sharper image
//  * Returns score normalized to 0-100 range
//  * 
//  * FIXED: Better normalization for hand images
//  * - Sharp hand images: variance typically 500-2000
//  * - Blurry hand images: variance typically 50-200
//  */
// double calculateBlurScore(const cv::Mat& gray) {
//     if (gray.empty()) return 0.0;

//     cv::Mat laplacian;
//     cv::Laplacian(gray, laplacian, CV_64F);

//     cv::Scalar mean, stddev;
//     cv::meanStdDev(laplacian, mean, stddev);

//     double variance = stddev.val[0] * stddev.val[0];

//     // 🔥 NEW REALISTIC MAPPING
//     double blurScore;

//     if (variance >= 150.0) {
//         blurScore = 100.0;
//     } else if (variance <= 10.0) {
//         blurScore = 10.0; // never fully 0 → avoids blocking
//     } else {
//         blurScore = ((variance - 10.0) / 140.0) * 100.0;
//     }

//     blurScore = std::max(10.0, std::min(100.0, blurScore));

//     LOGD("Blur detection - Variance: %.2f, Score: %.2f", variance, blurScore);

//     return blurScore;
// }

// /**
//  * Calculate brightness score from grayscale image
//  * Returns score 0-100 (0=too dark, 100=optimal)
//  */
// double calculateBrightnessScore(const cv::Mat& gray) {
//     if (gray.empty()) return 0.0;
    
//     cv::Scalar mean = cv::mean(gray);
//     double brightness = mean[0] / 255.0 * 100.0;
    
//     // Optimal brightness range: 40-80% of 255 (40-80 brightness score)
//     double brightnessScore;
//     if (brightness < 40) {
//         brightnessScore = (brightness / 40.0) * 50.0;
//     } else if (brightness > 80) {
//         brightnessScore = std::max(0.0, 100.0 - ((brightness - 80.0) / 20.0 * 50.0));
//     } else {
//         brightnessScore = 50.0 + ((brightness - 40.0) / 40.0 * 50.0);
//     }
    
//     LOGD("Brightness: %.2f%%, Score: %.2f", brightness, brightnessScore);
//     return brightnessScore;
// }

// /**
//  * Calculate hand centering score
//  * Uses hand center point (landmark 9 - middle finger MCP)
//  * Returns score 0-100 (higher = better centered)
//  */
// double calculateCenteringScore(float handX, float handY, int imageWidth, int imageHeight) {
//     float centerX = imageWidth / 2.0f;
//     float centerY = imageHeight / 2.0f;
    
//     // Calculate normalized distance from center (0 to 1)
//     float distanceX = std::abs(handX - centerX) / imageWidth;
//     float distanceY = std::abs(handY - centerY) / imageHeight;
    
//     // Average distance
//     float avgDistance = (distanceX + distanceY) / 2.0f;
    
//     // Convert to score: 100% at center, 0% at edges (50% of frame away)
//     double centeringScore = 100.0f - (avgDistance / 0.5f * 100.0f);
//     centeringScore = std::max(0.0, std::min(100.0, centeringScore));
    
//     LOGD("Hand centering - X: %.2f, Y: %.2f, Distance: %.2f%%, Score: %.2f", 
//          handX, handY, avgDistance * 100, centeringScore);
    
//     return centeringScore;
// }

// /**
//  * Calculate hand size score
//  * Checks if hand occupies optimal portion of frame
//  */
// double calculateHandSizeScore(const cv::Mat& gray, const cv::Rect& handROI) {
//     if (gray.empty()) return 50.0;

//     double handArea = handROI.width * handROI.height;
//     double frameArea = gray.cols * gray.rows;
//     double areaRatio = handArea / frameArea;

//     double sizeScore;

//     // 🔥 FIXED LOGIC
//     if (areaRatio >= 0.25 && areaRatio <= 0.75) {
//         sizeScore = 100.0; // GOOD RANGE
//     } 
//     else if (areaRatio < 0.25) {
//         sizeScore = (areaRatio / 0.25) * 80.0;
//     } 
//     else {
//         sizeScore = std::max(40.0, 100.0 - ((areaRatio - 0.75) * 200));
//     }

//     sizeScore = std::max(40.0, std::min(100.0, sizeScore));

//     LOGD("Hand size - Area Ratio: %.2f%%, Score: %.2f", areaRatio * 100, sizeScore);

//     return sizeScore;
// }

// /**
//  * Get hand ROI from landmarks with adaptive padding
//  */
// cv::Rect getHandROI(const cv::Mat& img, const jfloat* landmarks, int width, int height) {
//     float minX = 1.0f, minY = 1.0f, maxX = 0.0f, maxY = 0.0f;
//     for (int i = 0; i < 21; i++) {
//         float x = landmarks[i * 2] * width;
//         float y = landmarks[i * 2 + 1] * height;
//         minX = std::min(minX, x);
//         minY = std::min(minY, y);
//         maxX = std::max(maxX, x);
//         maxY = std::max(maxY, y);
//     }
    
//     // Add padding (15% around the hand for better context)
//     int handWidth = maxX - minX;
//     int handHeight = maxY - minY;
//     int paddingX = handWidth * 0.15f;
//     int paddingY = handHeight * 0.15f;
    
//     int x1 = std::max(0, (int)minX - paddingX);
//     int y1 = std::max(0, (int)minY - paddingY);
//     int x2 = std::min(width, (int)maxX + paddingX);
//     int y2 = std::min(height, (int)maxY + paddingY);
    
//     return cv::Rect(x1, y1, x2 - x1, y2 - y1);
// }

// // ============================================================================
// // JNI FUNCTIONS
// // ============================================================================

// /**
//  * Process fingerprint image (existing function)
//  */
// extern "C"
// JNIEXPORT jbyteArray JNICALL
// Java_com_example_biometric_1sdk_NativeProcessor_processFingerprint(
//         JNIEnv* env, jclass clazz, jbyteArray imageBytes, jint width, jint height, jfloatArray landmarks) {

//     LOGD("processFingerprint called - Width: %d, Height: %d", width, height);
    
//     // A. Convert Kotlin ByteArray to OpenCV Mat
//     jbyte* bytes = env->GetByteArrayElements(imageBytes, nullptr);
//     jsize length = env->GetArrayLength(imageBytes);
//     std::vector<uchar> buffer(bytes, bytes + length);
//     cv::Mat img = cv::imdecode(buffer, cv::IMREAD_COLOR);
//     env->ReleaseByteArrayElements(imageBytes, bytes, JNI_ABORT);

//     if (img.empty()) {
//         LOGE("processFingerprint: Failed to decode image");
//         return nullptr;
//     }

//     // B. Read landmarks and crop finger region
//     jfloat* lms = env->GetFloatArrayElements(landmarks, nullptr);
//     float index_tip_x = lms[8 * 2] * img.cols;
//     float index_tip_y = lms[8 * 2 + 1] * img.rows;
//     float index_pip_x = lms[6 * 2] * img.cols;
//     float index_pip_y = lms[6 * 2 + 1] * img.rows;
//     env->ReleaseFloatArrayElements(landmarks, lms, JNI_ABORT);

//     int padding = 60;
//     int x1 = std::max(0, (int)std::min(index_tip_x, index_pip_x) - padding);
//     int y1 = std::max(0, (int)std::min(index_tip_y, index_pip_y) - padding);
//     int x2 = std::min(img.cols, (int)std::max(index_tip_x, index_pip_x) + padding);
//     int y2 = std::min(img.rows, (int)std::max(index_tip_y, index_pip_y) + padding);

//     cv::Rect roi(x1, y1, x2 - x1, y2 - y1);
//     if (roi.width <= 0 || roi.height <= 0) {
//         LOGE("processFingerprint: Invalid ROI");
//         return nullptr;
//     }
    
//     cv::Mat cropped_finger = img(roi);
//     cv::Mat gray;
//     cv::cvtColor(cropped_finger, gray, cv::COLOR_BGR2GRAY);


//     LivenessResult liveness = checkAdvancedLiveness(cropped_finger, 0.95f);

//     if (!liveness.passed) {
//         // Agar fraud detect hua, toh 1 byte (0) bhej kar Kotlin ko bata do
//         jbyteArray fake_result = env->NewByteArray(1);
//         jbyte fake_flag[] = {0};
//         env->SetByteArrayRegion(fake_result, 0, 1, fake_flag);
//         return fake_result;
//     }

//     // C. Helper function call karein (Clean approach!)
//     cv::Mat final_processed = enhanceFingerprint(cropped_finger);


//     // D. Return to Kotlin
//   // 🔥 1. Extract Minutiae & Build Template 🔥
//     std::vector<Minutia> minutiae = extractMinutiae(final_processed);
//     std::vector<uchar> iso_template = createIsoTemplate(minutiae, final_processed.cols, final_processed.rows);

//     // 🔥 2. Encode Processed Image to JPG 🔥
//     std::vector<uchar> img_buf;
//     cv::imencode(".jpg", final_processed, img_buf);
    
//     // 🔥 3. Pack Both into a Single Array for Kotlin 🔥
//     // Format: [4 Bytes for Image Size] + [Image Bytes] + [Template Bytes]
//     int img_size = img_buf.size();
//     int total_size = 4 + img_size + iso_template.size();
    
//     jbyteArray result = env->NewByteArray(total_size);
//     jbyte* result_ptr = env->GetByteArrayElements(result, nullptr);
    
//     // Write 4 bytes for Image Size (Big Endian format)
//     result_ptr[0] = (img_size >> 24) & 0xFF;
//     result_ptr[1] = (img_size >> 16) & 0xFF;
//     result_ptr[2] = (img_size >> 8) & 0xFF;
//     result_ptr[3] = img_size & 0xFF;
    
//     // Copy the Image Bytes
//     std::memcpy(result_ptr + 4, img_buf.data(), img_size);
//     // Copy the Template Bytes right after the image
//     std::memcpy(result_ptr + 4 + img_size, iso_template.data(), iso_template.size());
    
//     // Memory release karein aur result return karein
//     env->ReleaseByteArrayElements(result, result_ptr, 0);
//     // Calculate blur score for the cropped finger
//     double blurScore = calculateBlurScore(gray);
//     int normalizedBlur = (int)blurScore;
//     LOGD("Fingerprint blur score: %d", normalizedBlur);

//     // Enhance fingerprint
//     cv::Mat final_processed = enhanceFingerprint(cropped_finger);

//     // Return processed image with blur score appended
//     std::vector<uchar> out_buf;
//     cv::imencode(".jpg", final_processed, out_buf);
//     out_buf.push_back((uchar)normalizedBlur);
    
//     jbyteArray result = env->NewByteArray(out_buf.size());
//     env->SetByteArrayRegion(result, 0, out_buf.size(), reinterpret_cast<jbyte*>(out_buf.data()));
    
//     LOGD("processFingerprint completed - Output size: %zu", out_buf.size());
//     return result;
    
// }

// /**
//  * NEW FUNCTION: Calculate comprehensive quality metrics
//  * Returns float array: [blurScore, brightnessScore, centeringScore, sizeScore, overallScore]
//  * Weights per PDF spec: 40% blur, 30% brightness, 30% centering
//  */
// extern "C"
// JNIEXPORT jfloatArray JNICALL
// Java_com_example_biometric_1sdk_NativeProcessor_calculateQualityMetrics(
//         JNIEnv* env,
//         jclass clazz,
//         jbyteArray imageBytes,
//         jint width,
//         jint height,
//         jfloatArray landmarks,
//         jint handCenterX,
//         jint handCenterY) {
    
//     LOGD("calculateQualityMetrics called - Image: %dx%d, Hand center: (%d, %d)", 
//          width, height, handCenterX, handCenterY);
    
//     // Convert image bytes to Mat
//     jbyte* bytes = env->GetByteArrayElements(imageBytes, nullptr);
//     jsize length = env->GetArrayLength(imageBytes);
//     std::vector<uchar> buffer(bytes, bytes + length);
//     cv::Mat img = cv::imdecode(buffer, cv::IMREAD_COLOR);
//     env->ReleaseByteArrayElements(imageBytes, bytes, JNI_ABORT);
    
//     // Default values (0-100 range)
//     float metrics[5] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    
//     if (!img.empty()) {
//         // Get hand ROI from landmarks for accurate quality analysis
//         cv::Mat gray;
//         cv::Mat croppedHand;
        
//         if (landmarks != nullptr) {
//             jfloat* lms = env->GetFloatArrayElements(landmarks, nullptr);
//             if (lms != nullptr) {
//                 // Get hand bounding box
//                 cv::Rect handROI = getHandROI(img, lms, width, height);
                
//                 if (handROI.width > 0 && handROI.height > 0) {
//                     // Crop to hand region for accurate quality metrics
//                     croppedHand = img(handROI);
//                     cv::cvtColor(croppedHand, gray, cv::COLOR_BGR2GRAY);
//                     LOGD("Hand ROI extracted - Size: %dx%d", handROI.width, handROI.height);
//                 } else {
//                     // Fallback to full image if ROI invalid
//                     cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
//                     LOGD("Invalid ROI, using full image");
//                 }
//                 env->ReleaseFloatArrayElements(landmarks, lms, JNI_ABORT);
//             } else {
//                 cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
//             }
//         } else {
//             cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
//         }
        
//         // Ensure we have a valid grayscale image
//         if (gray.empty()) {
//             cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
//         }
        
//         // 1. Calculate blur score using Laplacian variance (40% weight)
//         double blurScore = calculateBlurScore(gray);
//         if (blurScore < 20) {
//             blurScore = 20; // prevent hard rejection
//         }
//         metrics[0] = (float)blurScore;
        
//         // 2. Calculate brightness score (30% weight)
//         double brightnessScore = calculateBrightnessScore(gray);
//         metrics[1] = (float)brightnessScore;
        
//         // 3. Calculate centering score using hand center point (30% weight)
//         double centeringScore = calculateCenteringScore(handCenterX, handCenterY, width, height);
//         metrics[2] = (float)centeringScore;
        
//         // 4. Calculate hand size score
//         double sizeScore = 50.0;
//         if (landmarks != nullptr) {
//             jfloat* lms = env->GetFloatArrayElements(landmarks, nullptr);
//             if (lms != nullptr) {
//                 cv::Rect handROI = getHandROI(img, lms, width, height);
//                 if (handROI.width > 0 && handROI.height > 0) {
//                     sizeScore = calculateHandSizeScore(gray, handROI);
//                 }
//                 env->ReleaseFloatArrayElements(landmarks, lms, JNI_ABORT);
//             }
//         }
//         metrics[3] = (float)sizeScore;
        
//         // 5. Calculate weighted overall quality (per PDF spec: 40% blur, 30% brightness, 30% centering)
//         double overallQuality = (blurScore * 0.35) + 
//                         (brightnessScore * 0.25) + 
//                         (centeringScore * 0.25) +
//                         (sizeScore * 0.15);
        
//         // Ensure within 0-100 range
//         overallQuality = std::max(0.0, std::min(100.0, overallQuality));
//         metrics[4] = (float)overallQuality;
        
//         LOGD("Quality Metrics - Blur:%.1f, Brightness:%.1f, Centering:%.1f, Size:%.1f, Overall:%.1f",
//              metrics[0], metrics[1], metrics[2], metrics[3], metrics[4]);
//     } else {
//         LOGE("calculateQualityMetrics: Failed to decode image");
//     }
    
//     // Return float array to Kotlin
//     jfloatArray result = env->NewFloatArray(5);
//     env->SetFloatArrayRegion(result, 0, 5, metrics);
//     return result;
// }

// /**
//  * Helper function: Simple motion blur detection with guidance
//  * Returns blur level: 0=sharp, 1=moderate, 2=severe
//  */
// extern "C"
// JNIEXPORT jint JNICALL
// Java_com_example_biometric_1sdk_NativeProcessor_detectMotionBlur(
//         JNIEnv* env,
//         jclass clazz,
//         jbyteArray imageBytes,
//         jint width,
//         jint height) {
    
//     // Convert image bytes to Mat
//     jbyte* bytes = env->GetByteArrayElements(imageBytes, nullptr);
//     jsize length = env->GetArrayLength(imageBytes);
//     std::vector<uchar> buffer(bytes, bytes + length);
//     cv::Mat img = cv::imdecode(buffer, cv::IMREAD_COLOR);
//     env->ReleaseByteArrayElements(imageBytes, bytes, JNI_ABORT);
    
//     if (img.empty()) {
//         return 2; // Severe blur if no image
//     }
    
//     cv::Mat gray;
//     cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
    
//     double blurScore = calculateBlurScore(gray);
    
//     LOGD("Motion blur detection - Score: %.2f", blurScore);
    
//     // FIXED: Updated thresholds for new blur score range
//     if (blurScore >= 60) {
//         return 0; // Sharp
//     } else if (blurScore >= 30) {
//         return 1; // Moderate blur
//     } else {
//         return 2; // Severe blur - need to hold camera still
//     }
// }

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
// SECTION 1: FINGERPRINT ENHANCEMENT PIPELINE
// ============================================================================

/**
 * Gabor filter bank for ridge enhancement.
 * Detects edges in 4 directions (0, 45, 90, 135 degrees) and takes max response.
 * This enhances fingerprint ridges regardless of their orientation.
 */
cv::Mat gaborBankEnhance(const cv::Mat& gray) {
    cv::Mat temp32;
    gray.convertTo(temp32, CV_32F);

    cv::Mat gx, gy;
    cv::Sobel(temp32, gx, CV_32F, 1, 0, 3);
    cv::Sobel(temp32, gy, CV_32F, 0, 1, 3);

    cv::Mat result = cv::Mat::zeros(gray.size(), CV_32F);
    int angles[] = {0, 45, 90, 135};

    for (int angle_deg : angles) {
        float theta      = angle_deg * CV_PI / 180.0f;
        cv::Mat response = cv::abs(gx * std::cos(theta) + gy * std::sin(theta));
        result = cv::max(result, response);
    }

    cv::Mat out8u;
    cv::normalize(result, out8u, 0, 255, cv::NORM_MINMAX, CV_8U);
    return out8u;
}

/**
 * Skeletonization via iterative erosion.
 * Converts binary fingerprint to thin (1-pixel width) ridge structure.
 * Used after thresholding to get clean ridge skeleton for minutiae extraction.
 */
cv::Mat skeletonize(const cv::Mat& binary) {
    cv::Mat skeleton = cv::Mat::zeros(binary.size(), CV_8UC1);
    cv::Mat element  = cv::getStructuringElement(cv::MORPH_CROSS, cv::Size(3, 3));
    cv::Mat src      = binary.clone();
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

/**
 * Complete fingerprint enhancement pipeline:
 *   1. Grayscale conversion
 *   2. CLAHE (contrast limited adaptive histogram equalization)
 *   3. Gaussian blur (noise reduction)
 *   4. Gabor bank (ridge enhancement in all orientations)
 *   5. Adaptive threshold (binarization)
 *   6. Skeletonization (thin ridges to 1-pixel width)
 */
cv::Mat enhanceFingerprint(const cv::Mat& src) {
    cv::Mat gray;
    if      (src.channels() == 3) cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
    else if (src.channels() == 4) cv::cvtColor(src, gray, cv::COLOR_RGBA2GRAY);
    else                          gray = src.clone();

    // CLAHE
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(3.0, cv::Size(8, 8));
    cv::Mat clahe_out;
    clahe->apply(gray, clahe_out);

    // Gaussian blur
    cv::Mat blurred;
    cv::GaussianBlur(clahe_out, blurred, cv::Size(3, 3), 1.0);

    // Gabor bank ridge enhancement
    cv::Mat ridge_enhanced = gaborBankEnhance(blurred);

    // Adaptive threshold
    cv::Mat binary;
    cv::adaptiveThreshold(ridge_enhanced, binary, 255,
                          cv::ADAPTIVE_THRESH_GAUSSIAN_C,
                          cv::THRESH_BINARY_INV, 11, 2);

    // Skeletonize
    return skeletonize(binary);
}

// ============================================================================
// SECTION 2: LIVENESS DETECTION (RULE-BASED, PYTHON-MATCHED)
// ============================================================================

struct LivenessResult {
    bool        passed;
    std::string reason;
    float       confidence;
};

/**
 * Rule-based liveness detection — matched to Python implementation.
 *
 * Layer 1: Glare guard
 *   Pixels > 245 brightness. If ratio > 0.12 -> screen replay detected.
 *
 * Layer 2: LBP texture variance
 *   Local stddev map. Mean variance >= 18.0 = real skin texture present.
 *
 * Layer 3: HSV skin detection
 *   Primary HSV range: H[0-25], S[20-170], V[50-255]
 *   Reddish wrap:      H[165-180], S[20-170], V[50-255]
 *   Skin ratio >= 0.15 = skin present.
 *
 * Hard fail: BOTH lbp_ok AND skin_ok fail simultaneously.
 * Confidence is degraded proportionally per failed layer.
 */
LivenessResult checkAdvancedLiveness(const cv::Mat& bgr_roi, float initial_confidence) {
    if (bgr_roi.empty() || bgr_roi.rows < 20 || bgr_roi.cols < 20)
        return {false, "Invalid ROI", 0.0f};

    cv::Mat gray;
    cv::cvtColor(bgr_roi, gray, cv::COLOR_BGR2GRAY);

    // Layer 1: Glare guard
    cv::Mat bright_mask;
    cv::threshold(gray, bright_mask, 245, 255, cv::THRESH_BINARY);
    double glare_ratio = (double)cv::countNonZero(bright_mask) /
                         (gray.rows * gray.cols);
    if (glare_ratio > 0.12) {
        LOGD("Liveness FAIL — glare ratio: %.3f", glare_ratio);
        return {false, "Screen replay detected", 0.85f};
    }

    // Layer 2: LBP texture variance
    cv::Mat f, f_squared, mu, mu2;
    gray.convertTo(f, CV_32F);
    cv::multiply(f, f, f_squared);
    cv::blur(f,         mu,  cv::Size(5, 5));
    cv::blur(f_squared, mu2, cv::Size(5, 5));
    cv::Mat mu_squared, var, stddev_map;
    cv::multiply(mu, mu, mu_squared);
    cv::subtract(mu2, mu_squared, var);
    cv::max(var, 0.0, var);
    cv::sqrt(var, stddev_map);
    double lbp_var = cv::mean(stddev_map)[0];
    bool   lbp_ok  = (lbp_var >= 18.0);
    LOGD("Liveness — LBP var: %.2f, ok: %d", lbp_var, (int)lbp_ok);

    // Layer 3: HSV skin detection
    cv::Mat hsv, mask1, mask2, skin_mask;
    cv::cvtColor(bgr_roi, hsv, cv::COLOR_BGR2HSV);
    cv::inRange(hsv, cv::Scalar(0,   20, 50), cv::Scalar(25,  170, 255), mask1);
    cv::inRange(hsv, cv::Scalar(165, 20, 50), cv::Scalar(180, 170, 255), mask2);
    cv::bitwise_or(mask1, mask2, skin_mask);
    double skin_ratio = (double)cv::countNonZero(skin_mask) /
                        (bgr_roi.rows * bgr_roi.cols);
    bool skin_ok = (skin_ratio >= 0.15);
    LOGD("Liveness — skin ratio: %.3f, ok: %d", skin_ratio, (int)skin_ok);

    // Confidence degradation
    float confidence = initial_confidence;
    confidence *= (1.0f - 0.08f * (float)(glare_ratio / 0.12f));
    if (!lbp_ok)  confidence *= 0.75f;
    if (!skin_ok) confidence *= 0.80f;
    confidence = std::max(0.0f, std::min(confidence, 1.0f));

    // Hard fail — both checks failed
    if (!lbp_ok && !skin_ok) {
        LOGD("Liveness FAIL — low texture AND no skin");
        return {false, "Spoof: low texture and no skin tone", confidence};
    }

    LOGD("Liveness PASS — confidence: %.2f", confidence);
    return {true, "Real finger detected", confidence};
}

// ============================================================================
// SECTION 3: MINUTIAE EXTRACTION + ISO 19794-2 TEMPLATE
// ============================================================================

struct Minutia {
    int x, y;
    int type;   // 1 = Ridge Ending, 3 = Bifurcation
};

/**
 * Extract minutiae from skeleton using Crossing Number (CN) method.
 *   CN == 1 => Ridge Ending
 *   CN == 3 => Bifurcation
 *
 * For each skeleton pixel, counts 0->1 transitions in 8-neighbor ring.
 * CN = transitions / 2.
 */
std::vector<Minutia> extractMinutiae(const cv::Mat& skeleton) {
    std::vector<Minutia> minutiae;

    for (int y = 1; y < skeleton.rows - 1; y++) {
        for (int x = 1; x < skeleton.cols - 1; x++) {
            if (skeleton.at<uchar>(y, x) != 255) continue;

            // 8-neighbors clockwise from top
            int p[8];
            p[0] = skeleton.at<uchar>(y - 1, x)     > 0 ? 1 : 0;
            p[1] = skeleton.at<uchar>(y - 1, x + 1) > 0 ? 1 : 0;
            p[2] = skeleton.at<uchar>(y,     x + 1) > 0 ? 1 : 0;
            p[3] = skeleton.at<uchar>(y + 1, x + 1) > 0 ? 1 : 0;
            p[4] = skeleton.at<uchar>(y + 1, x)     > 0 ? 1 : 0;
            p[5] = skeleton.at<uchar>(y + 1, x - 1) > 0 ? 1 : 0;
            p[6] = skeleton.at<uchar>(y,     x - 1) > 0 ? 1 : 0;
            p[7] = skeleton.at<uchar>(y - 1, x - 1) > 0 ? 1 : 0;

            int transitions = 0;
            for (int i = 0; i < 8; i++)
                transitions += std::abs(p[i] - p[(i + 1) % 8]);
            int cn = transitions / 2;

            if      (cn == 1) minutiae.push_back({x, y, 1});
            else if (cn == 3) minutiae.push_back({x, y, 3});
        }
    }

    LOGD("Minutiae extracted: %zu", minutiae.size());
    return minutiae;
}

/**
 * Pack minutiae into ISO/IEC 19794-2 binary template.
 *
 * Record layout:
 *   [8  bytes] Magic "FMR\0 20\0" + version
 *   [4  bytes] Total record length (big-endian)
 *   [2  bytes] Capture equipment ID (0x0000)
 *   [2  bytes] Image width  (pixels, big-endian)
 *   [2  bytes] Image height (pixels, big-endian)
 *   [2  bytes] X resolution (0x00C5 = 197 px/cm ~ 500 dpi)
 *   [2  bytes] Y resolution
 *   [1  byte]  Number of finger views (0x01)
 *   [1  byte]  Reserved (0x00)
 *   Finger View Header:
 *   [1  byte]  Finger position (0x00 = unknown)
 *   [1  byte]  View number (0x00)
 *   [1  byte]  Impression type (100 = contactless)
 *   [1  byte]  Quality (100)
 *   [1  byte]  Minutiae count
 *   Per Minutia (6 bytes):
 *   [1  byte]  Type(2 bits) + X_high(6 bits)
 *   [1  byte]  X_low
 *   [1  byte]  Reserved(2 bits) + Y_high(6 bits)
 *   [1  byte]  Y_low
 *   [1  byte]  Angle (0 = simplified)
 *   [1  byte]  Quality (100)
 *   Footer:
 *   [2  bytes] Extended data length (0x0000)
 */
std::vector<uchar> createIsoTemplate(const std::vector<Minutia>& minutiae,
                                     int width, int height) {
    std::vector<uchar> tmpl;

    // Magic + version
    tmpl.push_back('F'); tmpl.push_back('M'); tmpl.push_back('R'); tmpl.push_back(0x00);
    tmpl.push_back(' '); tmpl.push_back('2'); tmpl.push_back('0'); tmpl.push_back(0x00);

    // Total length = 24 (header) + 4 (finger view hdr) + N*6 (minutiae) + 2 (footer)
    int totalLen = 24 + 4 + ((int)minutiae.size() * 6) + 2;
    tmpl.push_back((totalLen >> 24) & 0xFF);
    tmpl.push_back((totalLen >> 16) & 0xFF);
    tmpl.push_back((totalLen >>  8) & 0xFF);
    tmpl.push_back( totalLen        & 0xFF);

    // Equipment ID
    tmpl.push_back(0x00); tmpl.push_back(0x00);

    // Image dimensions
    tmpl.push_back((width  >> 8) & 0xFF); tmpl.push_back(width  & 0xFF);
    tmpl.push_back((height >> 8) & 0xFF); tmpl.push_back(height & 0xFF);

    // Resolution 197 px/cm ~ 500 dpi
    tmpl.push_back(0x00); tmpl.push_back(0xC5);
    tmpl.push_back(0x00); tmpl.push_back(0xC5);

    // Finger count + reserved
    tmpl.push_back(0x01); tmpl.push_back(0x00);

    // Finger view header
    tmpl.push_back(0x00);  // finger position (unknown)
    tmpl.push_back(0x00);  // view number
    tmpl.push_back(100);   // impression type: contactless
    tmpl.push_back(100);   // quality
    tmpl.push_back((uchar)(minutiae.size() & 0xFF));  // minutiae count

    // Minutiae records
    for (const auto& m : minutiae) {
        int type = (m.type == 1) ? 1 : 2;  // 1=ending, 2=bifurcation
        tmpl.push_back((uchar)((type << 6) | ((m.x >> 8) & 0x3F)));
        tmpl.push_back((uchar)(m.x & 0xFF));
        tmpl.push_back((uchar)((m.y >> 8) & 0x3F));
        tmpl.push_back((uchar)(m.y & 0xFF));
        tmpl.push_back(0x00);   // angle (simplified)
        tmpl.push_back(100);    // per-minutia quality
    }

    // Extended data footer
    tmpl.push_back(0x00); tmpl.push_back(0x00);

    LOGD("ISO template: %zu bytes, %zu minutiae", tmpl.size(), minutiae.size());
    return tmpl;
}

// ============================================================================
// SECTION 4: QUALITY METRIC HELPERS
// ============================================================================

/**
 * Blur score via Laplacian variance.
 * variance <= 10   => score = 10  (minimum, never full block)
 * variance >= 150  => score = 100
 * linear between
 */
double calculateBlurScore(const cv::Mat& gray) {
    if (gray.empty()) return 0.0;

    cv::Mat laplacian;
    cv::Laplacian(gray, laplacian, CV_64F);

    cv::Scalar mean, stddev;
    cv::meanStdDev(laplacian, mean, stddev);
    double variance = stddev.val[0] * stddev.val[0];

    double score;
    if      (variance >= 150.0) score = 100.0;
    else if (variance <=  10.0) score =  10.0;
    else                        score = ((variance - 10.0) / 140.0) * 100.0;

    score = std::max(10.0, std::min(100.0, score));
    LOGD("Blur — variance: %.2f, score: %.2f", variance, score);
    return score;
}

/**
 * Brightness score from mean pixel intensity.
 * Optimal 40%–80% of 255 maps to score 50–100.
 * Outside range degrades linearly.
 */
double calculateBrightnessScore(const cv::Mat& gray) {
    if (gray.empty()) return 0.0;

    double brightness = cv::mean(gray)[0] / 255.0 * 100.0;
    double score;

    if      (brightness < 40) score = (brightness / 40.0) * 50.0;
    else if (brightness > 80) score = std::max(0.0, 100.0 - ((brightness - 80.0) / 20.0 * 50.0));
    else                      score = 50.0 + ((brightness - 40.0) / 40.0 * 50.0);

    LOGD("Brightness — raw: %.2f%%, score: %.2f", brightness, score);
    return score;
}

/**
 * Hand centering score.
 * 100 = perfectly centered, 0 = at frame edge.
 * handX/handY = pixel coords of hand center (typically landmark 9).
 */
double calculateCenteringScore(float handX, float handY, int imageWidth, int imageHeight) {
    float distX  = std::abs(handX - imageWidth  / 2.0f) / imageWidth;
    float distY  = std::abs(handY - imageHeight / 2.0f) / imageHeight;
    double score = 100.0 - ((distX + distY) / 2.0f / 0.5f * 100.0);
    score = std::max(0.0, std::min(100.0, score));
    LOGD("Centering — handX: %.1f, handY: %.1f, score: %.2f", handX, handY, score);
    return score;
}

/**
 * Hand size score.
 * Optimal: hand area = 25%–75% of frame.
 * Outside range degrades. Minimum returned = 40.
 */
double calculateHandSizeScore(const cv::Mat& gray, const cv::Rect& handROI) {
    if (gray.empty()) return 50.0;

    double areaRatio = (double)(handROI.width * handROI.height) /
                       (double)(gray.cols * gray.rows);
    double score;

    if      (areaRatio >= 0.25 && areaRatio <= 0.75) score = 100.0;
    else if (areaRatio <  0.25)                       score = (areaRatio / 0.25) * 80.0;
    else                                              score = std::max(40.0, 100.0 - ((areaRatio - 0.75) * 200.0));

    score = std::max(40.0, std::min(100.0, score));
    LOGD("HandSize — ratio: %.2f%%, score: %.2f", areaRatio * 100.0, score);
    return score;
}

/**
 * Compute hand bounding box from 21 MediaPipe landmarks.
 * Adds 15% padding each side. Clamps to image boundaries.
 */
cv::Rect getHandROI(const cv::Mat& img, const jfloat* landmarks, int width, int height) {
    float minX = (float)width, minY = (float)height;
    float maxX = 0.0f,         maxY = 0.0f;

    for (int i = 0; i < 21; i++) {
        float x = landmarks[i * 2]     * width;
        float y = landmarks[i * 2 + 1] * height;
        minX = std::min(minX, x); minY = std::min(minY, y);
        maxX = std::max(maxX, x); maxY = std::max(maxY, y);
    }

    int paddingX = (int)((maxX - minX) * 0.15f);
    int paddingY = (int)((maxY - minY) * 0.15f);

    int x1 = std::max(0,      (int)minX - paddingX);
    int y1 = std::max(0,      (int)minY - paddingY);
    int x2 = std::min(width,  (int)maxX + paddingX);
    int y2 = std::min(height, (int)maxY + paddingY);

    return cv::Rect(x1, y1, x2 - x1, y2 - y1);
}

// ============================================================================
// SECTION 5: JNI — processFingerprint
//
// Full pipeline per call:
//   1. Decode image bytes -> cv::Mat
//   2. Crop finger region using index tip (landmark 8) + PIP (landmark 6)
//   3. Liveness check -> if spoof: return 1-byte {0} signal to Kotlin
//   4. Enhance fingerprint -> skeleton image
//   5. Extract minutiae -> ISO 19794-2 template bytes
//   6. Encode skeleton to JPEG
//   7. Pack and return: [4-byte big-endian img size][img bytes][template bytes]
//
// Kotlin unpacks as:
//   buffer.int        -> imgSize
//   buffer.get(imgData)
//   buffer.get(templateData)
// ============================================================================

extern "C"
JNIEXPORT jbyteArray JNICALL
Java_com_example_biometric_1sdk_NativeProcessor_processFingerprint(
        JNIEnv* env, jclass clazz,
        jbyteArray imageBytes,
        jint width, jint height,
        jfloatArray landmarks) {

    LOGD("processFingerprint — %dx%d", width, height);

    // 1. Decode image
    jbyte* raw  = env->GetByteArrayElements(imageBytes, nullptr);
    jsize  len  = env->GetArrayLength(imageBytes);
    std::vector<uchar> buf(raw, raw + len);
    cv::Mat img = cv::imdecode(buf, cv::IMREAD_COLOR);
    env->ReleaseByteArrayElements(imageBytes, raw, JNI_ABORT);

    if (img.empty()) {
        LOGE("processFingerprint: Failed to decode image");
        return nullptr;
    }

    // 2. Crop finger using index tip (landmark 8) and PIP (landmark 6)
    jfloat* lms   = env->GetFloatArrayElements(landmarks, nullptr);
    float   tip_x = lms[8 * 2]     * img.cols;
    float   tip_y = lms[8 * 2 + 1] * img.rows;
    float   pip_x = lms[6 * 2]     * img.cols;
    float   pip_y = lms[6 * 2 + 1] * img.rows;
    env->ReleaseFloatArrayElements(landmarks, lms, JNI_ABORT);

    int padding = 60;
    int x1 = std::max(0,        (int)std::min(tip_x, pip_x) - padding);
    int y1 = std::max(0,        (int)std::min(tip_y, pip_y) - padding);
    int x2 = std::min(img.cols, (int)std::max(tip_x, pip_x) + padding);
    int y2 = std::min(img.rows, (int)std::max(tip_y, pip_y) + padding);

    cv::Rect roi(x1, y1, x2 - x1, y2 - y1);
    if (roi.width <= 0 || roi.height <= 0) {
        LOGE("processFingerprint: Invalid ROI (%dx%d)", roi.width, roi.height);
        return nullptr;
    }

    cv::Mat cropped_finger = img(roi).clone();
    LOGD("Cropped finger: %dx%d", cropped_finger.cols, cropped_finger.rows);

    // 3. Liveness check
    LivenessResult liveness = checkAdvancedLiveness(cropped_finger, 0.95f);
    if (!liveness.passed) {
        LOGD("SPOOF DETECTED: %s (conf=%.2f)", liveness.reason.c_str(), liveness.confidence);
        jbyteArray spoof = env->NewByteArray(1);
        jbyte      flag  = {0};
        env->SetByteArrayRegion(spoof, 0, 1, &flag);
        return spoof;
    }
    LOGD("Liveness PASSED: %s", liveness.reason.c_str());

    // 4. Enhance fingerprint -> skeleton
    cv::Mat final_processed = enhanceFingerprint(cropped_finger);

    // 5. Extract minutiae + build ISO template
    std::vector<Minutia> minutiae     = extractMinutiae(final_processed);
    std::vector<uchar>   iso_template = createIsoTemplate(
        minutiae, final_processed.cols, final_processed.rows);

    // 6. Encode skeleton to JPEG
    std::vector<uchar> img_buf;
    cv::imencode(".jpg", final_processed, img_buf);

    // 7. Pack: [4-byte big-endian image size][image bytes][template bytes]
    int img_size   = (int)img_buf.size();
    int total_size = 4 + img_size + (int)iso_template.size();

    jbyteArray result = env->NewByteArray(total_size);
    jbyte*     ptr    = env->GetByteArrayElements(result, nullptr);

    ptr[0] = (img_size >> 24) & 0xFF;
    ptr[1] = (img_size >> 16) & 0xFF;
    ptr[2] = (img_size >>  8) & 0xFF;
    ptr[3] =  img_size        & 0xFF;

    std::memcpy(ptr + 4,             img_buf.data(),      img_size);
    std::memcpy(ptr + 4 + img_size,  iso_template.data(), iso_template.size());

    env->ReleaseByteArrayElements(result, ptr, 0);

    LOGD("processFingerprint OK — img=%d bytes, template=%zu bytes, total=%d bytes",
         img_size, iso_template.size(), total_size);
    return result;
}

// ============================================================================
// SECTION 6: JNI — calculateQualityMetrics
//
// Returns float[5]: [blurScore, brightnessScore, centeringScore, sizeScore, overallScore]
// Weighted overall: 35% blur + 25% brightness + 25% centering + 15% size
//
// Strategy:
//   - If landmarks provided: crop to hand ROI for accurate per-hand metrics
//   - If ROI invalid or no landmarks: use full image
//   - blurScore min clamped to 20 to prevent hard rejection on marginally blurry frames
// ============================================================================

extern "C"
JNIEXPORT jfloatArray JNICALL
Java_com_example_biometric_1sdk_NativeProcessor_calculateQualityMetrics(
        JNIEnv* env, jclass clazz,
        jbyteArray imageBytes,
        jint width, jint height,
        jfloatArray landmarks,
        jint handCenterX, jint handCenterY) {

    LOGD("calculateQualityMetrics — %dx%d, center=(%d,%d)",
         width, height, handCenterX, handCenterY);

    // Decode image
    jbyte* raw = env->GetByteArrayElements(imageBytes, nullptr);
    jsize  len = env->GetArrayLength(imageBytes);
    std::vector<uchar> buf(raw, raw + len);
    cv::Mat img = cv::imdecode(buf, cv::IMREAD_COLOR);
    env->ReleaseByteArrayElements(imageBytes, raw, JNI_ABORT);

    float metrics[5] = {20.0f, 0.0f, 0.0f, 50.0f, 0.0f};  // safe defaults

    if (img.empty()) {
        LOGE("calculateQualityMetrics: Failed to decode image");
        jfloatArray out = env->NewFloatArray(5);
        env->SetFloatArrayRegion(out, 0, 5, metrics);
        return out;
    }

    cv::Mat gray;

    // Try to use hand ROI for more accurate quality measurement
    if (landmarks != nullptr) {
        jfloat* lms = env->GetFloatArrayElements(landmarks, nullptr);
        if (lms != nullptr) {
            cv::Rect handROI = getHandROI(img, lms, width, height);
            if (handROI.width > 0 && handROI.height > 0) {
                cv::cvtColor(img(handROI), gray, cv::COLOR_BGR2GRAY);
                metrics[3] = (float)calculateHandSizeScore(gray, handROI);
                LOGD("Hand ROI: %dx%d", handROI.width, handROI.height);
            } else {
                cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
                metrics[3] = 50.0f;
                LOGD("Invalid ROI — using full image");
            }
            env->ReleaseFloatArrayElements(landmarks, lms, JNI_ABORT);
        } else {
            cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
        }
    } else {
        cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
    }

    if (gray.empty()) cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);

    // Blur (min 20 to avoid hard rejection on slightly blurry frames)
    double blurScore = calculateBlurScore(gray);
    metrics[0] = (float)std::max(20.0, blurScore);

    // Brightness
    metrics[1] = (float)calculateBrightnessScore(gray);

    // Centering
    metrics[2] = (float)calculateCenteringScore(
        (float)handCenterX, (float)handCenterY, width, height);

    // Weighted overall
    double overall = metrics[0] * 0.35
                   + metrics[1] * 0.25
                   + metrics[2] * 0.25
                   + metrics[3] * 0.15;
    metrics[4] = (float)std::max(0.0, std::min(100.0, overall));

    LOGD("Quality — blur:%.1f bright:%.1f center:%.1f size:%.1f overall:%.1f",
         metrics[0], metrics[1], metrics[2], metrics[3], metrics[4]);

    jfloatArray out = env->NewFloatArray(5);
    env->SetFloatArrayRegion(out, 0, 5, metrics);
    return out;
}

// ============================================================================
// SECTION 7: JNI — detectMotionBlur
//
// Quick single-call blur check for real-time preview feedback.
// Returns:
//   0 = Sharp    (blur score >= 60)
//   1 = Moderate (blur score 30-59)
//   2 = Severe   (blur score < 30)
// ============================================================================

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_biometric_1sdk_NativeProcessor_detectMotionBlur(
        JNIEnv* env, jclass clazz,
        jbyteArray imageBytes,
        jint width, jint height) {

    jbyte* raw = env->GetByteArrayElements(imageBytes, nullptr);
    jsize  len = env->GetArrayLength(imageBytes);
    std::vector<uchar> buf(raw, raw + len);
    cv::Mat img = cv::imdecode(buf, cv::IMREAD_COLOR);
    env->ReleaseByteArrayElements(imageBytes, raw, JNI_ABORT);

    if (img.empty()) {
        LOGE("detectMotionBlur: Failed to decode image");
        return 2;  // treat as severe blur
    }

    cv::Mat gray;
    cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
    double blurScore = calculateBlurScore(gray);

    LOGD("detectMotionBlur — score: %.2f", blurScore);

    if      (blurScore >= 60) return 0;  // sharp
    else if (blurScore >= 30) return 1;  // moderate
    else                      return 2;  // severe
}