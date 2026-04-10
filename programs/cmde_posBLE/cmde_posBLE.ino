#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif
//This example creates a bridge between Serial and Classical Bluetooth (SPP)
//and also demonstrate that SerialBT have the same functionalities of a normal Serial
#include "BluetoothSerial.h"

#define RXD2 16
#define TXD2 17

uint8_t txBuffer[20];

uint32_t pas = 0;
uint16_t runSpeed = 0;
uint8_t runDir = 0;
uint8_t sAddr = 0; // adresse 0 = broadcast
uint8_t accel = 0;
uint8_t emStop = 0;
String instruction;
String header;

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
/*
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

  SerialBT.println("Choose the number of steps :");
  while (SerialBT.available() == 0);
  pas = SerialBT.parseInt();
  while (SerialBT.available() > 0) SerialBT.read();
*/
}

void loop()
{
  SerialBT.println("waiting for instructions...");
  while (SerialBT.available() == 0);
  instruction = SerialBT.readString();
  while (SerialBT.available() > 0) SerialBT.read();
  header = instruction[0];
  instruction.remove(0,1);
  //instruction.toInt();
  SerialBT.println(instruction);
  SerialBT.println(header);

  if(header == E)
  {

  }
  else if(header == M)
  {

  }
  else if(header == Z || header == Q || header == S || header == D)
  {
    //ajouter l'insruction en la convertissant en int (et traduite en commande moteur ?) dans la file
  }
  else if(header == C)
  {

  }
  else if(header == L)
  {

  }
  else
  {

  }

  /*
  if (emStop == 1)
  {
    stopComm();
  }
  else
  {
    positionMode(sAddr, runDir, runSpeed, accel, pas); // STEEL BALL RUN
    digitalWrite(LED_BUILTIN, digitalRead(LED_BUILTIN)^1);
  }

  while (SerialBT.available() == 0);
  emStop = SerialBT.parseInt();
  while (SerialBT.available() > 0) SerialBT.read();
  */

  delay(3000);
}

/*--------------------------------*/
/* Commande mouvement             */
/*--------------------------------*/
void positionMode(uint8_t slaveAddr,uint8_t dir,uint16_t speed,uint8_t acc,uint32_t pulses) //speed : 1-3000 , dir : 0-1, acc : 0-255, pulse : nombre de pas
{
  //-----------------------------------------
  txBuffer[0] = 0xFA;       //frame header
  txBuffer[1] = slaveAddr;  //slave address
  txBuffer[2] = 0xFD;       //function code
  txBuffer[3] = (dir<<7) | ((speed>>8)&0x0F); //High 4 bits for direction and speed
  txBuffer[4] = speed&0x00FF;   //8 bits lower
  txBuffer[5] = acc;            //acceleration
  txBuffer[6] = (pulses >> 24)&0xFF;  //Pulse bit31 - bit24
  txBuffer[7] = (pulses >> 16)&0xFF;  //Pulse bit23 - bit16
  txBuffer[8] = (pulses >> 8)&0xFF;   //Pulse bit15 - bit8
  txBuffer[9] = (pulses >> 0)&0xFF;   //Pulse bit7 - bit0
  txBuffer[10] = getCheckSum(txBuffer,10);  //Calculate checksum
  //-----------------------------------------

  SerialBT.print("TX : ");

  for(int i=0;i<11;i++)
  {
    SerialBT.printf("%02X ", txBuffer[i]);
  }

  SerialBT.println();

  Serial2.write(txBuffer,11);
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