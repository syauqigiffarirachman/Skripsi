#include <OneWire.h>


OneWire  ds(PA15);
// USART3 uses PB10 (TX) and PB11 (RX)
HardwareSerial ser(PB11, PB10);

// #define ser serial


#define RED(red){digitalWrite(PB6,red);}
#define GREEN(green){digitalWrite(PB5,green);};
#define RED_TIME 5000
#define GREEN_TIME 500


bool begi = 1;
bool auto_mode=0;

unsigned long red_timeot=0;
unsigned long green_timeot=0;
/* 
Format API:
delay|Param_length Command|Parameter
30|02 AA|00

Format API Auto:
I   : Read manID
Rx  : Read page x=3
Wx|p: Write page x=3
Sx|p: Get signature x=3
Ux  : Get signature with randomized challange x=3
argument|param
AR3|00
*/

static const uint16_t crctableMaxim[256] ={
  0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0,
  0x0280, 0xC241, 0xC601, 0x06C0, 0x0780, 0xC741,
  0x0500, 0xC5C1, 0xC481, 0x0440, 0xCC01, 0x0CC0,
  0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
  0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0,
  0x0880, 0xC841, 0xD801, 0x18C0, 0x1980, 0xD941,
  0x1B00, 0xDBC1, 0xDA81, 0x1A40, 0x1E00, 0xDEC1,
  0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
  0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0,
  0x1680, 0xD641, 0xD201, 0x12C0, 0x1380, 0xD341,
  0x1100, 0xD1C1, 0xD081, 0x1040, 0xF001, 0x30C0,
  0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
  0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0,
  0x3480, 0xF441, 0x3C00, 0xFCC1, 0xFD81, 0x3D40,
  0xFF01, 0x3FC0, 0x3E80, 0xFE41, 0xFA01, 0x3AC0,
  0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
  0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0,
  0x2A80, 0xEA41, 0xEE01, 0x2EC0, 0x2F80, 0xEF41,
  0x2D00, 0xEDC1, 0xEC81, 0x2C40, 0xE401, 0x24C0,
  0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
  0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0,
  0x2080, 0xE041, 0xA001, 0x60C0, 0x6180, 0xA141,
  0x6300, 0xA3C1, 0xA281, 0x6240, 0x6600, 0xA6C1,
  0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
  0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0,
  0x6E80, 0xAE41, 0xAA01, 0x6AC0, 0x6B80, 0xAB41,
  0x6900, 0xA9C1, 0xA881, 0x6840, 0x7800, 0xB8C1,
  0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
  0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1,
  0xBC81, 0x7C40, 0xB401, 0x74C0, 0x7580, 0xB541,
  0x7700, 0xB7C1, 0xB681, 0x7640, 0x7200, 0xB2C1,
  0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
  0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0,
  0x5280, 0x9241, 0x9601, 0x56C0, 0x5780, 0x9741,
  0x5500, 0x95C1, 0x9481, 0x5440, 0x9C01, 0x5CC0,
  0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
  0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0,
  0x5880, 0x9841, 0x8801, 0x48C0, 0x4980, 0x8941,
  0x4B00, 0x8BC1, 0x8A81, 0x4A40, 0x4E00, 0x8EC1,
  0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
  0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0,
  0x4680, 0x8641, 0x8201, 0x42C0, 0x4380, 0x8341,
  0x4100, 0x81C1, 0x8081, 0x4040
};

uint16_t CalculateCRC16( uint16_t crc, const void *c_ptr, int len)     
{
  const uint8_t *c = (uint8_t *)c_ptr;

  while (len--)
    crc = (crc >> 8 & 0xFF) ^ crctableMaxim[(crc ^ *c++) & 0xFF];
  // crc = crctableMaxim[((crc >> 8) ^ *c++) & 0xFF] ^ (crc << 8);

  crc ^= 0xFFFF;
  crc &= 0xFFFF;

  return crc;
}


bool ds_send(byte command, byte *param, int param_len){
  byte crc[2];
  byte data_send[param_len + 3];
  ds.write(0x66,1);
  ds.write(param_len + 1,1);
  ds.write(command,1);
  if(!auto_mode){
    ser.printf("\nData send:\n %02X %02X %02X\n", 0x66, param_len + 1, command);
  }
  for(int i = 0; i<param_len; i++){
    ds.write(param[i],1);
    data_send[i+3] = param[i];
    if(!auto_mode){
      ser.printf(" %02X", param[i]);
    }
  }
  crc[0] = ds.read();
  crc[1] = ds.read();
  ds.write(0xAA, 1);

  
  data_send[0] = 0x66;
  data_send[1] = (param_len + 1);
  data_send[2] = command;

  uint16_t calc_crc;
  uint16_t comb_crc = (crc[1]<<8) | (crc[0]); 
  //calc_crc = ds.crc16(data_send, param_len + 3);

  calc_crc = CalculateCRC16(0x0000, data_send, param_len +3);

  if(calc_crc == comb_crc){
    if(!auto_mode){
      ser.printf("\nCRC SUCCESS:\n %04X [%04X]", comb_crc, calc_crc);
    }
    return true;
  }

  if(!auto_mode){
     ser.printf("\nCRC FAILED:\n %04X [%04X]", comb_crc, calc_crc);
  }
  return false;
}

bool ds_read(byte *dat, int *len){
  byte dummy = ds.read();
  *len = ds.read() + 2;
  byte result = ds.read();
  if(*len >= 0xFE){
    ser.println("LENGTH ERROR");
    return false;
  }
  
  if(!auto_mode){
    ser.printf("\n\nData recieve:\n %02X %02X %02X\n ", dummy, *len-2, result);
  }
  
  byte dat_rec[*len+2];
  byte crc[2];
  int is = 0;
  for(int i = 0; i<*len; i++){
    if(i < *len-3){         
      dat[i] = ds.read();
      dat_rec[i+2] = dat[i];
      if(!auto_mode){
        ser.printf("%02X ", dat[i]);
      }
    }else{
      crc[is] = ds.read();
      is++;
    }
  }

  dat_rec[0] =  *len-2;
  dat_rec[1] =  result;
  
  uint16_t calc_crc;
  uint16_t comb_crc = (crc[1]<<8) | (crc[0]); 
  //uint16_t calc_crc = ds.crc16(dat_rec, *len + 2);

  calc_crc = CalculateCRC16(0x0000, dat_rec, *len-1);


  if(calc_crc == comb_crc){
    if(!auto_mode){
      ser.printf("\nCRC SUCCES:\n %04X [%04X]\n", comb_crc, calc_crc);
    }
    return true;
  }

  if(!auto_mode){
    ser.printf("\nCRC FAILED:\n %04X [%04X]\n", comb_crc, calc_crc);
  }
  return false;
}



void setup(void){
  pinMode(PB5,OUTPUT);
  pinMode(PB6,OUTPUT);
  pinMode(PC13,OUTPUT);
  digitalWrite(PC13,1);
  RED(1);
  GREEN(0);  
  delay(23000);
  ser.begin(115200);
  ser.println("START!!");
}

void loop(void) {
  byte addr[8];
  int dly = 30;
  byte command = 0xAA;
  byte para[50] = {0x00};
  byte param_len = 1;
  char buf[255];
  bool done = 0;
  int str_in=0;
  bool fail=0;

  if(!begi){
    while(!done){
      if((millis()-red_timeot)<RED_TIME){
        RED(1);
      }else{
        RED(0);
      }
      if((millis()-green_timeot)<GREEN_TIME){
        GREEN(1);
      }else{
        GREEN(0);
      }
      if(ser.available()>0){
        buf[str_in] = ser.read();
        if(buf[str_in] == '\n'){
          buf[str_in] = (char) 0;
          done = 1;
          break;
        }
        str_in++;
      }
    }
  }else{
    begi = 0;
    buf[0] = 'A';
    buf[1] = 'I';
    buf[2] = '\0';
  }
  
  //ser.println("\n");
  ser.println(buf);

  if(buf[0] == 'A'){
    auto_mode=1;
    switch(buf[1]){
      case 'I':{
        dly = 30;
        param_len=0x1;
        command=0xAA;
        para[0]=0;
      break;
      }
      
      case 'R':{
        dly = 30;
        param_len=0x1;
        command=0x44;
        para[0]=buf[2]-uint8_t('0');
        break;
      }
      
      case 'W':{
        dly = 60;
        param_len=0x21;
        command=0x96;
        strtok(buf,"|");
        char *param = strtok(NULL,"|");
        char buff[3];
        int u=0;
        para[0]=buf[2]-uint8_t('0');
        
        for(int i=1; i<33; i++){
          buff[0] = param[u++];
          buff[1] = param[u++];
          buff[2] = char(0);

          para[i] = strtol(buff, NULL, 16);
        }
        break;
      }

      case 'S':{
        dly = 130;
        param_len=0x21;
        command=0xA5;
        strtok(buf,"|");
        char *param = strtok(NULL,"|");
        char buff[3];
        int u=0;
        para[0]=buf[2]-uint8_t('0');
        
        for(int i=1; i<33; i++){
          buff[0] = param[u++];
          buff[1] = param[u++];
          buff[2] = char(0);

          para[i] = strtol(buff, NULL, 16);
        }
        break;
      }

      case 'U':{
        dly = 130;
        param_len=0x21;
        command=0xA5;
        para[0]=buf[2]-uint8_t('0');
        
        for(int i=1; i<33; i++){
          randomSeed(analogRead(PA0));
          para[i] = random(0x00,0xFF);
        }
        break;
      }

      case 'L':{
        if(buf[2] == 'R'){
          red_timeot=millis();  
          ser.println("1");
        }else if(buf[2] == 'G'){
          green_timeot=millis();
          ser.println("1");
        }else{
          ser.println("-1");
        }
        return;
        break;
      }
      
        default:{
          ser.println("-1");
          return;
        }
    }
  }else{
    auto_mode=0;
    dly = atoi(strtok(buf, "|"));
    char *comm = strtok(NULL, "|");
    char *parm = strtok(NULL, "|");
  
    param_len = strtol(strtok(comm, " "), NULL, 16) - 1;
    command = strtol(strtok(NULL, " "), NULL, 16);
  
    para[0] = strtol(strtok(parm, " "), NULL, 16);
    for(int x=1; x<param_len; x++){
      para[x] = strtol(strtok(NULL, " "), NULL, 16);
    }
  }

  digitalWrite(PC13, LOW);
   
  ds.reset_search();
  ds.search(addr);

  if (OneWire::crc8(addr, 7) != addr[7]) {
      ser.println("-1");
      return;
  }

  if(!auto_mode){
    ser.print("ROM =");
    for(int i = 0; i < 8; i++) {
        ser.write(' ');
        ser.print(addr[i], HEX);
    }
  }
  
  delay(1);
  
  ds.reset();
  ds.select(addr);

  for(int i=0; i<2; i++){
    if(ds_send(command, para, param_len)){
      fail = false;
      break;
    }else{
      fail = true;
    }
  }

  if(fail){
    ser.println("-1");
    return;
  }
  
  delay(dly);

  byte data[100];
  int len;
  for(int i=0; i<2; i++){
    if(ds_read(data,&len)){
      fail = false;
      break;
    }else{
      fail = true;
    }
  }

  if(fail){
    ser.println("-1");
    return;
  }

  ds.reset();

  if(auto_mode){
    switch(command){
      case 0xAA:{
        for(int i = 0; i < 8; i++) {
        ser.printf("%02X",addr[i]);
        }
        ser.print("|");
        for(int i = 0; i < len-3; i++) {
        ser.printf("%02X",data[i]);
        }
        ser.println();
        break;
      }
      case 0x44:{
        for(int i = 0; i < len-3; i++) {
        ser.printf("%02X",data[i]);
        }
        ser.println();
        break;
      }
      case 0x96:{
        ser.println("1");
        break;
      }
      case 0xA5:{
        for(int i = 1; i<33; i++){
          ser.printf("%02X",para[i]);
        }
        ser.print("|");
        for(int i = 32; i<64; i++) {
          ser.printf("%02X",data[i]);
        }
        for(int i = 0; i<32; i++) {
          ser.printf("%02X",data[i]);
        }
        ser.println();
        break;
      }
    }
  }
  digitalWrite(PC13, HIGH);
}

