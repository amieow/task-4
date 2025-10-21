#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <thread>
#include <chrono>
#include "serial/serial.h"

// =======================================================
// GLOBAL VARIABLE
// =======================================================
#if defined(_WIN32)
const std::string SERIAL_PORT = "COM5";
#else
const std::string SERIAL_PORT = "/dev/ttyUSB0";
#endif

const int SERIAL_BAUD = 115200;
const std::string JSON_FILENAME = "../../data/profile.json";
const std::string FILENAME_ON_ESP = "profile6.json";
const int CHUNK_SIZE_HEX = 180;

const uint8_t CMD_RECEIVE_FILE = 0;
const uint8_t CMD_PARSE_FILE = 2;

// =======================================================
// CLASS: FileReader
// =======================================================
class FileReader
{
public:
   explicit FileReader(const std::string &path) : filePath(path) {}

   bool loadFile()
   {
      std::ifstream file(filePath, std::ios::binary);
      if (!file)
      {
         std::cerr << "[FileReader] Gagal membuka file: " << filePath << std::endl;
         return false;
      }
      content.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
      file.close();
      std::cout << "[FileReader] File terbaca, ukuran: " << content.size() << " bytes.\n";
      return true;
   }

   std::string toHex() const
   {
      std::stringstream ss;
      ss << std::hex << std::setfill('0');
      for (unsigned char c : content)
      {
         ss << std::setw(2) << static_cast<int>(c);
      }
      return ss.str();
   }

   const std::vector<char> &getContent() const { return content; }

private:
   std::string filePath;
   std::vector<char> content;
};

// =======================================================
// CLASS: SerialConnection
// =======================================================
class SerialConnection
{
public:
   SerialConnection(std::string port, int baud) : portName(std::move(port)), baudRate(baud) {}

   bool open()
   {
      try
      {
         serialPort.setPort(portName);
         serialPort.setBaudrate(baudRate);
         serial::Timeout timeout = serial::Timeout::simpleTimeout(1000);
         serialPort.setTimeout(timeout);
         serialPort.open();
      }
      catch (const serial::IOException &e)
      {
         std::cerr << "[SerialConnection] Gagal membuka port: " << e.what() << std::endl;
         return false;
      }

      if (!serialPort.isOpen())
      {
         std::cerr << "[SerialConnection] Port tidak terbuka.\n";
         return false;
      }

      std::cout << "[SerialConnection] Terhubung ke " << portName << " @ " << baudRate << " baud.\n";
      std::this_thread::sleep_for(std::chrono::seconds(2)); // beri waktu ESP siap
      return true;
   }

   void write(const std::vector<uint8_t> &data)
   {
      try
      {
         serialPort.write(data);
      }
      catch (const serial::IOException &e)
      {
         std::cerr << "[SerialConnection] Error kirim data: " << e.what() << std::endl;
      }
   }

   void close()
   {
      if (serialPort.isOpen())
      {
         serialPort.close();
         std::cout << "[SerialConnection] Port ditutup.\n";
      }
   }

private:
   std::string portName;
   int baudRate;
   serial::Serial serialPort;
};

// =======================================================
// CLASS: PacketSender
// =======================================================
class PacketSender
{
public:
   explicit PacketSender(SerialConnection &conn) : connection(conn) {}

   void sendPayload(const std::vector<uint8_t> &payload)
   {
      const size_t MAX_PAYLOAD = 251;

      if (payload.size() <= MAX_PAYLOAD)
      {
         sendPacket(payload);
         return;
      }

      size_t totalParts = (payload.size() + MAX_PAYLOAD - 1) / MAX_PAYLOAD;
      std::cout << "[PacketSender] Payload besar (" << payload.size()
                << " bytes) → " << totalParts << " bagian.\n";

      for (size_t offset = 0; offset < payload.size(); offset += MAX_PAYLOAD)
      {
         size_t chunkSize = std::min(MAX_PAYLOAD, payload.size() - offset);
         std::vector<uint8_t> chunk(payload.begin() + offset, payload.begin() + offset + chunkSize);
         sendPacket(chunk);
         std::this_thread::sleep_for(std::chrono::milliseconds(30));
      }
   }

private:
   SerialConnection &connection;

   void sendPacket(const std::vector<uint8_t> &data)
   {
      std::vector<uint8_t> packet;
      packet.push_back(0xFF);
      packet.push_back(0xFF);
      packet.push_back(0x00);
      packet.push_back(static_cast<uint8_t>(data.size()));
      packet.insert(packet.end(), data.begin(), data.end());
      connection.write(packet);
      std::cout << "[PacketSender] Mengirim paket " << packet.size() << " bytes.\n";
   }
};

// =======================================================
// CLASS: FileTransferManager
// =======================================================
class FileTransferManager
{
public:
   FileTransferManager(FileReader &reader, SerialConnection &conn)
       : fileReader(reader), connection(conn), sender(conn) {}

   void transferFile(const std::string &destFilename)
   {
      if (!connection.open())
         return;
      std::string hexData = fileReader.toHex();
      std::cout << "[Transfer] Mulai kirim " << destFilename
                << " (" << hexData.size() << " chars HEX)\n";

      for (size_t i = 0; i < hexData.length(); i += CHUNK_SIZE_HEX)
      {
         std::string chunk = hexData.substr(i, CHUNK_SIZE_HEX);
         uint8_t isFirstChunk = i == 0;
         std::vector<uint8_t> payload;

         payload.push_back(CMD_RECEIVE_FILE);
         payload.push_back(isFirstChunk);
         payload.insert(payload.end(), destFilename.begin(), destFilename.end());
         payload.push_back('\0');
         payload.insert(payload.end(), chunk.begin(), chunk.end());

         sender.sendPayload(payload);
         std::this_thread::sleep_for(std::chrono::milliseconds(500));
      }

      connection.close();
      std::cout << "[Transfer] File " << destFilename << " selesai dikirim.\n";
   }

private:
   FileReader &fileReader;
   SerialConnection &connection;
   PacketSender sender;
};

// =======================================================
// MAIN PROGRAM
// =======================================================
int main()
{
   FileReader reader(JSON_FILENAME);
   if (!reader.loadFile())
      return 1;

   SerialConnection serial(SERIAL_PORT, SERIAL_BAUD);
   FileTransferManager manager(reader, serial);
   manager.transferFile(FILENAME_ON_ESP);

   return 0;
}
