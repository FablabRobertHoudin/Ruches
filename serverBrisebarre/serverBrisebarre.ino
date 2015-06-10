// Include the GSM library
#include <OneWire.h>
#include <GSM.h>
#include <dht.h>
// #include <LowPower.h>

#define useGSM 1
#define sendSMS 0
#define sendHTTP 1

#define PINNUMBER "0000"
// APN data
#define GPRS_APN       "sl2sfr" // prixtel SFR GPRS APN
//#define GPRS_APN       "free" // Free GPRS APN
#define GPRS_LOGIN     "test"    // replace with your GPRS login
#define GPRS_PASSWORD  "pass" // replace with your GPRS password

OneWire  ds(5);  // on pin 10 (a 4.7K resistor is necessary)

dht DHT;
#define DHT22_PIN 6

// initialize the library instance
#if (sendHTTP)
  GSMClient client;
  GPRS gprs;
extern const char* host;
extern char* URL;
  int port = 80; // port 80 is the default for HTTP
#endif
  GSM gsmAccess(true); // include a 'true' parameter for debug enabled
#if (useGSM)
  GSM_SMS sms;
#endif

void setup()
{
  // initialize serial communications and wait for port to open:
  Serial.begin(9600);


  // connection state
  boolean notConnected = true;

#if (useGSM)
  Serial.println("GSM Sender");
  // Start GSM shield
  // If your SIM has PIN, pass it as a parameter of begin() in quotes
  while (notConnected)
  {
    if ((gsmAccess.begin(PINNUMBER) == GSM_READY)
#if (sendHTTP)
       & (gprs.attachGPRS(GPRS_APN, GPRS_LOGIN, GPRS_PASSWORD) == GPRS_READY)
#endif
      )
      notConnected = false;
    else
    {
      Serial.println("Not connected");
      delay(1000);
    }
  }
#endif

  Serial.println("GSM initialized");
}

unsigned long t, t1=-60000*10, t2=0;
int no;
char remoteNum[] = "0685679839";  // telephone number to send sms
//char txtMsg[200] = "Essai de SMS Arduino...";
int V;
float f;
#define k 20,2884
//#define k 17,4028
// 10.0V alim AN5=537 (10.7V)
float T1, T2;

// DS18B20 (type_s = 0;)
//ROM1 = 28 8A AB 44 6 0 0 47
//ROM2 = 28 59 9B 45 6 0 0 D1
  byte addr1[8] = {0x28, 0x8A, 0xAB, 0x44, 0x06, 0x00, 0x00, 0x47};
  byte addr2[8] = {0x28, 0x59, 0x9B, 0x45, 0x06, 0x00, 0x00, 0xD1};


float temp(byte *addr)
{
  byte data[12];
  byte i;

  ds.reset();
  ds.select(addr);
  ds.write(0xBE);         // Read Scratchpad
  Serial.print("  Data = ");
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  Serial.print(" CRC=");
  Serial.print(OneWire::crc8(data, 8), HEX);
  Serial.print(" adr1=");
  Serial.print(addr[1], HEX);

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  float celsius = (float)raw / 16.0;
  Serial.print("  Temperature = ");
  Serial.print(celsius);
  Serial.print(" Celsius");

  Serial.println();
  return celsius;
}


void loop()
{
  // ATmega328P, ATmega168
  // Wait 60sec (7*8+4)
  // idle vs powerDown
  /*
  for(int i=0; i<7; i++)
  LowPower.idle(SLEEP_8S, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF, SPI_OFF, USART0_OFF, TWI_OFF);
  LowPower.idle(SLEEP_4S, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF, SPI_OFF, USART0_OFF, TWI_OFF);
  */
  //LowPower.idle(SLEEP_4S, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF, SPI_OFF, USART0_ON, TWI_OFF);

    // Sleep for 8 s with ADC module and BOD module off
    // LowPower.powerDown(SLEEP_8S, ADC_CONTROL_OFF, BOD_OFF);


  ds.reset();
  ds.write(0xCC);        // skip ROM
  ds.write(0x44);        // start conversion, with parasite power on at the end
  delay(1000);     // maybe 750ms is enough, maybe not

  T1 = temp(addr1);
  T2 = temp(addr2);

  int chk = DHT.read22(DHT22_PIN);
  Serial.print("DHT, read=");
  Serial.print(chk);
  Serial.print(", humidity=");
  Serial.print(DHT.humidity, 1);
  Serial.print(", temp=");
  Serial.print(DHT.temperature, 1);
  Serial.print("\n");


  t = millis();
  if(t - t1 >= 60000*10) {
    t1 = t;
    no++;
    // send the message
#if (sendHTTP)
  if (client.connect(host, port))
  {
    // Make a HTTP request:
    client.print("GET ");
    client.print(URL);
    //client.print("?AN0=");
    //client.print(analogRead(A0));
    //client.print("&AN1=");
    //client.print(analogRead(A1));
    //client.print("&AN5=");
    //client.print(analogRead(A5));
    //client.print("&T1=");
    client.print(T1);
    client.print(";"); // "&T2="
    client.print(T2);
    client.print(";"); // "&Tdht="
    client.print(DHT.temperature);
    client.print(";"); // "&Humidity="
    client.print(DHT.humidity);

    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(host);
    client.println("Connection: close");
    client.println();

    client.stop();
    Serial.println("GET HTTP sent!\n");
  }
#endif

#if (sendSMS)
    sms.beginSMS(remoteNum);
    sms.print("MSG");
    sms.print(no);
    sms.print(",AN0=");
    sms.print(analogRead(A0));
    sms.print(",AN1=");
    sms.print(analogRead(A1));
    sms.print(",AN5=");
      V = analogRead(A5);
      f = V * k;
    sms.print(V);
    sms.print(":");
    sms.print(f);
    sms.print("mV");
    sms.endSMS();
    Serial.println("SMS sent!\n");
#endif
  }
  
  if(t - t2 >= 10000) {
    t2 = t;
      Serial.print("t(ms)==");
      Serial.print(millis());
      Serial.print(", AN0=");
      Serial.print(analogRead(A0));
      Serial.print(", AN1=");
      Serial.print(analogRead(A1));
      Serial.print(", AN5=");
      V = analogRead(A5);
      Serial.print(V);
      f = V * k;
      Serial.print("=");
      Serial.print(f);
      Serial.print("mV");
      Serial.println();
  }


}


