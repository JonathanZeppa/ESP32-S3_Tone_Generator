//  This simple tone generator allows you to dial a note with a rotary encoder, 
//  and play and stop the note with a momentary pushbutton. An OLED displays the note name. 
// The note name and frequency are displayed in the serial monitor.

#include <Arduino.h>
#include <Bounce2.h> // thomasfredericks/Bounce2@^2.72
#include <ESP32Encoder.h> // madhephaestus/ESP32Encoder@^0.11.6
#include <Adafruit_SSD1306.h> // adafruit/Adafruit SSD1306@^2.5.10
#include <Wire.h>
#include <driver/i2s.h> // ESP32 I2S driver earlephilhower/ESP8266Audio@^1.9.7

// Pin for the momentary pushbutton
const int buttonPin = 18;
#define ENCODER_PIN1 47
#define ENCODER_PIN2 48

// OLED display parameters
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1  // Reset pin (or -1 if sharing Arduino reset pin)
#define SDA_PIN 17     // Change to your specific pin
#define SCL_PIN 16     // Change to your specific pin

// I2S configuration
#define I2S_BCK_PIN 15
#define I2S_WS_PIN 13
#define I2S_DATA_PIN 14
#define SAMPLE_RATE 44100
#define AMPLITUDE 10000

// Button object
Bounce b01 = Bounce();

// Note names
String Key1[] = {
  "C -1", "C#-1", "D -1", "D#-1", "E -1", "F -1", "F#-1", "G -1", "G#-1", "A -1", "A#-1", "B -1",
  "C 0", "C#0", "D 0", "D#0", "E 0", "F 0", "F#0", "G 0", "G#0", "A 0", "A#0", "B 0",
  "C 1", "C#1", "D 1", "D#1", "E 1", "F 1", "F#1", "G 1", "G#1", "A 1", "A#1", "B 1",
  "C 2", "C#2", "D 2", "D#2", "E 2", "F 2", "F#2", "G 2", "G#2", "A 2", "A#2", "B 2",
  "C 3", "C#3", "D 3", "D#3", "E 3", "F 3", "F#3", "G 3", "G#3", "A 3", "A#3", "B 3",
  "C 4", "C#4", "D 4", "D#4", "E 4", "F 4", "F#4", "G 4", "G#4", "A 4", "A#4", "B 4",
  "C 5", "C#5", "D 5", "D#5", "E 5", "F 5", "F#5", "G 5", "G#5", "A 5", "A#5", "B 5",
  "C 6", "C#6", "D 6", "D#6", "E 6", "F 6", "F#6", "G 6", "G#6", "A 6", "A#6", "B 6",
  "C 7", "C#7", "D 7", "D#7", "E 7", "F 7", "F#7", "G 7", "G#7", "A 7", "A#7", "B 7",
  "C 8", "C#8", "D 8", "D#8", "E 8", "F 8", "F#8", "G 8", "G#8", "A 8", "A#8", "B 8",
  "C 9", "C#9", "D 9", "D#9", "E 9", "F 9"
};

// Frequency table
const float freq[128] = {
  // Frequencies from C-1 to B8
  8.176, 8.662, 9.177, 9.723, 10.301, 10.913, 11.562, 12.25, 12.978, 13.75, 14.568, 15.434, 
  16.352, 17.324, 18.354, 19.445, 20.602, 21.827, 23.125, 24.5, 25.957, 27.5, 29.135, 30.868,
  32.703, 34.648, 36.708, 38.891, 41.203, 43.654, 46.249, 48.999, 51.913, 55.0, 58.27, 61.735,
  65.406, 69.296, 73.416, 77.782, 82.407, 87.307, 92.499, 97.999, 103.826, 110.0, 116.541, 123.471,
  130.813, 138.591, 146.832, 155.563, 164.814, 174.614, 184.997, 195.998, 207.652, 220.0, 233.082, 246.942,
  261.626, 277.183, 293.665, 311.127, 329.628, 349.228, 369.994, 391.995, 415.305, 440.0, 466.164, 493.883,
  523.251, 554.365, 587.33, 622.254, 659.255, 698.456, 739.989, 783.991, 830.609, 880.0, 932.328, 987.767,
  1046.502, 1108.731, 1174.659, 1244.508, 1318.51, 1396.913, 1479.978, 1567.982, 1661.219, 1760.0, 1864.655, 1975.533,
  2093.005, 2217.461, 2349.318, 2489.016, 2637.02, 2793.826, 2959.955, 3135.963, 3322.438, 3520.0, 3729.31, 3951.066,
  4186.009, 4434.922, 4698.636, 4978.032, 5274.041, 5587.652, 5919.911, 6271.927, 6644.875, 7040.0, 7458.62, 7902.133,
  8372.018, 8869.844, 9397.273, 9956.063, 10548.08, 11175.3, 11839.82, 12543.85
};

// Define the bounds for the encoder
const int encoderMinIndex = 24; // 24 is the index for C1
const int encoderMaxIndex = 96; // 96 is the index for C7
const int startEncoderIndex = 60;  // 60 is the index for C4

ESP32Encoder r01;
int currentKeyIndex = startEncoderIndex;

// Initialize the OLED display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

bool tonePlaying = false;
float hz = 440.0;
double phase = 0.0;
const double two_pi = 6.283185307179586476925286766559;

void initializeEncoder(ESP32Encoder &encoder, int pin1, int pin2) {
  ESP32Encoder::useInternalWeakPullResistors = puType::up;
  encoder.attachSingleEdge(pin1, pin2);
  encoder.clearCount();
  encoder.setCount(currentKeyIndex); // Start the encoder at index 36
}

void setupI2S() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCK_PIN,
    .ws_io_num = I2S_WS_PIN,
    .data_out_num = I2S_DATA_PIN,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);
}

void generateAndPlayTone(float frequency) {
  hz = frequency;
  phase = 0; // reset phase for a new tone
  Serial.print("Playing tone: ");
  Serial.print(Key1[currentKeyIndex]);
  Serial.print(" at frequency: ");
  Serial.println(hz);
}

void stopTone() {
  i2s_zero_dma_buffer(I2S_NUM_0);
  Serial.println("Tone Stopped");
}

void setup() {
  // Initialize serial communication
  Serial.begin(115200);

  // Setup the pushbutton pin as an input with an internal pull-up resistor
  pinMode(buttonPin, INPUT_PULLUP);

  // Attach the debouncer to the pushbutton pin
  b01.attach(buttonPin);
  b01.interval(5); // Debounce interval in milliseconds

  // Initialize the encoder
  initializeEncoder(r01, ENCODER_PIN1, ENCODER_PIN2);

  // Initialize I2C for the OLED display
  Wire.begin(SDA_PIN, SCL_PIN);

  // Initialize the OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }

  // Clear the buffer immediately after initializing the display
  display.clearDisplay();
  display.display();

  // Clear the buffer and set initial display parameters
  display.clearDisplay();
  display.setTextSize(4);      // Set text size to 4
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner
  display.println(Key1[currentKeyIndex]);
  display.display();

  // Initialize the audio output
  setupI2S();
}

void updateDisplay() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(Key1[currentKeyIndex]);
  display.display();
}

void loop() {
  // Update the Bounce instance
  b01.update();

  // Check if the button was pressed
  if (b01.fell()) {
    tonePlaying = !tonePlaying;
    if (tonePlaying) {
      generateAndPlayTone(freq[currentKeyIndex]);
    } else {
      stopTone();
    }
  }

  // Check the encoder
  int newPosition = r01.getCount();
  if (newPosition != currentKeyIndex) {
    currentKeyIndex = newPosition;

    // Ensure circular navigation through Key1 array
    if (currentKeyIndex < 12) {
      currentKeyIndex = 96;
      r01.clearCount();
      r01.setCount(currentKeyIndex);
    } else if (currentKeyIndex > 96) {
      currentKeyIndex = 12;
      r01.clearCount();
      r01.setCount(currentKeyIndex);
    }

    Serial.print("Encoder Position: ");
    Serial.print(currentKeyIndex);
    Serial.print(" Key: ");
    Serial.println(Key1[currentKeyIndex]);
    Serial.print(" Frequency: ");
    Serial.println(freq[currentKeyIndex]);

    // Update the OLED display
    updateDisplay();

    if (tonePlaying) {
      generateAndPlayTone(freq[currentKeyIndex]);
    }
  }

  // Continuously generate and play tone if tonePlaying is true
  if (tonePlaying) {
    const double increment = two_pi * hz / SAMPLE_RATE;
    int16_t sample;
    size_t bytes_written;

    for (int i = 0; i < 256; ++i) {
      sample = (int16_t)(AMPLITUDE * sin(phase));
      phase += increment;
      if (phase >= two_pi) {
        phase -= two_pi;
      }
      int16_t samples[2] = { sample, sample }; // Stereo output
      i2s_write(I2S_NUM_0, samples, sizeof(samples), &bytes_written, portMAX_DELAY);
    }
  }
}
