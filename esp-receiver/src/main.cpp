#include "main.h"
#include "CommandHandler.h"
const int mac_index_ku = 3; //....

void setup()
{
    Serial.begin(115200);
    Serial.println("Menyalakan ESP-NOW");
    if (mulai_esp_now(mac_index_ku) != ESP_OK)
    {
        Serial.println("Gagal Inisialisasi ESP-NOW");
        ESP.restart();
    }
    if (!SPIFFS.begin(true))
    {
        Serial.println("FATAL: Gagal me-mount SPIFFS!");
        Serial.println("Restarting...");
        delay(1000);
        ESP.restart();
    }

    Serial.println("Menunggu perintah...");
}
void loop()
{
    if (Serial.available())
    {
        baca_serial(callback_data_serial);
    }
}