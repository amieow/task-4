#pragma once
#include "main.h"
#include <vector>
#include "rapidjson/document.h"

class CommandHandler
{
public:
   CommandHandler(); // Konstruktor
   void processCommand(const uint8_t *data, int len, int macIndex = -1);

private:
   std::vector<String> scannedFiles;
   void listFilesInDir(const char *dirname);
   void parseAndPrintJson(String locationFile);
};