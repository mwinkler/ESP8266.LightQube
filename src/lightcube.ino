
#include <Wire.h>
#include <Adafruit_NeoPixel.h>

// config ------------------------
const int loopDelay = 100;
const int waitUntilFinishRotation = 1000;

// members ------------------------

#define LED 0			// LED stripe pin
int LED_COUNT = 24;		// LED count

int16_t accelX, accelY, accelZ;
float gForceX, gForceY, gForceZ;

int16_t gyroX, gyroY, gyroZ;
float rotX, rotY, rotZ;

int topCubePosition = -1;
String topCubePositionTxt = "";

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(LED_COUNT, LED, NEO_GRB + NEO_KHZ800);
uint32_t colors[5];

// set leds positions
byte sideLed[6][4] = {
	{ 18, 19, 20, 21 },	// 0 = right
	{ 0, 1, 2, 23 },	// 1 = front
	{ 3, 8, 17, 22 },	// 2 = bottom
	{ 11, 12, 13, 14 },	// 3 = top
	{ 9, 10, 15, 16 },	// 4 = back
	{ 4, 5, 6, 7 }		// 5 = left
};

int timer = 0;
byte state = 0;


// start ------------------------
void setup() 
{
	Serial.begin(115200);
	Wire.begin();
	setupMPU();
	pixels.begin();

	colors[0] = pixels.Color(255, 0, 0);
	colors[1] = pixels.Color(0, 255, 0);
	colors[2] = pixels.Color(0, 0, 255);
	colors[3] = pixels.Color(255, 255, 0);
	colors[4] = pixels.Color(0, 255, 255);
	colors[5] = pixels.Color(255, 0, 255);
}

void loop() 
{
	recordAccelRegisters();
	//recordGyroRegisters();
	
	bool hasChanged = getPosition();

	if (hasChanged)
	{
		state = 10;
	}

	switch (state)
	{
		// sleep -> do nothing
		case 0:
			break;

		
		// cube as rotated -> light each side with color
		case 10:
			for (byte i = 0; i < 6; i++)
			{
				ledSetSide(i, colors[i]);
			}
			timer = 0;
			state = 15;
			break;

		// wait cube has finish rotating
		case 15:
			timer++;
			
			if (timer * loopDelay > waitUntilFinishRotation)
			{
				state = 20;
			}
			
			break;

		// light up all sides
		case 20:
			ledSetAll(colors[topCubePosition]);
			state = 0;
			break;

		
		// light up curren top side
		case 30:
			//resetAllLed();
			//setSideColor(topCubePosition, colors[topCubePosition]);
			//setAllLedColor(colors[topCubePosition]);
			//pixels.show();
			break;

		// turn off all lights
		case 40:
			//resetAllLed();
			//pixels.show();
			break;
	}

	delay(loopDelay);
}

void setupMPU() 
{
	Wire.beginTransmission(0x68); //This is the I2C address of the MPU (b1101000/b1101001 for AC0 low/high datasheet sec. 9.2)
	Wire.write(0x6B); //Accessing the register 6B - Power Management (Sec. 4.28)
	Wire.write(0b00000000); //Setting SLEEP register to 0. (Required; see Note on p. 9)
	Wire.endTransmission();
	
	//Accessing the register 1B - Gyroscope Configuration (Sec. 4.4) 
	Wire.beginTransmission(0x68);
	Wire.write(0x1B); 
	Wire.write(0x00000000); //Setting the gyro to full scale +/- 250deg./s 
	Wire.endTransmission();
	
	//Accessing the register 1C - Acccelerometer Configuration (Sec. 4.5) 
	Wire.beginTransmission(0x68);
	Wire.write(0x1C); 
	Wire.write(0b00000000); //Setting the accel to +/- 2g
	Wire.endTransmission();
}

void recordAccelRegisters() 
{
	Wire.beginTransmission(0x68);
	Wire.write(0x3B);
	Wire.endTransmission();
	Wire.requestFrom(0x68, 6);
	
	while (Wire.available() < 6);
	
	accelX = Wire.read() << 8 | Wire.read(); //Store first two bytes into accelX
	accelY = Wire.read() << 8 | Wire.read(); //Store middle two bytes into accelY
	accelZ = Wire.read() << 8 | Wire.read(); //Store last two bytes into accelZ

	gForceX = accelX / 16384.0;
	gForceY = accelY / 16384.0;
	gForceZ = accelZ / 16384.0;
}

void recordGyroRegisters() 
{
	Wire.beginTransmission(0x68);
	Wire.write(0x43);
	Wire.endTransmission();
	Wire.requestFrom(0x68, 6);
	
	while (Wire.available() < 6);
	
	gyroX = Wire.read() << 8 | Wire.read(); //Store first two bytes into accelX
	gyroY = Wire.read() << 8 | Wire.read(); //Store middle two bytes into accelY
	gyroZ = Wire.read() << 8 | Wire.read(); //Store last two bytes into accelZ

	rotX = gyroX / 131.0;
	rotY = gyroY / 131.0;
	rotZ = gyroZ / 131.0;
}

void printData() 
{
	/*Serial.print("Gyro (deg)");
	Serial.print(" X=");
	Serial.print(rotX);
	Serial.print("\tY=");
	Serial.print(rotY);
	Serial.print("\tZ=");
	Serial.print(rotZ);
	Serial.print("\tAccel (g)");*/
	Serial.print("\tX=");
	Serial.print(gForceX);
	Serial.print("\tY=");
	Serial.print(gForceY);
	Serial.print("\tZ=");
	Serial.print(gForceZ);
}

bool getPosition()
{
	int newPos = -1;

	/* X > 0.5 */
	if (gForceX > 0.5 && gForceY <= 0.5 && gForceY >= -0.5)
		newPos = 0; /* sdie left */
	
	/* X > -0.5 && X < 0.5 */
	else if (gForceX <= 0.5 && gForceX >= -0.5 && gForceY > 0.5)
		newPos = 1;	/* side back */
	else if (gForceX <= 0.5 && gForceX >= -0.5 && gForceY <= 0.5 && gForceY >= -0.5 && gForceZ > 0.5)
		newPos = 2;	 /* top */
	else if (gForceX <= 0.5 && gForceX >= -0.5 && gForceY <= 0.5 && gForceY >= -0.5 && gForceZ < -0.5)
		newPos = 3;	 /* bottom */
	else if (gForceX <= 0.5 && gForceX >= -0.5 && gForceY < -0.5)
		newPos = 4;	/* side front */

	/* X < -0.5 */
	else if (gForceX < -0.5 && gForceY <= 0.5 && gForceY >= -0.5)
		newPos = 5;	/* side right */
	
	if (newPos != -1 && newPos != topCubePosition)
	{
		topCubePosition = newPos;

		switch (topCubePosition)
		{
			case 0: topCubePositionTxt = "RIGHT"; break;
			case 1: topCubePositionTxt = "FRONT"; break;
			case 2: topCubePositionTxt = "BOTTOM"; break;
			case 3: topCubePositionTxt = "TOP"; break;
			case 4: topCubePositionTxt = "BACK"; break;
			case 5: topCubePositionTxt = "LEFT"; break;
		}

		return true;
	}

	return false;
}

void printPosition()
{
	Serial.println(topCubePositionTxt);
}

void ledResetAll()
{
	for (int i = 0; i < LED_COUNT; i++)
	{
		pixels.setPixelColor(i, 0, 0, 0);
	}

	pixels.show();
}

void ledSetAll(uint32_t color)
{
	for (int i = 0; i < LED_COUNT; i++)
	{
		pixels.setPixelColor(i, color);
	}
	
	pixels.show();
}

void ledSetSide(byte side, uint32_t color)
{
	for (byte i = 0; i < 4; i++)
	{
		pixels.setPixelColor(sideLed[side][i], color);
	}

	pixels.show();
}
