#include <jni.h>
#include <string>
#include <opencv2/opencv.hpp>
#include <vector>
#include <cmath>



// 1. Python ka `_gabor_bank_enhance` C++ mein

// Ye function fingerprint image me ridges (lines) ko clear aur sharp banata hai.
// 👉 Ye multiple directions me edges detect karke best fingerprint pattern highlight karta hai.
// 👉 SDK me iska use hota hai quality improve karne aur accurate feature extraction (template generation) ke liye.
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

// // 2. Python ka `_skeletonize` C++ mein
// 👉 Converts binary fingerprint image into **thin (1-pixel width) ridge structure** while preserving shape.
// 👉 Helps in accurate **minutiae detection (ridge endings & bifurcations)**.
// 👉 Used before **template generation for better feature extraction**.

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


// 3. Python ka main `enhance` function C++ mein
cv::Mat enhanceFingerprint(const cv::Mat& src) {
    cv::Mat gray;
    // BGR/RGBA se Grayscale (to_gray)
    if (src.channels() == 3) {
        cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
    } else if (src.channels() == 4) {
        cv::cvtColor(src, gray, cv::COLOR_RGBA2GRAY);
    } else {
        gray = src.clone();
    }

    // CLAHE (clipLimit=3.0, tileGridSize=8x8)
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(3.0, cv::Size(8, 8));
    cv::Mat clahe_out;
    clahe->apply(gray, clahe_out);

    // Gaussian Blur
    cv::Mat blurred;
    cv::GaussianBlur(clahe_out, blurred, cv::Size(3, 3), 1.0);

    // Gabor Bank (Ridge Enhancement)
    cv::Mat ridge_enhanced = gaborBankEnhance(blurred);

    // Adaptive Threshold
    cv::Mat binary;
    cv::adaptiveThreshold(ridge_enhanced, binary, 255,
                          cv::ADAPTIVE_THRESH_GAUSSIAN_C,
                          cv::THRESH_BINARY_INV, 11, 2);

    // Skeletonize
    return skeletonize(binary);
}

// Liveness Result Structure
struct LivenessResult {
    bool passed;
    std::string reason;
    float confidence;
};




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

//     // ── Layer 1: Glare Guard ──────────────────────────────
//     cv::Mat bright_mask;
//     cv::threshold(gray, bright_mask, 240, 255, cv::THRESH_BINARY);
//     double glare_ratio = (double)cv::countNonZero(bright_mask) /
//                          (gray.rows * gray.cols);
//     if (glare_ratio > 0.20) {
//         return {false, "Screen replay detected", 0.9f};
//     }

//     // ── Layer 2: Moire Pattern Detection ─────────────────
//     // Screen pe repetitive pattern hota hai
//     // Real finger mein nahi hota
//     cv::Mat fft_input;
//     gray.convertTo(fft_input, CV_32F);
//     cv::Mat planes[] = {fft_input, cv::Mat::zeros(
//         fft_input.size(), CV_32F)};
//     cv::Mat complex_img;
//     cv::merge(planes, 2, complex_img);
//     cv::dft(complex_img, complex_img);
//     cv::split(complex_img, planes);

//     cv::Mat magnitude;
//     cv::magnitude(planes[0], planes[1], magnitude);
//     magnitude += cv::Scalar::all(1);
//     cv::log(magnitude, magnitude);

//     // High frequency peaks = Screen/Print
//     cv::Scalar mean_mag, std_mag;
//     cv::meanStdDev(magnitude, mean_mag, std_mag);
//     double freq_variance = std_mag[0];

//     // Screen mein freq_variance zyada hoti hai
//     bool moire_detected = (freq_variance > 3.5);

//     // ── Layer 3: Adaptive LBP Texture ────────────────────
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

//     double mean_brightness = cv::mean(gray)[0];
//     double adaptive_threshold;
//     if (mean_brightness > 180)      adaptive_threshold = 18.0;
//     else if (mean_brightness > 120) adaptive_threshold = 22.0;
//     else                            adaptive_threshold = 15.0;

//     bool lbp_ok = (lbp_var >= adaptive_threshold);

//     // ── Layer 4: Color Uniformity Check ──────────────────
//     // Screen pe color zyada uniform hota hai
//     // Real finger mein variation hoti hai
//     cv::Mat hsv;
//     cv::cvtColor(bgr_roi, hsv, cv::COLOR_BGR2HSV);
//     std::vector<cv::Mat> hsv_channels;
//     cv::split(hsv, hsv_channels);

//     cv::Scalar mean_s, std_s;
//     cv::meanStdDev(hsv_channels[1], mean_s, std_s);
//     double saturation_std = std_s[0];

//     // Real finger: saturation variation zyada hoti hai
//     // Screen/print: saturation uniform hoti hai
//     bool color_variation_ok = (saturation_std > 15.0);

//     // ── Layer 5: Skin Detection ───────────────────────────
//     cv::Mat mask1, mask2, mask3, skin_mask;
//     cv::inRange(hsv,
//         cv::Scalar(0, 15, 30),
//         cv::Scalar(25, 255, 255), mask1);
//     cv::inRange(hsv,
//         cv::Scalar(0, 8, 40),
//         cv::Scalar(50, 200, 255), mask2);
//     cv::inRange(hsv,
//         cv::Scalar(155, 10, 30),
//         cv::Scalar(180, 255, 255), mask3);
//     cv::bitwise_or(mask1, mask2, skin_mask);
//     cv::bitwise_or(skin_mask, mask3, skin_mask);

//     double skin_ratio = (double)cv::countNonZero(skin_mask) /
//                         (bgr_roi.rows * bgr_roi.cols);
//     bool skin_ok = (skin_ratio >= 0.03);

//     // ── Layer 6: Edge Density ─────────────────────────────
//     cv::Mat edges;
//     cv::Canny(gray, edges, 30, 100);
//     double edge_density = (double)cv::countNonZero(edges) /
//                           (gray.rows * gray.cols);
//     bool edge_ok = (edge_density >= 0.03 && edge_density <= 0.65);

//     // ── Scoring ───────────────────────────────────────────
//     int score = 0;
//     if (lbp_ok)              score += 30;
//     if (skin_ok)             score += 20;
//     if (edge_ok)             score += 20;
//     if (color_variation_ok)  score += 20;
//     if (!moire_detected)     score += 10;

//     // ── Instant Reject Rules ──────────────────────────────
//     // Screen pe moire hoga
//     if (moire_detected && !lbp_ok) {
//         return {false, "Screen replay detected", 0.0f};
//     }

//     // Print pe color variation nahi hogi
//     if (!color_variation_ok && !lbp_ok) {
//         return {false, "Printed photo detected", 0.0f};
//     }

//     // ── Confidence ────────────────────────────────────────
//     float confidence = initial_confidence;
//     if (!lbp_ok)             confidence *= 0.80f;
//     if (!skin_ok)            confidence *= 0.85f;
//     if (!edge_ok)            confidence *= 0.85f;
//     if (!color_variation_ok) confidence *= 0.75f;
//     if (moire_detected)      confidence *= 0.70f;
//     confidence = std::max(0.0f, std::min(confidence, 1.0f));

//     // ── Decision ─────────────────────────────────────────
//     if (score >= 50) {
//         return {true, "Real finger detected", confidence};
//     }

//     if (score >= 30 && skin_ok && lbp_ok) {
//         return {true, "Real finger (ISP smoothed)", 0.70f};
//     }

//     return {false, "Spoof detected", 0.0f};
// }


LivenessResult checkAdvancedLiveness(
    const cv::Mat& bgr_roi,
    float initial_confidence) {

    if (bgr_roi.empty() ||
        bgr_roi.rows < 20 ||
        bgr_roi.cols < 20) {
        return {false, "Invalid ROI", 0.0f};
    }

    cv::Mat gray;
    cv::cvtColor(bgr_roi, gray, cv::COLOR_BGR2GRAY);

    // ── Layer 2: Glare — Python se match ─────────────────
    cv::Mat bright_mask;
    cv::threshold(gray, bright_mask, 245, 255, cv::THRESH_BINARY);
    double glare_ratio = (double)cv::countNonZero(bright_mask) /
                         (gray.rows * gray.cols);

    if (glare_ratio > 0.12) {  // Python: 0.12
        return {false, "Screen replay detected", 0.85f};
    }

    // ── Layer 3: LBP Texture ──────────────────────────────
    cv::Mat f, f_squared, mu, mu2;
    gray.convertTo(f, CV_32F);
    cv::multiply(f, f, f_squared);
    cv::blur(f, mu, cv::Size(5, 5));
    cv::blur(f_squared, mu2, cv::Size(5, 5));

    cv::Mat mu_squared, var, stddev_map;
    cv::multiply(mu, mu, mu_squared);
    cv::subtract(mu2, mu_squared, var);
    cv::max(var, 0.0, var);
    cv::sqrt(var, stddev_map);

    double lbp_var = cv::mean(stddev_map)[0];
    bool lbp_ok = (lbp_var >= 18.0);  // Python: 18.0

    // ── Layer 4: Skin Detection — Python se match ─────────
    cv::Mat hsv;
    cv::cvtColor(bgr_roi, hsv, cv::COLOR_BGR2HSV);

    cv::Mat mask1, mask2, skin_mask;

    // Primary skin range — Python se same
    cv::inRange(hsv,
        cv::Scalar(0, 20, 50),
        cv::Scalar(25, 170, 255), mask1);

    // Reddish wrap — Python se same
    cv::inRange(hsv,
        cv::Scalar(165, 20, 50),
        cv::Scalar(180, 170, 255), mask2);

    cv::bitwise_or(mask1, mask2, skin_mask);

    double skin_ratio = (double)cv::countNonZero(skin_mask) /
                        (bgr_roi.rows * bgr_roi.cols);

    bool skin_ok = (skin_ratio >= 0.15);  // Python: 0.15

    // ── Confidence — Python se same logic ─────────────────
    float confidence = initial_confidence;
    confidence *= (1.0f - 0.08f * (float)(glare_ratio / 0.12f));

    if (!lbp_ok) confidence *= 0.75f;
    if (!skin_ok) confidence *= 0.80f;
    confidence = std::max(0.0f, std::min(confidence, 1.0f));

    // ── Hard Fail — Python se same ────────────────────────
    if (!lbp_ok && !skin_ok) {
        return {false,
                "Spoof: low texture and no skin tone",
                confidence};
    }

    return {true, "Real finger detected", confidence};
}


struct Minutia {
    int x, y;
    int type; // 1 = Ridge Ending, 3 = Bifurcation
};

// 1. Extract Minutiae using Crossing Number
std::vector<Minutia> extractMinutiae(const cv::Mat& skeleton) {
    std::vector<Minutia> minutiae;
    
    // Scan every pixel (ignoring borders to prevent out-of-bounds)
    for (int y = 1; y < skeleton.rows - 1; y++) {
        for (int x = 1; x < skeleton.cols - 1; x++) {
            if (skeleton.at<uchar>(y, x) == 255) { // If it's a ridge pixel
                
                // Get 8 neighbors (Clockwise starting from top)
                int p[8];
                p[0] = skeleton.at<uchar>(y-1, x)   > 0 ? 1 : 0;
                p[1] = skeleton.at<uchar>(y-1, x+1) > 0 ? 1 : 0;
                p[2] = skeleton.at<uchar>(y, x+1)   > 0 ? 1 : 0;
                p[3] = skeleton.at<uchar>(y+1, x+1) > 0 ? 1 : 0;
                p[4] = skeleton.at<uchar>(y+1, x)   > 0 ? 1 : 0;
                p[5] = skeleton.at<uchar>(y+1, x-1) > 0 ? 1 : 0;
                p[6] = skeleton.at<uchar>(y, x-1)   > 0 ? 1 : 0;
                p[7] = skeleton.at<uchar>(y-1, x-1) > 0 ? 1 : 0;

                // Calculate Crossing Number (transitions from 0 to 1)
                int transitions = 0;
                for (int i = 0; i < 8; i++) {
                    transitions += std::abs(p[i] - p[(i + 1) % 8]);
                }
                int cn = transitions / 2;

                // Classify based on Crossing Number
                if (cn == 1) {
                    minutiae.push_back({x, y, 1}); // Ending
                } else if (cn == 3) {
                    minutiae.push_back({x, y, 3}); // Bifurcation
                }
            }
        }
    }
    return minutiae;
}

// 2. Pack into ISO/IEC 19794-2 Binary Format
std::vector<uchar> createIsoTemplate(const std::vector<Minutia>& minutiae, int width, int height) {
    std::vector<uchar> tmpl;
    
    // --- ISO Magic Header (24 bytes) ---
    tmpl.push_back('F'); tmpl.push_back('M'); tmpl.push_back('R'); tmpl.push_back(0); // Magic
    tmpl.push_back(' '); tmpl.push_back('2'); tmpl.push_back('0'); tmpl.push_back(0); // Version
    
    // Total Length = Header(24) + FingerHeader(4) + (MinutiaeCount * 6) + Footer(2)
    int totalLen = 24 + 4 + (minutiae.size() * 6) + 2; 
    tmpl.push_back((totalLen >> 24) & 0xFF); tmpl.push_back((totalLen >> 16) & 0xFF);
    tmpl.push_back((totalLen >> 8) & 0xFF);  tmpl.push_back(totalLen & 0xFF);
    
    tmpl.push_back(0); tmpl.push_back(0); // Capture Equipment ID
    tmpl.push_back((width >> 8) & 0xFF);  tmpl.push_back(width & 0xFF); // Width
    tmpl.push_back((height >> 8) & 0xFF); tmpl.push_back(height & 0xFF); // Height
    tmpl.push_back(0); tmpl.push_back(197); // X Resolution (197 pixels/cm = ~500 dpi)
    tmpl.push_back(0); tmpl.push_back(197); // Y Resolution (~500 dpi)
    tmpl.push_back(1); // Finger Count (1)
    tmpl.push_back(0); // Reserved byte

    // --- Finger View Header (4 bytes) ---
    tmpl.push_back(0);   // Finger position (0 = Unknown, Kotlin handles mapping)
    tmpl.push_back(0);   // View number
    tmpl.push_back(100); // Impression type (Contactless)
    tmpl.push_back(100); // Quality (0-100)
    tmpl.push_back(minutiae.size() & 0xFF); // Total Minutiae Count

    // --- Minutiae Data (6 bytes per point) ---
    for(const auto& m : minutiae) {
        int type = (m.type == 1) ? 1 /*Ending*/ : 2 /*Bifurcation*/;
        
        // Byte 1: Type (first 2 bits) + X-coord high bits
        tmpl.push_back((type << 6) | ((m.x >> 8) & 0x3F)); 
        // Byte 2: X-coord low bits
        tmpl.push_back(m.x & 0xFF);
        // Byte 3: Reserved (first 2 bits) + Y-coord high bits
        tmpl.push_back((m.y >> 8) & 0x3F);
        // Byte 4: Y-coord low bits
        tmpl.push_back(m.y & 0xFF);
        // Byte 5: Angle (Simplified to 0 for this version)
        tmpl.push_back(0); 
        // Byte 6: Quality (Set to 100 for all points)
        tmpl.push_back(100); 
    }
    
    // --- Footer ---
    tmpl.push_back(0); tmpl.push_back(0); // Extended data length (0)
    
    return tmpl;
}




// --- 2. Main JNI Bridge (Kotlin se yahan aayega) ---
// --- 4. Main JNI Bridge ---
extern "C"
JNIEXPORT jbyteArray JNICALL
Java_com_example_biometric_1sdk_NativeProcessor_processFingerprint(
        JNIEnv* env, jclass clazz, jbyteArray imageBytes, jint width, jint height, jfloatArray landmarks) {

    // A. Kotlin ByteArray ko OpenCV Image mein convert karein
    jbyte* bytes = env->GetByteArrayElements(imageBytes, nullptr);
    jsize length = env->GetArrayLength(imageBytes);
    std::vector<uchar> buffer(bytes, bytes + length);
    cv::Mat img = cv::imdecode(buffer, cv::IMREAD_COLOR);
    env->ReleaseByteArrayElements(imageBytes, bytes, JNI_ABORT);

    if (img.empty()) return nullptr;

    // B. Landmarks read karein aur Crop karein
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
    if (roi.width <= 0 || roi.height <= 0) return nullptr;
    cv::Mat cropped_finger = img(roi);


    LivenessResult liveness = checkAdvancedLiveness(cropped_finger, 0.95f);

    if (!liveness.passed) {
        // Agar fraud detect hua, toh 1 byte (0) bhej kar Kotlin ko bata do
        jbyteArray fake_result = env->NewByteArray(1);
        jbyte fake_flag[] = {0};
        env->SetByteArrayRegion(fake_result, 0, 1, fake_flag);
        return fake_result;
    }

    // C. Helper function call karein (Clean approach!)
    cv::Mat final_processed = enhanceFingerprint(cropped_finger);


    // D. Return to Kotlin
  // 🔥 1. Extract Minutiae & Build Template 🔥
    std::vector<Minutia> minutiae = extractMinutiae(final_processed);
    std::vector<uchar> iso_template = createIsoTemplate(minutiae, final_processed.cols, final_processed.rows);

    // 🔥 2. Encode Processed Image to JPG 🔥
    std::vector<uchar> img_buf;
    cv::imencode(".jpg", final_processed, img_buf);
    
    // 🔥 3. Pack Both into a Single Array for Kotlin 🔥
    // Format: [4 Bytes for Image Size] + [Image Bytes] + [Template Bytes]
    int img_size = img_buf.size();
    int total_size = 4 + img_size + iso_template.size();
    
    jbyteArray result = env->NewByteArray(total_size);
    jbyte* result_ptr = env->GetByteArrayElements(result, nullptr);
    
    // Write 4 bytes for Image Size (Big Endian format)
    result_ptr[0] = (img_size >> 24) & 0xFF;
    result_ptr[1] = (img_size >> 16) & 0xFF;
    result_ptr[2] = (img_size >> 8) & 0xFF;
    result_ptr[3] = img_size & 0xFF;
    
    // Copy the Image Bytes
    std::memcpy(result_ptr + 4, img_buf.data(), img_size);
    // Copy the Template Bytes right after the image
    std::memcpy(result_ptr + 4 + img_size, iso_template.data(), iso_template.size());
    
    // Memory release karein aur result return karein
    env->ReleaseByteArrayElements(result, result_ptr, 0);
    return result;
    
}
