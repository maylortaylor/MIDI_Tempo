void solidDownBeat(int *downBeatColors){
  setPixel(downbeatone,downBeatColors[0],downBeatColors[1],downBeatColors[2]);
  setPixel(downbeattwo,downBeatColors[0],downBeatColors[1],downBeatColors[2]);
  setPixel(downbeatthree,downBeatColors[0],downBeatColors[1],downBeatColors[2]);
  setPixel(downbeatfour,downBeatColors[0],downBeatColors[1],downBeatColors[2]);
}


void downBeatFillerBeat(uint8_t *downBeat, uint8_t *fillerBeat){
  for (uint8_t i = 0; i < 8; i++){
	  switch(i){   
		  case 0:
			setPixel(i,downBeat[0],downBeat[1],downBeat[2]); 
			break;
		  case 2:
			setPixel(i,downBeat[0],downBeat[1],downBeat[2]); 
			break;
		  case 4:
			setPixel(i,downBeat[0],downBeat[1],downBeat[2]); 
			break;
		  case 6:
			setPixel(i,downBeat[0],downBeat[1],downBeat[2]); 
			break; 
		  default:
			setPixel(i, fillerBeat[0],fillerBeat[1],fillerBeat[2]);
	  }
  }
}

void measureSteps(uint8_t fullMeasure,uint8_t *newColors, uint8_t *oldColors){
     switch(fullMeasure){   
		case 0:
		case 1:
			setPixel(0, newColors[0],newColors[1],newColors[2]); 
			break;
		case 2:
		case 3:
			setPixel(0, oldColors[0],oldColors[1],oldColors[2]);
			setPixel(2, newColors[0],newColors[1],newColors[2]); 
			break;
		case 4:
		case 5:
			setPixel(2, oldColors[0],oldColors[1],oldColors[2]);
			setPixel(4, newColors[0],newColors[1],newColors[2]); 
			break;
		case 6:
		case 7:
			setPixel(4, oldColors[0],oldColors[1],oldColors[2]);
			setPixel(6, newColors[0],newColors[1],newColors[2]); 
			break; 
		default:
			setPixel(6, oldColors[0],oldColors[1],oldColors[2]);
			fullMeasure = 0;
			break;
  }
}

//Parameter 'wait': determines how long rainbow color lasts in seconds
void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    
  //Read brightness from potentiometer
  int potentiometerKnob = analogRead(A1);

  //scale from 10-bit to 8-bit
  potentiometerKnob = potentiometerKnob / 4;

  //set brightness
  strip.setBrightness(potentiometerKnob);
    strip.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
//Parameter 'wait': determines how long rainbow color lasts in seconds
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    
  //Read brightness from potentiometer
  int potentiometerKnob = analogRead(A1);

  //scale from 10-bit to 8-bit
  potentiometerKnob = potentiometerKnob / 4;

  //set brightness
  strip.setBrightness(potentiometerKnob);
    strip.show();
    delay(wait);
  }
}

void allRed(){
  for (uint8_t i = 0; i < 8; i++){
	setPixel(i,255,0,0); 
  }
}
void allWhite() {
	for (uint8_t i = 0; i < 8; i++) {
		setPixel(i,255,255,255);	
	}
}

void clearNonDownbeatNotes(){
  setPixel(1, 0,0,0);
  setPixel(2, 0,0,0);
  setPixel(3, 0,0,0);
  
  setPixel(5, 0,0,0);
  setPixel(6, 0,0,0);
  setPixel(7, 0,0,0);
  
  setPixel(9, 0,0,0);
  setPixel(10, 0,0,0);
  setPixel(11, 0,0,0);

  setPixel(13, 0,0,0);
  setPixel(14, 0,0,0);
  setPixel(15, 0,0,0);
}
void clearAllNotes(){
  for (uint8_t i = 0; i < 8; i++){
	setPixel(i,0,0,0); 
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}
