# 📡 Dokumentasi Fungsi Serial & Proses Perintah (ESP-NOW)
---

## 🔧 Alur Kerja keseluruhan

pertama-tama sumber data berasal dari serial dan juga esp lain,lalu data tersebut diolah di fungsi `process_perintah()`

data yang berasal dari serial diolah sama fungsi `baca_serial()`

data yang berasal dari esp lain di terima oleh fungsi callback `callback_data_esp_now()`

## 🔧 Alur Kerja fungsi `baca_serial()`
1. Membaca byte demi byte dari `Serial` dan menyimpannya ke `buffer`.
2. Mengecek:
   - Apakah data dimulai dengan header `0xFF 0xFF 0x00`.
   - Apakah panjang data (`dataLen`) valid.
   - Apakah jumlah byte sudah sesuai panjang yang dijanjikan.
3. Jika semua valid:
   - Payload (`buffer[4]` s.d. `buffer[4 + dataLen - 1]`) dikirim ke fungsi callback.
   - Buffer dan variabel indeks di-reset agar siap menerima paket baru.
---

## 🔧 Alur Kerja fungsi `callback_data_esp_now()`
1. Mencari index asal ESP yang sudah terdaftar jika tidak ketemu bakal ditandai `"Unknown"`
2. memanggil fungsi `process_perintah()` beserta nama yang sudah diperoses sebelumnya

## 🔧 Alur Kerja fungsi `process_perintah()`
1. Fungsi membaca dua byte pertama dari data:

    -  `data[0]` disimpan ke variabel perintah → menunjukkan jenis perintah (`HALO`, `CEK`, atau `JAWAB`).

    - `data[1]` disimpan ke variabel tujuan → menunjukkan tujuan pesan.

2. Fungsi menentukan apakah pesan datang dari luar atau dari serial.
Jika `index_mac_address_asal >= 0`, berarti pesan datang dari luar → `dariLuar = true`.

3. Fungsi menyiapkan beberapa variabel kosong seperti `kirimPesan`, `kirimData`, dan `kirimLen` untuk menampung balasan yang akan dikirim.

4. Fungsi memasuki blok switch (perintah) untuk menentukan jenis perintah apa yang diterima:
    - `HALO`

    - `CEK`

    - `JAWAB`

5. Jika perintah = HALO:

    - Jika pesan dari luar, robot membalas:
`"Halo Juga <nama pengirim>, Aku namanya <nama robot ini>"`

    - Jika pesan dari serial, robot mengirim perkenalan:
`"Halo dengan nama <tujuan>, kamu bisa panggil aku <nama robot ini>"`
6. Jika perintah = CEK:

    - Jika pesan dari luar → robot menjawab:
`"Iya Aku <nama pengirim> Disini - <nama robot ini>"`

    - Jika pesan dari serial → robot mengirim pertanyaan:
  `"<tujuan>, ini <nama robot ini> apa kamu disana?"`
7. Jika perintah = JAWAB:

    - Jika pesan datang dari luar → robot menampilkan isi pesan ke Serial Monitor:
    `Serial.println("Pesan diterima: " + msg);
`