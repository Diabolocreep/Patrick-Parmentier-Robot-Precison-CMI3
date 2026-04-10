#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif
//This example creates a bridge between Serial and Classical Bluetooth (SPP)
//and also demonstrate that SerialBT have the same functionalities of a normal Serial
#include "BluetoothSerial.h"

#define RXD2 16
#define TXD2 17

uint8_t txBuffer[20];

uint16_t runSpeed = 0;
uint8_t runDir = 0;
uint8_t sAddr = 0; // adresse 0 = broadcast
uint8_t accel = 0;
uint8_t emStop = 0;

uint8_t getCheckSum(uint8_t *buffer,uint8_t len);
void speedModeRun(uint8_t slaveAddr,uint8_t dir,uint16_t speed,uint8_t acc);

BluetoothSerial SerialBT;

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  Serial.begin(38400);                          // sniff USB
  Serial2.begin(38400, SERIAL_8N1, RXD2, TXD2);  // bus moteurs

  SerialBT.begin("ESP32test"); //Bluetooth device name
  Serial.println("The device started, now you can pair it with bluetooth!");

  delay(3000);

  SerialBT.println("Choose the running motor : RIGHT = 1 | LEFT = 2 | ALL = 0");
  while (SerialBT.available() == 0); //attend une entrée
  sAddr = SerialBT.parseInt(); //recupere l'entrée du serial monitor
  while (SerialBT.available() > 0) SerialBT.read(); //vide le Serial.available()

  SerialBT.println("Choose the direction : FORWARD = 0 | BACKWARD = 1");
  while (SerialBT.available() == 0);
  runDir = SerialBT.parseInt();
  while (SerialBT.available() > 0) SerialBT.read();

  SerialBT.println("Choose the speed : 0-3000");
  while (SerialBT.available() == 0);
  runSpeed = SerialBT.parseInt();
  while (SerialBT.available() > 0) SerialBT.read();

  SerialBT.println("Choose the acceleration : 0-255");
  while (SerialBT.available() == 0);
  accel = SerialBT.parseInt();
  while (SerialBT.available() > 0) SerialBT.read();
}

void loop()
{
  if (emStop == 1)
  {
    stopComm();
  }
  else
  {
    speedModeRun(sAddr, runDir, runSpeed, accel); // STEEL BALL RUN
    digitalWrite(LED_BUILTIN, digitalRead(LED_BUILTIN)^1);
  }

  while (SerialBT.available() == 0);
  emStop = SerialBT.parseInt();
  while (SerialBT.available() > 0) SerialBT.read();

  delay(3000);
}

/*--------------------------------*/
/* Commande vitesse mode broadcast*/
/*--------------------------------*/

void speedModeRun(uint8_t slaveAddr,uint8_t dir,uint16_t speed,uint8_t acc) //speed : 1-3000 , dir : 0-1, acc : 0-255
{
  //-----------------------------------------
  txBuffer[0] = 0xFA;           // header
  txBuffer[1] = slaveAddr;      // adresse (0 = broadcast)
  txBuffer[2] = 0xF6;           // fonction speed mode
  txBuffer[3] = (dir<<7) | ((speed>>8)&0x0F); //le dernier bit est la direction et les 4 premiers sont la vitesse (avec le 5eme octet)
  txBuffer[4] = speed & 0xFF;   // vitesse (avec les 4 premiers bits de l'octet 4)
  txBuffer[5] = acc;            //acceleration 0-255
  txBuffer[6] = getCheckSum(txBuffer,6);
  //-----------------------------------------

  SerialBT.print("TX : ");

  for(int i=0;i<7;i++)
  {
    SerialBT.printf("%02X ", txBuffer[i]);
  }

  SerialBT.println();

  Serial2.write(txBuffer,7);
}

void stopComm()
{
  //-----------------------------------------
  txBuffer[0] = 0xFA;
  txBuffer[1] = 0;
  txBuffer[2] = 0xF6;
  txBuffer[3] = 0;
  txBuffer[4] = 0;
  txBuffer[5] = 2; //si acc = 0 les moteurs s'arretent immédiatement, en mettant 4 ils vont décélérer d'abord
  txBuffer[6] = getCheckSum(txBuffer,6);
  //-----------------------------------------

  SerialBT.print("TX : ");

  for(int i=0;i<7;i++)
  {
    SerialBT.printf("%02X ", txBuffer[i]);
  }

  SerialBT.println();

  Serial2.write(txBuffer,7);
}

/*---------------------------*/
/* Calcul checksum           */
/*---------------------------*/

uint8_t getCheckSum(uint8_t *buffer,uint8_t size)
{
  uint16_t sum = 0;

  for(uint8_t i=0;i<size;i++)
  {
    sum += buffer[i];
  }

  return(sum & 0xFF);
}