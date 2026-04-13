#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

//This example creates a bridge between Serial and Classical Bluetooth (SPP)
//and also demonstrate that SerialBT have the same functionalities of a normal Serial
#include "BluetoothSerial.h"

#define RXD2 16
#define TXD2 17

uint8_t txBuffer[20];
uint8_t rep[5];

uint16_t pas = 0;
uint16_t runSpeed = 300;
uint8_t accel = 25;
String instruction;
char header;
bool execution = false;
uint16_t value = 0;
const float pasMm = 0.097193; //   99*pi (=périmetre roue) / 3600 = 0.097193 (=distance en mm parcourue en 1pas)
const float pasDeg= 0.0495; //   1 / (entraxe(=225)*pi (=distance parcourue par chaque roue pour 360degrés) / pasMm) / 360 = 20.2 (=nombre de pas pour 1 degré)) = 0.0495 (=degré par pas)

uint8_t getCheckSum(uint8_t *buffer,uint8_t len);
void speedModeRun(uint8_t slaveAddr,uint8_t dir,uint16_t speed,uint8_t acc);

BluetoothSerial SerialBT;

class Maillon
{ 
  friend class Liste;
    
	String instr;
	Maillon *suiv;  
	
	public:
	
	Maillon(String V);
	~Maillon();
  String getInstr();
  Maillon* getSuiv();
};
class Liste
{	
	Maillon *tete;

	public :
	
	Liste();
	~Liste();
  void Enfiler(String V);
  void SuppLast();
  void Vider();
  Maillon* getTete();
};

Liste::Liste() // Constructeur liste vide
{
	tete = nullptr;
}
Maillon::Maillon(String V) // Creation de maillon avec instruction
{
	instr = V;
	suiv = nullptr;
}
Liste::~Liste() // Destructeur de liste
{
  if (tete != nullptr)
    delete tete;
}
Maillon::~Maillon() // Destructeur de maillon
{
  if (suiv != nullptr)
    delete suiv;
}
void Liste::Enfiler(String V)
{
	Maillon * temp1 = tete;
  Maillon * temp2 = nullptr;
	while (temp1 != nullptr)
	{
		temp2 = temp1;
		temp1 = temp1 -> suiv;
	}
	if (temp2 == nullptr)
	{
		tete = new Maillon(V);
	}
	else
	{
		temp2 -> suiv = new Maillon(V);
	}
}
void Liste::SuppLast()
{
	if (tete == nullptr) 
  {
    return;
  }
  if (tete->suiv == nullptr) 
  {
    delete tete;
    tete = nullptr;
    return;
  }
  Maillon *temp = tete;
  while (temp->suiv->suiv != nullptr) 
  {
    temp = temp->suiv;
  }
  delete temp->suiv;
  temp->suiv = nullptr;
}
void Liste::Vider()
{
  if (tete != nullptr) {
    delete tete;
    tete = nullptr;
  }
}
Maillon* Liste::getTete()
{ 
  return tete; 
}
String Maillon::getInstr()
{ 
  return instr; 
}
Maillon* Maillon::getSuiv()  
{
  return suiv; 
}

Liste file;
Maillon * tmp;

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  Serial.begin(38400);                          // sniff USB
  Serial2.begin(38400, SERIAL_8N1, RXD2, TXD2);  // bus moteurs

  SerialBT.begin("ESP32test"); //Bluetooth device name
  Serial.println("The device started, now you can pair it with bluetooth!");

  delay(3000);
}

void loop()
{
  SerialBT.println("waiting for instructions...");
  if (SerialBT.available() > 0) 
  {
    instruction = SerialBT.readString();
    while (SerialBT.available() > 0) SerialBT.read(); //sécurité pour etre sûr que le buffer soit vide
    header = instruction[0];

    if(header == 'E')
    {
      stopComm();
      execution = false;
    }
    else if(header == 'C')
    {
      file.Vider();
    }
    else if(header == 'L')
    {
      file.SuppLast();
    }
    else if(header == 'M')
    {
      tmp = file.getTete();
      execution = true;
      SerialBT.println("exe = true");
    }
    else
    {
      file.Enfiler(instruction);
      SerialBT.println((file.getTete()) -> getInstr());
    }
  }

  if(tmp != nullptr && execution && queryMotorStatus(2) == 1)
  {
    instruction = tmp -> getInstr();
    header = instruction[0];
    instruction.remove(0,1);
    value = instruction.toInt();
    SerialBT.println(instruction);
    SerialBT.println(header);

    if(header == 'Z')
    {
      value = value / pasMm;
      SerialBT.println(value);
      positionMode(1, 0, runSpeed, accel, value);
      positionMode(2, 1, runSpeed, accel, value); // léger délai, à voir si c'est réglable
    }
    else if(header == 'Q')
    {
      value = value / pasDeg;
      SerialBT.println(value);
      positionMode(1, 0, runSpeed, accel, value);
      positionMode(2, 0, runSpeed, accel, value);
    }
    else if(header == 'S')
    {
      value = value / pasMm;
      SerialBT.println(value);
      positionMode(1, 1, runSpeed, accel, value);
      positionMode(2, 0, runSpeed, accel, value); // léger délai, à voir si c'est réglable
    }
    else if(header == 'D')
    {
      value = value / pasDeg;
      SerialBT.println(value);
      positionMode(1, 1, runSpeed, accel, value);
      positionMode(2, 1, runSpeed, accel, value);
    }
    tmp = tmp->getSuiv();
    SerialBT.println(queryMotorStatus(2));
  }
  delay(1000);
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
  Serial2.write(txBuffer,7);
}

uint8_t queryMotorStatus(uint8_t slaveAddr)
{
  // Vider le buffer avant d'envoyer
  while (Serial2.available() > 0) Serial2.read();

  txBuffer[0] = 0xFA;
  txBuffer[1] = slaveAddr;
  txBuffer[2] = 0xF1;
  txBuffer[3] = getCheckSum(txBuffer, 3);
  Serial2.write(txBuffer, 4);

  unsigned long t = millis();
  while (Serial2.available() < 5) {
    if (millis() - t > 50) return 0; // si le moteur répond pas, renvoie 0 (=échec) (sinon le programe est bloqué dans le while)
  }

  Serial2.readBytes(rep, 5);
  return rep[3];
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