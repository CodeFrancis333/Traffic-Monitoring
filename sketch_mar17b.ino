#include <Arduino.h>
#include "esp_camera.h"
#include "FS.h"
#include "SD_MMC.h"    
#include "SPI.h"

// Camera configuration ESP32-CAM board (AI Thinker variant)
camera_config_t config = {
  .pin_pwdn       = 32,
  .pin_reset      = -1, // Not used
  .pin_xclk       = 0,
  .pin_sscb_sda   = 26,
  .pin_sscb_scl   = 27,
  .pin_d7         = 35,
  .pin_d6         = 34,
  .pin_d5         = 39,
  .pin_d4         = 36,
  .pin_d3         = 21,
  .pin_d2         = 19,
  .pin_d1         = 18,
  .pin_d0         = 5,
  .pin_vsync      = 25,
  .pin_href       = 23,
  .pin_pclk       = 22,
  .xclk_freq_hz   = 20000000,
  .ledc_timer     = LEDC_TIMER_0,
  .ledc_channel   = LEDC_CHANNEL_0,
  .pixel_format   = PIXFORMAT_JPEG,  
  .frame_size     = FRAMESIZE_QQVGA, 
  .jpeg_quality   = 12,              
  .fb_count       = 1                
};

uint32_t imageCounter = 0; // 

bool mountSDCard() {
  const int maxAttempts = 3;
  int attempts = 0;
  while (attempts < maxAttempts) {
    Serial.printf("Attempting to mount SD card (attempt %d)...\n", attempts + 1);
    if (SD_MMC.begin("/sdcard", false)) {
      Serial.println("SD card mounted successfully!");
      return true;
    } else {
      Serial.println("SD_MMC Mount Failed, retrying...");
      attempts++;
      delay(2000);
    }
  }
  Serial.println("SD_MMC Mount Failed after retries");
  return false;
}

void createDatasetFolder() {
  if (!SD_MMC.exists("/dataset")) {
    if (SD_MMC.mkdir("/dataset")) {
      Serial.println("Dataset folder created.");
    } else {
      Serial.println("Failed to create dataset folder.");
    }
  } else {
    Serial.println("Dataset folder already exists.");
  }
}

void listDir(fs::FS &fs, const char * dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\n", dirname);
  File root = fs.open(dirname);
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }
  
  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels) {
        listDir(fs, file.name(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000); 
  Serial.println("Starting dataset collection setup...");

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return;
  }

  Serial.println("Camera initialized successfully!");
  if (!mountSDCard()) {
    return; 
  }

  File testFile = SD_MMC.open("/test.txt", FILE_WRITE);
  if (testFile) {
    testFile.println("This is a test file.");
    testFile.close();
    Serial.println("Test file written successfully!");
  } else {
    Serial.println("Failed to open test file for writing");
  }
  
  listDir(SD_MMC, "/", 0);
  
  createDatasetFolder();
  
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);
  delay(500);
  digitalWrite(4, LOW);
}

void loop() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    delay(2000);
    return;
  }
  
  Serial.printf("Captured image with size: %d bytes\n", fb->len);
  
  char filename[30];
  sprintf(filename, "/dataset/img_%06lu.jpg", imageCounter++);
  
  if (SD_MMC.exists(filename)) {
    SD_MMC.remove(filename);
    delay(100);
  }
  
  File file = SD_MMC.open(filename, FILE_WRITE);
  if (!file) {
    Serial.printf("Failed to open file %s in writing mode\n", filename);
  } else {
    file.write(fb->buf, fb->len); 
    Serial.printf("Dataset image saved: %s\n", filename);
    file.close();
    
    digitalWrite(4, HIGH);
    delay(100); // LED 
    digitalWrite(4, LOW);
  }
  
  esp_camera_fb_return(fb);
  
  delay(5000);
}
