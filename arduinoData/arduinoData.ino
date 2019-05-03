String data = "";
int PosXYZ[3] = {0, 0, 0}, tempPosXYZ[3] = {-1, -1, -1}, shift[3] = {1, 1, 1};

void setup() {
  Serial.begin(9600);
}

void respone(String message)
{          
//    String a1 =  String(PosXYZ[0])+ ","+  String(PosXYZ[1]) + ","+ String(PosXYZ[2])+"A1";
//    Serial.println(a1);
//    String b1 =  String(tempPosXYZ[0])+ ","+  String(tempPosXYZ[1]) + ","+ String(tempPosXYZ[2])+"B1";
//    Serial.println(b1);
    
  if(message == "x+")
    PosXYZ[0] += shift[0];
  else if(message == "x-")
    PosXYZ[0] -= shift[0];
  else if(message == "y+")
    PosXYZ[1] += shift[1];
  else if(message == "y-")
    PosXYZ[1] -= shift[1];
  else if(message == "z+")
    PosXYZ[2] += shift[2];
  else if(message == "z-")
    PosXYZ[2] -= shift[2];
}

void receive()
{
  if (Serial.available() > 0)
  {
    char buff = Serial.read();
    if (buff == '\n')
    {      
//      Serial.print("> ");
//      Serial.println(data);
      respone(data);
      data = "";
    }
    else
      data += buff;
  }
}

void send()
{
  if ((tempPosXYZ[0] != PosXYZ[0]) || (tempPosXYZ[1] != PosXYZ[1]) || (tempPosXYZ[2] != PosXYZ[2]))
  {
    String message =  String(PosXYZ[0])+ ","+  String(PosXYZ[1]) + ","+ String(PosXYZ[2])+"E";
    Serial.println(message);
    
    for(int i = 0; i < 3; i++)    
      tempPosXYZ[i] = PosXYZ[i];
  }
}

void loop() {
  receive();
  send();  
  delay(50);
}

