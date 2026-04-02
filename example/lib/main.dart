import 'dart:convert';
import 'dart:async';
import 'dart:typed_data';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:camera/camera.dart';
import 'package:permission_handler/permission_handler.dart';
import 'package:biometric_sdk/biometric_sdk.dart';

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

// ── Home Screen ───────────────────────────────────────────────
class HomeScreen extends StatefulWidget {
  const HomeScreen({super.key});

  @override
  State<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends State<HomeScreen> {
  String _result = '';
  bool _loading = false;

  Future<void> _startCapture(String mode) async {
    final status = await Permission.camera.request();
    if (!status.isGranted) {
      setState(() => _result = 'Camera permission denied!');
      return;
    }
    if (!mounted) return;
    final captureResult = await Navigator.push(
      context,
      MaterialPageRoute(
        builder: (_) => CameraScreen(mode: mode),
      ),
    );
    if (captureResult != null) {
      setState(() => _result = captureResult.toString());
    }
  }

  @override
  Widget build(BuildContext context) {
    // 🔥 COLOR SYSTEM (TOP PE ADD KARO)
    const primaryYellow = Color(0xFFFFC107);
    const lightYellow = Color(0xFFFFF8E1);
    const darkYellow = Color(0xFFFFA000);
    return Scaffold(
      appBar: AppBar(
        backgroundColor: const Color(0xFFFFC107),
        title: const Text(
          'YellowSense Biometric SDK',
          style: TextStyle(
            color: Colors.white,
            fontWeight: FontWeight.bold,
          ),
        ),
        centerTitle: true,
      ),
      body: SingleChildScrollView(
        padding: const EdgeInsets.all(20),
        child: Column(
          children: [

            const SizedBox(height: 10),

            // Result Box
            if (_result.isNotEmpty)
              Container(
                width: double.infinity,
                padding: const EdgeInsets.all(16),
                margin: const EdgeInsets.only(bottom: 20),
                decoration: BoxDecoration(
                  color: lightYellow,
                  border: Border.all(color: Color(0xFFFFC107)),
                  borderRadius: BorderRadius.circular(12),
                ),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    const Row(
                      children: [
                        Icon(Icons.check_circle,
                            color: Color(0xFFFFC107), size: 20),
                        SizedBox(width: 8),
                        Text(
                          'Capture Result',
                          style: TextStyle(
                            fontWeight: FontWeight.bold,
                            color: Color(0xFFFFC107)
                          ),
                        ),
                      ],
                    ),
                    const SizedBox(height: 8),
                    Text(_result),
                  ],
                ),
              ),

            // Hand Diagram Card
            HandSelectionCard(
              onCapture: _startCapture,
            ),
          ],
        ),
      ),
    );
  }
}

// ── Hand Selection Card ───────────────────────────────────────
class HandSelectionCard extends StatelessWidget {
  final Function(String) onCapture;

  const HandSelectionCard({
    super.key,
    required this.onCapture,
  });

  @override
  Widget build(BuildContext context) {
    return Column(
      children: [

        // Title
        const Text(
          'Select Capture Mode',
          style: TextStyle(
            fontSize: 18,
            fontWeight: FontWeight.bold,
            color: Color((0xFFFFC107)),
          ),
        ),

        const SizedBox(height: 6),

        const Text(
          'Select the fingers you want to capture',
          style: TextStyle(
            color: Colors.grey,
            fontSize: 13,
          ),
        ),

        const SizedBox(height: 20),

        // Hand Diagram Row
        Row(
          mainAxisAlignment: MainAxisAlignment.spaceEvenly,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            // Left Hand
            _HandDiagram(
              label: 'Left Hand',
              isLeft: true,
              onCapture: onCapture,
            ),
            // Right Hand
            _HandDiagram(
              label: 'Right Hand',
              isLeft: false,
              onCapture: onCapture,
            ),
          ],
        ),

        const SizedBox(height: 24),
        const Divider(),
        const SizedBox(height: 16),

        // Quick Action Buttons
        const Text(
          'Quick Capture',
          style: TextStyle(
            fontSize: 16,
            fontWeight: FontWeight.bold,
          ),
        ),

        const SizedBox(height: 12),

        Row(
          children: [
            Expanded(
              child: _QuickButton(
                label: 'Left\nThumb',
                icon: Icons.thumb_up,
                mode: 'LEFT_THUMB',
                onTap: onCapture,
              ),
            ),
            const SizedBox(width: 10),
            Expanded(
              child: _QuickButton(
                label: 'Right\nThumb',
                icon: Icons.thumb_up,
                mode: 'RIGHT_THUMB',
                onTap: onCapture,
              ),
            ),
            const SizedBox(width: 10),
            Expanded(
              child: _QuickButton(
                label: 'Single\nFinger',
                icon: Icons.touch_app,
                mode: 'SINGLE_FINGER',
                onTap: onCapture,
              ),
            ),
          ],
        ),
      ],
    );
  }
}

// ── Hand Diagram ──────────────────────────────────────────────
class _HandDiagram extends StatelessWidget {
  final String label;
  final bool isLeft;
  final Function(String) onCapture;

  const _HandDiagram({
    required this.label,
    required this.isLeft,
    required this.onCapture,
  });

  @override
  Widget build(BuildContext context) {
    // 🔥 COLOR SYSTEM (TOP PE ADD KARO)
const primaryYellow = Color(0xFFFFC107);
const lightYellow = Color(0xFFFFF8E1);
const darkYellow = Color(0xFFFFA000);
    final mode = isLeft ? 'LEFT_FOUR' : 'RIGHT_FOUR';
    final fingers = isLeft
        ? ['L4', 'L3', 'L2', 'L1']
        : ['R1', 'R2', 'R3', 'R4'];

    return GestureDetector(
      onTap: () => onCapture(mode),
      child: Column(
        children: [
          Text(
            label,
            style: const TextStyle(
              fontWeight: FontWeight.bold,
              fontSize: 14,
            ),
          ),
          const SizedBox(height: 8),

          // Hand Icon
          Container(
            width: 130,
            height: 160,
            decoration: BoxDecoration(
              color: Colors.green[50],
              borderRadius: BorderRadius.circular(12),
              border: Border.all(
                color: primaryYellow,
                width: 1.5,
              ),
            ),
            child: Column(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                Icon(
                  Icons.back_hand,
                  size: 80,
                  color: const Color(0xFF1B5E20),
                ),
                const SizedBox(height: 8),
                const Text(
                  'Tap to Capture',
                  style: TextStyle(
                    fontSize: 11,
                    color: primaryYellow,
                  ),
                ),
              ],
            ),
          ),

          const SizedBox(height: 8),

          // Finger Labels
          Row(
            mainAxisSize: MainAxisSize.min,
            children: fingers.map((f) => Container(
              margin: const EdgeInsets.symmetric(horizontal: 2),
              padding: const EdgeInsets.symmetric(
                horizontal: 6, vertical: 4,
              ),
              decoration: BoxDecoration(
                color: primaryYellow,
                borderRadius: BorderRadius.circular(6),
              ),
              child: Text(
                f,
                style: const TextStyle(
                  color: Colors.white,
                  fontSize: 11,
                  fontWeight: FontWeight.bold,
                ),
              ),
            )).toList(),
          ),

          const SizedBox(height: 8),

          // Capture Button
          ElevatedButton(
            onPressed: () => onCapture(mode),
            style: ElevatedButton.styleFrom(
              backgroundColor: primaryYellow,
              shape: RoundedRectangleBorder(
                borderRadius: BorderRadius.circular(8),
              ),
              padding: const EdgeInsets.symmetric(
                horizontal: 16, vertical: 8,
              ),
            ),
            child: const Text(
              'Capture All',
              style: TextStyle(
                color: Colors.white,
                fontSize: 12,
              ),
            ),
          ),
        ],
      ),
    );
  }
}

// ── Quick Button ──────────────────────────────────────────────
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
    // 🔥 COLOR SYSTEM (TOP PE ADD KARO)
const primaryYellow = Color(0xFFFFC107);
const lightYellow = Color(0xFFFFF8E1);
const darkYellow = Color(0xFFFFA000);
    return GestureDetector(
      onTap: () => onTap(mode),
      child: Container(
        padding: const EdgeInsets.symmetric(
          vertical: 12, horizontal: 8,
        ),
        decoration: BoxDecoration(
          color: Colors.green[50],
          borderRadius: BorderRadius.circular(12),
          border: Border.all(
            color: primaryYellow,
          ),
        ),
        child: Column(
          children: [
            Icon(icon,
                color: primaryYellow, size: 28),
            const SizedBox(height: 6),
            Text(
              label,
              textAlign: TextAlign.center,
              style: const TextStyle(
                fontSize: 11,
                fontWeight: FontWeight.bold,
                color: primaryYellow,
              ),
            ),
          ],
        ),
      ),
    );
  }
}

// ── Camera Screen ─────────────────────────────────────────────
class CameraScreen extends StatefulWidget {
  final String mode;
  const CameraScreen({super.key, required this.mode});

  @override
  State<CameraScreen> createState() => _CameraScreenState();
}

class _CameraScreenState extends State<CameraScreen> {

  CameraController? _controller;
  bool _initialized = false;
  String _guidance = 'Camera starting...';
  bool _fingerDetected = false;
  int _qualityScore = 0;
  int _fingerCount = 0;
  bool _capturing = false;
  bool _isProcessingFrame = false;
  String _processedBase64 = '';
  String _detectedFingers = '';
  List<dynamic> _fingerBoxes = [];

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
        setState(() => _guidance = 'Camera not found!');
        return;
      }

      _controller = CameraController(
        cameras.first,
        ResolutionPreset.medium,
        enableAudio: false,
      );

      await _controller!.initialize();

      if (mounted) {
        setState(() {
          _initialized = true;
          _guidance = 'Place your finger in front of camera';
        });
        _startTimer();
      }
    } catch (e) {
      setState(() => _guidance = 'Camera error: $e');
    }
  }

 void _startTimer() {
    _timer = Timer.periodic(
      const Duration(milliseconds: 1500), // 1.5 seconds thoda fast hai, par lock ke saath theek hai
      (timer) async {
        // 🔥 Agar capture ho raha hai, ya pehla frame process ho raha hai, toh skip karo!
        if (_capturing || !_initialized || _isProcessingFrame) return; 
        await _processFrame();
      },
    );
  }

  Future<void> _processFrame() async {
    _isProcessingFrame = true;
    try {
      final image = await _controller!.takePicture();
      final bytes = await image.readAsBytes();

      final result = await _channel.invokeMethod('processFrame', {
        'frame': bytes,
      });

      if (!mounted) return;

      setState(() {
        _fingerDetected = result['fingerDetected'] as bool? ?? false;
        _guidance       = result['guidance'] as String? ?? '';
        _qualityScore   = result['qualityScore'] as int? ?? 0;
        _fingerCount    = result['fingerCount'] as int? ?? 0;
        _processedBase64 = result['processedImage'] as String? ?? '';
        _detectedFingers = result['detectedFingersName'] as String? ?? '';
        _fingerBoxes = result['fingerBoxes'] as List<dynamic>? ?? [];
      });

      final ready = result['readyToCapture'] as bool? ?? false;
      if (ready && !_capturing) {
        _timer?.cancel();
        await _capture(bytes);
      }
    } catch (e) {
      print('Frame error: $e');
    }finally {
      _isProcessingFrame = false; // 🔥 FRAME PROCESSING KHATAM - LOCK KHOL DO
    }
  }

  

  Future<void> _capture([Uint8List? existingBytes]) async {
    if (_capturing) return;
    setState(() {
      _capturing = true;
      _guidance  = 'Processing...';
    });
    _timer?.cancel();

    try {
      Uint8List bytes;
      if (existingBytes != null) {
        bytes = existingBytes;
      } else {
        final image = await _controller!.takePicture();
        bytes = await image.readAsBytes();
      }

      final result = await _channel.invokeMethod('startCapture', {
        'captureMode': widget.mode,
        'image': bytes,
      });

      if (mounted) {
        // ← Yeh change karo — Result Screen pe jaao
        Navigator.pushReplacement(
          context,
          MaterialPageRoute(
            builder: (_) => ResultScreen(
              result: result,
              qualityScore: _qualityScore,
              capturedImage: bytes,
              mode: widget.mode,
            ),
          ),
        );
      }
    } catch (e) {
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
    const lightYellow = Color(0xFFFFF8E1);
    const darkYellow = Color(0xFFFFA000);
    return Scaffold(
      backgroundColor: Colors.black,
      appBar: AppBar(
        backgroundColor:Color(0xFFFFC107),
        title: Text(
          widget.mode.replaceAll('_', ' '),
          style: const TextStyle(
            color: Colors.white,
            fontWeight: FontWeight.bold,
          ),
        ),
        iconTheme: const IconThemeData(color: Colors.white),
        centerTitle: true,
      ),
      body: Column(
        children: [
          // Camera Preview
          Expanded(
            child: _initialized
                // 🔥 NAYA FIX: LayoutBuilder ab Stack ke UPAR hai 🔥
                ? LayoutBuilder(
                    builder: (context, constraints) {
                      // Screen ki width aur height yahan pehle hi nikal li
                      final stackWidth = constraints.maxWidth;
                      final stackHeight = constraints.maxHeight;

                      return Stack(
                        fit: StackFit.expand,
                        children: [
                          // 1. Camera
                          CameraPreview(_controller!),

                          // 2. Central Static Box Overlay
                          Center(
                            child: Container(
                              width: 250,
                              height: 300,
                              decoration: BoxDecoration(
                                border: Border.all(
                                  color: _fingerDetected ? primaryYellow : Colors.white24,
                                  width: 1.5,
                                ),
                                borderRadius: BorderRadius.circular(12),
                              ),
                            ),
                          ),
                           
                          // 3. 🚀 GREEN BOXES LAYER 🚀
                          ..._fingerBoxes.map((box) {
                            try {
                              final data = box as Map<dynamic, dynamic>;
                              final label = data['label'] as String;
                              
                              // Safe Parsing
                              double parseCoord(dynamic val) => (val is num) ? val.toDouble() : 0.0;
                              
                              final x1 = parseCoord(data['x1']);
                              final y1 = parseCoord(data['y1']);
                              final x2 = parseCoord(data['x2']);
                              final y2 = parseCoord(data['y2']);

                              final left = x1 * stackWidth;
                              final top = y1 * stackHeight;
                              final width = (x2 - x1) * stackWidth;
                              final height = (y2 - y1) * stackHeight;

                              if (width <= 0 || height <= 0 || left >= stackWidth || top >= stackHeight) {
                                return const SizedBox.shrink();
                              }

                              // 🔥 AB POSITIONED DIRECTLY STACK KE ANDAR HAI 🔥
                              return Positioned(
                                left: left,
                                top: top,
                                width: width,
                                height: height,
                                child: Container(
                                  decoration: BoxDecoration(
                                    border: Border.all(
                                      color: Colors.greenAccent, 
                                      width: 2.5,
                                    ),
                                    borderRadius: BorderRadius.circular(6),
                                  ),
                                  child: Align(
                                    alignment: Alignment.topCenter,
                                    child: Container(
                                      margin: const EdgeInsets.only(top: 2),
                                      padding: const EdgeInsets.symmetric(horizontal: 4, vertical: 2),
                                      decoration: BoxDecoration(
                                        color: Colors.greenAccent,
                                        borderRadius: BorderRadius.circular(4),
                                      ),
                                      child: Text(
                                        label.replaceAll('_', ' '),
                                        style: const TextStyle(
                                          color: Colors.black,
                                          fontSize: 9,
                                          fontWeight: FontWeight.bold,
                                        ),
                                      ),
                                    ),
                                  ),
                                ),
                              );
                            } catch (e) {
                              return const SizedBox.shrink(); 
                            }
                          }).toList(),
                          // ──────────────────────────

                          // 4. Quality Badge
                          Positioned(
                            top: 16,
                            left: 16,
                            child: Container(
                              padding: const EdgeInsets.symmetric(
                                horizontal: 12, vertical: 6,
                              ),
                              decoration: BoxDecoration(
                                color: _qualityScore >= 70
                                    ? primaryYellow
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

                          // 5. Finger Count Badge
                          if (_fingerDetected)
                            Positioned(
                              top: 16,
                              right: 16,
                              child: Container(
                                padding: const EdgeInsets.symmetric(
                                  horizontal: 12, vertical: 6,
                                ),
                                decoration: BoxDecoration(
                                  color: primaryYellow,
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
                      );
                    },
                  )
                : const Center(
                    child: Column(
                      mainAxisAlignment: MainAxisAlignment.center,
                      children: [
                        CircularProgressIndicator(
                          color: Colors.white,
                        ),
                        SizedBox(height: 16),
                        Text(
                          'Initializing camera...',
                          style: TextStyle(color: Colors.white),
                        ),
                      ],
                    ),
                  ),
          ),

          // Bottom Panel
          Container(
            color: const Color(0xFFFFC107),
            padding: const EdgeInsets.all(16),
            child: Column(
              children: [
                // 🚀 🔥 NAYA CODE: YAHAN PROCESSED IMAGE DIKHEGI 🔥 🚀
                if (_processedBase64.isNotEmpty)
                  Container(
                    margin: const EdgeInsets.only(bottom: 12),
                    height: 120, // Chhoti Preview Window
                    width: 90,
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
                        gaplessPlayback: true, // Camera frame par blink na ho
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
                      _guidance.contains('Spoof') 
                          ? Icons.warning_amber_rounded // Fraud ke liye Warning icon
                          : (_fingerDetected ? Icons.check_circle : Icons.info_outline),
                      color: _guidance.contains('Spoof') ? Colors.redAccent : Colors.white,
                      size: 20,
                    ),
                    const SizedBox(width: 8),
                    Expanded(
                      child: Text(
                        _guidance,
                        textAlign: TextAlign.center,
                        style: TextStyle(
                          color: _guidance.contains('Spoof') ? Colors.redAccent : Colors.white,
                          fontSize: 15,
                          fontWeight: FontWeight.bold, // Text ko bold rakha hai
                        ),
                      ),
                    ),
                  ],
                ),

             if (_detectedFingers.isNotEmpty) ...[
                  const SizedBox(height: 8),
                  Text(
                    'Detecting: $_detectedFingers',
                    style: TextStyle(
                      // Agar Spoof hai toh isko bhi Red kar do
                      color: _guidance.contains('Spoof') ? Colors.redAccent.withOpacity(0.8) : Colors.white70, 
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

// ── Result Screen ─────────────────────────────────────────────
class ResultScreen extends StatelessWidget {
  final Map result;
  final int qualityScore;
  final Uint8List capturedImage;
  final String mode;

  const ResultScreen({
    super.key,
    required this.result,
    required this.qualityScore,
    required this.capturedImage,
    required this.mode,
  });

  @override
  Widget build(BuildContext context) {
    final fingers = result['results'] as List;
    final status  = result['overallStatus'] as String;
    final txnId   = result['transactionId'] as String;

    const primaryYellow = Color(0xFFFFC107);
    const lightYellow = Color(0xFFFFF8E1);
    const darkYellow = Color(0xFFFFA000);
    
  

    return Scaffold(
      backgroundColor: Colors.white,
      appBar: AppBar(
        backgroundColor: const Color(0xFFFFC107),
        title: const Text(
          'Capture Result',
          style: TextStyle(
            color: Colors.white,
            fontWeight: FontWeight.bold,
          ),
        ),
        centerTitle: true,
        automaticallyImplyLeading: false,
      ),
      body: SingleChildScrollView(
        padding: const EdgeInsets.all(20),
        child: Column(
          children: [

            // ── Success Icon ──────────────────────────────
            Container(
              width: 80,
              height: 80,
              decoration: BoxDecoration(
                color: Colors.green[50],
                shape: BoxShape.circle,
                border: Border.all(
                  color: Colors.green,
                  width: 2,
                ),
              ),
              child: const Icon(
                Icons.check_circle,
                color:primaryYellow,
                size: 50,
              ),
            ),

            const SizedBox(height: 12),

            Text(
              status == 'success'
                  ? 'Capture Successful!'
                  : 'Capture Failed',
              style: TextStyle(
                fontSize: 22,
                fontWeight: FontWeight.bold,
                color: status == 'success'
                    ? Color(0xFFFFC107)
                    : Colors.red,
              ),
            ),

            const SizedBox(height: 4),

            Text(
              mode.replaceAll('_', ' '),
              style: const TextStyle(
                color: Colors.grey,
                fontSize: 14,
              ),
            ),

            const SizedBox(height: 20),

            // ── Captured Image ────────────────────────────
            ClipRRect(
              borderRadius: BorderRadius.circular(12),
              child: Image.memory(
                capturedImage,
                height: 200,
                width: double.infinity,
                fit: BoxFit.cover,
              ),
            ),

            const SizedBox(height: 20),

            // ── Quality Score ─────────────────────────────
            Container(
              padding: const EdgeInsets.all(16),
              decoration: BoxDecoration(
                color: Colors.grey[50],
                borderRadius: BorderRadius.circular(12),
                border: Border.all(color: Colors.grey[200]!),
              ),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  const Text(
                    'Quality Score',
                    style: TextStyle(
                      fontWeight: FontWeight.bold,
                      fontSize: 14,
                    ),
                  ),
                  const SizedBox(height: 8),
                  Row(
                    children: [
                      Expanded(
                        child: ClipRRect(
                          borderRadius: BorderRadius.circular(10),
                          child: LinearProgressIndicator(
                            value: qualityScore / 100,
                            minHeight: 12,
                            backgroundColor: Colors.grey[200],
                            valueColor: AlwaysStoppedAnimation<Color>(
                              qualityScore >= 70
                                  ? Colors.yellow
                                  : Colors.orange,
                            ),
                          ),
                        ),
                      ),
                      const SizedBox(width: 12),
                      Text(
                        '$qualityScore%',
                        style: TextStyle(
                          fontWeight: FontWeight.bold,
                          color: qualityScore >= 70
                              ? primaryYellow
                              : Colors.orange,
                          fontSize: 16,
                        ),
                      ),
                    ],
                  ),
                ],
              ),
            ),

            const SizedBox(height: 16),

            // ── Transaction Info ──────────────────────────
            Container(
              padding: const EdgeInsets.all(16),
              decoration: BoxDecoration(
                color: Colors.grey[50],
                borderRadius: BorderRadius.circular(12),
                border: Border.all(color: Colors.grey[200]!),
              ),
              child: Column(
                children: [
                  _InfoRow(
                    icon: Icons.tag,
                    label: 'Transaction ID',
                    value: txnId.substring(0, 8) + '...',
                  ),
                  const Divider(height: 16),
                  _InfoRow(
                    icon: Icons.fingerprint,
                    label: 'Fingers Captured',
                    value: '${fingers.length}',
                  ),
                  const Divider(height: 16),
              _InfoRow(
    icon: Icons.security,
    label: 'Liveness Check',
    value: (fingers.isNotEmpty && 
            fingers[0]['livenessPassed'] == true)
        ? 'Passed ✓'
        : 'Failed ✗',
    valueColor: (fingers.isNotEmpty && 
                 fingers[0]['livenessPassed'] == true)
        ? Colors.green
        : Colors.red,
),
                ],
              ),
            ),

            const SizedBox(height: 16),

            // ── Finger Results ────────────────────────────
            Container(
              padding: const EdgeInsets.all(16),
              decoration: BoxDecoration(
                color: Colors.grey[50],
                borderRadius: BorderRadius.circular(12),
                border: Border.all(color: Colors.grey[200]!),
              ),
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
                  // ...fingers.map((f) => Padding(
                  //   padding: const EdgeInsets.only(bottom: 8),
                  //   child: Row(
                  //     children: [
                  //       const Icon(
                  //         Icons.check_circle,
                  //         color: primaryYellow,
                  //         size: 18,
                  //       ),
                  //       const SizedBox(width: 8),
                  //       Text(
                  //         (f['fingerId'] as String)
                  //             .replaceAll('_', ' '),
                  //         style: const TextStyle(
                  //           fontSize: 13,
                  //         ),
                  //       ),
                        
                  //       const Spacer(),
                  //       // 🔥 NAYA BADGE: ISO Template Ready 🔥
                  //       if ((f['template'] as String).isNotEmpty)
                  //         Container(
                  //           margin: const EdgeInsets.only(right: 8),
                  //           padding: const EdgeInsets.symmetric(
                  //             horizontal: 6, vertical: 2,
                  //           ),
                  //           decoration: BoxDecoration(
                  //             color: Colors.blue[50],
                  //             borderRadius: BorderRadius.circular(6),
                  //             border: Border.all(color: Colors.blue.withOpacity(0.3)),
                  //           ),
                  //           child: const Text(
                  //             'ISO Ready ✓',
                  //             style: TextStyle(
                  //               color: Colors.blue,
                  //               fontSize: 10,
                  //               fontWeight: FontWeight.bold,
                  //             ),
                  //           ),
                  //         ),

                  //       // Purana Quality Score Badge
                  //       Container(
                  //         padding: const EdgeInsets.symmetric(
                  //           horizontal: 8, vertical: 2,
                  //         ),
                  //         decoration: BoxDecoration(
                  //           color: Colors.green[50],
                  //           borderRadius: BorderRadius.circular(10),
                  //         ),
                  //         child: Text(
                  //           'Q: ${f['qualityScore']}',
                  //           style: const TextStyle(
                  //             color: primaryYellow,
                  //             fontSize: 12,
                  //             fontWeight: FontWeight.bold,
                  //           ),
                  //         ),
                  //       ),
                  //     ],
                  //   ),
                  // )
                  // ),
                    ...fingers.map((f) => Padding(
                    padding: const EdgeInsets.only(bottom: 12), // Thodi spacing badha di
                    child: Column( // 🔥 ROW KO COLUMN MEIN DAAL DIYA 🔥
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        Row(
                          children: [
                            const Icon(
                              Icons.check_circle,
                              color: primaryYellow,
                              size: 18,
                            ),
                            const SizedBox(width: 8),
                            Text(
                              (f['fingerId'] as String).replaceAll('_', ' '),
                              style: const TextStyle(
                                fontSize: 13,
                                fontWeight: FontWeight.bold,
                              ),
                            ),
                            const Spacer(),
                            
                            // 🔥 ISO Ready Badge 🔥
                            if ((f['template'] as String).isNotEmpty)
                              Container(
                                margin: const EdgeInsets.only(right: 8),
                                padding: const EdgeInsets.symmetric(
                                  horizontal: 6, vertical: 2,
                                ),
                                decoration: BoxDecoration(
                                  color: Colors.blue[50],
                                  borderRadius: BorderRadius.circular(6),
                                  border: Border.all(color: Colors.blue.withOpacity(0.3)),
                                ),
                                child: const Text(
                                  'ISO Ready ✓',
                                  style: TextStyle(
                                    color: Colors.blue,
                                    fontSize: 10,
                                    fontWeight: FontWeight.bold,
                                  ),
                                ),
                              ),

                            // Quality Score
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
                        
                        // 🚀 YAHAN TEMPLATE KI VALUE PRINT HOGI 🚀
                        if ((f['template'] as String).isNotEmpty)
                          Padding(
                            padding: const EdgeInsets.only(left: 26, top: 4), // Icon ke theek neeche align karne ke liye
                            child: Text(
                              // Sirf shuru ke 30 characters dikha rahe hain taaki UI na bigde
                              'Template Data: ${(f['template'] as String).length > 30 ? (f['template'] as String).substring(0, 30) + '...' : f['template']}',
                              style: const TextStyle(
                                color: Colors.grey,
                                fontSize: 10,
                                fontFamily: 'monospace', // Code jaisa font
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

            // ── Buttons ───────────────────────────────────
            Row(
              children: [
                // Capture Again
                Expanded(
                  child: OutlinedButton.icon(
                    onPressed: () => Navigator.pop(context),
                    icon: const Icon(
                      Icons.refresh,
                      color:primaryYellow
                    ),
                    label: const Text(
                      'Capture Again',
                      style: TextStyle(
                        color: primaryYellow
                      ),
                    ),
                    style: OutlinedButton.styleFrom(
                      side: const BorderSide(
                        color: primaryYellow
                      ),
                      padding: const EdgeInsets.symmetric(
                        vertical: 12,
                      ),
                      shape: RoundedRectangleBorder(
                        borderRadius: BorderRadius.circular(10),
                      ),
                    ),
                  ),
                ),

                const SizedBox(width: 12),

                // Done
                Expanded(
                  child: ElevatedButton.icon(
                    onPressed: () => Navigator.popUntil(
                      context,
                      (route) => route.isFirst,
                    ),
                    icon: const Icon(
                      Icons.home,
                      color: Colors.white,
                    ),
                    label: const Text(
                      'Done',
                      style: TextStyle(color: Colors.white),
                    ),
                    style: ElevatedButton.styleFrom(
                      backgroundColor: primaryYellow,
                      padding: const EdgeInsets.symmetric(
                        vertical: 12,
                      ),
                      shape: RoundedRectangleBorder(
                        borderRadius: BorderRadius.circular(10),
                      ),
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

// ── Info Row ──────────────────────────────────────────────────
class _InfoRow extends StatelessWidget {
  final IconData icon;
  final String label;
  final String value;
  final Color? valueColor;

  const _InfoRow({
    required this.icon,
    required this.label,
    required this.value,
    this.valueColor,
  });

  @override
  Widget build(BuildContext context) {
    return Row(
      children: [
        Icon(icon, size: 18, color: Colors.grey),
        const SizedBox(width: 8),
        Text(
          label,
          style: const TextStyle(
            color: Colors.grey,
            fontSize: 13,
          ),
        ),
        const Spacer(),
        Text(
          value,
          style: TextStyle(
            fontWeight: FontWeight.bold,
            fontSize: 13,
            color: valueColor ?? Colors.black,
          ),
        ),
      ],
    );
  }
}
