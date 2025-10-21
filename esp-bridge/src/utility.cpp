#include "main.h"

esp_now_peer_info_t peer_info;

int MAC_RECEIVER_INDEX = 3;

// Ini adalah daftar MAC untuk node-node Anda (termasuk Bridge ini)
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

   /* Init ESP-NOW */
   esp_err_t result = esp_now_init();
   if (result != ESP_OK)
      return result;

   /* Set callback function to handle received data */
   result = esp_now_register_recv_cb(callback_data_esp_now);
   if (result != ESP_OK)
      return result;

   result = esp_now_register_send_cb(callback_pengiriman_esp_now);
   if (result != ESP_OK)
      return result; // (Saya uncomment ini, sebaiknya dicek)

   /* Set MAC Address */
   uint8_t mac[MAC_ADDRESS_LENGTH];
   memcpy(mac, mac_addresses[index_mac_address], MAC_ADDRESS_LENGTH);
   result = esp_wifi_set_mac(WIFI_IF_STA, mac);
   if (result != ESP_OK)
      return result;

   /* Initialize peer_info and set fields*/
   memset(&peer_info, 0, sizeof(esp_now_peer_info_t));
   peer_info.channel = 0;
   peer_info.encrypt = false;

   // ===================================================================
   // PERUBAHAN KRITIS: Daftarkan Receiver Utama Anda sebagai Peer
   // ===================================================================
   memcpy(peer_info.peer_addr, mac_addresses[MAC_RECEIVER_INDEX], MAC_ADDRESS_LENGTH);
   result = esp_now_add_peer(&peer_info);
   if (result != ESP_OK)
   {
      Serial.println("FATAL: Gagal mendaftarkan Receiver Utama sebagai peer.");
      return result;
   }
   // ===================================================================

   /* Add All MAC lain ke peer list (Opsional, tapi biarkan) */
   for (int i = 0; i < MAC_ADDRESS_TOTAL; i++)
   {
      // Hindari mendaftarkan peer yang sama dua kali
      if (memcmp(mac_addresses[i], mac_addresses[MAC_RECEIVER_INDEX], MAC_ADDRESS_LENGTH) != 0)
      {
         memcpy(peer_info.peer_addr, mac_addresses[i], MAC_ADDRESS_LENGTH);
         result = esp_now_add_peer(&peer_info);
         if (result != ESP_OK && result != ESP_ERR_ESPNOW_EXIST) // ESP_ERR_ESPNOW_EXIST = Peer sudah ada
         {
            return result;
         }
      }
   }

   return ESP_OK;
}

int cari_mac_index(const uint8_t *mac)
{
   for (int i = 0; i < MAC_ADDRESS_TOTAL; i++)
   {
      // Compare the MAC address
      if (memcmp(mac, mac_addresses[i], MAC_ADDRESS_LENGTH) == 0)
         return i;
   }

   // if not found return -1
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
   // Ini sudah BENAR untuk Bridge (tidak melakukan apa-apa saat terima data)
   int index_mac_asal = cari_mac_index(mac);
   if (false)
   {
      process_perintah(data, len, index_mac_asal);
   }
}

void callback_pengiriman_esp_now(const uint8_t *mac_addr, esp_now_send_status_t status)
{
   // Ini bagus untuk debugging
   if (status == ESP_NOW_SEND_SUCCESS)
   {
      Serial.println("Status Pengiriman: Sukses");
   }
   else
   {
      Serial.println("Status Pengiriman: Gagal");
   }
}

void callback_data_serial(const uint8_t *data, int len)
{
   // Panggil process_perintah, default index_mac_address_asal adalah -1 (dari header)
   process_perintah(data, len);
}

void baca_serial(void (*callback)(const uint8_t *data, int len))
{
   // Logika ini sudah terlihat BENAR untuk protokol custom Anda
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

void process_perintah(const uint8_t *data, int len, int index_mac_address_asal)
{
   if (index_mac_address_asal == -1)
   {
      Serial.printf("Menerima %d bytes dari Serial, meneruskan ke ESP-NOW...\n", len);
      esp_err_t result = esp_now_send(mac_addresses[MAC_RECEIVER_INDEX], data, len);
      if (result != ESP_OK)
      {
         Serial.printf("Gagal mengirim: %s\n", esp_err_to_name(result));
      }
   }
}