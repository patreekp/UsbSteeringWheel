unsigned int zAxis_ = 0;
unsigned int xAxis_ = 0;

void Clutch(void) {
  //Kuplung Slave
  int KuplungS = analogRead(A2);
  int mappedSlave = map(KuplungS, 594, 534, 0, 1023);
  Joystick.setZAxis(mappedSlave);

  //Kuplung Master
  //int KuplungM = analogRead(A3);
  //int mapped = map(KuplungM, 470, 407, 0, 255);
  //Joystick.setXAxis(mapped);

  Serial.println(KuplungS);
  Serial.println(mappedSlave);
  
}
