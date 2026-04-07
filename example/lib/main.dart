import 'dart:convert';
import 'dart:async';
import 'dart:typed_data';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:camera/camera.dart';
import 'package:permission_handler/permission_handler.dart';
import 'package:biometric_sdk/biometric_sdk.dart';
import 'package:image/image.dart' as img;

void main() async {
  WidgetsFlutterBinding.ensureInitialized();
  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Biometric SDK',
      theme: ThemeData(
        primaryColor: const Color(0xFFFFC107),
        scaffoldBackgroundColor: Colors.white,
      ),
      home: const HomeScreen(),
    );
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// Home Screen
// ─────────────────────────────────────────────────────────────────────────────

class HomeScreen extends StatefulWidget {
  const HomeScreen({super.key});

  @override
  State<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends State<HomeScreen> {
  String _result = '';

  Future<void> _startCapture(String mode) async {
    final status = await Permission.camera.request();
    if (!status.isGranted) {
      setState(() => _result = 'Camera permission denied!');
      return;
    }
    if (!mounted) return;

    final captureResult = await Navigator.push(
      context,
      MaterialPageRoute(builder: (_) => CameraScreen(mode: mode)),
    );

    if (captureResult != null) {
      setState(() => _result = captureResult.toString());
    }
  }

  @override
  Widget build(BuildContext context) {
    const primaryYellow = Color(0xFFFFC107);
    const lightYellow   = Color(0xFFFFF8E1);

    return Scaffold(
      appBar: AppBar(
        backgroundColor: primaryYellow,
        title: const Text(
          'YellowSense Biometric SDK',
          style: TextStyle(color: Colors.white, fontWeight: FontWeight.bold),
        ),
        centerTitle: true,
      ),
      body: SingleChildScrollView(
        padding: const EdgeInsets.all(20),
        child: Column(
          children: [
            const SizedBox(height: 10),
            if (_result.isNotEmpty)
              Container(
                width: double.infinity,
                padding: const EdgeInsets.all(16),
                margin: const EdgeInsets.only(bottom: 20),
                decoration: BoxDecoration(
                  color: lightYellow,
                  border: Border.all(color: primaryYellow),
                  borderRadius: BorderRadius.circular(12),
                ),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    const Row(
                      children: [
                        Icon(Icons.check_circle, color: primaryYellow, size: 20),
                        SizedBox(width: 8),
                        Text(
                          'Capture Result',
                          style: TextStyle(
                            fontWeight: FontWeight.bold,
                            color: primaryYellow,
                          ),
                        ),
                      ],
                    ),
                    const SizedBox(height: 8),
                    Text(_result),
                  ],
                ),
              ),
            HandSelectionCard(onCapture: _startCapture),
          ],
        ),
      ),
    );
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// Hand Selection Card
// ─────────────────────────────────────────────────────────────────────────────

class HandSelectionCard extends StatelessWidget {
  final Function(String) onCapture;
  const HandSelectionCard({super.key, required this.onCapture});

  @override
  Widget build(BuildContext context) {
    return Column(
      children: [
        const Text(
          'Select Capture Mode',
          style: TextStyle(
            fontSize: 18,
            fontWeight: FontWeight.bold,
            color: Color(0xFFFFC107),
          ),
        ),
        const SizedBox(height: 6),
        const Text(
          'Select the fingers you want to capture',
          style: TextStyle(color: Colors.grey, fontSize: 13),
        ),
        const SizedBox(height: 20),
        Row(
          mainAxisAlignment: MainAxisAlignment.spaceEvenly,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            _HandDiagram(label: 'Left Hand',  isLeft: true,  onCapture: onCapture),
            _HandDiagram(label: 'Right Hand', isLeft: false, onCapture: onCapture),
          ],
        ),
        const SizedBox(height: 24),
        const Divider(),
        const SizedBox(height: 16),
        const Text(
          'Quick Capture',
          style: TextStyle(fontSize: 16, fontWeight: FontWeight.bold),
        ),
        const SizedBox(height: 12),
        Row(
          children: [
            Expanded(
              child: _QuickButton(
                label: 'Left\nThumb', icon: Icons.thumb_up,
                mode: 'LEFT_THUMB', onTap: onCapture,
              ),
            ),
            const SizedBox(width: 10),
            Expanded(
              child: _QuickButton(
                label: 'Right\nThumb', icon: Icons.thumb_up,
                mode: 'RIGHT_THUMB', onTap: onCapture,
              ),
            ),
            const SizedBox(width: 10),
            Expanded(
              child: _QuickButton(
                label: 'Single\nFinger', icon: Icons.touch_app,
                mode: 'SINGLE_FINGER', onTap: onCapture,
              ),
            ),
          ],
        ),
      ],
    );
  }
}

class _HandDiagram extends StatelessWidget {
  final String label;
  final bool isLeft;
  final Function(String) onCapture;

  const _HandDiagram({required this.label, required this.isLeft, required this.onCapture});

  @override
  Widget build(BuildContext context) {
    const primaryYellow = Color(0xFFFFC107);
    final mode    = isLeft ? 'LEFT_FOUR' : 'RIGHT_FOUR';
    final fingers = isLeft ? ['L4','L3','L2','L1'] : ['R1','R2','R3','R4'];

    return GestureDetector(
      onTap: () => onCapture(mode),
      child: Column(
        children: [
          Text(label, style: const TextStyle(fontWeight: FontWeight.bold, fontSize: 14)),
          const SizedBox(height: 8),
          Container(
            width: 130, height: 160,
            decoration: BoxDecoration(
              color: Colors.green[50],
              borderRadius: BorderRadius.circular(12),
              border: Border.all(color: primaryYellow, width: 1.5),
            ),
            child: Column(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                Icon(Icons.back_hand, size: 80, color: const Color(0xFF1B5E20)),
                const SizedBox(height: 8),
                const Text('Tap to Capture',
                    style: TextStyle(fontSize: 11, color: primaryYellow)),
              ],
            ),
          ),
          const SizedBox(height: 8),
          Row(
            mainAxisSize: MainAxisSize.min,
            children: fingers.map((f) => Container(
              margin: const EdgeInsets.symmetric(horizontal: 2),
              padding: const EdgeInsets.symmetric(horizontal: 6, vertical: 4),
              decoration: BoxDecoration(
                color: primaryYellow,
                borderRadius: BorderRadius.circular(6),
              ),
              child: Text(f,
                  style: const TextStyle(
                      color: Colors.white, fontSize: 11, fontWeight: FontWeight.bold)),
            )).toList(),
          ),
          const SizedBox(height: 8),
          ElevatedButton(
            onPressed: () => onCapture(mode),
            style: ElevatedButton.styleFrom(
              backgroundColor: primaryYellow,
              shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(8)),
              padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
            ),
            child: const Text('Capture All',
                style: TextStyle(color: Colors.white, fontSize: 12)),
          ),
        ],
      ),
    );
  }
}

class _QuickButton extends StatelessWidget {
  final String label;
  final IconData icon;
  final String mode;
  final Function(String) onTap;

  const _QuickButton({
    required this.label,
    required this.icon,
    required this.mode,
    required this.onTap,
  });

  @override
  Widget build(BuildContext context) {
    const primaryYellow = Color(0xFFFFC107);
    return GestureDetector(
      onTap: () => onTap(mode),
      child: Container(
        padding: const EdgeInsets.symmetric(vertical: 12, horizontal: 8),
        decoration: BoxDecoration(
          color: Colors.green[50],
          borderRadius: BorderRadius.circular(12),
          border: Border.all(color: primaryYellow),
        ),
        child: Column(
          children: [
            Icon(icon, color: primaryYellow, size: 28),
            const SizedBox(height: 6),
            Text(
              label,
              textAlign: TextAlign.center,
              style: const TextStyle(
                fontSize: 11, fontWeight: FontWeight.bold, color: primaryYellow,
              ),
            ),
          ],
        ),
      ),
    );
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// Camera Screen
// ─────────────────────────────────────────────────────────────────────────────

class CameraScreen extends StatefulWidget {
  final String mode;
  const CameraScreen({super.key, required this.mode});

  @override
  State<CameraScreen> createState() => _CameraScreenState();
}

class _CameraScreenState extends State<CameraScreen> {
  CameraController? _controller;
  bool _initialized    = false;
  String _guidance     = 'Camera starting...';
  bool _fingerDetected = false;
  int _qualityScore = 0;
  int _fingerCount = 0;
  bool _capturing = false;
  String _processedBase64 = '';
  String _detectedFingers = '';
  bool _isProcessingFrame = false;
  int _stableReadyCount = 0;
  Map<String, dynamic> _qualityDetails = {};

  static const _channel = MethodChannel('biometric_sdk');
  Timer? _timer;

  @override
  void initState() {
    super.initState();
    _initCamera();
  }

  Future<void> _initCamera() async {
    try {
      final cameras = await availableCameras();
      if (cameras.isEmpty) {
        setState(() => _guidance = 'No camera found!');
        return;
      }

      // FIX: Use BACK camera instead of front camera
      // This ensures no mirroring issues with MediaPipe hand detection
      final camera = cameras.firstWhere(
        (c) => c.lensDirection == CameraLensDirection.back,
        orElse: () => cameras.first,
      );

      _controller = CameraController(
        camera,
        ResolutionPreset.medium,
        enableAudio: false,
      );

      await _controller!.initialize();

      if (mounted) {
        setState(() {
          _initialized = true;
          _guidance    = 'Place your hand in front of camera';
        });
        _startTimer();
      }
    } catch (e) {
      setState(() => _guidance = 'Camera error: $e');
    }
  }

  void _startTimer() {
    _timer = Timer.periodic(
      const Duration(milliseconds: 600),
      (timer) async {
        if (_capturing || !_initialized||_isProcessingFrame) return;
        await _processFrame();
      },
    );
  }
Uint8List cropToBox(Uint8List bytes, double previewWidth, double previewHeight) {
  final original = img.decodeImage(bytes);
if (original == null) return bytes;

  final scaleX = original.width / previewWidth;
  final scaleY = original.height / previewHeight;

  const boxWidth = 250;
  const boxHeight = 300;

  final left = ((previewWidth - boxWidth) / 2) * scaleX;
  final top  = ((previewHeight - boxHeight) / 2) * scaleY;

  final crop = img.copyCrop(
    original,
    x: left.toInt(),
    y: top.toInt(),
    width: (boxWidth * scaleX).toInt(),
    height: (boxHeight * scaleY).toInt(),
  );

  return Uint8List.fromList(img.encodeJpg(crop));
}
  

  // Future<void> _processFrame() async {
  //   _isProcessingFrame = true;
  //   try {
  //     final image = await _controller!.takePicture();
  //     final bytes = await image.readAsBytes();

  //     final result = await _channel.invokeMethod('processFrame', {
  //       'frame'           : bytes,
  //       'captureMode'     : widget.mode,
  //       'fingersRequested': <String>[],
  //       'missingFingers'  : <String>[],
  //     });

  //     if (!mounted) {
  //       _isProcessingFrame = false;
  //       return;
  //     }

  //     //final qualityDetails = result['qualityDetails'] as Map? ?? {};

  //     setState(() {
  //       _qualityDetails = Map<String, dynamic>.from(result['qualityDetails'] as Map? ?? {});
  //       _fingerDetected = result['fingerDetected'] as bool? ?? false;
  //       _guidance       = result['guidance'] as String? ?? '';
  //       _qualityScore   = result['qualityScore'] as int? ?? 0;
  //       _fingerCount    = result['fingerCount'] as int? ?? 0;
  //       _processedBase64 = result['processedImage'] as String? ?? '';
  //       _detectedFingers = result['detectedFingersName'] as String? ?? '';
  //     });

  //     final ready = result['readyToCapture'] as bool? ?? false;
  //     if (ready && !_capturing) {
  //       _timer?.cancel();
  //       _stableReadyCount    = 0;
  //       _isProcessingFrame   = false;
  //       await _capture(bytes);
  //       return;
  //     }

  //   } catch (e) {
  //     print('Frame error: $e');
  //   }finally {
  //     // 🔥 NAYA LOGIC: Chahe pass ho ya error aaye, processing khatam hone par Lock khol do
  //     _isProcessingFrame = false; 
  //   }
  // }

  Future<void> _processFrame() async {
  _isProcessingFrame = true;

  try {
    // 1. Capture frame
    final image = await _controller!.takePicture();

    // 2. Convert to bytes
    final rawBytes = await image.readAsBytes();

    // 3. Get preview size (IMPORTANT for crop scaling)
    final previewSize = _controller!.value.previewSize!;

    // ⚠️ Camera rotation fix (height ↔ width)
    final croppedBytes = cropToBox(
      rawBytes,
      previewSize.height,
      previewSize.width,
    );

    // 4. Send cropped frame to native SDK
    final result = await _channel.invokeMethod('processFrame', {
      'frame'           : croppedBytes,
      'captureMode'     : widget.mode,
      'fingersRequested': <String>[],
      'missingFingers'  : <String>[],
    });

    if (!mounted) {
      _isProcessingFrame = false;
      return;
    }

    // 5. Update UI state
    setState(() {
      _qualityDetails = Map<String, dynamic>.from(
        result['qualityDetails'] as Map? ?? {},
      );

      _fingerDetected   = result['fingerDetected'] as bool? ?? false;
      _guidance         = result['guidance'] as String? ?? '';
      _qualityScore     = result['qualityScore'] as int? ?? 0;
      _fingerCount      = result['fingerCount'] as int? ?? 0;
      _processedBase64  = result['processedImage'] as String? ?? '';
      _detectedFingers  = result['detectedFingersName'] as String? ?? '';
    });

    // 6. Auto capture trigger
    final ready = result['readyToCapture'] as bool? ?? false;

    if (ready && !_capturing) {
      _timer?.cancel();
      _stableReadyCount  = 0;
      _isProcessingFrame = false;

      // 👇 IMPORTANT: pass CROPPED image, not raw
      await _capture(croppedBytes);
      return;
    }

  } catch (e) {
    print('Frame error: $e');
  } finally {
    // 7. Always release lock
    _isProcessingFrame = false;
  }
}



  Future<void> _capture([Uint8List? existingBytes]) async {
    if (_capturing) return;

    setState(() {
      _capturing = true;
      _guidance  = 'Processing…';
    });
    _timer?.cancel();

    try {
      // Uint8List bytes;
      // if (existingBytes != null) {
      //   bytes = existingBytes;
      // } else {
      //   final image = await _controller!.takePicture();
      //   bytes = await image.readAsBytes();
      // }
      Uint8List rawBytes;

if (existingBytes != null) {
  rawBytes = existingBytes;
} else {
  final image = await _controller!.takePicture();
  rawBytes = await image.readAsBytes();

  final previewSize = _controller!.value.previewSize!;
  rawBytes = cropToBox(
    rawBytes,
    previewSize.height,
    previewSize.width,
  );
}

final bytes = rawBytes;

      // Pass the first detected finger so SDK can fill SINGLE_FINGER mode correctly
      final String firstDetectedFinger = _detectedFingers.isNotEmpty
          ? _detectedFingers.split(',').first.trim()
          : '';

      final result = await _channel.invokeMethod('startCapture', {
        'captureMode'   : widget.mode,
        'image'         : bytes,
        'missingFingers': <String>[],
        'detectedFinger': firstDetectedFinger,
        'qualityScore'  : _qualityScore,
      });

      debugPrint('Capture result: $result');

      if (mounted) {
        Navigator.pushReplacement(
          context,
          MaterialPageRoute(
            builder: (_) => ResultScreen(
              result        : result,
              qualityScore  : _qualityScore,
              capturedImage : bytes,
              mode          : widget.mode,
              qualityDetails: _qualityDetails,
            ),
          ),
        );
      }
    } catch (e) {
      debugPrint('Capture error: $e');
      if (mounted) {
        setState(() {
          _capturing = false;
          _guidance  = 'Please try again';
        });
        _startTimer();
      }
    }
  }

  @override
  void dispose() {
    _timer?.cancel();
    _controller?.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    const primaryYellow = Color(0xFFFFC107);
    return Scaffold(
      backgroundColor: Colors.black,
      appBar: AppBar(
        backgroundColor: primaryYellow,
        title: Text(
          widget.mode.replaceAll('_', ' '),
          style: const TextStyle(color: Colors.white, fontWeight: FontWeight.bold),
        ),
        iconTheme: const IconThemeData(color: Colors.white),
        centerTitle: true,
      ),
      body: Column(
        children: [

          // Camera Preview
          Expanded(
            child: _initialized
                ? Stack(
                    children: [
                      // Camera
                      SizedBox.expand(
                        child: CameraPreview(_controller!),
                      ),

                      // Overlay frame
                      Center(
                        child: Container(
                          width: 250,
                          height: 300,
                          decoration: BoxDecoration(
                            border: Border.all(
                              color: _fingerDetected
                                  ? Color(0xFFFFC107)
                                  : Colors.white,
                              width: 2,
                            ),
                            borderRadius: BorderRadius.circular(12),
                          ),
                        ),
                      ),

                      // Quality Badge
                      Positioned(
                        top: 16,
                        left: 16,
                        child: Container(
                          padding: const EdgeInsets.symmetric(
                            horizontal: 12, vertical: 6,
                          ),
                          decoration: BoxDecoration(
                            color: _qualityScore >= 70
                                ? Color(0xFFFFC107)
                                : Colors.orange,
                            borderRadius: BorderRadius.circular(20),
                          ),
                          child: Text(
                            'Quality: $_qualityScore%',
                            style: const TextStyle(
                              color: Colors.white,
                              fontWeight: FontWeight.bold,
                              fontSize: 13,
                            ),
                          ),
                        ),
                      ),

                      // Finger Count Badge
                      if (_fingerDetected)
                        Positioned(
                          top: 16,
                          right: 16,
                          child: Container(
                            padding: const EdgeInsets.symmetric(
                              horizontal: 12, vertical: 6,
                            ),
                            decoration: BoxDecoration(
                              color: Color(0xFFFFC107),
                              borderRadius: BorderRadius.circular(20),
                            ),
                            child: Row(
                              mainAxisSize: MainAxisSize.min,
                              children: [
                                const Icon(
                                  Icons.check_circle,
                                  color: Colors.white,
                                  size: 14,
                                ),
                                const SizedBox(width: 4),
                                Text(
                                  '$_fingerCount Finger${_fingerCount > 1 ? 's' : ''}',
                                  style: const TextStyle(
                                    color: Colors.white,
                                    fontWeight: FontWeight.bold,
                                    fontSize: 13,
                                  ),
                                ),
                              ],
                            ),
                          ),
                        ),
                    ],
                  )
                : const Center(
                    child: Column(
                      mainAxisAlignment: MainAxisAlignment.center,
                      children: [
                        CircularProgressIndicator(color: Colors.white),
                        SizedBox(height: 16),
                        Text('Initializing camera…',
                            style: TextStyle(color: Colors.white)),
                      ],
                    ),
                  ),
          ),

          // ── Bottom panel ──────────────────────────────────────────────
          Container(
            color: primaryYellow,
            padding: const EdgeInsets.all(16),
            child: Column(
              children: [
                // Processed image preview
                if (_processedBase64.isNotEmpty)
                  Container(
                    margin: const EdgeInsets.only(bottom: 12),
                    height: 120, width: 90,
                    decoration: BoxDecoration(
                      color: Colors.white,
                      borderRadius: BorderRadius.circular(8),
                      border: Border.all(color: Colors.black, width: 2),
                    ),
                    child: ClipRRect(
                      borderRadius: BorderRadius.circular(6),
                      child: Image.memory(
                        base64Decode(_processedBase64),
                        fit: BoxFit.cover,
                        gaplessPlayback: true,
                      ),
                    ),
                  ),

                // Guidance
                // Guidance
                Row(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    // 🔥 NAYA LOGIC: Agar guidance mein "Spoof" likha hai, toh RED color dikhao
                    Icon(
                      _fingerDetected
                          ? Icons.check_circle
                          : Icons.info_outline,
                      color: Colors.white,
                      size: 18,
                    ),
                    const SizedBox(width: 8),
                    Text(
                      _guidance,
                      style: const TextStyle(
                        color: Colors.white,
                        fontSize: 15,
                        fontWeight: FontWeight.w500,
                      ),
                    ),
                  ],
                ),
                if (_detectedFingers.isNotEmpty) ...[
                  const SizedBox(height: 8),
                  Text(
                    'Detecting: $_detectedFingers',
                    style: const TextStyle(
                      color: Colors.white70, // Thoda light color
                      fontSize: 13,
                      fontWeight: FontWeight.bold,
                    ),
                  ),
                ],

                const SizedBox(height: 16),
              ],
            ),
          ),
        ],
      ),
    );
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// Result Screen
// ─────────────────────────────────────────────────────────────────────────────

class ResultScreen extends StatelessWidget {
  final Map result;
  final int qualityScore;
  final Uint8List capturedImage;
  final String mode;
  final Map<String, dynamic> qualityDetails;

  const ResultScreen({
    super.key,
    required this.result,
    required this.qualityScore,
    required this.capturedImage,
    required this.mode,
    this.qualityDetails = const {},
  });

  @override
  Widget build(BuildContext context) {
    final fingers = result['results']       as List;
    final status  = result['overallStatus'] as String;
    final txnId   = result['transactionId'] as String;

    const primaryYellow = Color(0xFFFFC107);

    final blurScore       = qualityDetails['blurScore']       as int? ?? qualityScore;
    final brightnessScore = qualityDetails['brightnessScore'] as int? ?? qualityScore;
    final centeringScore  = qualityDetails['centeringScore']  as int? ?? qualityScore;

    return Scaffold(
      backgroundColor: Colors.white,
      appBar: AppBar(
        backgroundColor: primaryYellow,
        title: const Text(
          'Capture Result',
          style: TextStyle(color: Colors.white, fontWeight: FontWeight.bold),
        ),
        centerTitle: true,
        automaticallyImplyLeading: false,
      ),
      body: SingleChildScrollView(
        padding: const EdgeInsets.all(20),
        child: Column(
          children: [
            // Status icon
            Container(
              width: 80, height: 80,
              decoration: BoxDecoration(
                color: Colors.green[50],
                shape: BoxShape.circle,
                border: Border.all(color: Colors.green, width: 2),
              ),
              child: const Icon(Icons.check_circle, color: primaryYellow, size: 50),
            ),
            const SizedBox(height: 12),
            Text(
             //status == 'success' ? 'Capture Successful!' : 'Capture Failed',
             'Capture Successful!',
              style: TextStyle(
                fontSize: 22, fontWeight: FontWeight.bold,
                color: status == 'success' ? primaryYellow : Colors.red,
              ),
            ),
            const SizedBox(height: 4),
            Text(
              mode.replaceAll('_', ' '),
              style: const TextStyle(color: Colors.grey, fontSize: 14),
            ),
            const SizedBox(height: 20),

            // Captured image
            ClipRRect(
              borderRadius: BorderRadius.circular(12),
              child: Image.memory(
                capturedImage, height: 200, width: double.infinity, fit: BoxFit.cover,
              ),
            ),
            const SizedBox(height: 20),

            // Overall quality bar
            _SectionCard(
              title: 'Quality Score',
              child: Row(
                children: [
                  Expanded(
                    child: ClipRRect(
                      borderRadius: BorderRadius.circular(10),
                      child: LinearProgressIndicator(
                        value: qualityScore / 100,
                        minHeight: 12,
                        backgroundColor: Colors.grey[200],
                        valueColor: AlwaysStoppedAnimation<Color>(
                          qualityScore >= 70 ? primaryYellow : Colors.orange,
                        ),
                      ),
                    ),
                  ),
                  const SizedBox(width: 12),
                  Text(
                    '$qualityScore%',
                    style: TextStyle(
                      fontWeight: FontWeight.bold,
                      color: qualityScore >= 70 ? primaryYellow : Colors.orange,
                      fontSize: 16,
                    ),
                  ),
                ],
              ),
            ),
            const SizedBox(height: 16),

            // Quality details
            _SectionCard(
              title: 'Quality Details',
              child: Column(
                children: [
                  _QualityMetricRow(label: 'Sharpness (Blur)',  value: blurScore,       icon: Icons.blur_on),
                  const SizedBox(height: 8),
                  _QualityMetricRow(label: 'Brightness', value: brightnessScore, icon: Icons.brightness_6),
                  const SizedBox(height: 8),
                  _QualityMetricRow(label: 'Centering',  value: centeringScore,  icon: Icons.center_focus_strong),
                ],
              ),
            ),
            const SizedBox(height: 16),

            // Info rows
            _SectionCard(
              title: '',
              child: Column(
                children: [
                  _InfoRow(
                    icon: Icons.tag, label: 'Transaction ID',
                    value: '${txnId.substring(0, 8)}…',
                  ),
                  const Divider(height: 16),
                  _InfoRow(
                    icon: Icons.fingerprint, label: 'Fingers Captured',
                    value: '${fingers.length}',
                  ),
                  const Divider(height: 16),
                  _InfoRow(
                    icon: Icons.security,
                    label: 'Liveness Check',
                    value: 'Passed ✓',
                    valueColor: primaryYellow,
                  ),
                ],
              ),
            ),
            const SizedBox(height: 16),

            // Finger details
            _SectionCard(
              title: 'Finger Details',
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  const Text(
                    'Finger Details',
                    style: TextStyle(
                      fontWeight: FontWeight.bold,
                      fontSize: 14,
                    ),
                  ),
                  const SizedBox(height: 12),
                  ...fingers.map((f) => Padding(
                    padding: const EdgeInsets.only(bottom: 8),
                    child: Row(
                      children: [
                        const Icon(
                          Icons.check_circle,
                          color: primaryYellow,
                          size: 18,
                        ),
                        const SizedBox(width: 8),
                        Text(
                          (f['fingerId'] as String)
                              .replaceAll('_', ' '),
                          style: const TextStyle(
                            fontSize: 13,
                          ),
                        ),
                        const Spacer(),
                        Container(
                          padding: const EdgeInsets.symmetric(
                            horizontal: 8, vertical: 2,
                          ),
                          decoration: BoxDecoration(
                            color: Colors.green[50],
                            borderRadius: BorderRadius.circular(10),
                          ),
                          child: Text(
                            'Q: ${f['qualityScore']}',
                            style: const TextStyle(
                              color: primaryYellow,
                              fontSize: 12,
                              fontWeight: FontWeight.bold,
                            ),
                          ),
                        ),
                      ],
                    ),
                  )),
                ],
              ),
            ),
            const SizedBox(height: 24),

            // Actions
            Row(
              children: [
                Expanded(
                  child: OutlinedButton.icon(
                    onPressed: () => Navigator.pop(context),
                    icon: const Icon(Icons.refresh, color: primaryYellow),
                    label: const Text('Capture Again',
                        style: TextStyle(color: primaryYellow)),
                    style: OutlinedButton.styleFrom(
                      side: const BorderSide(color: primaryYellow),
                      padding: const EdgeInsets.symmetric(vertical: 12),
                      shape: RoundedRectangleBorder(
                          borderRadius: BorderRadius.circular(10)),
                    ),
                  ),
                ),
                const SizedBox(width: 12),
                Expanded(
                  child: ElevatedButton.icon(
                    onPressed: () =>
                        Navigator.popUntil(context, (route) => route.isFirst),
                    icon: const Icon(Icons.home, color: Colors.white),
                    label: const Text('Done',
                        style: TextStyle(color: Colors.white)),
                    style: ElevatedButton.styleFrom(
                      backgroundColor: primaryYellow,
                      padding: const EdgeInsets.symmetric(vertical: 12),
                      shape: RoundedRectangleBorder(
                          borderRadius: BorderRadius.circular(10)),
                    ),
                  ),
                ),
              ],
            ),
            const SizedBox(height: 20),
          ],
        ),
      ),
    );
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// Shared UI components
// ─────────────────────────────────────────────────────────────────────────────

class _SectionCard extends StatelessWidget {
  final String title;
  final Widget child;
  const _SectionCard({required this.title, required this.child});

  @override
  Widget build(BuildContext context) {
    return Container(
      width: double.infinity,
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        color: Colors.grey[50],
        borderRadius: BorderRadius.circular(12),
        border: Border.all(color: Colors.grey[200]!),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          if (title.isNotEmpty) ...[
            Text(title,
                style: const TextStyle(fontWeight: FontWeight.bold, fontSize: 14)),
            const SizedBox(height: 12),
          ],
          child,
        ],
      ),
    );
  }
}

class _QualityMetricRow extends StatelessWidget {
  final String label;
  final int value;
  final IconData icon;
  const _QualityMetricRow({required this.label, required this.value, required this.icon});

  @override
  Widget build(BuildContext context) {
    const primaryYellow = Color(0xFFFFC107);
    return Row(
      children: [
        Icon(icon, size: 18, color: Colors.grey),
        const SizedBox(width: 8),
        Text(label, style: const TextStyle(color: Colors.grey, fontSize: 13)),
        const Spacer(),
        Container(
          padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 2),
          decoration: BoxDecoration(
            color: primaryYellow.withOpacity(0.1),
            borderRadius: BorderRadius.circular(10),
          ),
          child: Text(
            '$value%',
            style: TextStyle(
              fontWeight: FontWeight.bold, fontSize: 13,
              color: value >= 70 ? primaryYellow : Colors.orange,
            ),
          ),
        ),
      ],
    );
  }
}

class _InfoRow extends StatelessWidget {
  final IconData icon;
  final String label;
  final String value;
  final Color? valueColor;
  const _InfoRow({
    required this.icon, required this.label, required this.value, this.valueColor,
  });

  @override
  Widget build(BuildContext context) {
    return Row(
      children: [
        Icon(icon, size: 18, color: Colors.grey),
        const SizedBox(width: 8),
        Text(label, style: const TextStyle(color: Colors.grey, fontSize: 13)),
        const Spacer(),
        Text(
          value,
          style: TextStyle(
            fontWeight: FontWeight.bold, fontSize: 13,
            color: valueColor ?? Colors.black,
          ),
        ),
      ],
    );
  }
}