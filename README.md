# README: Alur Kerja Program Transfer File (Laptop -> Bridge -> Receiver)

## 1. Ikhtisar Proyek

Proyek ini adalah implementasi sistem transfer file satu arah (*one-way*) dari aplikasi C++ berbasis OOP di laptop (Windows/Linux/macOS) ke mikrokontroler ESP32-Receiver.

Untuk menjembatani komunikasi, sistem ini menggunakan ESP32-Bridge sebagai perantara (*intermediary*) yang mengubah data dari komunikasi Serial (USB) menjadi komunikasi nirkabel (ESP-NOW).

Arsitektur sistem adalah sebagai berikut:
`[Laptop C++] --(Serial/USB)--> [ESP32-Bridge] --(ESP-NOW)--> [ESP32-Receiver]`

---

## 2. Protokol Komunikasi

Sistem ini menggunakan protokol paket *custom* untuk komunikasi yang andal.

### A. Protokol Serial (Laptop ke Bridge)

Laptop mengirimkan data ke Bridge menggunakan format paket yang ketat:

* **Struktur Paket:** `[HEADER_1] [HEADER_2] [HEADER_3] [LENGTH] [PAYLOAD]`
    * `[0xFF]`: Byte header 1
    * `[0xFF]`: Byte header 2
    * `[0x00]`: Byte header 3
    * `[LENGTH]`: 1 byte yang menunjukkan panjang *payload* (maksimum 251 byte).
    * `[PAYLOAD]`: Data aktual yang akan dikirim (lihat di bawah).

### B. Protokol Payload (Data Aktual)

*Payload* yang dikirim berisi perintah dan data yang akan diproses oleh ESP32-Receiver.

* **Perintah `RECEIVE_FILE` (0x00):**
    * Struktur: `[CMD] [isFirstChunk] [Filename] [Data Chunk]`
    * `[CMD]`: `0x00` (Perintah `RECEIVE_FILE`)
    * `[isFirstChunk]`: 1 byte (`0x01` jika ini adalah *chunk* pertama, `0x00` jika bukan).
    * `[Filename]`: Nama file tujuan, diakhiri dengan *null terminator* (`\0`). (Misal: `profile.json\0`)
    * `[Data Chunk]`: Potongan file yang sudah di-encode sebagai **HEX String**.

* **Perintah `LIST_FILES` (0x03):**
    * Struktur: `[CMD]`
    * `[CMD]`: `0x03` (Perintah `LIST_FILES`)

* **Perintah `PARSE_BY_INDEX` (0x04):**
    * Struktur: `[CMD] [Index]`
    * `[CMD]`: `0x04` (Perintah `PARSE_BY_INDEX`)
    * `[Index]`: 1 byte yang berisi nomor indeks file yang akan di-parse (dimulai dari 1).

* **Perintah `PARSE_FILE` (0x02):**
    * Struktur: `[CMD] [FileName]`
    * `[CMD]`: `0x04` (Perintah `PARSE_BY_INDEX`)
    * `[FileName]`: Nama file tujuan beserta extensionnya.

---

## 3. Komponen Sistem

### A. Program Laptop (C++ OOP)

Program di laptop berfungsi sebagai pengirim. Ditulis dalam C++ OOP dan di-*build* menggunakan CMake.

* **Tanggung Jawab Class:**
    * `FileReader`: Membaca file `.json` lokal (misal: `profile.json`) dari disk.
    * `SerialConnection`: Mengelola koneksi *low-level* ke port serial (USB) menggunakan *library* `wjwwood/serial`. Bertugas membuka, menutup, dan menulis data biner ke port 115200 ke bridge COM5.
    * `PacketSender`: Mengabstraksi **Protokol Serial**. Class ini menerima *payload* mentah, membungkusnya dengan *header* `0xFF 0xFF 0x00 <len>`, dan memastikannya tidak melebihi batas 251 byte per paket.
    * `FileTransferManager`: Orkestrator(pengelola alur kerja) utama.
* **Alur Kerja Laptop:**
    1.  `main()` menginisialisasi `FileReader`, `SerialConnection`, dan `FileTransferManager`.
    2.  `FileTransferManager` memanggil `fileReader.loadFile()` dan `fileReader.toHex()` untuk mendapatkan seluruh konten file sebagai satu *string* HEX yang panjang.
    3.  Manager memecah *string* HEX ini menjadi *chunk-chunk* kecil (misal: 180 byte per *chunk*).
    4.  Untuk **setiap *chunk***:
        a.  Ia membuat *payload* `RECEIVE_FILE` (mengisi `CMD`, `isFirstChunk`, nama file, dan data *chunk*).
        b.  *Payload* ini diserahkan ke `PacketSender`.
    5.  `PacketSender` membungkus *payload* ke dalam **Protokol Serial** (`0xFF...`) dan mengirimkannya melalui `SerialConnection`.
    6.  (Opsional) Setelah semua *chunk* terkirim, `main()` dapat menginstruksikan `PacketSender` untuk mengirim *payload* `LIST_FILES` atau `PARSE_BY_INDEX`.

### B. ESP32-Bridge

ESP32 ini berfungsi murni sebagai perantara/jembatan.

* **Tanggung Jawab:**
    * Mendengarkan data dari **Serial** (dari Laptop).
    * Meneruskan data (hanya *payload*) ke **ESP-NOW** (ke Receiver).
* **Alur Kerja Bridge:**
    1.  `setup()`: Menginisialisasi `Serial.begin()`, `WiFi.mode(WIFI_STA)`, dan `esp_now_init()`. Yang terpenting, ia **mendaftarkan MAC Address ESP32-Receiver** sebagai *peer* (`esp_now_add_peer()`).
    2.  `loop()`: Terus-menerus memanggil fungsi `baca_serial()`.
    3.  `baca_serial()`: Mendengarkan data masuk di port serial. Ia secara aktif mencari *header* protokol `0xFF 0xFF 0x00`.
    4.  Jika paket valid diterima, `baca_serial()` mengekstrak `[LENGTH]` dan membaca *payload* sejumlah *length* tersebut.
    5.  `callback_data_serial()` dipanggil **hanya dengan *payload* yang bersih** (tanpa header `0xFF...`).
    6.  `process_perintah()` (versi Bridge) menerima *payload* ini dan langsung memanggil `esp_now_send()` untuk meneruskan *payload* tersebut ke MAC Address Receiver.

### C. ESP32-Receiver (OOP)

ESP32 ini adalah tujuan akhir. Ia menerima data nirkabel, menyimpannya ke *file system*, dan menampilkannya.

* **Tanggung Jawab:**
    * Menerima data dari **ESP-NOW**.
    * Memproses perintah dari *payload*.
    * Menyimpan file ke **SPIFFS**.
    * Mem-parsing JSON dan mencetaknya ke Serial Monitor.
* **Struktur OOP:**
    * `main.cpp`: Berisi `setup()` dan `loop()`. Tugas utamanya adalah menginisialisasi `SPIFFS.begin(true)`, `mulai_esp_now()`, dan membuat instansi `CommandHandler` global.
    * `CommandHandler` (Class): Ini adalah otak dari *receiver*. Ia menyimpan *state* (seperti `scannedFiles`) dan berisi logika untuk setiap perintah.
    * Pada file `utility.cpp`: Berisi kode C-style untuk `mulai_esp_now`, `cari_mac_index`, dll.
* **Alur Kerja Receiver:**
    1.  Saat paket ESP-NOW diterima, *callback* `global_on_data_recv` (yang didaftarkan saat `setup()`) akan terpicu.
    2.  *Callback* ini segera meneruskan data *payload* ke `myCommandHandler.processCommand()`.
    3.  `CommandHandler::processCommand()` menjalankan `switch` pada byte perintah (`data[0]`):
        * **`case RECEIVE_FILE`**:
            a.  Mengekstrak `isFirstChunk`, `fileName`, dan *data chunk* (HEX) dari *payload*.
            b.  Menentukan mode buka file: `FILE_WRITE` (timpa) jika `isFirstChunk == true`, atau `FILE_APPEND` (tambah) jika `false`.
            c.  Membuka file di SPIFFS (misal: `/data/profile5.json`).
            d.  Mengonversi *data chunk* (HEX String) kembali menjadi data biner (`strtol`).
            e.  Menulis data biner ke file dan menutupnya.
        * **`case LIST_FILES`**:
            a.  Memanggil *method* privat `listFilesInDir()`.
            b.  *Method* ini mengosongkan `scannedFiles`, memindai direktori `/data`, mencetak daftar berindeks ke Serial, dan menyimpan *path* lengkap ke `scannedFiles` (sebuah `std::vector`).
        * **`case PARSE_BY_INDEX`**:
            a.  Mengekstrak `[Index]` dari *payload*.
            b.  Mengambil *path* file dari `scannedFiles` (misal: `scannedFiles[Index - 1]`).
            c.  Memanggil *method* privat `parseAndPrintJson()` dengan *path* tersebut.
        * **`parseAndPrintJson()`**:
            a.  Membaca seluruh file dari SPIFFS ke sebuah `String`.
            b.  Menggunakan **RapidJSON** (`doc.Parse()`) untuk mem-parsing konten `String` tersebut.
            c.  Jika sukses, ia akan mengekstrak nilai (`nama`, `jurusan`, `umur`, `deskripsi`) dan mencetaknya ke Serial Monitor sesuai format tugas.

## 4. Demo
