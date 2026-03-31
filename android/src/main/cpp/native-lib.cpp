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

    // C. Helper function call karein (Clean approach!)
    cv::Mat final_processed = enhanceFingerprint(cropped_finger);

    // D. Return to Kotlin
    std::vector<uchar> out_buf;
    cv::imencode(".jpg", final_processed, out_buf);
    
    jbyteArray result = env->NewByteArray(out_buf.size());
    env->SetByteArrayRegion(result, 0, out_buf.size(), reinterpret_cast<jbyte*>(out_buf.data()));
    return result;
}
