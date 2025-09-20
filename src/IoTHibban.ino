// Project: Temperature & Humidity Monitor with WhatsApp Chatbot + Auto Alert + Blynk Dashboard + Buzzer
// Board   : NodeMCU ESP8266
// Sensor  : DHT11 on Pin D4 (GPIO2)

#define BLYNK_TEMPLATE_ID "TMPL6zCVsOGI-"
#define BLYNK_TEMPLATE_NAME "IoTHibban"
#define BLYNK_AUTH_TOKEN "2sIgjM_0fBGpNoGFeuAIYImnoB8fQyBJ"

#include <ESP8266WiFi.h>
#include <ThingESP.h>
#include <DHT.h>
#include <BlynkSimpleEsp8266.h>

#define DHTPIN D4
#define DHTTYPE DHT11     
DHT dht(DHTPIN, DHTTYPE);

// --- ThingESP Config (WhatsApp) ---
ThingESP8266 thing("HibbanRdn", "TemperatureIot", "techhibban");

// --- Blynk Config ---
char auth[] = BLYNK_AUTH_TOKEN;

// --- LED Indikator Suhu ---
#define LED_RENDAH D5
#define LED_SEDANG D6
#define LED_TINGGI D7

// --- LED AC & Kulkas ---
#define LED_AC     D2
#define LED_KULKAS D3

// --- Buzzer ---
#define BUZZER_PIN D8

// --- Variabel sistem ---
unsigned long previousMillis = 0;
const long INTERVAL = 5000;
bool autoAlert = false;        
bool alertSent = false;        

// status perangkat
String statusAC = "OFF";
String statusKulkas = "OFF";

// --- Handle kontrol AC & Kulkas dari Blynk ---
BLYNK_WRITE(V3) {   // Switch AC
  int state = param.asInt();
  digitalWrite(LED_AC, state);
  statusAC = (state == 1) ? "ON ✅" : "OFF ❌";
  updateStatusPerangkat();
}

BLYNK_WRITE(V4) {   // Switch Kulkas
  int state = param.asInt();
  digitalWrite(LED_KULKAS, state);
  statusKulkas = (state == 1) ? "ON ✅" : "OFF ❌";
  updateStatusPerangkat();
}

// --- Update Label Status Perangkat (AC & Kulkas) ---
void updateStatusPerangkat() {
  String status = "AC: " + statusAC + "\nKulkas: " + statusKulkas;
  Blynk.virtualWrite(V5, status);
}

void setup() {
  Serial.begin(9600);
  Serial.println("Initializing...");

  dht.begin();

  // Inisialisasi LED & Buzzer
  pinMode(LED_RENDAH, OUTPUT);
  pinMode(LED_SEDANG, OUTPUT);
  pinMode(LED_TINGGI, OUTPUT);
  pinMode(LED_AC, OUTPUT);
  pinMode(LED_KULKAS, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // --- WiFi untuk ThingESP & Blynk ---
  thing.SetWiFi("hib", "12345678");
  thing.initDevice();
  Blynk.begin(auth, "hib", "12345678");

  // status awal perangkat
  updateStatusPerangkat();

  Serial.println("Device started. Ready for WhatsApp & Blynk!");
}

// Fungsi respon query WhatsApp
String HandleResponse(String query) {
  query.toLowerCase();

  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float f = dht.readTemperature(true);

  if (isnan(h) || isnan(t)) {
    return "⚠️ *Sensor Error!* Silakan coba lagi.";
  }

  String humid = "🌫 *Humidity* : " + String(h) + " %\n";
  String temp  = "🌡 *Temperature* : " + String(t) + "°C | " + String(f) + "°F\n";

  if (query == "menu" || query == "help") {
    return "📋 *IoT Chatbot Menu* \n\n"
           "1️⃣ *temperature* → Cek Suhu 🌡\n"
           "2️⃣ *humidity* → Cek Kelembapan 🌫\n"
           "3️⃣ *about* → Info Perangkat 🤖\n"
           "4️⃣ *alert on* → Aktifkan auto alert 🔔\n"
           "5️⃣ *alert off* → Nonaktifkan auto alert 🚫\n";
  }
  else if (query == "temperature") {
    return "✅ Data Suhu:\n" + temp;
  }
  else if (query == "humidity") {
    return "✅ Data Kelembapan:\n" + humid;
  }
  else if (query == "about") {
    return "🤖 *Device Info*\n"
           "Board   : NodeMCU ESP8266\n"
           "Sensor  : DHT11\n"
           "Fitur   : Monitoring Suhu & Kelembapan + WhatsApp & Blynk\n";
  }
  else if (query == "alert on") {
    autoAlert = true;
    return "🔔 *Auto Alert* suhu > 31°C telah *AKTIF*.";
  }
  else if (query == "alert off") {
    autoAlert = false;
    return "🚫 *Auto Alert* suhu telah *NONAKTIF*.";
  }
  else {
    return "❌ *Perintah tidak dikenali.*\n"
           "Ketik *menu* untuk daftar perintah.";
  }
}

void loop() {
  thing.Handle();
  Blynk.run();

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= INTERVAL) {
    previousMillis = currentMillis;

    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (!isnan(t) && !isnan(h)) {
      Serial.print("Temperature: ");
      Serial.print(t);
      Serial.print(" °C   ||   Humidity: ");
      Serial.print(h);
      Serial.println(" %");

      // --- Kirim ke Blynk Gauge ---
      Blynk.virtualWrite(V0, t);  
      Blynk.virtualWrite(V1, h);  

      // --- LED indikator suhu & Buzzer ---
      if (t < 25) {
        digitalWrite(LED_RENDAH, HIGH);
        digitalWrite(LED_SEDANG, LOW);
        digitalWrite(LED_TINGGI, LOW);
        Blynk.virtualWrite(V2, "Suhu: RENDAH 🔵");
        noTone(BUZZER_PIN);
      }
      else if (t >= 25 && t < 30) {
        digitalWrite(LED_RENDAH, LOW);
        digitalWrite(LED_SEDANG, HIGH);
        digitalWrite(LED_TINGGI, LOW);
        Blynk.virtualWrite(V2, "Suhu: SEDANG 🟢");
        noTone(BUZZER_PIN);
      }
      else {
        digitalWrite(LED_RENDAH, LOW);
        digitalWrite(LED_SEDANG, LOW);
        digitalWrite(LED_TINGGI, HIGH);
        Blynk.virtualWrite(V2, "Suhu: TINGGI 🔴");

        // --- Buzzer nit nit nit ---
        for (int i = 0; i < 3; i++) {
          tone(BUZZER_PIN, 2000);  
          delay(1000);
          noTone(BUZZER_PIN);
          delay(500);
        }
      }

      // --- Auto Alert WhatsApp ---
      if (autoAlert && t > 31 && !alertSent) {
        thing.sendMsg("+6289531861388", 
                      "⚠️ *ALERT SUHU TINGGI!* ⚠️\n\n"
                      "🌡 Suhu sekarang: *" + String(t) + "°C*\n"
                      "📌 Batas aman: *31°C*\n\n"
                      "Segera cek ruangan Anda! 🏠");
        Serial.println("✅ WA Alert dikirim!");
        alertSent = true;
      } 
      else if (t <= 31) {
        alertSent = false;
      }
    }
  }
}
