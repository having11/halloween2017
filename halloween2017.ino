//Halloween Project

//Header includes
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <SD.h>
#include <SPI.h>
#include <Servo.h>

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif
#define NUMPIXELS      106

//Button pin
#define BUTTON 32

//OLED pins
#define DC 5
#define CS 6
#define MOSI 7
#define SCLK 8
#define RST 9

#define SDCARD_CS_PIN BUILTIN_SDCARD
#define SDCARD_MOSI_PIN  11  // not actually used
#define SDCARD_SCK_PIN   13  // not actually used


//LED strip pins
#define COFFIN 31
#define PUMPKIN 30

//Servo pins
#define HEAD 29
#define ARM 28

// Color definitions
#define	BLACK           0x0000
#define	BLUE            0x001F
#define	RED             0xF800
#define	GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0  
#define WHITE           0xFFFF

#define upperLimit 125
#define lowerLimit 175

const char* BMP_BKGD = "IMG.bmp";

Adafruit_SSD1351 tft = Adafruit_SSD1351(CS, DC, MOSI, SCLK, RST);
Adafruit_NeoPixel CoffinPixels = Adafruit_NeoPixel(NUMPIXELS, COFFIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel PumpkinPixels = Adafruit_NeoPixel(44, PUMPKIN, NEO_GRB + NEO_KHZ800);

int greenVal = 0;
bool directionFlag = false;

File bmpFile;
int bmpWidth, bmpHeight;
uint8_t bmpDepth, bmpImageoffset;

Servo armServo;

void setup() {
	Serial.begin(9600);
	pinMode(BUTTON, INPUT_PULLUP);
	attachInterrupt(BUTTON, run_effects, FALLING);
	SPI.setMOSI(SDCARD_MOSI_PIN);
	SPI.setSCK(SDCARD_SCK_PIN);
	if (!(SD.begin(SDCARD_CS_PIN))) {
		while (true)
		{
			Serial.println("Unable to use SD card");
			delay(500);
		}
	}
	pinMode(CS, OUTPUT);
	digitalWrite(CS, HIGH);
	tft.begin();
	tft.fillScreen(BLUE);
	delay(500);
	bmpDraw("IMG.bmp",0,0);
	drawText();
 CoffinPixels.begin();
 CoffinPixels.show();
 PumpkinPixels.begin();
 PumpkinPixels.show();
	setPumpkinColor(200, 200, 0);
	armServo.attach(ARM);
  armServo.write(upperLimit);
}

void loop() {
  armServo.write(upperLimit);
	pulseLights();
	delay(10);
}

void setPumpkinColor(int r, int g, int b) {
	for (int i = 26; i < 44; i++) {
		PumpkinPixels.setPixelColor(i, PumpkinPixels.Color(r, g, b));
	}
 PumpkinPixels.show();
}

void drawText() {
	tft.setTextColor(BLUE);
	tft.setTextSize(2);
	tft.println("Made by: \n\nEvan Rust\n\nHelped by\nLiana Rust");
}

void run_effects(){
	for (int i = 0; i < 106; i++) {
		CoffinPixels.setPixelColor(i, CoffinPixels.Color(200, 0, 200));
	}
 CoffinPixels.show();
	pumpkinActivate();
	//raiseHead();
	//Arm wave
	/*for (int i = 0; i < 5; i++) {
		for (int i = lowerLimit; i < upperLimit; i++) {
			waveArm(i);
			delay(100);
		}
		for (int i = upperLimit; i > lowerLimit; i--) {
			waveArm(i);
			delay(100);
		}
	}*/
 armServo.write(upperLimit);
 armServo.write(lowerLimit);
	//lowerHead();
	
 delay(5000);
 setPumpkinColor(200, 200, 0);
}

void waveArm(int degree) {
	armServo.write(degree);
}

void pumpkinActivate() {
	setPumpkinColor(255, 50, 0);
}


void pulseLights() {
	for (int i = 0; i < NUMPIXELS; i++) {
		CoffinPixels.setPixelColor(i, CoffinPixels.Color(255, greenVal, 0));
	}
 CoffinPixels.show();
	if (!directionFlag) {
		greenVal += 1;
	}
	else if (directionFlag) {
		greenVal -= 1;
	}
	if (greenVal >= 255) {
		directionFlag = true; //Count down
	}
	else if (greenVal < 0) {
    greenVal= 0;
		directionFlag = false; //Count up
	}
}

#define BUFFPIXEL 20

void bmpDraw(char *filename, uint8_t x, uint8_t y) {

	File     bmpFile;
	int      bmpWidth, bmpHeight;   // W+H in pixels
	uint8_t  bmpDepth;              // Bit depth (currently must be 24)
	uint32_t bmpImageoffset;        // Start of image data in file
	uint32_t rowSize;               // Not always = bmpWidth; may have padding
	uint8_t  sdbuffer[3 * BUFFPIXEL]; // pixel buffer (R+G+B per pixel)
	uint8_t  buffidx = sizeof(sdbuffer); // Current position in sdbuffer
	boolean  goodBmp = false;       // Set to true on valid header parse
	boolean  flip = true;        // BMP is stored bottom-to-top
	int      w, h, row, col;
	uint8_t  r, g, b;
	uint32_t pos = 0, startTime = millis();

	if ((x >= tft.width()) || (y >= tft.height())) return;

	Serial.println();
	Serial.print("Loading image '");
	Serial.print(filename);
	Serial.println('\'');

	// Open requested file on SD card
	if ((bmpFile = SD.open(filename)) == NULL) {
		Serial.print("File not found");
		return;
	}

	// Parse BMP header
	if (read16(bmpFile) == 0x4D42) { // BMP signature
		Serial.print("File size: "); Serial.println(read32(bmpFile));
		(void)read32(bmpFile); // Read & ignore creator bytes
		bmpImageoffset = read32(bmpFile); // Start of image data
		Serial.print("Image Offset: "); Serial.println(bmpImageoffset, DEC);
		// Read DIB header
		Serial.print("Header size: "); Serial.println(read32(bmpFile));
		bmpWidth = read32(bmpFile);
		bmpHeight = read32(bmpFile);
		if (read16(bmpFile) == 1) { // # planes -- must be '1'
			bmpDepth = read16(bmpFile); // bits per pixel
			Serial.print("Bit Depth: "); Serial.println(bmpDepth);
			if ((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed

				goodBmp = true; // Supported BMP format -- proceed!
				Serial.print("Image size: ");
				Serial.print(bmpWidth);
				Serial.print('x');
				Serial.println(bmpHeight);

				// BMP rows are padded (if needed) to 4-byte boundary
				rowSize = (bmpWidth * 3 + 3) & ~3;

				// If bmpHeight is negative, image is in top-down order.
				// This is not canon but has been observed in the wild.
				if (bmpHeight < 0) {
					bmpHeight = -bmpHeight;
					flip = false;
				}

				// Crop area to be loaded
				w = bmpWidth;
				h = bmpHeight;
				if ((x + w - 1) >= tft.width())  w = tft.width() - x;
				if ((y + h - 1) >= tft.height()) h = tft.height() - y;

				for (row = 0; row<h; row++) { // For each scanline...
					tft.goTo(x, y + row);

					// Seek to start of scan line.  It might seem labor-
					// intensive to be doing this on every line, but this
					// method covers a lot of gritty details like cropping
					// and scanline padding.  Also, the seek only takes
					// place if the file position actually needs to change
					// (avoids a lot of cluster math in SD library).
					if (flip) // Bitmap is stored bottom-to-top order (normal BMP)
						pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
					else     // Bitmap is stored top-to-bottom
						pos = bmpImageoffset + row * rowSize;
					if (bmpFile.position() != pos) { // Need seek?
						bmpFile.seek(pos);
						buffidx = sizeof(sdbuffer); // Force buffer reload
					}

					// optimize by setting pins now
					for (col = 0; col<w; col++) { // For each pixel...
												  // Time to read more pixel data?
						if (buffidx >= sizeof(sdbuffer)) { // Indeed
							bmpFile.read(sdbuffer, sizeof(sdbuffer));
							buffidx = 0; // Set index to beginning
						}

						// Convert pixel from BMP to TFT format, push to display
						b = sdbuffer[buffidx++];
						g = sdbuffer[buffidx++];
						r = sdbuffer[buffidx++];

						tft.drawPixel(x + col, y + row, tft.Color565(r, g, b));
						// optimized!
						//tft.pushColor(tft.Color565(r,g,b));
					} // end pixel
				} // end scanline
				Serial.print("Loaded in ");
				Serial.print(millis() - startTime);
				Serial.println(" ms");
			} // end goodBmp
		}
	}

	bmpFile.close();
	if (!goodBmp) Serial.println("BMP format not recognized.");
}

uint16_t read16(File f) {
	uint16_t result;
	((uint8_t *)&result)[0] = f.read(); // LSB
	((uint8_t *)&result)[1] = f.read(); // MSB
	return result;
}

uint32_t read32(File f) {
	uint32_t result;
	((uint8_t *)&result)[0] = f.read(); // LSB
	((uint8_t *)&result)[1] = f.read();
	((uint8_t *)&result)[2] = f.read();
	((uint8_t *)&result)[3] = f.read(); // MSB
	return result;
}

