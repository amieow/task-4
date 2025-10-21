#include "main.h" // (atau header Anda)

const int mac_index_ku = 1; // Contoh: Bridge adalah "JSON"
void setup()
{
    Serial.begin(115200);
    Serial.println("\nBooting ESP32-Bridge...");

    if (mulai_esp_now(mac_index_ku) != ESP_OK)
    {
        Serial.println("Gagal Inisialisasi ESP-NOW");
        ESP.restart();
    }

    Serial.println("Bridge Siap. Menerima data dari Serial...");
}

void loop()
{
    // Cek data dari serial
    baca_serial(callback_data_serial);
}