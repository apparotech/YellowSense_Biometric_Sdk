import UIKit
import Flutter

@main
@objc class AppDelegate: FlutterAppDelegate {
    
    // Camera manager instance
    var cameraManager = CameraManager()
    
    override func application(
        _ application: UIApplication,
        didFinishLaunchingWithOptions launchOptions: [UIApplication.LaunchOptionsKey: Any]?
    ) -> Bool {
        
        let controller = window?.rootViewController as! FlutterViewController
        
        // Create MethodChannel
        let channel = FlutterMethodChannel(
            name: "biometric_sdk",
            binaryMessenger: controller.binaryMessenger
        )
        
        // Handle Flutter calls
        channel.setMethodCallHandler { (call, result) in
            
            switch call.method {
                
            case "processFrame":
                print("iOS: processFrame called")
                
                // Start camera session
                self.cameraManager.startSession()
                
                // Send temporary response to Flutter
                result([
                    "fingerDetected": false,
                    "guidance": "Camera started",
                    "qualityScore": 0,
                    "fingerCount": 0,
                    "readyToCapture": false
                ])
                
            case "startCapture":
                print("iOS: startCapture called")
                
                result([
                    "transactionId": "ios-test",
                    "overallStatus": "success",
                    "results": []
                ])
                
            default:
                result(FlutterMethodNotImplemented)
            }
        }
        
        // Register Flutter plugins
        GeneratedPluginRegistrant.register(with: self)
        
        return super.application(application, didFinishLaunchingWithOptions: launchOptions)
    }
}