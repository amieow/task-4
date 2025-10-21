#include "CommandHandler.h"

CommandHandler::CommandHandler()
{
}
void CommandHandler::listFilesInDir(const char *dirname)
{
   Serial.printf("Listing directory: %s\n", dirname);
   scannedFiles.clear();

   fs::FS &fs = SPIFFS;
   File root = fs.open(dirname);
   if (!root || !root.isDirectory())
   {
      Serial.println("Gagal membuka direktori");
      return;
   }

   File file = root.openNextFile();
   int index = 1;
   while (file)
   {
      if (!file.isDirectory())
      {
         String filePath = String(dirname) + "/" + file.name();
         Serial.printf("  [%d] %s\n", index++, filePath.c_str());
         scannedFiles.push_back(filePath);
      }
      file = root.openNextFile();
   }
   root.close();
}

void CommandHandler::parseAndPrintJson(String locationFile)
{
   fs::FS &fs = SPIFFS;
   if (!fs.exists(locationFile.c_str()))
   {
      Serial.println("Error: File tidak ditemukan: " + locationFile);
      return;
   }

   File file = fs.open(locationFile.c_str(), FILE_READ);
   if (!file)
   {
      Serial.println("Gagal membuka file untuk dibaca.");
      return;
   }

   String fileContent = file.readString();
   file.close();

   rapidjson::Document doc;
   if (doc.Parse(fileContent.c_str()).HasParseError())
   {
      Serial.println("Gagal parse JSON");
      return;
   }

   Serial.println("\n[KONTEN FILE YANG DITERIMA]");
   Serial.println("File: " + locationFile);

   if (doc.HasMember("nama") && doc["nama"].IsString())
   {
      Serial.print("NAMA: ");
      Serial.println(doc["nama"].GetString());
   }
   if (doc.HasMember("jurusan") && doc["jurusan"].IsString())
   {
      Serial.print("JURUSAN: ");
      Serial.println(doc["jurusan"].GetString());
   }
   if (doc.HasMember("umur") && doc["umur"].IsInt())
   {
      Serial.print("UMUR: ");
      Serial.println(doc["umur"].GetInt());
   }
   if (doc.HasMember("deskripsi") && doc["deskripsi"].IsString())
   {
      Serial.print("DESKRIPSI DIRI: ");
      Serial.println(doc["deskripsi"].GetString());
   }
}

// =======================================================
// METODE PUBLIK (Ini adalah fungsi process_perintah Anda yang lama)
// =======================================================

void CommandHandler::processCommand(const uint8_t *data, int len, int macIndex)
{
   uint8_t perintah = data[0];
   bool dariLuar = macIndex >= 0;

   switch (perintah)
   {
   case RECEIVE_FILE:
   {
      // ... (Salin-tempel SEMUA logika dari 'case RECEIVE_FILE' Anda ke sini) ...
      bool isFirstChunk = (bool)data[1];
      std::string fileName;
      int i = 2;
      for (; i < len; i++)
      {
         if (data[i] == 0)
         {
            i++;
            break;
         }
         fileName += char(data[i]);
      }
      if (fileName.empty())
         break;
      std::string locationFile = "/data";
      fs::FS &fs = SPIFFS;
      if (!fs.exists("/data"))
         fs.mkdir("/data");
      locationFile += "/" + fileName;
      const char *open_mode = isFirstChunk ? FILE_WRITE : FILE_APPEND;
      File file = fs.open(locationFile.c_str(), open_mode);
      if (!file)
      {
         Serial.println("Gagal membuka file: " + String(locationFile.c_str()));
         break;
      }
      if (isFirstChunk)
      {
         Serial.println("File baru (overwrite): " + String(locationFile.c_str()));
      }
      for (int j = i; j < len; j += 2)
      {
         if (j + 1 >= len)
            break;
         char hexByte[3] = {(char)data[j], (char)data[j + 1], '\0'};
         uint8_t byteValue = (uint8_t)strtol(hexByte, nullptr, 16);
         file.write(byteValue);
      }
      file.close();
      Serial.println("Chunk ditulis ke: " + String(locationFile.c_str()));
      break;
   }
   case PARSE_FILE:
   {
      Serial.println("\n[PERINTAH PARSE DITERIMA]");
      std::string fileName;
      for (int i = 1; i < len; i++)
      {
         if (data[i] == 0)
            break;
         fileName += char(data[i]);
      }
      String locationFile = "/data/" + String(fileName.c_str());

      // Panggil method privat, BUKAN fungsi global
      parseAndPrintJson(locationFile);
      break;
   }
   case LIST_FILES:
   {
      Serial.println("\n[PERINTAH LIST FILES DITERIMA]");

      // Panggil method privat, BUKAN fungsi global
      listFilesInDir("/data");
      break;
   }
   case PARSE_BY_INDEX:
   {
      Serial.println("\n[PERINTAH PARSE BY INDEX DITERIMA]");
      if (len < 2)
      {
         Serial.println("Error: Index tidak diberikan.");
         break;
      }
      uint8_t index = data[1];

      // 'scannedFiles' sekarang adalah member privat
      if (scannedFiles.empty())
      {
         Serial.println("Error: Belum ada daftar file. Jalankan LIST_FILES dulu.");
         break;
      }
      if (index < 1 || index > scannedFiles.size())
      {
         Serial.printf("Error: Index %d di luar jangkauan (1 s/d %d).\n", index, scannedFiles.size());
         break;
      }
      String locationFile = scannedFiles[index - 1];

      // Panggil method privat
      parseAndPrintJson(locationFile);
      break;
   }
   case JAWAB:
   {
      // ... (Salin-tempel logika 'case JAWAB' Anda ke sini) ...
      if (dariLuar)
      {
         String msg = "";
         for (int i = 1; i < len; i++)
            msg += (char)data[i];
         Serial.println("Pesan diterima: " + msg);
      }
      break;
   }
   }
}