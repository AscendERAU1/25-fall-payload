// ------- //
/* Imports */
// ------- //

#include <Arduino.h>
#include <Wire.h>
#include <STM32SD.h>
#include <Adafruit_GPS.h>
#include <Adafruit_BNO055.h>
#include <Adafruit_ISM330DHCX.h>
#include <SensirionI2cScd4x.h>
#include <Adafruit_AS7341.h>
#include <Adafruit_BMP3XX.h>
#include <Adafruit_LTR390.h>
#include <Adafruit_BME680.h>
#include <Adafruit_NeoPixel.h>

// ---- //
/* Vars */
// ---- //

// Logging
File dataFile; //Instance of the File class
char buffer[50];
int floatBuffer[2];

// GPS
Adafruit_GPS GPS(&Wire);
char c;

// BNO055
Adafruit_BNO055 bno;
sensors_event_t orientationData, angVelocityData, linearAccelData, magnetometerData, accelerometerData, gravityData;
float x, y, z; // Variables to store the x, y, z values of the sensor data

// ISM 330DLC
Adafruit_ISM330DHCX ism330;
sensors_event_t ismTemperatureData, ismAccelerometerData, ismGyroData;

// SCD41
SensirionI2cScd4x scd41;
int16_t scd41data;
uint16_t co2Concentration = 0;
float temperature = 0.0;
float relativeHumidity = 0.0;

// AS7341
Adafruit_AS7341 as7341;

// BPM390
Adafruit_BMP3XX baro;
const float seaLevelPressure = 1013.25;

// LTR390
Adafruit_LTR390 ltr390;
uint32_t als, uvs;
static unsigned long lastMillis = 0;
const unsigned long interval = 30000;
unsigned long currentMillis;
bool readingUV = false;

// BME688
Adafruit_BME680 bme688;

// LED
Adafruit_NeoPixel led(1, 8, NEO_RGB); // NeoPixel LED. Remember to use .show() to actually update the LED
bool ledCurrentlyShowing = false; // If the LED is currently showing. Used to blink the LED
bool showErrorLed = false; // If the LED should show there is an error
String errorMsg = ""; // Debug message for why there is an error

// --------------------- //
/* Function Declarations */
// --------------------- //

void updateGPS();

void splitFloat(int buffer[], float in);

void toCharArray(String in);
void toCharArray(char* in);
void toCharArray(const char* in);
void toCharArray(int in);
void toCharArray(float in);
void toCharArray(bool in);
void toCharArray(uint8_t in);
void toCharArray(uint16_t in);
void toCharArray(uint32_t in);
void toCharArray(int8_t in);
void toCharArray(int32_t in);

// -------------------- //
/* Function Definitions */
// -------------------- //

void updateGPS() {
    for (int i = 0; i < 64; i++) {
        GPS.read();

        if (GPS.newNMEAreceived()) {
            GPS.parse(GPS.lastNMEA());
        }
    }
}

void splitFloat(int buffer[], float in) {
    buffer[0] = in;
    buffer[1] = abs(trunc((in - buffer[0]) * 1000));
}

void toCharArray(String in) {
    in.toCharArray(buffer, sizeof(buffer));
}
void toCharArray(char* in) {
    sprintf(buffer, in);
}
void toCharArray(const char* in) {
    sprintf(buffer, in);
}
void toCharArray(int in) {
    sprintf(buffer, "%i", in);
}
void toCharArray(float in) {
    splitFloat(floatBuffer, in);
    sprintf(buffer, "%i.%i", floatBuffer[0], floatBuffer[1]);
}
void toCharArray(bool in) {
    int convertedIn = in;
    sprintf(buffer, (in ? "true" : "false"));
}
void toCharArray(uint8_t in) {
    sprintf(buffer, "%i", in);
}
void toCharArray(uint16_t in) {
    sprintf(buffer, "%i", in);
}
void toCharArray(uint32_t in) {
    sprintf(buffer, "%i", in);
}
void toCharArray(int8_t in) {
    sprintf(buffer, "%i", in);
}
void toCharArray(int32_t in) {
    sprintf(buffer, "%i", in);
}

// ---- //
/* Main */
// ---- //

void setup() {
    Serial.begin(9600);
    Serial.setTimeout(0);
    Wire.begin();

    // GPS
    // while (!GPS.begin(0x10)) {
    //     showErrorLed = true;
    //     errorMsg += "There was a problem detecting the GPS\n";
    // }
    if (!GPS.begin(0x10)) {
        showErrorLed = true;
        errorMsg += "There was a problem detecting the GPS\n";
        Serial.println("ERROR: GPS not detected!");
    } else {
        Serial.println("GPS initialized successfully.");
    }
    delay(1000);
    GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
    GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); // 1 Hz update rate

    // BNO055
    if (!bno.begin()) {
        showErrorLed = true;
        errorMsg += "There was a problem detecting the BNO055 sensor\n";
    }

    // ISM330DLC
    if (!ism330.begin_I2C()) {
        showErrorLed = true;
        errorMsg += "There was a problem detecting the ISM330DLC sensor\n";
    }
    ism330.configInt1(false, false, true);
    ism330.configInt2(false, true, false);

    // SCD 41
    scd41.begin(Wire, SCD41_I2C_ADDR_62);
    int scd41good = scd41.wakeUp();
    if (scd41good != 0) {
        showErrorLed = true;
        errorMsg += "There was a problem detecting the SCD41 sensor\n";
    }
    scd41.startPeriodicMeasurement();

    // AS7341
    if (!as7341.begin()) {
        showErrorLed = true;
        errorMsg += "There was a problem detecting the AS7341 sensor\n";
    }

    as7341.setATIME(100);
    as7341.setASTEP(999);
    as7341.setGain(AS7341_GAIN_256X);

    // BMP390
    if (!baro.begin_I2C(0x76)) {
        showErrorLed = true;
        errorMsg += "There was a problem detecting the BMP390 sensor\n";
    }

    baro.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
    baro.setPressureOversampling(BMP3_OVERSAMPLING_8X);
    baro.setIIRFilterCoeff(BMP3_IIR_FILTER_DISABLE);
    baro.setOutputDataRate(BMP3_ODR_100_HZ);

    // LTR390
    if (!ltr390.begin()) {
        showErrorLed = true;
        errorMsg += "There was a problem detecting the LTR390 sensor\n";
    }
    ltr390.enable(true);
    ltr390.setGain(LTR390_GAIN_18);
    ltr390.setResolution(LTR390_RESOLUTION_20BIT);
    ltr390.setMode(LTR390_MODE_ALS);
    lastMillis = millis();

    // BME688
    if (!bme688.begin()) {
        showErrorLed = true;
        errorMsg += "There was a problem detecting the BME688 sensor\n";
    }

    // SD logging
    // Docs: https://docs.arduino.cc/learn/programming/sd-guide/
    Serial.print("Initializing SD card...");
    if (!SD.begin()) {
        showErrorLed = true;
        errorMsg += "SD init failed\n";
    }

    // Create a new file with a different name if the file already exists
    int fileIndex = 1;
    char dataFileName[25] = "flightdata.csv";

    while (SD.exists(dataFileName)) {
        sprintf(dataFileName, "flightdata%d.csv", fileIndex);
        fileIndex++;
    }

    // Open the file //
    Serial.println("Initialization successful.");
    dataFile = SD.open(dataFileName, FILE_WRITE);
    if (dataFile) {
        Serial.print("Writing to ");
        Serial.println(dataFileName);

        dataFile.print("Millis,");
        dataFile.print("Time, Fix, Quality, Latitude_Degrees, Longitute_Degrees, Speed, Angle, Altitude, Sats, Magnetic_Variation, HDOP, VDOP, PDOP, "); //GPS Header
        dataFile.print("BNO055_Temperature, Orientation_X, Orientation_Y, Orientation_Z, BNO055_Gyro_X, BNO055_Gyro_Y, BNO055_Gyro_Z, Linear_Accel_X, Linear_Accel_Y, Linear_Accel_Z, Mag_X, Mag_Y, Mag_Z, BNO055_Accel_X, BNO055_Accel_Y, BNO055_Accel_Z,  Accel_Gravity_X, Accel_Gravity_Y, Accel_Gravity_Z, "); // BNO055 Header
        dataFile.print("ISM330DLC_Temperature, ISM330DLC_Accel_X, ISM330DLC_Accel_Y, ISM330DLC_Accel_Z, ISM330DLC_Gyro_X, ISM330DLC_Gyro_Y, ISM330DLC_Gyro_Z, "); // ISM 330DLC Header
        dataFile.print("CO2_Concentration, SCD41_Temperature, Relative_Humidity, "); // SCD41 Header
        dataFile.print("415nm, 445nm, 480nm, 515nm, 555nm, 590nm, 630nm, 680nm, Clear, NIR, "); // AS7341 Header
        dataFile.print("BMP390_Altitude, BMP390_Pressure, BMP390_Temperature, "); // BMP390 Header
        dataFile.print("UV Light, Ambient Light, "); // LTR390 Header
        dataFile.println("BME688_Temperature, BME688_Pressure, BME688_Humidity, BME688_Gas_Resistance, BME688_Altitude"); // BME688 Header

        Serial.println("File setup complete.");
    } else{
        showErrorLed = true;
        errorMsg += "Error opening file\n";
    }

    // Telemetry header
    Serial.print("Millis, ");
    Serial.print("Time, Fix, Quality, Latitude_Degrees, Longitute_Degrees, Speed, Angle, Altitude, Sats, "); //GPS Header
    Serial.print("Linear_Accel_X, Linear_Accel_Y, Linear_Accel_Z, "); // BNO055 Header
    Serial.print("ISM_330DLC_Accel_X, ISM_330DLC_Accel_Y, ISM_330DLC_Accel_Z, "); // ISM 330DLC Header
    Serial.print("CO2_Concentration, SCD41_Temperature, Relative_Humidity, "); // SCD41 Header
    Serial.println("Altitude, Pressure, BMP390_Temperature"); // BMP390 Header

    // LED
    led.begin();
    led.setBrightness(255);
    led.setPixelColor(0, 255, 255, 255);
    led.show();
}

void loop() {
    // ------------ //
    /* Data Logging */
    // ------------ //
    updateGPS(); 

    if(dataFile){
        toCharArray((long)millis()); dataFile.print(buffer); dataFile.print(", "); // Millis

        // GPS data
        // if(GPS.available()){
        //    c = GPS.read();
        //     if (GPS.newNMEAreceived()) {
        //         if (!GPS.parse(GPS.lastNMEA())) {
        //         }
        //     }
        // }a

        toCharArray(GPS.year); dataFile.print(buffer); dataFile.print("-"); // Year
        toCharArray(GPS.month); dataFile.print(buffer); dataFile.print("-"); // Month
        toCharArray(GPS.day); dataFile.print(buffer); dataFile.print(" "); // Day
        toCharArray(GPS.hour); dataFile.print(buffer); toCharArray(':'); // Hour
        toCharArray(GPS.minute); dataFile.print(buffer); toCharArray(':'); // Minute
        toCharArray(GPS.seconds); dataFile.print(buffer); toCharArray('.'); // Second
        toCharArray(GPS.milliseconds); dataFile.print(buffer); dataFile.print(", "); // Milliseconds

        toCharArray((int)GPS.fix); dataFile.print(buffer); dataFile.print(", "); // Fix

        if (GPS.fix) {
            toCharArray((int)GPS.fixquality); dataFile.print(buffer); dataFile.print(", "); // Quality
            toCharArray(GPS.latitudeDegrees); dataFile.print(buffer); toCharArray(GPS.lat); dataFile.print(", "); // Latitude_Degrees
            toCharArray(GPS.longitudeDegrees); dataFile.print(buffer); toCharArray(GPS.lon); dataFile.print(", "); // Longitude_Degrees
            toCharArray(GPS.speed); dataFile.print(buffer); dataFile.print(", "); // Speed
            toCharArray(GPS.angle); dataFile.print(buffer); dataFile.print(", "); // Angle
            toCharArray(GPS.altitude); dataFile.print(buffer); dataFile.print(", "); // Altitude
            toCharArray((int)GPS.satellites); dataFile.print(buffer); dataFile.print(", "); // Sats
            toCharArray(GPS.magvariation); dataFile.print(buffer); dataFile.print(", "); // Magnetic_Variation
            toCharArray(GPS.HDOP); dataFile.print(buffer); dataFile.print(", "); // HDOP
            toCharArray(GPS.VDOP); dataFile.print(buffer); dataFile.print(", "); // VDOP
            toCharArray(GPS.PDOP); dataFile.print(buffer); dataFile.print(", "); // PDOP
        } else {
            dataFile.print(",,,,,,,,,,,");
        }

        // BNO055 Data
        bno.getEvent(&orientationData, Adafruit_BNO055::VECTOR_EULER);            // - VECTOR_EULER         - degrees
        bno.getEvent(&angVelocityData, Adafruit_BNO055::VECTOR_GYROSCOPE);        // - VECTOR_GYROSCOPE     - rad/s
        bno.getEvent(&linearAccelData, Adafruit_BNO055::VECTOR_LINEARACCEL);      // - VECTOR_LINEARACCEL   - m/s^2
        bno.getEvent(&magnetometerData, Adafruit_BNO055::VECTOR_MAGNETOMETER);    // - VECTOR_MAGNETOMETER  - uT
        bno.getEvent(&accelerometerData, Adafruit_BNO055::VECTOR_ACCELEROMETER);  // - VECTOR_ACCELEROMETER - m/s^2
        bno.getEvent(&gravityData, Adafruit_BNO055::VECTOR_GRAVITY);              // - VECTOR_GRAVITY       - m/s^2

        toCharArray(bno.getTemp()); dataFile.print(buffer); dataFile.print(", "); // BNO055_Temperature
        toCharArray(orientationData.orientation.x); dataFile.print(buffer); dataFile.print(", "); // Orientation_X
        toCharArray(orientationData.orientation.y); dataFile.print(buffer); dataFile.print(", "); // Orientation_Y
        toCharArray(orientationData.orientation.z); dataFile.print(buffer); dataFile.print(", "); // Orientation_Z

        toCharArray(angVelocityData.gyro.x); dataFile.print(buffer); dataFile.print(", "); // BNO055_Gyro_X
        toCharArray(angVelocityData.gyro.y); dataFile.print(buffer); dataFile.print(", "); // BNO055_Gyro_Y
        toCharArray(angVelocityData.gyro.z); dataFile.print(buffer); dataFile.print(", "); // BNO055_Gyro_Z

        toCharArray(linearAccelData.acceleration.x); dataFile.print(buffer); dataFile.print(", "); // Linear_Accel_X
        toCharArray(linearAccelData.acceleration.y); dataFile.print(buffer); dataFile.print(", "); // Linear_Accel_Y
        toCharArray(linearAccelData.acceleration.z); dataFile.print(buffer); dataFile.print(", "); // Linear_Accel_Z

        toCharArray(magnetometerData.magnetic.x); dataFile.print(buffer); dataFile.print(", "); // Mag_X
        toCharArray(magnetometerData.magnetic.y); dataFile.print(buffer); dataFile.print(", "); // Mag_Y
        toCharArray(magnetometerData.magnetic.z); dataFile.print(buffer); dataFile.print(", "); // Mag_Z

        toCharArray(accelerometerData.acceleration.x); dataFile.print(buffer); dataFile.print(", "); // BNO055_Accel_X
        toCharArray(accelerometerData.acceleration.y); dataFile.print(buffer); dataFile.print(", "); // BNO055_Accel_Y
        toCharArray(accelerometerData.acceleration.z); dataFile.print(buffer); dataFile.print(", "); // BNO055_Accel_Z

        toCharArray(gravityData.acceleration.x); dataFile.print(buffer); dataFile.print(", "); // Accel_Gravity_X
        toCharArray(gravityData.acceleration.y); dataFile.print(buffer); dataFile.print(", "); // Accel_Gravity_Y
        toCharArray(gravityData.acceleration.z); dataFile.print(buffer); dataFile.print(", "); // Accel_Gravity_Z

        // ISM330DLC Data
        ism330.getEvent(&ismAccelerometerData, &ismGyroData, &ismTemperatureData);

        toCharArray(ismTemperatureData.temperature); dataFile.print(buffer); dataFile.print(", "); // ISM330DLC_Temperature

        toCharArray(ismAccelerometerData.acceleration.x); dataFile.print(buffer); dataFile.print(", "); // ISM330DLC_Accel_X
        toCharArray(ismAccelerometerData.acceleration.y); dataFile.print(buffer); dataFile.print(", "); // ISM330DLC_Accel_Y
        toCharArray(ismAccelerometerData.acceleration.z); dataFile.print(buffer); dataFile.print(", "); // ISM330DLC_Accel_Z

        toCharArray(ismGyroData.gyro.x); dataFile.print(buffer); dataFile.print(", "); // ISM330DLC_Gyro_X
        toCharArray(ismGyroData.gyro.y); dataFile.print(buffer); dataFile.print(", "); // ISM330DLC_Gyro_Y
        toCharArray(ismGyroData.gyro.z); dataFile.print(buffer); dataFile.print(", "); // ISM330DLC_Gyro_Z

        // SCD41 Data
        scd41data = scd41.readMeasurement(co2Concentration, temperature, relativeHumidity);

        toCharArray(co2Concentration); dataFile.print(buffer); dataFile.print(", "); // CO2_Concentration
        toCharArray(temperature); dataFile.print(buffer); dataFile.print(", "); // SCD41_Temperature
        toCharArray(relativeHumidity); dataFile.print(buffer); dataFile.print(", "); // Relative_Humidity

        // AS7341 Data
        as7341.readAllChannels();

        toCharArray(as7341.getChannel(AS7341_CHANNEL_415nm_F1)); dataFile.print(buffer); dataFile.print(", "); // 415nm
        toCharArray(as7341.getChannel(AS7341_CHANNEL_445nm_F2)); dataFile.print(buffer); dataFile.print(", "); // 445nm
        toCharArray(as7341.getChannel(AS7341_CHANNEL_480nm_F3)); dataFile.print(buffer); dataFile.print(", "); // 480nm
        toCharArray(as7341.getChannel(AS7341_CHANNEL_515nm_F4)); dataFile.print(buffer); dataFile.print(", "); // 515nm
        toCharArray(as7341.getChannel(AS7341_CHANNEL_555nm_F5)); dataFile.print(buffer); dataFile.print(", "); // 555nm
        toCharArray(as7341.getChannel(AS7341_CHANNEL_590nm_F6)); dataFile.print(buffer); dataFile.print(", "); // 590nm
        toCharArray(as7341.getChannel(AS7341_CHANNEL_630nm_F7)); dataFile.print(buffer); dataFile.print(", "); // 630nm
        toCharArray(as7341.getChannel(AS7341_CHANNEL_680nm_F8)); dataFile.print(buffer); dataFile.print(", "); // 680nm

        toCharArray(as7341.getChannel(AS7341_CHANNEL_CLEAR)); dataFile.print(buffer); dataFile.print(", "); // Clear
        toCharArray(as7341.getChannel(AS7341_CHANNEL_NIR)); dataFile.print(buffer); dataFile.print(", "); // NIR

        // BMP390 Data
        toCharArray(baro.readAltitude(seaLevelPressure)); dataFile.print(buffer); dataFile.print(", "); // BMP390_Altitude
        toCharArray(baro.readPressure()); dataFile.print(buffer); dataFile.print(", "); // BMP390_Pressure
        toCharArray(baro.readTemperature()); dataFile.print(buffer); dataFile.print(", "); // BMP390_Temperature

        // LTR390 Data
        currentMillis = millis();
        if (currentMillis - lastMillis >= interval) {
            readingUV = !readingUV;
            if (readingUV) {
                ltr390.setMode(LTR390_MODE_UVS);
            } else {
                ltr390.setMode(LTR390_MODE_ALS);
            }
            lastMillis = currentMillis;
        }

        if (readingUV) {
            uvs = ltr390.readUVS();
            als = 0;
        } else {
            als = ltr390.readALS();
            uvs = 0;
        }

        toCharArray(uvs); dataFile.print(buffer); dataFile.print(", "); // UV Light
        toCharArray(als); dataFile.print(buffer); // Ambient Light

        // BME688 Data
        bme688.performReading();
        toCharArray(bme688.readTemperature()); dataFile.print(buffer); dataFile.print(", "); // BME688_Temperature
        toCharArray(bme688.readPressure() / 100.0F); dataFile.print(buffer); dataFile.print(", "); // BME688_Pressure
        toCharArray(bme688.readHumidity()); dataFile.print(buffer); dataFile.print(", "); // BME688_Humidity
        toCharArray(bme688.gas_resistance / 1000.0F); dataFile.print(buffer); dataFile.print(", "); // BME688_Gas_Resistances
        toCharArray(bme688.readAltitude(seaLevelPressure)); dataFile.print(buffer); dataFile.print(", "); // BME688_Altitude


        dataFile.println("");
        dataFile.flush();
        } else {
        Serial.println("Error writing to file");
    }

    // --------- //
    /* Telemetry */
    // --------- //

    Serial.print(millis()); Serial.print(", ");

    // GPS
    Serial.print("20"); Serial.print(GPS.year); Serial.print("-");
    Serial.print(GPS.month); Serial.print("-");
    Serial.print(GPS.day); Serial.print(" ");
    Serial.print(GPS.hour); Serial.print(':');
    Serial.print(GPS.minute); Serial.print(':');
    Serial.print(GPS.seconds); Serial.print('.');
    Serial.print(GPS.milliseconds); Serial.print(", ");
    Serial.print((int)GPS.fix); Serial.print(", ");
    Serial.print((int)GPS.fixquality); Serial.print(", ");
    if(GPS.fix){
        Serial.print(GPS.latitudeDegrees, 6); Serial.print(GPS.lat); Serial.print(", ");
        Serial.print(GPS.longitudeDegrees, 6); Serial.print(GPS.lon); Serial.print(", ");
        Serial.print(GPS.speed); Serial.print(", ");
        Serial.print(GPS.angle); Serial.print(", ");
        Serial.print(GPS.altitude); Serial.print(", ");
        Serial.print((int)GPS.satellites); Serial.print(", ");
    } else {
        Serial.print(",,,,,,");
    }

    // BNO055
    Serial.print(linearAccelData.acceleration.x); Serial.print(", ");
    Serial.print(linearAccelData.acceleration.y); Serial.print(", ");
    Serial.print(linearAccelData.acceleration.z); Serial.print(", ");

    // ISM 330DLC
    ism330.getEvent(&ismAccelerometerData, &ismGyroData, &ismTemperatureData);
    Serial.print(ismAccelerometerData.acceleration.x); Serial.print(", ");
    Serial.print(ismAccelerometerData.acceleration.y); Serial.print(", ");
    Serial.print(ismAccelerometerData.acceleration.z); Serial.print(", ");

    // SCD41
    scd41.readMeasurement(co2Concentration, temperature, relativeHumidity);

    Serial.print(co2Concentration); Serial.print(", ");
    Serial.print(temperature); Serial.print(", ");

    // BMP390
    Serial.print(baro.readAltitude(seaLevelPressure)); Serial.print(", ");
    Serial.print(baro.readPressure()); Serial.print(", ");
    Serial.print(baro.readTemperature()); Serial.print(", ");

    // LTR390
    Serial.print(uvs); Serial.print(", ");
    Serial.print(als);

    Serial.println();

    // ----- //
    // Other //
    // ----- //

    // Blink the LED
    if (ledCurrentlyShowing) {
        led.setPixelColor(0, 0, 0, 0);
    } else {
        if (showErrorLed) {
            led.setPixelColor(0, 0, 0, 255);

            Serial.print(errorMsg);
        } else {
            led.setPixelColor(0, 255, 0, 0);
        }
    }

    led.show();
    ledCurrentlyShowing = !ledCurrentlyShowing;
    updateGPS();
}