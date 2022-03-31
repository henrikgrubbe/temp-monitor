
#include <InfluxDbClient.h>
#include <ESP8266WiFi.h>
#include <Arduino.h>
#include <DHT.h>

InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN);

#define DHT_PIN 13
DHT sensor(DHT_PIN, DHT22);

typedef struct Sample {
  double temperature;
  double humidity;
} Sample;

Sample samples[AVERAGED_SAMPLES];

void setup() {
  Serial.begin(115200);
  while(!Serial) { }

  Serial.println();
  Serial.println("init!");

  pinMode(LED_BUILTIN, OUTPUT);
  

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  sensor.begin();
}

void loop() {
  bool wifiConnected = WiFi.status() != WL_CONNECTED;
  digitalWrite(LED_BUILTIN, !wifiConnected);

  static long lastLoop = 0;

  if(millis()-lastLoop < SAMPLE_PERIOD) return;
  

  Sample sample = { .temperature = sensor.readTemperature(false)
                  , .humidity    = sensor.readHumidity()
                  };
  // Check if any reads failed and exit early (to try again).
  
  if (isnan(sample.humidity) || isnan(sample.temperature)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  static int sampleIndex = 0;
  Serial.printf("Sample %d: temp %f - humid %f\n", sampleIndex+1, sample.temperature, sample.humidity);
  samples[sampleIndex++] = sample;



  if(sampleIndex == AVERAGED_SAMPLES) {
    sampleIndex = 0;
    while(WiFi.status() != WL_CONNECTED);

    Sample sum = { .temperature = 0, .humidity = 0 };
    for(int i = 0; i < AVERAGED_SAMPLES; i++) {
      sum.temperature += samples[i].temperature;
      sum.humidity    += samples[i].humidity;
    }
    sum.temperature /= AVERAGED_SAMPLES;
    sum.humidity    /= AVERAGED_SAMPLES;

    Point sensor("temp_humid");
    sensor.addField("degC", sum.temperature);
    sensor.addField("rel%", sum.humidity);
    
    // Print what are we exactly writing
    Serial.print("Writing: ");
    Serial.println(client.pointToLineProtocol(sensor));
    
    // Write point
    while (!client.writePoint(sensor)) {
      Serial.print("InfluxDB write failed: ");
      Serial.println(client.getLastErrorMessage());
      delay(100);
    }

  }

  lastLoop = millis();
}