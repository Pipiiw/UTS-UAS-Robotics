/*********************************************************
 * Project Monitoring Kecepatan & Kontrol Driver DC
 * ESP32 & Blynk Cloud Platform (Fixed Core v2.x PWM)
 *********************************************************/

#define BLYNK_TEMPLATE_ID   "TMPL6SqtseyJd"
#define BLYNK_TEMPLATE_NAME "New Template"
#define BLYNK_AUTH_TOKEN    "TtZnpbNazNb4R-5_AaG1mA1rm5ZdF97F"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

// Pengaturan Jaringan Nirkabel
char tokenBlynk[] = BLYNK_AUTH_TOKEN;
char namaWifi[]   = "Redmi Note 13 Pro+ 5G";
char sandiWifi[]  = "piwpiwpiw";

// Konfigurasi Pin Perangkat Keras
const int pinIndikator = 2;
const int pinDriver1   = 27;
const int pinDriver2   = 26;
const int pinPwmSpeed  = 12;

// Konfigurasi Channel PWM ESP32 Core v2.x
const int CHANNEL_PWM   = 0;
const int FREKWENSI_PWM = 30000;
const int RESOLUSI_BIT  = 8;

// Konfigurasi Sensor Penghitung Putaran
const byte pinTachometer    = 13;
const int JUMLAH_CELAH      = 2;
volatile unsigned int totalPulsa = 0;
volatile unsigned long waktuPulsaTerakhir = 0;
const unsigned long FILTER_DEBOUNCE = 500;

// Pengaturan Sinyal PWM
int nilaiKecepatan      = 200;
const int STEP_KENAIKAN = 10;
const int BATAS_BAWAH   = 0;
const int BATAS_ATAS    = 255;
bool statusAktif        = false;

// Variabel Perhitungan RPM
float kecepatanRpm = 0.0;
unsigned long waktuSebelumnya = 0;

// Buffer Komunikasi Serial & Timer Blynk
String bufferSerial = "";
BlynkTimer pengaturWaktu;

// Interrupt Service Routine untuk Tachometer
void IRAM_ATTR isrTachometer()
{
  unsigned long waktuSekarang = micros();
  if (waktuSekarang - waktuPulsaTerakhir > FILTER_DEBOUNCE)
  {
    totalPulsa++;
    waktuPulsaTerakhir = waktuSekarang;
  }
}

// Sinkronisasi Data Saat Terhubung Kembali ke Blynk
BLYNK_CONNECTED()
{
  Blynk.syncVirtual(V2);
  Blynk.syncVirtual(V3);
}

// Algoritma Kalkulasi RPM
void hitungKecepatanRpm()
{
  unsigned long waktuKini = millis();
  if (waktuKini - waktuSebelumnya >= 1000)
  {
    detachInterrupt(digitalPinToInterrupt(pinTachometer));

    float putaranPerDetik = (float)totalPulsa / (float)JUMLAH_CELAH;
    kecepatanRpm = putaranPerDetik * 60.0;
    totalPulsa = 0;

    attachInterrupt(digitalPinToInterrupt(pinTachometer), isrTachometer, FALLING);
    waktuSebelumnya = millis();
  }
}

// Eksekusi Sinyal ke Driver Motor
void terapkanKecepatanMotor()
{
  digitalWrite(pinDriver1, HIGH);
  digitalWrite(pinDriver2, LOW);

  // Pada ESP32 Core v2.x, ledcWrite wajib mengirim ke CHANNEL_PWM (Channel 0)
  ledcWrite(CHANNEL_PWM, nilaiKecepatan);
  digitalWrite(pinIndikator, nilaiKecepatan > 0);

  // Sinkronisasi data ke Blynk
  Blynk.virtualWrite(V1, nilaiKecepatan);
  Blynk.virtualWrite(V2, nilaiKecepatan);
  Blynk.virtualWrite(V3, nilaiKecepatan > 0);
}

// Tampilkan Log ke Serial Monitor
void cetakInformasiStatus(const char* keterangan)
{
  Serial.print(keterangan);
  Serial.print(" | Kecepatan PWM=");
  Serial.print(nilaiKecepatan);
  Serial.print(" | RPM=");
  Serial.println(kecepatanRpm, 1);
}

// Kirim Data Periodik ke Server Blynk
void unggahKeBlynk()
{
  Blynk.virtualWrite(V0, kecepatanRpm);
  Blynk.virtualWrite(V1, nilaiKecepatan);
  Blynk.virtualWrite(V2, nilaiKecepatan);
  Blynk.virtualWrite(V3, nilaiKecepatan > 0);
}

// Kontrol Slider dari Blynk (Virtual Pin V2)
BLYNK_WRITE(V2)
{
  nilaiKecepatan = constrain(param.asInt(), BATAS_BAWAH, BATAS_ATAS);
  statusAktif = (nilaiKecepatan > 0);
  terapkanKecepatanMotor();

  Serial.print("PWM via Blynk: ");
  Serial.println(nilaiKecepatan);
}

// Kontrol Tombol On/Off dari Blynk (Virtual Pin V3)
BLYNK_WRITE(V3)
{
  statusAktif = param.asInt();
  if (statusAktif)
  {
    if (nilaiKecepatan == 0) nilaiKecepatan = 200;
  }
  else
  {
    nilaiKecepatan = 0;
  }

  terapkanKecepatanMotor();
  Serial.print("Status Motor: ");
  Serial.println(statusAktif ? "HIDUP" : "MATI");
}

void setup()
{
  Serial.begin(115200);

  pinMode(pinIndikator, OUTPUT);
  pinMode(pinDriver1, OUTPUT);
  pinMode(pinDriver2, OUTPUT);
  pinMode(pinPwmSpeed, OUTPUT);
  pinMode(pinTachometer, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(pinTachometer), isrTachometer, FALLING);

  // Inisialisasi PWM ESP32 Core v2.x (Channel 0)
  ledcSetup(CHANNEL_PWM, FREKWENSI_PWM, RESOLUSI_BIT);
  ledcAttachPin(pinPwmSpeed, CHANNEL_PWM);

  digitalWrite(pinDriver1, HIGH);
  digitalWrite(pinDriver2, LOW);
  ledcWrite(CHANNEL_PWM, 0);

  waktuSebelumnya = millis();

  Blynk.begin(tokenBlynk, namaWifi, sandiWifi);
  pengaturWaktu.setInterval(1000L, unggahKeBlynk);

  Serial.println("=================================");
  Serial.println("Sistem Kontrol & Monitoring Aktif");
  Serial.println("=================================");
}

void loop()
{
  Blynk.run();
  pengaturWaktu.run();
  hitungKecepatanRpm();

  while (Serial.available() > 0)
  {
    char karakterMasuk = Serial.read();

    if (karakterMasuk == '\n' || karakterMasuk == '\r')
    {
      bufferSerial.trim();

      if (bufferSerial.startsWith("PWM:"))
      {
        int angkaPwm = bufferSerial.substring(4).toInt();
        angkaPwm = constrain(angkaPwm, BATAS_BAWAH, BATAS_ATAS);

        nilaiKecepatan = angkaPwm;
        statusAktif = (nilaiKecepatan > 0);
        terapkanKecepatanMotor();

        cetakInformasiStatus("Ubah PWM Manual");
      }
      else if (bufferSerial.length() > 0)
      {
        char perintah = bufferSerial.charAt(0);

        if (perintah == '1')
        {
          if (nilaiKecepatan == 0) nilaiKecepatan = 200;
          statusAktif = true;
          terapkanKecepatanMotor();
          cetakInformasiStatus("Motor Dinyalakan");
        }
        else if (perintah == '0')
        {
          nilaiKecepatan = 0;
          statusAktif = false;
          terapkanKecepatanMotor();
          Serial.println("Motor Dimatikan");
        }
        else if (perintah == '+')
        {
          nilaiKecepatan += STEP_KENAIKAN;
          if (nilaiKecepatan > BATAS_ATAS) nilaiKecepatan = BATAS_ATAS;
          statusAktif = true;
          terapkanKecepatanMotor();
          cetakInformasiStatus("PWM Dinaikkan");
        }
        else if (perintah == '-')
        {
          nilaiKecepatan -= STEP_KENAIKAN;
          if (nilaiKecepatan < BATAS_BAWAH) nilaiKecepatan = BATAS_BAWAH;
          statusAktif = (nilaiKecepatan > 0);
          terapkanKecepatanMotor();
          cetakInformasiStatus("PWM Diturunkan");
        }
        else if (perintah == 'R' || perintah == 'r')
        {
          Serial.print("Kecepatan RPM: ");
          Serial.print(kecepatanRpm, 1);
          Serial.print(" | Nilai PWM: ");
          Serial.println(nilaiKecepatan);
        }
      }

      bufferSerial = "";
    }
    else
    {
      bufferSerial += karakterMasuk;
    }
  }
}
