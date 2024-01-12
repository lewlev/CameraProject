// if the project fails to compile you may need to "include arduino_secrets.h" with #include line here
// the include is needed if the project is editied using the local PC IDE editor
// if the project is created and edited entirely with arduino cloud web editor it is not required


// Software Version information
#define VerMaj 3
#define VerMin 14 // 5 JAn 2024



// Overview
// The concept is to add a basic security camera to  previous IOT project an alarm controller
// the camera is to be triggered by the Alarm Controlller wirelessly via ArduinoCloud using a shared IOT variable.
//
// The camera will be limited to 30 jpg's per minute or 48 seconds of avi per minute
//  say 4 12 seconds or 2 24 second etc
// Avi frame rate is 5 per second
// The camera is controlled and monitored on a mobile phone using an ArduinoIOTCloud with a  dashboard
// additionaly the dashboard can configure the camera  to either capture avi or jpg image files.
// Images captured are to be uploaded to Google drive.

// Acknowledgments
// I am grateful to Paul Ibbotson and ChungYi Fu for code that is used in this project.
// I used code from  Paul Ibbotson's excellent 'Video Camera' project pretty much as is to generate
// avi files from the camera.

// description of Paul's project
######################################################################################################
// Program:  Video_Camera
// Summary:  Motion triggered video camera that saves AVI files to SD card.
// Author:   Paul Ibbotson (paul.w.ibbotson@gmail.com)
// Versions: 0.90 01-Jan-22 Initial version
//           0.91 15-Jan-22 Add WiFi access to get time for file timestamps (optional)
//
// Future:   Play around with the camera settings to get best picture quality.
//           Create custom PCB to hold ESP32-CAM, PIR sensor, & 3.3v power supply.
//
//            need to have eps-idf installed see
//           https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html#installing-using-boards-manager
//           also need to format up sd card to fat32 4Gig(some suggest its OK to go bigger) and add a folder named videos in the root dir
// Overview:
//  Based on ESP32-CAM AI Thinker board.
//  Uses both cores of the ESP32.
//   Core 0 is used to: check the motion sensor to trigger capturing frames and storing them in memory.
//   Core 1 is used to: retrieve frames from memory and create the AVI file on the SD card.
//  The on board flash LED is (stupidly) connected to GPIO4.  This pin is used by the SD card so will flash when accessing - recommend disconnecting the driving transistor.
//  PIR sensor is connected to GPIO3.  This is the RX for the serial connection.  The sensor should be disconnected when uploding the sketch to avoid issues.
//  I decided to go with S-VGA (800x600) resolution rather the XUGA (1600x1200) to be able to handle more fps, and ceate smaller files on the SD card.
//
//  It is useful to understand the AVI file format - Below is a summary (based on my understanding).
//  There are a number of fields in the file that need to be updated once sizes are known - these are highlighted below.
//
// +-- RIFF, sz, AVI --------+ sz = total file size - 8 bytes (updated at EOF)
// | +-- LIST, 68, hdrl ---+ |
// | | +--- avih, 56 --+   | |
// | | |               |   | |
// | | + --------------+   | |
// | +---------------------+ |
// |                         |
// | +-- LIST, 144, strl --+ |
// | | +-- strh, 48 --+    | |
// | | |              |    | | dwlength = fileFramesWritten (updated at EOF)
// | | +--------------+    | |
// | | +-- strf, 40 --+    | |
// | | |              |    | |
// | | +--------------+    | |
// | | +-- INFO, 28 --+    | |
// | | |              |    | |
// | | +--------------+    | |
// | +---------------------+ |
// |                         |
// | +-- LIST, sz, movi ---+ | sz = fileFramesTotalSize + (8 * fileFramesWritten) + sum of padding (updated at EOF)
// | | +-- 00dc, sz --+    | | sz = frameBuffer[]->len + padding.
// | | | frame 1      |    | |
// | | |              |    | |
// | | | padding      |    | | Each data chunk (00dc) must finish on a 2 byte boundary, so 1 extra byte might be required.
// | | +--------------+    | |
// | | . . .               | |
// | | +-- 00dc, sz --+    | |
// | | | frame x      |    | |
// | | |              |    | |
// | | | padding      |    | |
// | | +--------------+    | |
// | +---------------------+ |
// |                         |
// | +-- idx1, sz ---------+ | sz = fileFramesWritten * 16 (this entire section gets written at EOF)
// | | +-- frame 1 --+     | |
// | | | offset      |     | | offset points to 00dc, and is relative to start of movi. First frame offset 0x00.
// | | | sz          |     | | sz = frameBuffer[]->len.  Padding is not included.
// | | |             |     | |
// | | +-------------+     | |
// | | . . .               | |
// | | +-- frame x --+     | |
// | | |             |     | |
// | | +-------------+     | |
// | +---------------------+ |
// +-------------------------+
//
// Useful references:
//  https://docs.microsoft.com/en-us/windows/win32/directshow/avi-riff-file-reference
//  https://www.klennet.com/notes/2021-09-03-carving-avi-files.aspx
//  https://www.fileformat.info/format/riff/egff.htm
//  http://graphcomp.com/info/specs/ms/editmpeg.htm
//  https://cdn.hackaday.io/files/274271173436768/avi.pdf
//  https://www.opennet.ru/docs/formats/avi.txt
//  https://www.tutorialspoint.com/esp32_for_iot/esp32_for_iot_getting_current_time_using_ntp_client.htm
//
// ###############################################################################################################




// ChungYi Fu's code and google script are used to upload jpg files to google drive.
// Added an option to upload avi's as well.
// Significant changes to allow the upload of larger avi image files. The original code bufferd the
// entire file for upload in ram, this is not possibe the larger avi files.
// So uploading is done in chunks also changed the TCP coms to use block rather than byte by byte which
// improved speed significantly.


// ###############################################################################################################


//  Google App Script for google drive upload
/*  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  function doPost(e) {
  var myFoldername = e.parameter.myFoldername;
  var myFile = e.parameter.myFile;
  // var myFilename = Utilities.formatDate(new Date(), "GMT+8", "yyyyMMddHHmmss")+"_"+e.parameter.myFilename;
  var myFilename = e.parameter.myFilename;
  var contentType = myFile.substring(myFile.indexOf(":")+1, myFile.indexOf(";"));
  var data = myFile.substring(myFile.indexOf(",")+1);
  data = Utilities.base64Decode(data);
  var blob = Utilities.newBlob(data, contentType, myFilename);

  var folder, folders = DriveApp.getFoldersByName(myFoldername);
  if (folders.hasNext()) {
    folder = folders.next();
  } else {
    folder = DriveApp.createFolder(myFoldername);
  }
  var file = folder.createFile(blob);
  file.setDescription("Uploaded by " + myFilename);

  var imageID = file.getUrl().substring(file.getUrl().indexOf("/d/")+3,file.getUrl().indexOf("view")-1);
  var imageUrl = "https://drive.google.com/uc?authuser=0&id="+imageID;

  return  ContentService.createTextOutput(myFoldername+"/"+myFilename+"\n"+imageUrl);
  }
*/
// #################################################################






#include "Base64.h"            // required for Base64 encoding to send to google drive
#include "dirent.h"            // sd dir functions
#include "driver/sdmmc_host.h" // requried for SD functions
#include "esp_vfs_fat.h"       // requried for SD fat functions
#include "thingProperties.h"   // arduino IOT cloud integration
#include <ArduinoMqttClient.h> // may not be required
#include <EEPROM.h>           // Used to store the file number and configs.
#include <ESP32Time.h>        // v2.0.0    time
#include <WiFi.h>             // v2.0.0   local wifi
#include <WiFiClientSecure.h> // v2.0.0   TCP coms
#include <esp_camera.h>       // Camera library
#include <sstream>            // required for some string manipulations


// comment out the line below to turn off debug printing
#define DEBUG

#ifdef DEBUG
#define DPRINT(...) Serial.print(__VA_ARGS__)
#define DPRINTLN(...) Serial.println(__VA_ARGS__)

#else
#define DPRINT(...)   // DPRINT(...)
#define DPRINTLN(...) // DPRINTLN(...)
#endif

#define EEPROM_SIZE 10  // Size of EEPROM alllocated  (holds last used filesequence letter and Last DayofYear currently in use)

TaskHandle_t Core0Task; // freeRTOS task handler for core 0
TaskHandle_t Core1Task; // freeRTOS task handler for core 1



WiFiClient wifiClient;                    // wifiClient instance
MqttClient mqttClient(wifiClient);        // may not be required
sdmmc_card_t *card;                       // SD card


// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// WiFi creds
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

char ssid[] = SECRET_SSID;
char password[] = SECRET_OPTIONAL_PASS;



// function prototypes
void initialiseSDCard();
String Ulencode(String str);
void urlencode(char *, char *);
void initialiseCamera(bool);
void initialiseTime();
bool sendSDImageToGoogleDrive(char *);
void captureFrame();
bool startFile();
bool addToFile();
bool unSent();
void fatalError();
uint16_t framesInBuffer();
void closeFile();
unsigned long PreCalc(char *);
bool SendtoGoogle(char *);
void PrintMillis();
// bool UpdateIOTandWait(bool LO = false);
void onIoTSync();

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// CAMERA_MODEL_AI_THINKER Pins
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

/*
  MicroSD card	ESP32
  CLK	GPIO 14
  CMD	GPIO 15
  DATA0	GPIO 2
  DATA1 / flashlight	GPIO 4
  DATA2	GPIO 12
  DATA3	GPIO 13
*/


#define flashPin 33       // High flash on low off
#define PWDN_GPIO_NUM 32  //
#define RESET_GPIO_NUM -1 //
#define XCLK_GPIO_NUM 0   //
#define SIOD_GPIO_NUM 26  //
#define SIOC_GPIO_NUM 27  //
#define Y9_GPIO_NUM 35    //
#define Y8_GPIO_NUM 34    //
#define Y7_GPIO_NUM 39    //
#define Y6_GPIO_NUM 36    //
#define Y5_GPIO_NUM 21    //
#define Y4_GPIO_NUM 19    //
#define Y3_GPIO_NUM 18    //
#define Y2_GPIO_NUM 5     //
#define VSYNC_GPIO_NUM 25 //
#define HREF_GPIO_NUM 23  //
#define PCLK_GPIO_NUM 22  //

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// Camera AVI and JPG File creation
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

/*  From esp-32-camera.h
   https://github.com/espressif/esp32-camera/blob/master/driver/include/esp_camera.h
   variables len and buf are used in AddtoFile function
   @brief Data structure of camera frame buffer

  //typedef struct {
  //    uint8_t * buf;              //!< Pointer to the pixel data
  //    size_t len;                 //!< Length of the buffer in bytes
  //    size_t width;               //!< Width of the buffer in pixels
  //    size_t height;              //!< Height of the buffer in pixels
  //    pixformat_t format;         //!< Format of the pixel data
  //    struct timeval timestamp;   //!< Timestamp since boot of the first DMA buffer of the frame
  // } camera_fb_t;
*/

bool ImageMode = false;             // if false capture jpg's

FILE *ImgFile; // .avi or .jpg   file

FILE *idx1File; // Temporary file used to hold the index information

WiFiClientSecure client_tcp;

// Used when setting position within a file stream  in  addToFile();
// relates to file position within .avi
enum relative
{
  FROM_START,
  FROM_CURRENT,
  FROM_END
};

boolean fileOpen = false;      // This is set when we have an open AVI file.
// was having some issues with the startfile and CaptureFrame being rentered before the last instance was complete
// due to RTOS scheduling I think. Created these LO flags lock out the associated function until it is valid to run again

byte LOstartFile = 0;           //when true locksout the startFile function
byte LOcaptureFrame = 0;        //when true locksout the captureFrame function
byte LOaddToFile = 0;           //when true locksout the addToFile function
byte LOloop = 0;                //when true skips the code in the main loop
bool UNSENT = false;            // true when image files exist in the /sdcard/videos folder waiting to be sent to google
bool SysIdle = false;           // true when the system is in an idle state

const uint16_t AVI_HEADER_SIZE = 252;     // Size of the AVI file header.
const long unsigned FRAME_INTERVAL = 250; // Time (ms) between frame captures   4 frames per second
const uint8_t JPEG_QUALITY = 10;          // JPEG quality (0-63).
const uint8_t MAX_FRAMES = 10;            //  Maximum number of frames we hold at any time would have been nice to change this to 1 for jpg mode
//  but it proved very difficult to achieve see notes under Known limitations and bugs
camera_fb_t *frameBuffer[MAX_FRAMES]; // This is where we hold references to the captured frames in a circular buffer.

const byte buffer00dc[4] = {0x30, 0x30, 0x64, 0x63}; // "00dc"
const byte buffer0000[4] = {0x00, 0x00, 0x00, 0x00}; // 0x00000000
const byte bufferAVI1[4] = {0x41, 0x56, 0x49, 0x31}; // "AVI1"
const byte bufferidx1[4] = {0x69, 0x64, 0x78, 0x31}; // "idx1"

const byte aviHeader[AVI_HEADER_SIZE] = // This is the AVI file header.  Some of these values get overwritten.
{
  0x52, 0x49, 0x46,
  0x46, // 0x00 "RIFF"
  0x00, 0x00, 0x00,
  0x00, // 0x04           Total file size less 8 bytes [gets updated later]
  0x41, 0x56, 0x49,
  0x20, // 0x08 "AVI "
  0x4C, 0x49, 0x53,
  0x54, // 0x0C "LIST"
  0x44, 0x00, 0x00,
  0x00, // 0x10 68        Structure length
  0x68, 0x64, 0x72,
  0x6C, // 0x04 "hdrl"
  0x61, 0x76, 0x69,
  0x68, // 0x08 "avih"    fcc
  0x38, 0x00, 0x00,
  0x00, // 0x0C 56        Structure length
  0x90, 0xD0, 0x03,
  0x00, // 0x20 250000    dwMicroSecPerFrame     [based on FRAME_INTERVAL]
  0x00, 0x00, 0x00,
  0x00, // 0x24           dwMaxBytesPerSec       [gets updated later]
  0x00, 0x00, 0x00,
  0x00, // 0x28 0         dwPaddingGranularity
  0x10, 0x00, 0x00,
  0x00, // 0x2C 0x10      dwFlags - AVIF_HASINDEX set.
  0x00, 0x00, 0x00,
  0x00, // 0x30           dwTotalFrames          [gets updated later]
  0x00, 0x00, 0x00,
  0x00, // 0x34 0         dwInitialFrames (used for interleaved files only)
  0x01, 0x00, 0x00,
  0x00, // 0x38 1         dwStreams (just video)
  0x00, 0x00, 0x00,
  0x00, // 0x3C 0         dwSuggestedBufferSize
  0x20, 0x03, 0x00,
  0x00, // 0x40 800       dwWidth - 800 (S-VGA)  [based on FRAMESIZE]
  0x58, 0x02, 0x00,
  0x00, // 0x44 600       dwHeight - 600 (S-VGA) [based on FRAMESIZE]
  0x00, 0x00, 0x00,
  0x00, // 0x48           dwReserved
  0x00, 0x00, 0x00,
  0x00, // 0x4C           dwReserved
  0x00, 0x00, 0x00,
  0x00, // 0x50           dwReserved
  0x00, 0x00, 0x00,
  0x00, // 0x54           dwReserved
  0x4C, 0x49, 0x53,
  0x54, // 0x58 "LIST"
  0x84, 0x00, 0x00,
  0x00, // 0x5C 144
  0x73, 0x74, 0x72,
  0x6C, // 0x60 "strl"
  0x73, 0x74, 0x72,
  0x68, // 0x64 "strh"    Stream header
  0x30, 0x00, 0x00,
  0x00, // 0x68  48       Structure length
  0x76, 0x69, 0x64,
  0x73, // 0x6C "vids"    fccType - video stream
  0x4D, 0x4A, 0x50,
  0x47, // 0x70 "MJPG"    fccHandler - Codec
  0x00, 0x00, 0x00,
  0x00,       // 0x74           dwFlags - not set
  0x00, 0x00, // 0x78           wPriority - not set
  0x00, 0x00, // 0x7A           wLanguage - not set
  0x00, 0x00, 0x00,
  0x00, // 0x7C           dwInitialFrames
  0x01, 0x00, 0x00,
  0x00, // 0x80 1         dwScale
  0x04, 0x00, 0x00,
  0x00, // 0x84 4         dwRate (frames per second)         [based on FRAME_INTERVAL]
  0x00, 0x00, 0x00,
  0x00, // 0x88           dwStart
  0x00, 0x00, 0x00,
  0x00, // 0x8C           dwLength (frame count)             [gets updated later]
  0x00, 0x00, 0x00,
  0x00, // 0x90           dwSuggestedBufferSize
  0x00, 0x00, 0x00,
  0x00, // 0x94           dwQuality
  0x00, 0x00, 0x00,
  0x00, // 0x98           dwSampleSize
  0x73, 0x74, 0x72,
  0x66, // 0x9C "strf"    Stream format header
  0x28, 0x00, 0x00,
  0x00, // 0xA0 40        Structure length
  0x28, 0x00, 0x00,
  0x00, // 0xA4 40        BITMAPINFOHEADER length (same as above)
  0x20, 0x03, 0x00,
  0x00, // 0xA8 800       Width                  [based on FRAMESIZE]
  0x58, 0x02, 0x00,
  0x00,       // 0xAC 600       Height                 [based on FRAMESIZE]
  0x01, 0x00, // 0xB0 1         Planes
  0x18, 0x00, // 0xB2 24        Bit count (bit depth once uncompressed)
  0x4D, 0x4A, 0x50,
  0x47, // 0xB4 "MJPG"    Compression
  0x00, 0x00, 0x04,
  0x00, // 0xB8 262144    Size image (approx?)                              [what is this?]
  0x00, 0x00, 0x00,
  0x00, // 0xBC           X pixels per metre
  0x00, 0x00, 0x00,
  0x00, // 0xC0           Y pixels per metre
  0x00, 0x00, 0x00,
  0x00, // 0xC4           Colour indices used
  0x00, 0x00, 0x00,
  0x00, // 0xC8           Colours considered important (0 all important).
  0x49, 0x4E, 0x46,
  0x4F, // 0xCB "INFO"
  0x1C, 0x00, 0x00,
  0x00, // 0xD0 28         Structure length
  0x70, 0x61, 0x75,
  0x6c, // 0xD4
  0x2e, 0x77, 0x2e,
  0x69, // 0xD8
  0x62, 0x62, 0x6f,
  0x74, // 0xDC
  0x73, 0x6f, 0x6e,
  0x40, // 0xE0
  0x67, 0x6d, 0x61,
  0x69, // 0xE4
  0x6c, 0x2e, 0x63,
  0x6f, // 0xE8
  0x6d, 0x00, 0x00,
  0x00, // 0xEC
  0x4C, 0x49, 0x53,
  0x54, // 0xF0 "LIST"
  0x00, 0x00, 0x00,
  0x00, // 0xF4           Total size of frames        [gets updated later]
  0x6D, 0x6F, 0x76,
  0x69 // 0xF8 "movi"
};
// Following the header above are each of the frames.  Each one consists of
//  "00dc"      Stream 0, Uncompressed DIB format.
//   0x00000000 Length of frame
//   Frame      then the rest of the frame data received from the camera (note JFIF in the frame data gets overwritten with AVI1)
//   0x00       we also potentially add a padding byte to ensure the frame chunk is an even number of bytes.
//
// At the end of the file we add an idx1 index section.  Details in closeFile() routine.

uint8_t frameInPos = 0;  // Position within buffer where we write to.
uint8_t frameOutPos = 0; // Position within buffer where we read from.
// The following relate to the AVI file that gets created.
uint16_t fileFramescapturd = 0;   // Number of frames captured by camera.
uint16_t fileFramesWritten = 0;   // Number of frames written to the AVI file.
uint32_t fileFramesTotalSize = 0; // Total size of frames in file.
uint32_t fileStartTime = 0;       // Used to calculate FPS.
uint32_t filePadding = 0;         // Total padding in the file.

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//  Copy to Google Drive, sendSDImageToGoogle() related globals
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

const char *myDomain = "script.google.com"; // Google connection URL

// ##################  Enter Google Script Deployment ID here  ################
// Paste  ID (saved earlier) in the next line after "/macros/s/    and before    /exec";
String myScript = "/macros/s/your_google_deployment_ID/exec";


String myFoldername = "&myFoldername=ESP32-CAM"; // Google drive folder snme
String myImage = "&myFile=";                     //
char imageFileH[24];                             // Header sent to google drive before base64 encoded content


FILE *fp;                       // SD File object pointer for inputfile
unsigned long fileSize = 0;     // size if inputfile on SD to be sent to GD
uint8_t *fileinput;             // pointer to start byte buffer chunk
char *input;                    // pointer to unencoded image data buffer
char *OPbuffer;                 // stores base64URL encoded data ouput for upload to google drive
unsigned long BlockSize = 9000; // chunk buffer size must be multiple of 3
unsigned long IPindex = 0;      // index pointer for input file on SD card
unsigned long IPindexLast = 0;  // previous index pointer
bool LastBlock = false;         // Flag to indicate at the last input file chunk
bool GDUabort = false;          // Flag to indicate set when an alarm comes in, forces Google Upload to abort if true
char *OP = new char[10];        // output buffer for urlencode function
int pad = 0;                    // number of padding bytes to be added to OPbuffer to round length to a multiple of 3 ( 0,1 or 2)

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//  AVI file copy and verify
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

#define capDir "videos"        // folder for avi's yet to be uploaded to google
#define uplDir "upl"           // folder for avi's uploaded to google drive
#define MOUNT_POINT "/sdcard/" // sd file system mount point
#define cbufferSize 100        // file copy and verify buffer size

char sfile[13] {'\0'};       // Source copy file name buffer
char dfile[13] {'\0'};       // Destination copy file name buffer
char cspath[30] = {'\0'};    // Source path text  buffer
char cdpath[30] = {'\0'};    // Destination path test buffer
char *p_cspath = &cspath[0]; // pointer to Source path text  buffer
char *p_cdpath = &cdpath[0]; // pointer to Destination path test buffe
int DoYlast = 366;           // day of year last  new day resets filesequence = 'A';
char filesequence = 'A';     // seqence letter for avi file name
char filesequence2 = 'A';    // seqence letter2 for avi file name

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//  Arduino Cloud
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

bool Variablesynced = false; // flag that indicates ESP thing is lyncled to IoTcloud
bool ConnectionLost = false;
#define blinkInterval 1000                // time in ms between iot updates
unsigned long blinkLastUpdate = millis(); // time in ms last update occured

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// essential tasks timming controls etc
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// the next three unsigned long variables are time intervals in milli seconds that are setable through the ArduinoIOT Dasboard.
// the value of each is calculated based on the three dashboard dB..... variables that are declared in ThingProperties.h

// dBPicturesPerMinute   is the number of jpg stills taken per minute and is contrained t0   1 to 30
// dBVideosPerMinute    number of videos each minute constrained to 1 to 8
// dBVideoLength        maximium length in seconds of each video  constrained to 5 to 48

// additionaly the total length of all avi's in one minute is limited to 48 seconds so
// setting 3 videos per minute at 40 seconds length will result in the camera setting thr length to 16 seconds to keep the
// total length to 3*16 = 48 seconds

unsigned long MaxAviLen;   //                            dbVideolength*1000   limmited to 5000 to 4800   or 5000 to 48/dBVideosPerMinute*1000 which ever is less
unsigned long AviInterval; //                            (60/dBVideosPerMinute)*1000.   ie vidpermin= 5, vidlen =8   AviInterval = 12000ms or 12 seconds
unsigned long JpgInterval; //                            60000 / dBPicturesPerMinute    ie dBPicturesPerMinute=5 JPinterval will be 12000ms or 12 seconds

unsigned long JpgCapTime = millis();   //                capture time for last jpg
unsigned long AviStartTime = millis(); //                start time for avi file

// EEPROM config storage setup
byte EPVideoLength;
byte EPVideosPerMinute;
byte EPPicturesPerMinute;
byte EPImageMode;


// These  variables control the camera, and the actions required each processing loop.
boolean AlarmDetected = false;                // This is set when a Remote  Alarm is detected.  It stays set for Alarm_DELAY ms after Remote Alarm variable is reset
const long unsigned Alarm_INTERVAL = 500;     // Time (ms) between Alarm  checks
const long unsigned Alarm_DELAY = 3000;       // Time (ms) after Alarm last detected that the AlarmDetected variable is held in the true state

// Note Alarm_DELAY is only meaningful if a PIR motion sensor is in use, when the camera trigger is comming
// from an alarm panel it would be reasonable to assume the panel would have its own persistance or hold time
// on the alarm signal. so leave it at a low value.

unsigned long GDLockoutInterval = 5 * 1000 * 60; // time in ms, lockout period after last alarm before Google Drive updates can starts
//                                                  5 minutes without an alarm seems reasonable
unsigned long GDLockoutStart = millis();         // marker time in ms for the start of the lockout period.

bool GDuploadActive = false;                     // flag that indicates GD uploading is active
//                                                  inhibits avi or jpg capture when active
//                                                  however an alarm will cause gdupload to abort and clear the flag

unsigned long lastAlarm = 0;                    // Last time we detected an Alarm
unsigned long BlinkerTime = millis();           // Used with power / run dashboard widget

// ESP32 realtime clock
//#define Toffset 10                            //   GMT time offset in hours winter  time
#define Toffset 11                              //   GMT time offset in hours daylight saving
const long gmtOffset_sec = Toffset * 60 * 60;   //   AEST GMT (+10 hours)
ESP32Time rtc(gmtOffset_sec);                   //    Start real time clock





bool SysUp = false;                             // flag that indicates system boot is finished.
unsigned long Conwatch = millis();              // timmer control for monitoring Arduino cloud connection time outs
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// Setup
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

void setup()
{
  // Initialise serial monitor.
  Serial.begin(115200);
  while (!Serial) {}

  Serial.print("Ver ");
  Serial.print(VerMaj);
  Serial.print(".");
  Serial.println(VerMin);

  // Initialise EEPROM - used to hold file number used in the avi file names.

  EEPROM.begin(EEPROM_SIZE);
  // pinMode(16, INPUT_PULLUP);

  WiFi.mode(WIFI_STA);
  DPRINTLN("");
  DPRINT("Connecting to ");
  DPRINTLN(ssid);
  WiFi.begin(ssid, password);
  interrupts();
  long int StartTime = millis();
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    if ((StartTime + 10000) < millis())
    {
      DPRINT("!!!!WiFi connection failed!!!!!");
      delay(1000);
      ESP.restart();
    }
  }
  delay(500);
  // iot cloud
  DPRINTLN("initProperties();");
  initProperties();
  delay(3500);

  // WiFiClientSecure client_tcp;
  client_tcp.setInsecure(); // run version 1.0.5 or above

  EEPROM.get(4, EPImageMode);    // get the last set image mode 1= avi  0=jpg
  // Initialise the camera.
  initialiseCamera(EPImageMode);   // inti camera in ImageMode
  // Get the current time from the Internet and initialise the ESP32's RTC.
  initialiseTime();

  delay(2000);
  //  Initialise the SD card.
  initialiseSDCard();

  // Connect to Arduino IoT Cloud
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);

  // set up IOT call back events
  ArduinoCloud.addCallback(ArduinoIoTCloudEvent::SYNC, onIoTSync);
  ArduinoCloud.addCallback(ArduinoIoTCloudEvent::CONNECT, OnConnect);
  ArduinoCloud.addCallback(ArduinoIoTCloudEvent::DISCONNECT, OnDisconnect);


  delay(500);

  /*
    The following function allows you to obtain more information
    related to the state of network and IoT Cloud connection and errors
    the higher number the more granular information you’ll get.
    The default is 0 (only errors).
    Maximum is 4
  */
  DPRINTLN("  setDebugMessageLevel(4);");
  setDebugMessageLevel(4);
  DPRINTLN("  ArduinoCloud.printDebugInfo();");
  ArduinoCloud.printDebugInfo();

  RemoteAl = false; // shared alarm signal from main alarm thing

  // wait for IoTSync callback to initialise cloud variables
  int count = 0;
  while (Variablesynced == false)
  {
    ArduinoCloud.update(); // this loop hangs if I dont do this
    delay(400);
    DPRINT("+");
    count++;
    if (count > 50)
    {
      DPRINTLN("Timmed out waiting for IOT connect");
      break;
    }
  }
  if (Variablesynced) dBSaved = true; // Dash board will match their dynamic alter egos at this point

  EEPROM.get(2, DoYlast);                  // get last stored day of the year
  DoYlast = constrain(DoYlast, 0, 365);    // save it to the running variable
  DPRINT("DoYlast ");
  DPRINTLN(DoYlast);

  //  EEPROM.get(4, EPImageMode); // 1 byte
  EEPROM.get(5, EPVideoLength);
  EEPROM.get(6, EPVideosPerMinute);
  EEPROM.get(7, EPPicturesPerMinute);

  // copy the eprom values to the dashboard variables
  if (EPImageMode > 0) dBImageMode = 1;
  dBVideosPerMinute = (int)EPVideosPerMinute;
  dBVideosPerMinute = constrain(dBVideosPerMinute, 1, 8);
  dBVideoLength = (int)EPVideoLength;
  dBPicturesPerMinute = (int)EPPicturesPerMinute;
  dBPicturesPerMinute = constrain(dBPicturesPerMinute, 1, 30);
  int maxlen = 48 / dBVideosPerMinute;
  dBVideoLength = constrain(dBVideoLength, 5, maxlen);

  // update the running system variables to reflect the dashboard values
  MaxAviLen = dBVideoLength * 1000;
  AviInterval = (60 / dBVideosPerMinute) * 1000;
  JpgInterval = (60 / dBPicturesPerMinute) * 1000;
  ImageMode = (dBImageMode > 0);
  dBSaved = true; // as all the dash board and running variables have come from eeprom safe to indicate the
  // the config is saved.
  ArduinoCloud.update(); // force the dashboard to update
  DPRINT("dBImageMode: ");
  DPRINTLN(dBImageMode);
  DPRINT("ImageMode: ");
  DPRINTLN(ImageMode);
  DPRINT("Alarm_DELAY Alarmdetected holdtime: ");
  DPRINTLN(Alarm_DELAY);
  DPRINT("Alarm_INTERVAL beween alarm checks: ");
  DPRINTLN(Alarm_INTERVAL);
  DPRINT("GDLockoutInterval alarm to upload lockout: ");
  DPRINTLN(GDLockoutInterval);
  DPRINTLN("------avi-------------------");
  DPRINT("dBVideoLength dasboard: ");
  DPRINTLN(dBVideoLength);
  DPRINT("MaxAviLen: running system");
  DPRINTLN(MaxAviLen);
  DPRINT("EPVideosPerMinute videos(avi's) per/min in eeprom:  ");
  DPRINTLN(EPVideosPerMinute);
  DPRINT("dBVideosPerMinute: ");
  DPRINTLN(dBVideosPerMinute);
  DPRINT("AviInterval  lock out between writing avi files determind by "
         "dBVideosPerMinute: ");
  DPRINTLN(AviInterval);
  DPRINTLN("------jpg-------------------");
  DPRINT("dBPicturesPerMinute: ");
  DPRINTLN(dBPicturesPerMinute);
  DPRINT("EPPicturesPerMinute: ");
  DPRINTLN(EPPicturesPerMinute);
  DPRINT("JpgInterval      jpgtojpg: ");
  DPRINTLN(JpgInterval);
  DPRINTLN("------end-------------------");
  DPRINTLN();
  DPRINTLN("Waiting for Arduino IOT sync");
  unsigned long loopstart = millis();
  while (!Variablesynced && (millis() < (loopstart + 20000)))
  {
    DPRINT(".");
    delay(200);
  }
  if (millis() > (loopstart + 20000))
  {
    DPRINTLN("Warning not synced with cloud restarting");
    fatalError();
  }
  else
  {
    Serial.println("System ready!");
  }

  // Set up the task handlers on each core.
  xTaskCreatePinnedToCore(codeCore0Task, "Core0Task", 8192, NULL, 5, &Core0Task, 0);
  xTaskCreatePinnedToCore(codeCore1Task, "Core1Task", 8192, NULL, 5, &Core1Task, 1);
  pinMode(flashPin, OUTPUT);
  flashOn(true);
  delay(1000);
  flashOn(false);
  SysUp = true;
}

// ------------------------------------------------------------------------------------------
// loop most functionality in  freeRTOS task handers.
// puting ArduinoCloud.update() in one or both of the core tasks seems to cause problems
// ------------------------------------------------------------------------------------------
void loop()
{
  if (LOloop == 0)
  {

    ArduinoCloud.update();
    if (WiFi.status() != WL_CONNECTED)                  // if wifi lost reconnect
    {
      WiFi.begin(ssid, password);
      delay(200);
    }

    if (millis() > blinkInterval + blinkLastUpdate)    // periodically update Arduino IOT crashes if done on one of the core taskes
    {
      dBSysidle = SysIdle;                             // set dashboard indicator
      dBBlinkPower = !dBBlinkPower;                    // toggle dash board power blinker
      blinkLastUpdate = millis();                      // restart timer
    }
    // incase of loss of connection to Arduino cloud allow 60 seconds to recover if not restart
    if (!ConnectionLost) Conwatch = millis();          // reset timer each loop while connected
    if (SysUp && ((millis() - Conwatch) > 60000))      // off line 60 seconds?
    {
      Serial.println("Connection lost time out");
      fatalError();
    }
  }
}

// turn camera flash LED on or off
void flashOn(bool on)
{
  if (on)
  {
    digitalWrite(flashPin, HIGH);
  }
  else
  {
    digitalWrite(flashPin, LOW);
  }
}

// Signal from main alarm controller has changed
void onRemoteAlChange()
{
  if (LOloop || !SysUp) return;
  DPRINT("RemoteAl: ");
  DPRINTLN(RemoteAl);
  if (RemoteAl)
  {
    GDUabort = true; //                                        signal fn sendSDImageToGoogleDrive() to abort if active and disable restarting until GDUabort=false
  }
  else
  {
    if (millis() > (GDLockoutStart + GDLockoutInterval))
      GDUabort = false; //                                     reenable  fn sendSDImageToGoogleDrive() to upload
  }

  GDLockoutStart = millis(); //                                refresh lock out period when alarm status changes
}

void onDBSysidleChange()
{
}

// sets the image mode to either capture still jpg's or video avi images when selectd from the dash board
//
void onDBImageModeChange()
{
  // if (LOloop) return;
  DPRINT(" onDBImageModeChange   dBImageMode: "); //
  DPRINTLN(dBImageMode);
  if (LOloop) return;
  if (SysIdle && SysUp)                     // make sure the system has fully booted up andin and in idle state to run this
  {
    // shut down all processes on esp32 this allows a clean start after the restart
    // not doing this leads to reboot loops with either the camera, the SD drive, or both  failing to initialise
    dBSaved = false; //                                          update the dashboard to show a change is in progress
    dBSysidle = false;
    DPRINTLN("ArduinoCloud.update()");
    ArduinoCloud.update();
    LOloop = 1;                             // keep blocking updates and dash board changes
    delay(2000);                            // dashboard update delay time
    vTaskSuspend(Core0Task);                // kill both core tasks
    vTaskSuspend(Core1Task);
    // delay(200);
    noInterrupts();                         // dont allow interupts to get in the way
    // delay(200);
    EEPROM.put(4, (byte)dBImageMode);       // write and commit the new value to EEPROM
    EEPROM.commit();
    // delay(1000);
    DPRINTLN("esp_vfs_fat_sdcard_unmount;"); // Unmout the SD drive file system
    esp_err_t ret = esp_vfs_fat_sdcard_unmount("/sdcard", card);
    if (ret == ESP_OK)
    {
      DPRINTLN("SD unmounted restarting");
      // delay(5000);
      ESP.restart();
    }
    else
    {
      DPRINTLN("SD unmount failed");
    }
    // Need to restart to reconfigure the camera for jpg or video mode
    // On restart  setup will get the new setting from the EEPROM
  }
  else
  {
    DPRINTLN(" Cannot change ImageMode, system is not idle (ready). ");
  }
}

void onDBVideosPerMinuteChange()
{
  if (LOloop || !SysUp) return;

  // videos per minute has precedence over length of video, so length is limmted by videos per minute but no vice versa
  // This project sets an arbitary limit of 48 seconds of video per minute this is to keep avi files to a manageable size

  if (dBVideosPerMinute * dBVideoLength > 48) dBVideoLength = 48 / dBVideosPerMinute;
  AviInterval = (60 / dBVideosPerMinute) * 1000; // may need to limmit video length VideosPerMinute * Videolength is <= 48 seconds
  MaxAviLen = dBVideoLength * 1000;              // set the working value to match dashboard
  dBSaved = false;
  ArduinoCloud.update();
}

void onDBVideoLengthChange()
{
  if (LOloop || !SysUp) return;
  // videos per minute has precedence over length of video, so length is limmted by videos per minute but no vice versa
  if (dBVideosPerMinute * dBVideoLength > 48) dBVideoLength = 48 / dBVideosPerMinute;
  MaxAviLen = dBVideoLength * 1000; //  max avi length in ms
  dBSaved = false;
  ArduinoCloud.update();
}

void onDBPicturesPerMinuteChange()
{
  if (LOloop || !SysUp) return;
  JpgInterval = (60 / dBPicturesPerMinute) * 1000;
  dBSaved = false;
  ArduinoCloud.update();
}

void onDBSaveChangesChange()
{
  // if already in process of changing imagemode (LOloop =1) ignore
  if (LOloop || !SysUp) return;

  if (dBSaveChanges == 1)
  {
    // UpdateIOTandWait(true); // run ArduinoCloud updates block further updates
    ArduinoCloud.update();
    LOloop = 1; // keep blocking  ArduinoCloud updates till we are done here
    ImageMode = (dBImageMode > 0);
    EEPROM.put(4, (byte)dBImageMode); // 1 byte
    EEPROM.put(5, (byte)dBVideoLength);
    EEPROM.put(6, (byte)dBVideosPerMinute);
    EEPROM.put(7, (byte)dBPicturesPerMinute);
    EEPROM.commit();
    dBSaved = true;
    LOloop = 0; // reenable updates
    // UpdateIOTandWait(); // run ArduinoCloud update to update dBSaved on dash
    ArduinoCloud.update();
    DPRINTLN("=================");
    DPRINT("ImageMode: ");
    DPRINTLN(ImageMode);
    DPRINT(" MaxAviLen: ");
    DPRINTLN(MaxAviLen);
    DPRINT("AviInterval: ");
    DPRINTLN(AviInterval);
    DPRINT("JpgInterval: ");
    DPRINTLN(JpgInterval);
  }
}


// arduino cloud has connected
void OnConnect()
{
  DPRINTLN("OnConnect()");

  // if in process of changing imagemode (LOloop =1) ignore
  if (LOloop && SysUp)
  {
    DPRINTLN("LOloop && SysUp");
    return;
  }

  DPRINTLN("ReConnected to Arduino Cloud");
  ConnectionLost = false;
}


// arduino cloud connection lost
void OnDisconnect()
{
  DPRINTLN("OnDisconnect()");

  // if in process of changing imagemode (LOloop =1) ignore
  if (LOloop && SysUp)
  {
    DPRINTLN("LOloop && SysUp");
    return;
  }

  ConnectionLost = true;
  if (Variablesynced)
  {
    DPRINTLN("Disconnected from Arduino Cloud");
    Variablesynced = false;
  }
}

// cloud variables are insync with local values
void onIoTSync()
{
  // if in process of changing imagemode (LOloop =1) ignore
  if (LOloop && SysUp) return;
  DPRINTLN("Arduino synced to IoT Cloud"); // set your initial values here
  Variablesynced = true;
}

// ------------------------------------------------------------------------------------------
// Core 0 is used to capture frames, and check the motion sensor.
// ------------------------------------------------------------------------------------------
void codeCore0Task(void *parameter)
{
  unsigned long currentMillis = 0;                          // Current time
  unsigned long lastAlarmCheck = millis() - Alarm_INTERVAL; // Last time we checked the motion sensor
  unsigned long lastPictureTaken = 0;                       // Last time we captured a frame
  bool RemoteAlFlag = false;                                // just for debugging its a latch that allows one offserial print when RemoteAl goes low.

  for (;;)
  {
    // System idle (SysIdle) if true indicates that there is no activity in progres
    // ie No active alarm, No image file open, no files waiting to be sent, No file uploads in progress
    SysIdle = (!(AlarmDetected || fileOpen || UNSENT || GDuploadActive));
    delay(1); //    required for FreeRtoss stability  to stop watchdog time out errors?
    currentMillis = millis();

    // Check for alarm status from main unit every Alarm_INTERVAL ms.
    if (currentMillis - lastAlarmCheck > Alarm_INTERVAL)
    {
      lastAlarmCheck = currentMillis;
      // Check alarm status from main unit.
      if (RemoteAl) //  'Alarm' shared variable from main alarm when live
      {
        if (!AlarmDetected)
        {
          DPRINTLN("Remote Alarm detected.");
          lastAlarm = currentMillis;
          AlarmDetected = true;
          RemoteAlFlag = true;
        }
      }
      else if (lastAlarm == 0) //  must be at startup lastAlarm set to zero in setup
      {
        AlarmDetected = false;    // default to no alarm at system start
      }
      else if (currentMillis > (lastAlarm + Alarm_DELAY)) // local alarm times out after Alarm_DELAY
      {
        if (AlarmDetected) // time this out
        {
          DPRINTLN("Local Alarm cleared.");
          AlarmDetected = false;
        }
      }
      else // if (RemoteAl)
      {
        if (RemoteAlFlag)
        {
          DPRINTLN("Remote Alarm cleared."); // RemoteAlFlag, just allows a debug print when RemoteAlFlag goes true
          RemoteAlFlag = false;              // purely debuging no operational effect.
        }
      }
    }

    // If we need to, capture a frame every FRAME_INTERVAL ms.
    if (ImageMode)
    {
      //   may need to add in the avi length retrictions here some how
      if (fileOpen && ((currentMillis - lastPictureTaken) > FRAME_INTERVAL))
      {
        lastPictureTaken = currentMillis;
        captureFrame();
      }
      //  delay(1);   // required for FreeRtoss stability ?
    }
    else
    {
      if (((currentMillis - JpgCapTime) > JpgInterval) && fileOpen && (framesInBuffer() == 0))
      {
        DPRINTLN("core0 jpg capture frame");
        captureFrame(); // will only capture frame if frame buffer is empty (in jpg mode) and a file is open
      }                   // no need to capture more than one image for jpg
      //  delay(1);    // required for FreeRtoss stability ?
    }
  }
}

// ------------------------------------------------------------------------------------------
// Core 1
// baton 0 no alarm and no files to send
// baton 1 is writing frames captured to the AVI file.
// baton 2 unsent files exist in capture folder, move to upl folder and  upload avi's to google drive
// ------------------------------------------------------------------------------------------

void codeCore1Task(void *parameter)
{
  for (;;)
  {

    delay(1); // required for FreeRtoss stability  to stop watchdog time out errors?
    if (ImageMode)
    { //avi video mode
      if ((AlarmDetected && !fileOpen)) // when alarm controller signals alarm  we open a new file.
      {
        if (millis() > (AviStartTime + AviInterval)) // past Avi lock out period?
        {
          DPRINTLN("startfile ImageMode");
          bool dummy = startFile();

          if (!dummy)
          {
            DPRINTLN("Could not create input AVI file");
            fileOpen = false;
          }
        }
      }
      else if (fileOpen && (framesInBuffer() > 0)) // If there are frames waiting to be processed add these to the file.
      {
        if ((millis() < (AviStartTime + MaxAviLen)))
        {
          if (!addToFile())
          {
            DPRINTLN("Could not add frame to input AVI file");
          }
        }
        else // past max avi length so close the file
        {
          closeFile();
        }

      }
    }
    else // if(ImageMode)   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    {

      if (AlarmDetected && !fileOpen)                 //  when alarm controller signals alarm  we open a new file.                                             open a file if there is an alarm and a file is not open and the inerval
      { //                                                between frames has expired no need to check timmimg again just capture
        if (millis() > (JpgCapTime + JpgInterval))    // past interval between jpg captures        {
        {
          if (!startFile())
          {
            DPRINTLN("Could not create input JPG file");
            fileOpen = false;
          }
        }
      }
      // If there ia a frame waiting to be processed add these to the file.
      else if (fileOpen && (framesInBuffer() > 0)) //                             if there is an open file and an image is in the buffer add to the file.
      {
        DPRINTLN("AlarmDetected && fileOpen && framesInBuffer() > 0");

        addToFile(); //                                                         add image data to open blank jpg

        closeFile(); //                                                          job done close the file
      }

    } // else if(ImageMode)   !!!!
    // Check for Unsent files
    if (!fileOpen && !AlarmDetected)
    {
      if ((millis() > (GDLockoutInterval + GDLockoutStart))) //             lockout starting google drive upload until GDLockout ms las passed since  the last Alarm occerd out
      {
        if (unSent())
        {
          UNSENT = true;
          GDUabort = false;
          strcat(cspath, "/");   //                                      build cspath to file to be uploaded
          strcat(cspath, sfile); //                                      build cdpath where file goes once uploaded
          strcat(cdpath, "/");
          strcat(cdpath, dfile);
          if (sendSDImageToGoogleDrive(cspath))
          {
            DPRINTLN("Google Drive upload success ");
            if (!Copyf()) //                                          copy source file to upl folder
            {
              DPRINTLN("Copy file failed: ");
            }
            else if (!Verifyf()) //                                   verify copy
            {
              DPRINTLN("Verify file failed: "); //                  if verify fails delete the bad copy
              Dfile(cdpath);
            }
            else
            {
              Dfile(cspath); //                                     if verify OK delete the source
            }
          }
          else
          {
            DPRINTLN("Google Drive upload failed: ");
          }
        }
        else
        {
          UNSENT = false;
        } // if (unSent())   // nothing to send
      }     // if ((millis() > (GDLockoutInterval + GDLockoutStart)))
    }         //     else if (!fileOpen && !AlarmDetected)
  }             // for (;;)
}

void ResetPaths()
{
  strcpy(cspath, "/sdcard/videos");
  strcpy(cdpath, "/sdcard/upl");
}

//  Checks the Capture folder for any files
//  the file name of the first found file is coppied
//  to location cfile pointer p_cfile
//  returns true if a file was found

bool unSent()
{
  // DPRINTLN("unSent() ");
  bool Return = false;
  struct dirent *dp;

  ResetPaths();

  DIR *dir;
  // dir = opendir(cspath);
  try
  {
    dir = opendir(cspath);
  }
  catch (...) // will catch all possible errors thrown.
  {
    // throw;       // throw the same error again to be handled somewhere else
    DPRINTLN("Error Unsent could not read SD directory");
    fatalError();
  }
  if (dir == NULL)
  {
    DPRINT("Can't Open Dir: ");
    DPRINTLN(cspath);
    return false;
  }
  //  while ((dp = readdir (dir)) != NULL)

  while (true)
  {
    try
    {
      dp = readdir(dir);
    }
    catch (...) // will catch all possible errors thrown.
    {
      // throw;       // throw the same error again to be handled somewhere else
      DPRINTLN("Error Unsent could not read SD directory");
      fatalError();
    }
    if (dp == NULL)
      break;
    // DPRINTf("[%s]\n", dp->d_name);
    if ((strstr(dp->d_name, ".avi") != NULL) || (strstr(dp->d_name, ".jpg") != NULL))
    {
      strcpy(sfile, dp->d_name);
      strcpy(dfile, dp->d_name);
      //  DPRINT("strcpy(sfile, dp->d_name): ");
      //  DPRINTLN(sfile);
      Return = true;
      break;
    }
  }
  closedir(dir);
  return Return;
}

// ------------------------------------------------------------------------------------------
// Setup the camera.
// ------------------------------------------------------------------------------------------
void initialiseCamera(bool mode)
{

  /*
    You can only inititialise the camera once need to reboot to change config
    with at least ESP.restart() and maybe hardware reset;

    @brief Configuration structure for camera initialization

    typedef enum {
    CAMERA_GRAB_WHEN_EMPTY,         / *!< Fills buffers when they are empty. Less resources but first 'fb_count' frames might be old * /
    CAMERA_GRAB_LATEST              / *!< Except when 1 frame buffer is used, queue will always contain the last 'fb_count' frames * /
    } camera_grab_mode_t;
    esp32-camera/driver/include/esp_camera.h

    Line 42 in 86a4951

         .grab_mode      = CAMERA_GRAB_WHEN_EMPTY

    if (mode)
    {
     config.grab_mode      = CAMERA_GRAB_WHEN_EMPTY;      // Maximum frames (depends on frame_size, jpeg_quality, frameDelay)
    }
    else
    {
    config.grab_mode      = CAMERA_GRAB_LATEST;
    }


    @brief Camera frame buffer location

    typedef enum {
    CAMERA_FB_IN_PSRAM,         / *!< Frame buffer is placed in external PSRAM  * /
    CAMERA_FB_IN_DRAM           / *!< Frame buffer is placed in internal DRAM * /
    } camera_fb_location_t;

  */

#define CAMERA_MODEL_AI_THINKER

  camera_grab_mode_t GrabMode;
  if (mode)
  {
    GrabMode = CAMERA_GRAB_WHEN_EMPTY; // Maximum frames (depends on frame_size, jpeg_quality, frameDelay)
  }
  else
  {
    GrabMode = CAMERA_GRAB_LATEST;
  }

  DPRINTLN("Initialising camera");
  camera_config_t config;
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
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.pixel_format = PIXFORMAT_JPEG; // Image format PIXFORMAT_JPEG - possibly only format supported for OV2640.  PIXFORMAT_GRAYSCALE
  config.xclk_freq_hz = 20000000;
  // The follow fields determine the size of the buffer required.
  config.frame_size = FRAMESIZE_SVGA; //  800 x  600. 40ms capture.
  config.jpeg_quality = JPEG_QUALITY; // 0-63.  Only relevent if PIXFORMAT_JPEG format.  Very low may cause camera to crash at higher frame sizes.
  config.fb_count = MAX_FRAMES;       // Maximum frames (depends on frame_size, jpeg_quality, frameDelay)
  // config.camera_fb_location = CAMERA_FB_IN_PSRAM;
  config.grab_mode = GrabMode; // Maximum frames (depends on frame_size, jpeg_quality, frameDelay)
  esp_err_t cam_err = esp_camera_init(&config);
  DPRINT("config.grab_mode: ");
  DPRINTLN(config.grab_mode);

  if (cam_err == ESP_OK)
  {
    DPRINTLN("Camera ready");
  }
  else
  {

    delay(2000);
    esp_err_t cam_err = esp_camera_init(&config);
    if (cam_err == ESP_OK)
    {
      DPRINTLN("Camera ready");
    }
    else
    {

      DPRINT("Camera initialisation error ");
      DPRINTLN(esp_err_to_name(cam_err));
      fatalError();
    }
  }
  // Some camera settings can be set using the sensor values below.
  sensor_t *s = esp_camera_sensor_get();
  s->set_brightness(s, 1); // -2 to 2
  s->set_contrast(s, 0);   // -2 to 2
  s->set_saturation(s, 0); // -2 to 2
  s->set_special_effect(
    s,
    0);                // Special effect.  0 - None, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia
  s->set_whitebal(s, 1); // 0 = disable , 1 = enable
  s->set_awb_gain(s, 1); // 0 = disable , 1 = enable
  s->set_wb_mode(
    s,
    0);                                  // 0 to 4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
  s->set_exposure_ctrl(s, 1);              // 0 = disable , 1 = enable
  s->set_aec2(s, 1);                       // 0 = disable , 1 = enable
  s->set_ae_level(s, 0);                   // -2 to 2
  s->set_aec_value(s, 300);                // 0 to 1200
  s->set_gain_ctrl(s, 1);                  // 0 = disable , 1 = enable
  s->set_agc_gain(s, 0);                   // 0 to 30
  s->set_gainceiling(s, (gainceiling_t)0); // 0 to 6
  s->set_bpc(s, 0);                        // 0 = disable , 1 = enable
  s->set_wpc(s, 1);                        // 0 = disable , 1 = enable
  s->set_raw_gma(s, 1);                    // 0 = disable , 1 = enable
  s->set_lenc(s, 1);                       // 0 = disable , 1 = enable
  s->set_hmirror(s, 0);                    // Horizontal mirror.  0 = disable , 1 = enable
  s->set_vflip(s, 0);                      // Vertical flip.  0 = disable , 1 = enable
  s->set_dcw(s, 1);                        // 0 = disable , 1 = enable
}
// ------------------------------------------------------------------------------------------
// Routine to capture a single frame and write to memory.
// ------------------------------------------------------------------------------------------
void captureFrame()
{

  DPRINT("fn captureFrame()");

  if (LOcaptureFrame > 0)
    return;
  LOcaptureFrame = 1; // block capture until this iteration complete
  // Only start capturing frames when there is an open AVI file.
  if (!fileOpen)
  {
    LOcaptureFrame = 0;
    return;
  }
  DPRINTLN(" OK");

  if (ImageMode)
  {
    // DPRINTLN("Capturing frames.");
    // If the buffer is already full, skip this frame.
    if (framesInBuffer() == MAX_FRAMES)
    {
      DPRINTLN("Frame buffer full, frame skipped.");
      LOcaptureFrame = 0;
      return;
    }

    // Determine where to write the frame pointer in the buffer.
    frameInPos = fileFramescapturd % MAX_FRAMES;
    // Take a picture and store pointer to the frame in the buffer.
    frameBuffer[frameInPos] = esp_camera_fb_get();
    if (frameBuffer[frameInPos]->buf == NULL)
    {
      DPRINT("Frame capture failed.");
      LOcaptureFrame = 0;

      return;
    }
    // Keep track of the total frames captured and total size of frames (needed to update file header later).
    fileFramescapturd++;
    fileFramesTotalSize += frameBuffer[frameInPos]->len;
  }
  else // if (ImageMode)     so  jpg mode
  {
    if (framesInBuffer() > 0)
    {
      DPRINTLN("1 frame only for jpg, frame skipped.");
      LOcaptureFrame = 0;
      return;
    }
    flashOn(true);
    frameBuffer[0] = esp_camera_fb_get();
    if (frameBuffer[0]->buf == NULL)
    {
      fileFramescapturd = 0;
      frameInPos = 0;
      frameOutPos = 0;
      DPRINT("Frame capture failed.");
      LOcaptureFrame = 0;
      flashOn(false);
      return;
    }
    else
    {

      JpgCapTime = millis();
      flashOn(false);
    }
    // Keep track of the total frames captured and total size of frames (needed to update file header later).
    fileFramescapturd++;

  } // else if (ImageMode)
  LOcaptureFrame = 0;
  return;
}
// ---------------------------------------------------------------------------
// Set up the SD card.
// ---------------------------------------------------------------------------
void initialiseSDCard()
{
  int count = 0;
  while (true)
  {
    sdmmc_host_t host = SDMMC_HOST_DEFAULT(); // default host config macro
    // To use 1-line SD mode, uncomment the following line:
    // host.flags = SDMMC_HOST_FLAG_1BIT;  // commented out 12Jan24 I think this was uncommented some time back and got forgoten about
    // host.max_freq_khz = 10000;    // was commented out sets to 20000 by changed v3_3 21 nov
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT(); // default slot config macro
    // slot_config.width = 4;                                           // set one bit mode
    // ref https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/storage/sdmmc.html
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
      .format_if_mount_failed = false,
      .max_files = 3,
    }; // Convenience function to get FAT filesystem on SD card registered in VFS.


    esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);

    if (ret == ESP_OK)
    {
      DPRINTLN("SD card ready");
      break;
    }
    else
    {
      DPRINT("SD card initialisation error ");
      DPRINTLN(esp_err_to_name(ret));
      if (count < 2)
      {
        DPRINT("Retrying ");
        count++;
      }
      else
      {
        fatalError();
      }
    }
  }
}

// ----------------------------------------------------------------------------------
// Routine to connect to WiFi, get the time from NTP server, and set the ESP's RTC.
// This is only done so the timestamps on the AVI files created are correct. It's not
// critical for the functioning of the video camera.
// ----------------------------------------------------------------------------------
void initialiseTime()
{
  int count = 0;
  const char *ntpServer = "pool.ntp.org"; // NTP server

  const int daylightOffset_sec = 3600; // 1 hour daylight savings

  while (true)
  {
    // Get the time and set the RTC.
    DPRINTLN("Getting time.");
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    // Display the time
    struct tm timeinfo;
    if (getLocalTime(&timeinfo))
    {
      DPRINTLN(&timeinfo, "%A, %B %d %Y %H:%M:%S");
      break;
    }
    else
    {
      DPRINTLN("Failed to obtain time");
      count++;
      delay(5000);
      if (count > 10)
      {
        DPRINTLN("Given up getting time");
        rtc.setTime(30, 24, 15, 1, 1, 3000); // 1 Jan 3000
        return;
      }
    }
  }
  // ESP32Time rtc(gmtOffset_sec );                        // Start real time clock
}

// filesequence filesequence2 are global vaiables used to append a sequence
// number to date bases file names for the image files for any given day the
// seqence numbers can be from AA to ZZ  (676) files per day thats about 1 every 2 minutes over 24 hours
// if CommiT is false just update the global vaiables
// other wise just commit the global variables to EEPROM

bool fileSequInc(bool KommiT)
{
  DPRINTLN("fileSequInc");
  if (KommiT)
  {
    EEPROM.put(0, filesequence);
    EEPROM.put(1, filesequence2);
    EEPROM.commit();
  }
  else
  {
    if (filesequence < 'Z')
    {
      filesequence++;
    }
    else
    {

      if (filesequence2 == 'Z')
        return false;
      filesequence2++;
      filesequence = 'A';
    }
  }
  DPRINTLN("Exit fileSequInc");
  return true;
}

// ------------------------------------------------------------------------------------------
// Routine to create a new AVI file each time motion is detected.
// ------------------------------------------------------------------------------------------
bool startFile()
{
  if (LOstartFile > 0)
    return false;
  LOstartFile = 1;

  DPRINT("DoYlast: ");
  DPRINTLN(DoYlast);
  DPRINT("rtc.getDayofYear(): ");
  DPRINTLN(rtc.getDayofYear());
  if (rtc.getDayofYear() != DoYlast)
  {
    // starting a new fil
    filesequence = 'A';
    filesequence2 = 'A';
    EEPROM.put(0, filesequence);
    EEPROM.put(1, filesequence2);
    DoYlast = rtc.getDayofYear();
    EEPROM.put(2, DoYlast);
    EEPROM.commit();
  }
  else
  {
    EEPROM.get(0, filesequence);
    EEPROM.get(1, filesequence2);
    EEPROM.get(2, DoYlast);
    DoYlast = constrain(DoYlast, 0, 365);
    // Increment the file number, and format the new file name.
    fileSequInc(false); // bump the filesequence file name extention
  }
  DPRINT("File Sequence: ");
  DPRINT(filesequence);
  DPRINT("   File Sequence2: ");
  DPRINTLN(filesequence2);
  DPRINT("DoYlast: ");

  DPRINTLN(DoYlast);
  ResetPaths();

  // char AVIFilename[30] = "/sdcard/videos/";
  // uint16_t fileNumber = 0;
  // filesequence = 'A';
  char padded[12] = "";
  padded[0] = (rtc.getYear() - 2000) / 10 + 0x30;
  padded[1] = (rtc.getYear() - 2000) % 10 + 0x30;
  int mnth = rtc.getMonth(); // note Jan is mnth 0, dec 11
  mnth++;
  if (mnth < 10)
  {
    padded[2] = 0x30;
    padded[3] = mnth + 0x30;
  }
  else
  {
    padded[2] = 0x31;
    padded[3] = mnth - 10 + 0x30;
  }
  int day = rtc.getDay();
  if (day < 10)
  {
    padded[4] = 0x30;
    padded[5] = day + 0x30;
  }
  else
  {
    padded[4] = day / 10 + 0x30;
    padded[5] = day % 10 + 0x30;
  }
  padded[6] = filesequence2;
  padded[7] = filesequence;
  padded[8] = '\0';

  // Reset file statistics.
  fileFramesWritten = 0; // al added this 2/decv3_7  not so sure about it
  fileFramescapturd = 0;
  fileFramesTotalSize = 0;
  filePadding = 0;
  fileStartTime = millis();
  strcat(cspath, "/");
  strcat(cspath, padded);

  // fileOpen = false;
  if (ImageMode)
  { // avi mode
    strcat(cspath, ".avi");
    // Open the AVI file.
    ImgFile = fopen(p_cspath, "wb");
    if (ImgFile == NULL)
    {
      DPRINT("Unable to open AVI file ");
      DPRINTLN(p_cspath);
      fileOpen = false;
      LOstartFile = 0;

      return false;
    }
    else
    {
      DPRINT(p_cspath);
      DPRINTLN(" opened.");
      fileSequInc(true); // commit  filesequence file name extention to eeprom
      //  fileOpen = true;
    }

    // Write the AVI header to the file.
    size_t written = fwrite(aviHeader, 1, AVI_HEADER_SIZE, ImgFile);
    if (written != AVI_HEADER_SIZE)
    {
      DPRINTLN("Unable to write header to AVI file");
      fclose(ImgFile);
      fileOpen = false;
      LOstartFile = 0;
      return false;
    }

    delay(20); // not sure about this may make file opening more reliable
    idx1File = fopen("/sdcard/videos/idx1.tmp", "w+");
    if (idx1File == NULL)
    {
      DPRINTLN("Unable to open idx1 file for read/write");
      fclose(ImgFile);
      fileOpen = false;
      LOstartFile = 0;
      return false;
    }
    else
    {
      DPRINTLN("Opened idx1 file for read/write");
      flashOn(true);
      fileOpen = true;
      AviStartTime = millis();
      LOstartFile = 0;
      return true;
    }
  }
  else // if(ImageMode        jpg Mode
  {

    strcat(cspath, ".jpg");
    ImgFile = fopen(p_cspath, "wb");
    if (ImgFile == NULL)
    {
      DPRINT("Unable to open JPG file ");
      DPRINTLN(cspath);
      fileOpen = false;
      LOstartFile = 0;
      return false;
    }
    else
    {
      // DPRINT(AVIFilename);
      DPRINT(cspath);
      DPRINTLN(" opened.");
      fileSequInc(
        true); // commit  filesequence file name extention to eeprom
      fileOpen = true;
      // return true;
    }

  } // if(ImageMode

  LOstartFile = 0;
  return true;
}

// Routine to add a frame to the AVI file.  Should only be called when framesInBuffer() > 0,
// and there is already a file open.
// ------------------------------------------------------------------------------------------
bool addToFile()
{
  if (LOaddToFile > 0)
    return false;

  LOaddToFile = 1;
  DPRINTLN("addToFile() ");
  int Maxfretry = 3;
  size_t bytesWritten = 0;

  if (ImageMode)
  {
    // For each frame we write a chunk to the AVI file made up of:
    //  "00dc" - chunk header.  Stream ID (00) & type (dc = compressed video)
    //  The size of the chunk (frame size + padding)
    //  The frame from camera frame buffer
    //  Padding (0x00) to ensure an even number of bytes in the chunk.
    //
    // We then update the FOURCC in the frame from JFIF to AVI1
    //
    // We also write to the temporary idx file.  This keeps track of the offset & size of each frame.
    // This is read back later (before we close the AVI file) to update the idx1 chunk.

    // size_t bytesWritten;
    //  Determine the position to read from in the buffer.
    frameOutPos = fileFramesWritten % MAX_FRAMES;
    // Calculate if a padding byte is required (frame chunks need to be an even number of bytes).
    uint8_t paddingByte = frameBuffer[frameOutPos]->len & 0x00000001;
    // Keep track of the current position in the file relative to the start of the movi section.  This is used to update the idx1 file.
    uint32_t frameOffset = ftell(ImgFile) - AVI_HEADER_SIZE;

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // Creat the chunk file header "00dc" Only reqired for AVI file.
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    bytesWritten = fwrite(buffer00dc, 1, 4, ImgFile);
    if (bytesWritten != 4)
    {
      DPRINTLN("Unable to write 00dc header to AVI file");
      fclose(ImgFile);
      fclose(idx1File);
      fileOpen = false;
      DPRINTLN("AddtoFile return false1");
      LOaddToFile = 0;
      return false;
    }
    // Add the frame size to the file (including padding).
    uint32_t frameSize = frameBuffer[frameOutPos]->len + paddingByte;
    bytesWritten = writeLittleEndian(frameSize, ImgFile, 0x00, FROM_CURRENT);
    if (bytesWritten != 4)
    {
      DPRINTLN("Unable to write frame size to AVI file");
      fclose(ImgFile);
      fclose(idx1File);
      fileOpen = false;
      LOaddToFile = 0;
      DPRINTLN("AddtoFile return false2");
      return false;
    }

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // Write the frame from the camera to circular frameBuffer (ImageMode).
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    bytesWritten = fwrite(frameBuffer[frameOutPos]->buf, 1,
                          frameBuffer[frameOutPos]->len, ImgFile);
    if (bytesWritten != frameBuffer[frameOutPos]->len)
    {
      DPRINTLN("Unable to write frame to AVI file");
      fclose(ImgFile);
      fclose(idx1File);
      fileOpen = false;
      esp_camera_fb_return(frameBuffer[frameOutPos]);
      DPRINTLN("AddtoFile return false3");
      LOaddToFile = 0;
      return false;
    }
    // Release this frame from memory.
    esp_camera_fb_return(frameBuffer[frameOutPos]);
    // The frame from the camera contains a chunk header of JFIF (bytes 7-10) that we want to replace with AVI1.
    // So we move the write head back to where the frame was just written + 6 bytes.

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // Add the AVI file trailer and padding, edit as required (ImageMode).
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    fseek(ImgFile, (bytesWritten - 6) * -1, SEEK_END);
    // Then overwrite with the new chunk header value of AVI1.
    bytesWritten = fwrite(bufferAVI1, 1, 4, ImgFile);
    if (bytesWritten != 4)
    {
      DPRINTLN("Unable to write AVI1 to AVI file");
      fclose(ImgFile);
      fclose(idx1File);
      fileOpen = false;
      DPRINTLN("AddtoFile return false4");
      LOaddToFile = 0;
      return false;
    }
    // Move the write head back to the end of the file.
    fseek(ImgFile, 0, SEEK_END);
    // If required, add the padding to the file.
    if (paddingByte > 0)
    {
      bytesWritten = fwrite(buffer0000, 1, paddingByte, ImgFile);
      if (bytesWritten != paddingByte)
      {
        DPRINTLN("Unable to write padding to AVI file");
        fclose(ImgFile);
        fclose(idx1File);
        fileOpen = false;
        DPRINTLN("AddtoFile return false");
        LOaddToFile = 0;
        return false;
      }
    }
    // Write the frame offset to the idx1 file for this frame (used later).
    bytesWritten = writeLittleEndian(frameOffset, idx1File, 0x00, FROM_CURRENT);
    if (bytesWritten != 4)
    {
      DPRINTLN("Unable to write frame offset to idx1 file");
      fclose(ImgFile);
      fclose(idx1File);
      fileOpen = false;
      DPRINTLN("AddtoFile return false6");
      LOaddToFile = 0;
      return false;
    }
    // Write the frame size to the idx1 file for this frame (used later).
    bytesWritten = writeLittleEndian(frameSize - paddingByte, idx1File,
                                     0x00, FROM_CURRENT);
    if (bytesWritten != 4)
    {
      DPRINTLN("Unable to write frame size to idx1 file");
      fclose(ImgFile);
      fclose(idx1File);
      fileOpen = false;
      DPRINTLN("AddtoFile return false7");
      LOaddToFile = 0;
      return false;
    }
    // Increment the frames written count, and keep track of total padding.
    fileFramesWritten++;
    filePadding = filePadding + paddingByte;
    // DPRINTLN(" ImageMode AddtoFile return true");
  }
  else // if(ImageMode)
  {
    // Write the frame from the camera.
    //    DPRINTLN("bytesWritten = fwrite");
    DPRINTLN(" jpgmode AddtoFile");
    int fretry = 0;

    while (true)
    {
      bytesWritten = fwrite(frameBuffer[0]->buf, 1, frameBuffer[0]->len, ImgFile);
      if (bytesWritten != frameBuffer[0]->len)
      {
        DPRINT("Bad jpg frame write: ");
        DPRINTLN("bytes written does not match buffer size");
        // count does not match bytes read
        if (!ferror(ImgFile))
        {
          DPRINTLN("Software error");
          // expect error
          fclose(ImgFile);
          fileOpen = false;
          esp_camera_fb_return(frameBuffer[0]);
          LOaddToFile = 0;
          return false;
        }
        DPRINTLN("Retrying SD read");
        // clearerr(fp);          // clear the error
        fclose(ImgFile);
        ImgFile = fopen(p_cspath, "wb");
        if (ImgFile == NULL)
        {
          DPRINT("Unable to write frame to AVI file");
          // DPRINTLN(AVIFilename);
          DPRINTLN(cspath);
          fileOpen = false;
          esp_camera_fb_return(frameBuffer[0]);
          LOaddToFile = 0;
          return false;
        }
        fretry++;
        if (fretry > Maxfretry)
        {
          DPRINTLN("Unable to write frame to AVI file");
          fclose(ImgFile);
          //  fclose(idx1File);
          fileOpen = false;
          esp_camera_fb_return(frameBuffer[0]);
          LOaddToFile = 0;
          return false;
        }

      } // if (bytesWritten
      else
      {
        DPRINT("Writen frame to jpg file");
        DPRINTLN(" jpgmode AddtoFile return true");
        esp_camera_fb_return(frameBuffer[0]);
        fileFramesWritten++;
        break;
      }
    } // while true
  }     // else (if (ImageMode))
  LOaddToFile = 0;
  return true;
}

// copy file with path stored p_cspath to
// file with path stored p_cdpath
bool Copyf()
{

  DPRINTLN("Copyf()");
  DPRINT("p_cdpath: ");
  DPRINTLN(p_cdpath);
  FILE *sp;

  char cbuffer[cbufferSize];
  size_t size;
  sp = fopen(p_cspath, "rb");

  if (sp == NULL)
  {
    DPRINT("Unable to open source file ");
    return false;
  }
  DPRINTLN("sp open");
  FILE *dp;

  dp = fopen(p_cdpath, "wb");
  if (dp == NULL)
  {
    DPRINT("Unable to open dest file ");
    fclose(sp);
    return false;
  }

  long Bcount = 0;
  rewind(dp); // to be sure
  rewind(sp);
  while (size = fread(cbuffer, 1, cbufferSize, sp))
  {
    fwrite(cbuffer, 1, size, dp);
    if (ferror(dp))
    {

      DPRINTLN("File write error!");
      fclose(sp);
      fclose(dp);
      return false;
    }
    Bcount++;
  }
  DPRINT("Bcount: ");
  DPRINTLN(Bcount);
  fclose(sp);
  fclose(dp);
  // return Verifyf();
  return true;
}

// Compare two files
// the paths for two files must be stored  at cdpath  cspath
bool Verifyf()
{

  DPRINTLN("Verifyf() ");
  bool Ret = true;
  DPRINT("cdpath: ");
  DPRINTLN(cdpath);
  DPRINT("cspath: ");
  DPRINTLN(cspath);
  FILE *sp;
  char cbuffer[cbufferSize];
  char cbuffer2[cbufferSize];
  size_t size;
  sp = fopen(p_cspath, "rb");
  if (sp == NULL)
  {
    DPRINT("Unable to open source file ");
    return false;
  }
  DPRINTLN("sp open");
  FILE *dp;
  dp = fopen(p_cdpath, "rb");
  if (dp == NULL)
  {
    fclose(sp);
    DPRINT("Unable to open dest file ");
    return false;
  }
  long Bcount = 0;
  rewind(sp);
  rewind(dp);
  while (size = fread(cbuffer, 1, cbufferSize, sp))
  {
    fread(cbuffer2, 1, size, dp);
    for (int i = 0; i < size; i++)
    {
      if (cbuffer[i] != cbuffer2[i])
      {
        DPRINT("x");
        Ret = false;
        i = size + 1;
      }
    }
    if (!Ret)
      break;
    Bcount++;
  }

  fclose(sp);
  fclose(dp);
  return Ret;
}

// check if file exits on SD drive pointed to by *path
bool Efile(char *path)
{
  if (FILE *file = fopen(path, "r"))
  {
    fclose(file);
    return true;
  }
  else
  {
    return false;
  }
}

// Delete SD the file pointed to by *path
// returns true on success
bool Dfile(char *path)
{
  DPRINT("Delete File:  ");
  DPRINTLN(path);
  if (Efile(path))
  {
    remove(path); // delete file
    if (Efile(path))
    {
      DPRINTLN("Unable to delete file");
      return false;
    }
    else
    {
      DPRINTLN("File deleted");
      return true;
    }
  }
  else
  {
    DPRINTLN("File does not exist.");
    return true;
  }
}

// ------------------------------------------------------------------------------------------
// if its an avi file Updates the image file with any remaining frames updates file header and trailer close the file.
// if its jpg reset buffer pointer ect close the file
// ------------------------------------------------------------------------------------------
void closeFile()
{
  DPRINTLN("fn  closeFile() ");
  // Update the flag immediately to prevent any further frames getting written to the buffer.
  fileOpen = false;
  flashOn(false);
  if (ImageMode)
  {
    // Flush any remaining frames from the buffer.
    while (framesInBuffer() > 0)
    {
      addToFile();
    }
    // Calculate how long the AVI file runs for.
    unsigned long fileDuration = (millis() - fileStartTime) / 1000ULL;
    // Update AVI header with total file size. This is the sum of:
    //   AVI header (252 bytes less the first 8 bytes)
    //   fileFramesWritten * 8 (extra chunk bytes for each frame)
    //   fileFramesTotalSize (frames from the camera)
    //   filePadding
    //   idx1 section (8 + 16 * fileFramesWritten)
    writeLittleEndian((AVI_HEADER_SIZE - 8) + fileFramesWritten * 8 + fileFramesTotalSize + filePadding + (8 + 16 * fileFramesWritten),
                      ImgFile, 0x04, FROM_START);
    // Update the AVI header with maximum bytes per second.
    if (fileDuration < 10)
      fileDuration = 1;
    uint32_t maxBytes = fileFramesTotalSize / fileDuration;
    writeLittleEndian(maxBytes, ImgFile, 0x24, FROM_START);
    // Update AVI header with total number of frames.
    writeLittleEndian(fileFramesWritten, ImgFile, 0x30, FROM_START);
    // Update stream header with total number of frames.
    writeLittleEndian(fileFramesWritten, ImgFile, 0x8C, FROM_START);
    // Update movi section with total size of frames.  This is the sum of:
    //   fileFramesWritten * 8 (extra chunk bytes for each frame)
    //   fileFramesTotalSize (frames from the camera)
    //   filePadding
    writeLittleEndian(fileFramesWritten * 8 + fileFramesTotalSize + filePadding,
                      ImgFile, 0xF4, FROM_START);
    // Move the write head back to the end of the AVI file.
    fseek(ImgFile, 0, SEEK_END);
    // Add the idx1 section to the end of the AVI file
    writeIdx1Chunk(); // closes idx1 index file
    fclose(ImgFile);
    DPRINT("avi File closed, size: ");
    DPRINTLN(AVI_HEADER_SIZE + fileFramesWritten * 8 + fileFramesTotalSize + filePadding + (8 + 16 * fileFramesWritten));
    // Dfile("/sdcard/videos/idx1.tmp"); // al added this 2/decv3_7  not so sure about it
  }
  else
  {
    fileFramesWritten = 0; // al added this 2/decv3_7  not so sure about it
    fileFramescapturd = 0;
    frameInPos = 0;
    frameOutPos = 0;
    // calculate  fileSize
    fseek(ImgFile, 0L, SEEK_END);
    DPRINT("jpg File closed size: ");
    DPRINTLN(ftell(ImgFile));
    fclose(ImgFile);
  }
}
// ----------------------------------------------------------------------------------
// Routine to add the idx1 (frame index) chunk to the end of the file.
// ----------------------------------------------------------------------------------
void writeIdx1Chunk()
{
  // The idx1 chunk consists of:
  // +--- 1 per file ----------------------------------------------------------------+
  // | fcc         FOURCC 'idx1'                                                     |
  // | cb          DWORD  length not including first 8 bytes                         |
  // | +--- 1 per frame -----------------------------------------------------------+ |
  // | | dwChunkId DWORD  '00dc' StreamID = 00, Type = dc (compressed video frame) | |
  // | | dwFlags   DWORD  '0000'  dwFlags - none set                               | |
  // | | dwOffset  DWORD   Offset from movi for this frame                         | |
  // | | dwSize    DWORD   Size of this frame                                      | |
  // | +---------------------------------------------------------------------------+ |
  // +-------------------------------------------------------------------------------+
  // The offset & size of each frame are read from the idx1.tmp file that we created
  // earlier when adding each frame to the main file.
  // DPRINTLN("writeIdx1Chunk() ");
  //
  size_t bytesWritten = 0;
  // Write the idx1 header to the file
  bytesWritten = fwrite(bufferidx1, 1, 4, ImgFile);
  if (bytesWritten != 4)
  {
    DPRINTLN("Unable to write idx1 chunk header to AVI file");
    return;
  }
  // Write the chunk size to the file.
  bytesWritten = writeLittleEndian((uint32_t)fileFramesWritten * 16, ImgFile,
                                   0x00, FROM_CURRENT);
  if (bytesWritten != 4)
  {
    DPRINTLN("Unable to write idx1 size to AVI file");
    return;
  }
  // We need to read the idx1 file back in, so move the read head to the start of the idx1 file.
  fseek(idx1File, 0, SEEK_SET);
  // For each frame, write a sub chunk to the AVI file (offset & size are read from the idx file)
  char readBuffer[8];
  for (uint32_t x = 0; x < fileFramesWritten; x++)
  {
    // Read the offset & size from the idx file.
    bytesWritten = fread(readBuffer, 1, 8, idx1File);
    if (bytesWritten != 8)
    {
      DPRINTLN("Unable to read from idx file");
      return;
    }
    // Write the subchunk header 00dc
    bytesWritten = fwrite(buffer00dc, 1, 4, ImgFile);
    if (bytesWritten != 4)
    {
      DPRINTLN("Unable to write 00dc to AVI file idx");
      return;
    }
    // Write the subchunk flags
    bytesWritten = fwrite(buffer0000, 1, 4, ImgFile);
    if (bytesWritten != 4)
    {
      DPRINTLN("Unable to write flags to AVI file idx");
      return;
    }
    // Write the offset & size
    bytesWritten = fwrite(readBuffer, 1, 8, ImgFile);
    if (bytesWritten != 8)
    {
      DPRINTLN("Unable to write offset & size to AVI file idx");
      return;
    }
  }
  // Close the idx1 file.
  fclose(idx1File);
}

// ------------------------------------------------------------------------------------------
// Write a 32 bit value in little endian format (LSB first) to file at specific location.
// ------------------------------------------------------------------------------------------
uint8_t
writeLittleEndian(uint32_t value, FILE *file, int32_t offset,
                  relative position)
{
  uint8_t digit[1];
  uint8_t writeCount = 0;
  // Set position within file.  Either relative to: SOF, current position, or EOF.
  if (position == FROM_START)
    fseek(file, offset, SEEK_SET); // offset >= 0
  else if (position == FROM_CURRENT)
    fseek(file, offset, SEEK_CUR); // Offset > 0, < 0, or 0
  else if (position == FROM_END)
    fseek(file, offset, SEEK_END); // offset <= 0 ??
  else
    return 0;
  // Write the value to the file a byte at a time (LSB first).
  for (uint8_t x = 0; x < 4; x++)
  {
    digit[0] = value % 0x100;
    writeCount = writeCount + fwrite(digit, 1, 1, file);
    value = value >> 8;
  }
  // Return the number of bytes written to the file.
  return writeCount;
}
// ------------------------------------------------------------------------------------------
// Calculate how many frames are waiting to be processed.
// ------------------------------------------------------------------------------------------
uint16_t
framesInBuffer()
{
  // DPRINTLN("framesInBuffer()");
  return (fileFramescapturd - fileFramesWritten);
}
// ------------------------------------------------------------------------------------------
// If we get here, then something bad has happened so easiest thing is just to restart.
// ------------------------------------------------------------------------------------------
void fatalError()
{
  DPRINTLN("Fatal error - restarting.");
  delay(1000);
  ESP.restart();
}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!! Send file to google drive
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// while running monitors the global variable GDUabort and gracefully exits if it becomes true
// Opens input image file calcs the encoded file length
// establish TCP socket connection to google drive
// Uploads the file to GD
// Waits for response from GD
// the selected file name and path is stored as a char string pointed to by the FPath argument
// returns true if successful false if not

bool sendSDImageToGoogleDrive(char *filepath)
{
  DPRINTLN(" fn  sendSDImageToGoogleDrive()");
  if (GDUabort)
  {
    DPRINTLN("Locked out Aborting gdUpload: ");
    GDuploadActive = false;
    return false;
  }
  GDuploadActive = true;
  char imageFileH[24]; // Header sent to google drive before base64 encoded content
  int fretry = 0;
  int Maxfretry = 3;
  size_t result;
  bool exfun = false;
  // check file extention of target file and set header acordingly
  if ((strstr(filepath, ".avi") != NULL))
  {
    strcpy(imageFileH, "data:video/avi;base64,"); // MIME type Header sent to google drive before base64 encoded content
  }
  else if ((strstr(filepath, ".jpg") != NULL))
  {
    strcpy(imageFileH, "data:image/jpeg;base64,"); // MIME type Header sent to google drive before base64 encoded content
  }
  else
  {
    GDuploadActive = false;
    return false; // no files to upload
  }

  String FatFile(sfile);
  String myFilename = String("&myFilename=" + FatFile);
  DPRINTLN(filepath);         // input file to be sent to Google Drive
  fp = fopen(filepath, "rb"); // Open the AVI file.
  if (fp == NULL)
  {
    DPRINT("Unable to open image file ");
    DPRINTLN(filepath);
    GDuploadActive = false;
    return false;
  }
  else
  {
    DPRINT(filepath);
    DPRINTLN(" opened.");
  }
  // calculate input fileSize
  fseek(fp, 0L, SEEK_END);
  fileSize = ftell(fp); // get file size of input avi file
  if (fileSize < 1)
  {
    DPRINT("Input File is empty aborting upload file deleted ");
    Dfile(cspath);
    GDuploadActive = false;
    return false;
  }
  fclose(fp);
  // calculate required zero byes that are needed to pad out the input file to a multiple of 3
  pad = (fileSize) % 3;
  if (pad > 0)
    pad = 3 - pad;

  unsigned long OPfile_Len = PreCalc(filepath); // calculate te size of the output file we need to know
  //                                                this in advance, its sent to GD when establishing the upload
  if (OPfile_Len == 0)
  {
    GDuploadActive = false;
    DPRINTLN("Aborting gdupload Precalc failed: ");
    return false;
  }
  String Data = myFoldername + myFilename + myImage;
  const char *myDomain = "script.google.com";
  String getAll = "", getBody = "";
  DPRINTLN("Connect to " + String(myDomain));
  if (client_tcp.connect(myDomain, 443))
  {
    DPRINTLN("Connection successful");
    client_tcp.println("POST " + myScript + " HTTP/1.1"); //                           String myScript = "/macros/s/AKfycbw8UyFuM1nMaLwuAakuX7LSB-U3-Dr43NEjKNLqxhWvtfSYmEn6apCT7Wd9SuiD8IDBfA/exec";
    client_tcp.println("Host: " + String(myDomain));      //                           myDomain = "script.google.com";
    client_tcp.println("Content-Length: " + String(Data.length() + OPfile_Len));
    client_tcp.println("Content-Type: application/x-www-form-urlencoded"); //          original jpeg
    client_tcp.println("Connection: close");
    client_tcp.println();
    client_tcp.print(Data); //                                                         myFoldername+myFilename+myImage   google drive output file header data
    delay(100);             //                                                                     necessary delay (10ms does not work) if not included file fails to open
    if (GDUabort)
    {
      DPRINTLN("GDUabort true Aborting gduipload prior to data send : ");
      GDuploadActive = false;
      client_tcp.stop();
      return false;
    }
    if (!SendtoGoogle(filepath))
    {
      DPRINTLN("SendtoGoogle(filepath) failed!!");
      client_tcp.stop();
      return false;
    }

    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //!!!!!!!!!!!!!!!!!!!   Get respose from google and ignore read chunks from input file encode send to google d complete
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    int waitTime = 10000; // DEFAULT TO 10 SECONDS IF OPFILE < 500K SERIAL PRINT ANYTHING THAT COMES BACK
    long startTime = millis();
    boolean state = false;
    // wait for google to respond
    while ((startTime + waitTime) > millis())
    {
      if (GDUabort)
      {
        DPRINT("S6 Abort: OK to return true and delete file its been uploaded ");
        break;
      }
      DPRINT(".");
      delay(100);
      while (client_tcp.available())
      {
        char c = client_tcp.read(); //                                            read response from goole drive
        if (state == true)
          getBody += String(c); //                                              append to getBody
        if (c == '\n')            //                                              response ened?   (new line)
        {
          if (getAll.length() == 0)
            state = true; //                                                  new line will trigger cature to getbody
          getAll = "";      //                                                  clear get all
        }
        else if (c != '\r')      //                                               carridge return
          getAll += String(c); //                                               append to getall
        startTime = millis();    //                                               restart timeout
      }
      if (getBody.length() > 0)
      {
        DPRINTLN("Got Google response"); //                                       exit
        break;
      }
    } // while ((startTime + waitTime) > millis()) {
    if (getBody.length() > 0)
    {
      DPRINTLN(getBody);
    }
    else
    {
      DPRINTLN("Timmed out waiting for google");
    }
    // timming out is not a show stopper return true anyway
  } // if (client_tcp.connect(myDomain, 443))
  else
  {
    getBody = "Connected to " + String(myDomain) + " failed.";
    DPRINTLN("Connect to " + String(myDomain) + " failed.");
    GDUabort = true;
  }
  GDuploadActive = false;
  client_tcp.stop();
  DPRINTLN("TCP Stopped");
  if (GDUabort)
  {
    DPRINTLN("Returning false");
    return false;
  }
  DPRINTLN("Returning true");
  return true;
}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!  SendtoGoogle read chunks from input file encode send to google d start
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// send base64url encoded chunks of the selected file to Google drive
// the selected file name and path is stored as a char string pointed to by the FPath argument
// returns true if successful false if not

bool SendtoGoogle(char *Fpath)
{
  DPRINTLN("Sending to Google Drive.");
  unsigned long totalLength = 0;
  OPbuffer = (char *)malloc(BlockSize * 1.8);
  if (OPbuffer == NULL)
  {
    DPRINT("OPbuffer malloc failed");
    free(OPbuffer);
    return false;
  }
  fileinput = (uint8_t *)malloc(BlockSize + 3); // allocate memory to read in buffer on stack
  if (fileinput == NULL)
  {
    DPRINT("fileinput malloc failed");
    free(fileinput);
    free(OPbuffer);
    return false;
  }
  fileinput[0] = NULL;
  IPindex = 0;
  IPindexLast = 0;
  input = (char *)fileinput;
  LastBlock = false;
  unsigned long StartWrite = 0;  // wifi secure client millis() at start time of write command
  unsigned long StartUpload = 0; // millis() at start of google upload
  unsigned long Writemillis = 0;
  char output[base64_enc_len(3)];                // holds base64 enc
  unsigned long TByteswriten = 0;                // index pointer for input file on SD card
  unsigned long IPindex = 0;                     // index pointer for input file on SD card
  unsigned long IPindexLast = 0;                 // index pointer for input file on SD card
  unsigned long OPfile_Len = strlen(imageFileH); // total length of the encoded output file sent to Google Drive
  // need to add the header length into the first block when do precalc of length
  bool LastBlock = false; // Flag to indicate at the last input file chunk
  int fretry = 0;
  int Maxfretry = 3;
  bool exfun = false;
  size_t result;
  bool GoodRead = false;
  StartUpload = millis();
  fp = fopen(Fpath, "rb");
  if (fp == NULL)
  {
    DPRINT("Unable to open image file ");
    DPRINTLN(Fpath);
    GDuploadActive = false;
    free(fileinput);
    free(OPbuffer);
    return false;
  }
  else
  {
    DPRINT(Fpath);
    DPRINTLN(" opened.");
  }
  while (!GDUabort) // read chunks from input file encode chunk send chunk
  {
    // delay(10);   // tried removal 10Nov23
    GoodRead = false;
    if (IPindex == 0) // require this logic in case of a read error on the first Input file block
    { // that would wipe out the header when the read was retried.
      strcpy(
        OPbuffer,
        imageFileH); // imagefile holds chunkes of encoded data sent to Google drive, starts with unencoded header
    }
    else
    {
      strcpy(OPbuffer, "");
    }
    input = (char *)fileinput;             // pointer to start input buffer
    if (fseek(fp, IPindex, SEEK_SET) != 0) // point to next data chunk for avi on SD
    {
      DPRINTLN("fseek error!  ");
      fclose(fp);
      fp = fopen(Fpath, "rb");
      // Open the AVI file.
      if (fp == NULL)
      {
        DPRINT(
          " Sendsdtogoogle read chunks Unable to open file ");
        DPRINTLN(Fpath);
        exfun = true;
      }
      else if (fseek(fp, IPindex, SEEK_SET) != 0)
      {
        DPRINTLN(" Fatal Seek error 1");
        exfun = true;
        //  break;   // if fseek fails again give up
      }
    }
    else // if (fseek(fp, IPindex, SEEK_SET) != 0)
    {
      if ((fileSize - IPindex) < BlockSize) // last block
      {
        result = fread(fileinput, 1, (fileSize - IPindex), fp);
        if (((size_t)(fileSize - IPindex)) == result) // good read
        {
          // add the padding to the input buffer
          if (pad == 1)
          {
            DPRINTLN("Padding 1 byte");
            fileinput[(fileSize - IPindex) + 1] = '0';  // Padd 0
            fileinput[(fileSize - IPindex) + 2] = NULL; // terminate string
          }
          else if (pad == 2)
          {
            DPRINTLN("Padding 2 bytes");
            fileinput[(fileSize - IPindex) + 1] = '0';  // Padd 0
            fileinput[(fileSize - IPindex) + 2] = '0';  // Padd 0
            fileinput[(fileSize - IPindex) + 3] = NULL; // terminate string
          }
          LastBlock = true;
          // DPRINTLN("last block good Read ");
          GoodRead = true;
          for (long int i = 0; i < (fileSize - IPindex + pad); i++)
          {

            if (GDUabort)
            {
              DPRINTLN("ExSGD.S4");
              break;
            } // ok)
            // (input++) is next character in input file
            base64_encode(output, (input++), 3); // base64 encode input buffer to output buffer
            if (i % 3 == 0)
            {
              urlencode(output, OP);
              strcat(OPbuffer, OP);
            }
          }

          totalLength += strlen(OPbuffer);
        }
        else
        {
          DPRINT(" last block BAD Read IPindex: ");
          DPRINTLN(IPindex);
          DPRINTLN("Bytes read does not match request");
          DPRINT("Fpath: ");
          DPRINT(Fpath);
          // count does not match bytes read
          fretry++;
          if (fretry > Maxfretry)
          {
            DPRINTLN(
              "Cannot read Avi to upload Max retries exceeded");
            exfun = true;
            // break;
          }
          else if (!ferror(fp))
          {
            // expect error
            DPRINTLN("Software error aborting upload");
            exfun = true;
            // break;
          }
          DPRINTLN("Retrying SD read");
          clearerr(fp);          // clear the error
          IPindex = IPindexLast; // rewind to start of data block
        }
      } // if ((fileSize - IPindex) < BlockSize)  // last block
      else
      {
        DPRINTLN("Normal Block read");
        result = fread(fileinput, 1, BlockSize, fp);
        if (((size_t)BlockSize) == result) // good read     create the encoded chunkk (imageFile) to send to google
        {
          DPRINT(" good Read IPindex: ");
          DPRINTLN(IPindex);
          GoodRead = true;
          for (int i = 0; i < (BlockSize); i++)
          {
            if (GDUabort)
              break;                           // alarm has occured exit this loop then out of this function
            base64_encode(output, (input++), 3); // base64 encode input buffer to output buffer
            if (i % 3 == 0)
            {
              urlencode(output, OP);
              strcat(OPbuffer, OP);
            }
          }
          fretry = 0;
          IPindexLast = IPindex; // save the index for the start of next data block to read in case of error
          IPindex = IPindex + BlockSize;
          totalLength += strlen(OPbuffer);
          delay(1);
        }
        else
        {
          DPRINTLN("Bytes read does not match request");
          DPRINT("Fpath: ");
          DPRINT(Fpath);
          fretry++;
          if (fretry > Maxfretry)
          {
            DPRINTLN("Cannot read Avi to upload");
            exfun = true;
          }
          else if (!ferror(fp))
          {
            // expect error
            DPRINTLN("Software error aborting upload");
            exfun = true;
          }
          DPRINTLN("Retrying SD read");
          clearerr(fp);          // clear the error
          IPindex = IPindexLast; // rewind to start of last data block
        }
      }

      if (GoodRead)
      {
        size_t bwriten = 0;
        StartWrite = millis();
        bwriten = client_tcp.write((const uint8_t *)OPbuffer,
                                   (size_t)(strlen(OPbuffer)));
        Writemillis += (millis() - StartWrite);
        TByteswriten += bwriten;
        if (bwriten == strlen(OPbuffer))
        {
          DPRINT(".");
        }
        else
        {
          DPRINT("x");
        }
        delay(3); // required to stop mqtt drop outs the 3ms was plucked out of the air extensive testing could not get problem to reocur
      }
    }
    if (LastBlock)
      break; // if last block encoded exit true loop
    if (exfun)
      break;
    if (GDUabort)
      break;
  } // while (!GDUabort)
  DPRINTLN("Close fp fee buffers");
  fclose(fp);
  input[0] = '\0';
  free(fileinput);
  free(OPbuffer);
  GDuploadActive = false;

  if (exfun)
  {
    DPRINTLN("An error has occured Upload aborted: ");
    return false;
  }
  else if (GDUabort)
  {
    DPRINT(" Alarm raised Aborting gduipload: ");
    return false;
  }
  else
  {
    StartWrite = (totalLength / Writemillis) * 1000; // reuse startwrite of bytes per second
    DPRINT("Bytes buffered:  ");
    DPRINTLN(totalLength);
    DPRINT("Bytes Sent:  ");
    DPRINTLN(TByteswriten);
    DPRINT(" in:  ");
    DPRINT((millis() - StartUpload) / 1000);
    DPRINTLN("seconds:");
    DPRINTLN("Upload speed: ");
    DPRINT(StartWrite);
    DPRINTLN(" bytes per second.");
    return true;
  }
}
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!   END   of read chunks from input file encode send to google d
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!! pre calculate encoded output file size to be sent to Google drive
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// do a trial base64URL encode of the selected file
// calculate the length of the encloded file for inclusion in the TCP upload request sent to Google drive
// the selected file name and path is stored as a char string pointed to by the FPath argument
// the encoded length is returned as unsigned long.

unsigned long PreCalc(char *FPath)
{
  DPRINTLN("malloc fileinput");
  fileinput = (uint8_t *)malloc(BlockSize + 3); // allocate memory to read in buffer on stack
  if (fileinput == NULL)
  {
    DPRINT("fileinput malloc failed");
    free(fileinput);
    return false;
  }
  fileinput[0] = NULL;
  unsigned long StartWrite = 0;  // wifi secure client millis() at start time of write command
  unsigned long StartUpload = 0; // millis() at start of google upload
  unsigned long Writemillis = 0;
  char output[base64_enc_len(3)];                // holds base64 enc
  unsigned long IPindex = 0;                     // index pointer for input file on SD card
  unsigned long IPindexLast = 0;                 // index pointer for input file on SD card
  unsigned long OPfile_Len = strlen(imageFileH); // total length of the encoded output file sent to Google Drive
  // need to add the header length into the first block when do precalc of length
  bool LastBlock = false; // Flag to indicate at the last input file chunk
  int fretry = 0;
  int Maxfretry = 3;
  bool exfun = false;
  size_t result;
  StartUpload = millis();
  fp = fopen(FPath, "rb");
  // Open the AVI file.
  if (fp == NULL)
  {
    DPRINT("Unable to open image file ");
    DPRINTLN(FPath);
    free(fileinput);
    return 0;
  }
  DPRINT("Read from file: ");
  DPRINTLN(FPath);
  DPRINTLN("Input file size: " + String(fileSize));
  DPRINTLN("Calculating Output File Size");
  while (!GDUabort) //  Dummy run base 64 encode calc opfile len
  {
    IPindexLast = IPindex;
    input = (char *)fileinput; // pointer input to start input buffer
    if (IPindex == 0)          // require this logic in case of a read error on the first Input file block
    { // that would wipe out the header when the read was retried.
      OPfile_Len = strlen(imageFileH);
    }
    if (fseek(fp, IPindex, SEEK_SET) != 0) // point to next data chunk form avi input file  on SD
    {
      // deal with any errors
      DPRINTLN("fseek error!  ");
      fclose(fp); // close the file and reopen
      fp = fopen(FPath, "rb");
      // Open the AVI file.
      if (fp == NULL)
      {
        DPRINT("sendSDImageToGoogle precalc Unable to open AVI file "); // exit if file does not open
        DPRINTLN(FPath);
        exfun = true;
      }
      DPRINTLN(" reopened.");
      if (fseek(fp, IPindex, SEEK_SET) != 0) // go to IPindex
      {
        DPRINTLN(" Fatal Seek error 2");
        exfun = true; // if seek still fails give up.
      }
    } // if (fseek(fp, IPindex, SEEK_SET) != 0)
    else
    {
      if ((fileSize - IPindex) < BlockSize) // last block
      {
        //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // read and encode last partial block
        //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        result = fread(fileinput, 1, (fileSize - IPindex), fp);
        if (((size_t)(fileSize - IPindex)) == result) // good read
        {
          LastBlock = true;
          // add the padding
          if (pad == 1)
          {
            DPRINTLN("Padding 1 byte");
            fileinput[(fileSize - IPindex) + 1] = '0';  // Padd 0
            fileinput[(fileSize - IPindex) + 2] = NULL; // terminate string
          }
          else if (pad == 2)
          {
            DPRINTLN("Padding 2 bytes");
            fileinput[(fileSize - IPindex) + 1] = '0';  // Padd 0
            fileinput[(fileSize - IPindex) + 2] = '0';  // Padd 0
            fileinput[(fileSize - IPindex) + 3] = NULL; // terminate string
          }
          for (long int i = 0; i < (fileSize - IPindex + pad); i++)
          {
            base64_encode(output, (input++), 3); // base64 encode input buffer to output buffer
            if (i % 3 == 0)
            {
              urlencode(output, OP);
              OPfile_Len += strlen(OP);
            }
          } // for (int  i = 0; i < (fileSize - IPindex); i++)
          fretry = 0;
          DPRINTLN();
          DPRINT("Output file size: ");
          DPRINTLN(OPfile_Len);
        } // if (((size_t)(fileSize - IPindex )) ==  result)
        else
        {
          // count does not match bytes read
          DPRINTLN("Bytes read does not match request");
          DPRINT("FPath: ");
          DPRINTLN(FPath);
          fretry++;
          if (fretry > Maxfretry)
          {
            DPRINTLN("Cannot read Avi to upload max retries exceeded");
            exfun = true;
          }
          else if (!ferror(fp))
          {
            // expect error
            DPRINTLN("Software error aborting upload");
            exfun = true;
          }
          DPRINTLN("Retrying SD read");
          clearerr(fp);          // clear the error
          IPindex = IPindexLast; // rewind to start of data block
        }                          // else  if (((size_t)(fileSize - IPindex )) ==  result)

      } // if  ((fileSize - IPindex ) < BlockSize)   //last block
      else
      {
        //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // read and encode normal block
        //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        result = fread(fileinput, 1, BlockSize, fp);
        if (((size_t)BlockSize) == result) // good read
        {
          fretry = 0;
          for (int i = 0; i < (BlockSize); i++)
          {
            base64_encode(output, (input++), 3); // base64 encode input buffer to output buffer
            if (i % 3 == 0)
            {
              urlencode(output, OP);
              OPfile_Len += strlen(OP);
            }
          }
          DPRINT(".");

          IPindex = IPindex + BlockSize;
        }
        else
        {
          DPRINT("Bad normal read index: ");
          DPRINTLN(IPindex);
          DPRINTLN("Bytes read does not match request");
          DPRINT("FPath: ");
          DPRINT(FPath); // count does not match bytes read
          fretry++;
          if (fretry > Maxfretry)
          {
            DPRINTLN("Cannot read Avi to upload Max reties exeeded");
            exfun = true;
          }
          else if (!ferror(fp))
          {
            // expect error
            DPRINTLN("Software error aborting upload");
            exfun = true;
          }
          DPRINTLN("Retrying SD read");
          clearerr(fp);          // clear the error
          IPindex = IPindexLast; // rewind to start of data block
        }
      }
    }
    if (feof(fp) != NULL)
      DPRINTLN("EOF reached");
    if (LastBlock)
      break;
    if (exfun)
      break;
  } // while (!GDUabort)
  DPRINTLN("Close fp free fileinput");
  fclose(fp);
  input[0] = '\0';
  free(fileinput);
  GDuploadActive = false;
  if (exfun)
  {
    DPRINTLN("SD card read error aborting Precalc");
    return 0; // failed
  }
  else if (GDUabort)
  {
    DPRINTLN("Alarm Raised aborting Precalc");
    return 0; // failed
  }
  else
  {
    int calcsecs = (millis() - StartUpload) / 1000;
    calcsecs = constrain(calcsecs, 1, 200);
    DPRINT("Calc seconds ");
    DPRINTLN(calcsecs);
    return OPfile_Len; // success
  }
}

// URL encode the base64 encoded data to send over the internet
//  ns pointer to input char string
//  OP pointer to produced output char string
void urlencode(char *ns, char *OP)
{
  std::string s(ns);                               // input char string to string
  static const char lookup[] = "0123456789abcdef"; //
  std::stringstream e;
  for (int i = 0, ix = s.length(); i < ix; i++)
  {
    const char &c = s[i];
    if ((48 <= c && c <= 57) ||  // 0-9                        // safe characters
        (65 <= c && c <= 90) ||  // abc...xyz
        (97 <= c && c <= 122) || // ABC...XYZ
        (c == '-' || c == '_' || c == '.' || c == '~'))
    {
      e << c; // safe character leave character as is
    }
    else
    { // replace unsafe b64 characters like '+' '/' '='  with  safe ASSCI characters  %,  followed by msnibble of c, lsnibble of c
      e << '%';                     // append % to e
      e << lookup[(c & 0xF0) >> 4]; // lookup and append msnibble to e as safe character
      e << lookup[(c & 0x0F)];      // lookup and append lsnibble to e as safe character
    }
  }
  std::string ess = e.str();
  const char *p = ess.c_str(); // e to const char array
  strcpy(OP, p);               // copy result to OP buffer
  return;
}
