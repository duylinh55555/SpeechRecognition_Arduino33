#include <math.h>
#include <PDM.h>
#include <EloquentTinyML.h>
#include "model.h"


#define PDM_SOUND_GAIN     255 
#define PDM_BUFFER_SIZE    256

#define SAMPLE_THRESHOLD   900  
#define FEATURE_SIZE       32   
#define SAMPLE_DELAY       20  

#define NUMBER_OF_LABELS   4  

const String words[NUMBER_OF_LABELS] = {"Bat", "Tat", "Mo", "Xin chao"};

#define PREDIC_THRESHOLD   0.6 
#define RAW_OUTPUT         true 
#define NUMBER_OF_INPUTS   FEATURE_SIZE
#define NUMBER_OF_OUTPUTS  NUMBER_OF_LABELS
#define TENSOR_ARENA_SIZE  4 * 1024


Eloquent::TinyML::TfLite<NUMBER_OF_INPUTS, NUMBER_OF_OUTPUTS, TENSOR_ARENA_SIZE> model;
float feature_data[FEATURE_SIZE];
volatile float rms;
bool voice_detected;

void onPDMdata() 
{
  rms = -1;
  short sample_buffer[PDM_BUFFER_SIZE];
  int bytes_available = PDM.available();
  PDM.read(sample_buffer, bytes_available);

  unsigned int sum = 0;
  for (unsigned short i = 0; i < (bytes_available / 2); i++)
    sum += pow(sample_buffer[i], 2);

  rms = sqrt(float(sum) / (float(bytes_available) / 2.0));
}

void setup() 
{
  Serial.begin(115200);
  while (!Serial);

  PDM.onReceive(onPDMdata);
  PDM.setBufferSize(PDM_BUFFER_SIZE);
  PDM.setGain(PDM_SOUND_GAIN);

  if (!PDM.begin(1, 16000)) 
  {
    Serial.println("Failed to start PDM!");
    while (1);
  }

  pinMode(LED_BUILTIN, OUTPUT);

  delay(900);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);

  model.begin((unsigned char*) model_data);
  Serial.println("=== Classifier start ===\n");
}

void loop() 
{
  while (rms < SAMPLE_THRESHOLD);

  digitalWrite(LED_BUILTIN, HIGH);
  for (int i = 0; i < FEATURE_SIZE; i++) 
  {
    while (rms < 0);
    feature_data[i] = rms;
    delay(SAMPLE_DELAY);
  }

  digitalWrite(LED_BUILTIN, LOW);

  float prediction[NUMBER_OF_LABELS];
  model.predict(feature_data, prediction);

  Serial.println("Predicting the word:");
  if (RAW_OUTPUT) 
  {
    for (int i = 0; i < NUMBER_OF_LABELS; i++) 
    {
      Serial.print("Label ");
      Serial.print(i);
      Serial.print(" = ");
      Serial.println(prediction[i]);
    }
  }

  voice_detected = false;
  for (int i = 0; i < NUMBER_OF_LABELS; i++) 
  {
    if (prediction[i] >= PREDIC_THRESHOLD) 
    {
      Serial.print("Word detected: ");
      Serial.println(words[i]);
      Serial.println("");
      voice_detected = true;
    }
  }

  if (!voice_detected && !RAW_OUTPUT) 
    Serial.println("Word not recognized\n");

  delay(900);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
}