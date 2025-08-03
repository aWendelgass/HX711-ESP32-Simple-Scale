#include <HX711_ADC.h>
#include <Preferences.h>

// GPIOs für HX711
const int HX711_dout = 25;
const int HX711_sck = 27;

// HX711-Instanz
HX711_ADC LoadCell(HX711_dout, HX711_sck);

// Genauigkeit in Gramm
const float GENAUIGKEIT = 10.0;

// Flash-Speicher
Preferences prefs;

// Variablen
float calFactor = 0.0;
float lastWeight = 0.0;
bool isCalibrated = false;

void kalibriereWaage() {
  Serial.println("Nimm alles von der Waage.");
  delay(5000);

  LoadCell.tare();
  Serial.println("Tare abgeschlossen.");

  Serial.println("Lege nun ein 1 kg Gewicht auf die Waage.");
  delay(10000);

  LoadCell.refreshDataSet();
  calFactor = LoadCell.getNewCalibration(1000.0); // 1000 g Referenzgewicht
  LoadCell.setCalFactor(calFactor);

  long tareOffset = LoadCell.getTareOffset();

  // Speichern im Flash
  prefs.begin("waage", false); // Schreibmodus
  prefs.putFloat("calFactor", calFactor);
  prefs.putLong("tareOffset", tareOffset);
  prefs.end();

  Serial.print("Kalibrierung abgeschlossen. Faktor: ");
  Serial.println(calFactor, 5);
  Serial.print("Tare Offset: ");
  Serial.println(tareOffset);

  isCalibrated = true;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Starte Waage...");

  LoadCell.begin();
  LoadCell.setSamplesInUse(16);
  LoadCell.start(2000); // Vorwärmzeit

  // Lade Kalibrierungsdaten aus Flash
  prefs.begin("waage", true); // Lesemodus
  calFactor = prefs.getFloat("calFactor", 0.0);
  long tareOffset = prefs.getLong("tareOffset", 0);
  prefs.end();

  if (calFactor > 0.0 && calFactor < 100000.0) {
    LoadCell.setCalFactor(calFactor);
    LoadCell.setTareOffset(tareOffset);
    isCalibrated = true;
    Serial.println("Die Waage ist kalibriert.");
  } else {
    Serial.println("Kalibrierung erforderlich.");
    kalibriereWaage();
  }

  Serial.println("Starte Messung...");
}

void loop() {
  static unsigned long lastUpdate = 0;
  LoadCell.update();

  if (millis() - lastUpdate > 500) {
    float gewicht = LoadCell.getData();
    float gewichtKg = gewicht / 1000.0;

    // Kaufmännisches Runden auf Genauigkeit
    float rundung = GENAUIGKEIT / 1000.0; // z. B. 0.1 kg bei 100 g
    float gewichtGerundet = round(gewichtKg / rundung) * rundung;

    // Negative Null vermeiden
    if (abs(gewichtGerundet) < rundung / 2.0) {
      gewichtGerundet = 0.0;
    }

    if (abs(gewicht - lastWeight) >= GENAUIGKEIT) {
      int dezimalstellen = (GENAUIGKEIT < 100.0) ? 2 : 1;

      Serial.print("Gewicht: ");
      Serial.print(gewichtGerundet, dezimalstellen);
      Serial.println(" kg");

      lastWeight = gewicht;
    }

    lastUpdate = millis();
  }
}
