/*
  ESP32-CAM Wi-Fi robot controller
  Board: AI Thinker ESP32-CAM pin map (camera module must match this pinout)

  L298N:
    GPIO12 -> IN1
    GPIO13 -> IN2
    GPIO14 -> IN3
    GPIO15 -> IN4
    ENA and ENB jumpers installed
    Motor battery -> L298N motor supply
    ESP32-CAM GND and L298N GND MUST be connected together

  Web page is stored separately in webpage.h.
  Uno obstacle warning is independent and does not stop the motors.
*/

#include "esp_camera.h"
#include "img_converters.h"
#include <WiFi.h>
#include <WebServer.h>
#include "esp_http_server.h"
#include "webpage.h"

// Change these to your Wi-Fi network.
const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// L298N direction pins
#define IN1 12
#define IN2 13
#define IN3 14
#define IN4 15

// AI Thinker ESP32-CAM camera pin mapping
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

WebServer server(80);
httpd_handle_t stream_httpd = NULL;

const unsigned long MOTOR_FAILSAFE_MS = 1500;
unsigned long lastMoveTime = 0;
bool motorsRunning = false;

static const char* STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=frame";
static const char* STREAM_BOUNDARY = "\r\n--frame\r\n";
static const char* STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  motorsRunning = false;
}

void setDirection(const String& direction) {
  // Stop first to avoid briefly carrying over the previous direction.
  stopMotors();

  if (direction == "F") {          // Forward
    digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
    digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  } else if (direction == "B") {   // Backward
    digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
    digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  } else if (direction == "L") {   // Pivot left
    digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
    digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  } else if (direction == "R") {   // Pivot right
    digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  } else {
    return;
  }

  motorsRunning = true;
  lastMoveTime = millis();
}

void handleRoot() {
  server.send_P(200, "text/html", webpage);
}

void handleMove() {
  if (!server.hasArg("dir")) {
    server.send(400, "text/plain", "Missing dir parameter");
    return;
  }

  String direction = server.arg("dir");
  direction.toUpperCase();

  if (direction == "S") {
    stopMotors();
    server.send(200, "text/plain", "STOP");
    return;
  }

  if (direction != "F" && direction != "B" &&
      direction != "L" && direction != "R") {
    server.send(400, "text/plain", "Invalid direction");
    return;
  }

  setDirection(direction);
  server.send(200, "text/plain", "OK");
}

bool captureJpeg(uint8_t** jpgBuf, size_t* jpgLen) {
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) return false;

  bool converted = false;

  if (fb->format == PIXFORMAT_JPEG) {
    *jpgBuf = (uint8_t*)malloc(fb->len);
    if (*jpgBuf) {
      memcpy(*jpgBuf, fb->buf, fb->len);
      *jpgLen = fb->len;
      converted = true;
    }
  } else {
    converted = frame2jpg(fb, 80, jpgBuf, jpgLen);
  }

  esp_camera_fb_return(fb);
  return converted;
}

void handleCapture() {
  uint8_t* jpgBuf = NULL;
  size_t jpgLen = 0;

  if (!captureJpeg(&jpgBuf, &jpgLen)) {
    server.send(500, "text/plain", "Camera capture/conversion failed");
    return;
  }

  server.sendHeader("Content-Disposition", "attachment; filename=robot_photo.jpg");
  server.sendHeader("Cache-Control", "no-store");
  server.send_P(200, "image/jpeg", (const char*)jpgBuf, jpgLen);
  free(jpgBuf);
}

static esp_err_t streamHandler(httpd_req_t* req) {
  esp_err_t res = httpd_resp_set_type(req, STREAM_CONTENT_TYPE);
  if (res != ESP_OK) return res;

  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

  while (true) {
    uint8_t* jpgBuf = NULL;
    size_t jpgLen = 0;

    if (!captureJpeg(&jpgBuf, &jpgLen)) {
      res = ESP_FAIL;
      break;
    }

    char partHeader[64];
    size_t headerLen = snprintf(partHeader, sizeof(partHeader), STREAM_PART, (unsigned int)jpgLen);

    res = httpd_resp_send_chunk(req, STREAM_BOUNDARY, strlen(STREAM_BOUNDARY));
    if (res == ESP_OK) res = httpd_resp_send_chunk(req, partHeader, headerLen);
    if (res == ESP_OK) res = httpd_resp_send_chunk(req, (const char*)jpgBuf, jpgLen);

    free(jpgBuf);

    if (res != ESP_OK) break;
    vTaskDelay(pdMS_TO_TICKS(60));
  }

  httpd_resp_send_chunk(req, NULL, 0);
  return res;
}

void startStreamServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 81;
  config.ctrl_port = 32769;

  httpd_uri_t streamUri = {};
  streamUri.uri = "/stream";
  streamUri.method = HTTP_GET;
  streamUri.handler = streamHandler;
  streamUri.user_ctx = NULL;

  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &streamUri);
    Serial.println("Camera stream server started on port 81");
  } else {
    Serial.println("ERROR: Could not start stream server");
  }
}

bool startCamera() {
  camera_config_t config = {};
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;

  // RGB565 is used here because the supplied camera module was being
  // handled through software JPEG conversion in the earlier setup.
  config.pixel_format = PIXFORMAT_RGB565;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 12;
  config.fb_count = 1;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x\n", err);
    return false;
  }
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\nStarting ESP32-CAM robot...");

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  stopMotors();

  if (!startCamera()) {
    Serial.println("Camera initialization failed. Check board/pin mapping.");
    while (true) delay(1000);
  }
  Serial.println("Camera initialized successfully");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");

  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 25000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nWi-Fi connection failed. Check SSID/password.");
    while (true) delay(1000);
  }

  Serial.println("\nWi-Fi connected");
  Serial.print("Robot webpage: http://");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, handleRoot);
  server.on("/move", HTTP_GET, handleMove);
  server.on("/capture", HTTP_GET, handleCapture);
  server.begin();

  startStreamServer();
  Serial.println("Web server started on port 80");
  Serial.print("Stream URL: http://");
  Serial.print(WiFi.localIP());
  Serial.println(":81/stream");
}

void loop() {
  server.handleClient();

  // Safety timeout: stop motors if browser stops sending movement commands.
  if (motorsRunning && millis() - lastMoveTime > MOTOR_FAILSAFE_MS) {
    stopMotors();
  }

  delay(2);
}
