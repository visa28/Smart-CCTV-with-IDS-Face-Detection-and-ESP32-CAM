#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"
#include <fd_forward.h>
#include <mtmn.h>



#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22



const char* ssid = "M Wi-Fi";
const char* password = "Mikes254";

// Initialize Telegram BOT
String token = "6419867233:AAERhulfDFJcMxjisvdPpICREV1-OQdRnXk";  // your Bot Token (Get from Botfather)

// Use @myidbot to find out the chat ID of an individual or a group
// Also note that you need to click "start" on a bot before it can
// message you
String chat_id = "5716129963";

typedef struct {
  int type;
  int min_face;
  float pyramid;
  int pyramid_times;
} &mtmn_config_t;

//mtmn_config_t mtmn_config;


void loop() {
  camera_fb_t * fb = esp_camera_fb_get();  
  if(!fb) {
    Serial.println("Camera capture failed");
    delay(1000);
    ESP.restart();
  }  

  // Konfigurasi deteksi wajah
  mtmn_config.type = FAST;
  mtmn_config.min_face = 80;
  mtmn_config.pyramid = 0.707;
  mtmn_config.pyramid_times = 4;

  int numFacesDetected = detectFaces(fb);

  if (numFacesDetected > 0) {
    // Jika wajah terdeteksi, kirim pesan ke Telegram
    String message = "Detected " + String(numFacesDetected) + " face(s)";
    
    WiFiClientSecure client;
    if (!client.connect("api.telegram.org", 443)) {
      Serial.println("Connection failed");
      return;
    }

    String request = "POST /bot" + token + "/sendMessage?chat_id=" + chat_id +
                     "&text=" + message +
                     " HTTP/1.1\r\n" +
                     "Host: api.telegram.org\r\n" +
                     "Content-Type: application/x-www-form-urlencoded\r\n\r\n";

    client.print(request);

    String response;
    while (client.connected()) {
      String line = client.readStringUntil('\r');
      response += line;
    }

    Serial.println("Face detection result sent to Telegram");
  } else {
    Serial.println("No faces detected");
  }

  esp_camera_fb_return(fb);

  delay(1000);  
}

int detectFaces(camera_fb_t *fb) {
  dl_matrix3du_t *image_matrix = dl_matrix3du_alloc(1, fb->width, fb->height, 3);

  fmt2rgb888(fb->buf, fb->len, fb->format, image_matrix->item);

  box_array_t *net_boxes = face_detect(image_matrix, &mtmn_config);

  int num_faces = net_boxes->len;

  dl_matrix3du_free(image_matrix);
  free(net_boxes->box);
  free(net_boxes->landmark);
  free(net_boxes);

  return num_faces;
} 

String alerts2Telegram(String token, String chat_id) 
{
  const char* myDomain = "api.telegram.org";
  String getAll="", getBody = "";

  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();  
  if(!fb) 
{
    Serial.println("Camera capture failed");
    delay(1000);
    ESP.restart();
    return "Camera capture failed";
  }  
  

WiFiClientSecure client_tcp;
  
  if (client_tcp.connect(myDomain, 443)) 
{
    Serial.println("Connected to " + String(myDomain));
    
    String head = "--India\r\nContent-Disposition: form-data; name=\"chat_id\"; \r\n\r\n" + chat_id + "\r\n--India\r\nContent-Disposition: form-data; name=\"photo\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--India--\r\n";

    uint16_t imageLen = fb->len;
    uint16_t extraLen = head.length() + tail.length();
    uint16_t totalLen = imageLen + extraLen;
  
    client_tcp.println("POST /bot"+token+"/sendPhoto HTTP/1.1");
    client_tcp.println("Host: " + String(myDomain));
    client_tcp.println("Content-Length: " + String(totalLen));
    client_tcp.println("Content-Type: multipart/form-data; boundary=India");
    client_tcp.println();
    client_tcp.print(head);
  
    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;


    for (size_t n=0;n<fbLen;n=n+1024)
 {

      if (n+1024<fbLen) 
{
        client_tcp.write(fbBuf, 1024);
        fbBuf += 1024;
      }
      else if (fbLen%1024>0) 
{
        size_t remainder = fbLen%1024;
        client_tcp.write(fbBuf, remainder);
      }
    }  
    
    client_tcp.print(tail);
    
    esp_camera_fb_return(fb);
    
    int waitTime = 10000;   // timeout 10 seconds
    long startTime = millis();
    boolean state = false;
    
    while ((startTime + waitTime) > millis())
    {
      Serial.print(".");
      delay(100);      
      while (client_tcp.available()) 
      {
          char c = client_tcp.read();
          if (c == '\n') 
          {
            if (getAll.length()==0) state=true; 
            getAll = "";
          } 
          else if (c != '\r')
            getAll += String(c);
          if (state==true) getBody += String(c);
          startTime = millis();
       }
       if (getBody.length()>0) break;
    }
    client_tcp.stop();
    Serial.println(getBody);
  }
  else {
    getBody = "Connection to telegram failed.";
    Serial.println("Connection to telegram failed.");
  }
  
  return getBody;
}