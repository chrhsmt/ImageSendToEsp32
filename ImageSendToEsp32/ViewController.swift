//
//  ViewController.swift
//  ImageSendToEsp32
//
//  Created by chihiro hashimoto on 2017/04/17.
//  Copyright © 2017年 chihiro hashimoto. All rights reserved.
//

import UIKit
import AFNetworking

class ViewController: UIViewController {

    override func viewDidLoad() {
        super.viewDidLoad()
        // Do any additional setup after loading the view, typically from a nib.
    }

    @IBAction func sendToEsp32(_ sender: UIButton) {
        NSLog("sendToEsp32")
        let url = "http://192.168.1.101"
        let request: NSMutableURLRequest = AFHTTPRequestSerializer().multipartFormRequest(withMethod: "POST", urlString: url, parameters: nil, constructingBodyWith: { (data: AFMultipartFormData) in
                let imageData:Data = UIImagePNGRepresentation(#imageLiteral(resourceName: "Image"))!
                data.appendPart(withFileData: imageData, name: "file", fileName: "file.png", mimeType: "image/png")
            }, error: nil)
        let manager:AFHTTPSessionManager = AFHTTPSessionManager(sessionConfiguration: URLSessionConfiguration.default)
        let task:URLSessionUploadTask = manager.uploadTask(withStreamedRequest: request as URLRequest, progress: { (progress: Progress) in
            NSLog("Progress: %d", progress.fractionCompleted)
        }) { (response: URLResponse, id: Any?, error: Error?) in
            if ((error) != nil) {
                NSLog(error!.localizedDescription)
            }
        }
        task.resume()
    }

    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }


}

