#include "main.h"
#include "CommandHandler.h"

esp_now_peer_info_t peer_info;

uint8_t mac_addresses[MAC_ADDRESS_TOTAL][MAC_ADDRESS_LENGTH] = {
    {0x24, 0x0A, 0xC4, 0x0A, 0x21, 0x11}, // BISMA
    {0x24, 0x0A, 0xC4, 0x0A, 0x21, 0x10}, // JSON
    {0x24, 0x0A, 0xC4, 0x0A, 0x20, 0x11}, // FARUG
    {0x24, 0x0A, 0xC4, 0x0A, 0x10, 0x10}, // Fauzan Firdaus
    {0x24, 0x0A, 0xC4, 0x0A, 0x10, 0x11}, // Africha Sekar wangi
    {0x24, 0x0A, 0xC4, 0x0A, 0x11, 0x10}, // Rafaina Erin Sadia
    {0x24, 0x0A, 0xC4, 0x0A, 0x11, 0x11}, // Antonius Michael Yordanis Hartono
    {0x24, 0x0A, 0xC4, 0x0A, 0x12, 0x10}, // Dinda Sofi Azzahro
    {0x24, 0x0A, 0xC4, 0x0A, 0x12, 0x11}, // Muhammad Fahmi Ilmi
    {0x24, 0x0A, 0xC4, 0x0A, 0x13, 0x10}, // Dhanishara Zaschya Putri Syamsudin
    {0x24, 0x0A, 0xC4, 0x0A, 0x13, 0x11}, // Irsa Fairuza
    {0x24, 0x0A, 0xC4, 0x0A, 0x14, 0x10}, // Revalinda Bunga Nayla Laksono

};

const char *mac_names[MAC_ADDRESS_TOTAL] = {
    "BISMA",                              // 0
    "JSON",                               // 1
    "FARUG",                              // 2
    "Fauzan Firdaus",                     // 3
    "Africha Sekar Wangi",                // 4
    "Rafaina Erin Sadia",                 // 5
    "Antonius Michael Yordanis Hartono",  // 6
    "Dinda Sofi Azzahro",                 // 7
    "Muhammad Fahmi Ilmi",                // 8
    "Dhanishara Zaschya Putri Syamsudin", // 9
    "Irsa Fairuza",                       // 10
    "Revalinda Bunga Nayla Laksono",      // 11
};

esp_err_t mulai_esp_now(int index_mac_address)
{
   WiFi.mode(WIFI_STA);
   WiFi.disconnect();

   esp_err_t result = esp_now_init();
   if (result != ESP_OK)
      return result;

   result = esp_now_register_recv_cb(callback_data_esp_now);
   if (result != ESP_OK)
      return result;

   result = esp_now_register_send_cb(callback_pengiriman_esp_now);
   //     if (result != ESP_OK)
   //         return result;

   uint8_t mac[MAC_ADDRESS_LENGTH];
   memcpy(mac, mac_addresses[index_mac_address], MAC_ADDRESS_LENGTH);
   result = esp_wifi_set_mac(WIFI_IF_STA, mac);
   if (result != ESP_OK)
      return result;

   memset(&peer_info, 0, sizeof(esp_now_peer_info_t));
   peer_info.channel = 0;
   peer_info.encrypt = false;

   for (int i = 0; i < MAC_ADDRESS_TOTAL; i++)
   {
      memcpy(peer_info.peer_addr, mac_addresses[i], MAC_ADDRESS_LENGTH);
      result = esp_now_add_peer(&peer_info);
      if (result != ESP_OK)
         return result;
   }

   return ESP_OK;
}

int cari_mac_index(const uint8_t *mac)
{
   for (int i = 0; i < MAC_ADDRESS_TOTAL; i++)
   {
      if (memcmp(mac, mac_addresses[i], MAC_ADDRESS_LENGTH) == 0)
         return i;
   }

   return -1;
}

String mac_index_to_names(int mac_index)
{
   if ((mac_index < 0 || mac_index >= MAC_ADDRESS_TOTAL) || (mac_index == -1))
   {
      return "Unknown";
   }
   return mac_names[mac_index];
}

void callback_data_esp_now(const uint8_t *mac, const uint8_t *data, int len)
{
   int index_mac_asal = cari_mac_index(mac);
   process_perintah(data, len, index_mac_asal);
}
void callback_pengiriman_esp_now(const uint8_t *mac_addr, esp_now_send_status_t status)
{
   Serial.printf("\nStatus pengiriman ESP-NOW: %s\n", esp_err_to_name(status));
}
void callback_data_serial(const uint8_t *data, int len)
{
   process_perintah(data, len);
}

void baca_serial(void (*callback)(const uint8_t *data, int len))
{
   static uint8_t buffer[256]; // max uint8_t
   static int index = 0;
   static int dataLen = -1;
   while (Serial.available() > 0)
   {
      uint8_t byteReaded = Serial.read();
      // Serial.println("test : " + String(byteReaded));
      buffer[index++] = byteReaded;
      if (index >= 256)
      {
         index = 0;
         dataLen = -1;
         continue;
      }
      if (index == 3)
      {
         if (buffer[0] != 0xFF || buffer[1] != 0xFF || buffer[2] != 0x00)
         {
            index = 0;
            continue;
         }
      }
      if (index == 4)
      {
         dataLen = buffer[3];
         if (dataLen <= 0 or dataLen >= 256 - 4)
         {
            index = 0;
            dataLen = -1;
            continue;
         }
      }
      if (dataLen != -1 && index >= 4 + dataLen)
      {
         int total_len = 4 + dataLen;

         callback(&buffer[4], dataLen);

         index = 0;
         dataLen = -1;
      }
   }
}
CommandHandler myCommandHandler;

void process_perintah(const uint8_t *data, int len, int index_mac_address_asal)
{
   myCommandHandler.processCommand(data, len, index_mac_address_asal);
}