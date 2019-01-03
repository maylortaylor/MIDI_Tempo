/***************************************************************************************
Project: Visual MIDI Metronome
Description: Receives MIDI clock signals and displays a visual metronome on a WS2812
          based strip.

Connections:
  WS2812 Strip:
    GND -> GND
    DTA -> Digital 6
    VCC -> VCC (5v)

Software:
  Hairless MIDI Bridge  http://projectgus.github.io/hairless-midiserial/
  
  loopMIDI (Windows)  http://www.tobias-erichsen.de/software/loopmidi.html
    NOTE: For Mac OS use the fiollowing instead of loopMIDI:
      You may need to enable the “IAC” virtual MIDI port. This makes a “channel”
        that can join the two together:
      1.) Open the OS X built-in “Audio MIDI Setup” application 
        (under Applications->Utilities.)
      2.) Choose “Show MIDI Window” from the Window menu.
      3.) Double-click the “IAC Driver” in the MIDI window.
      4.) Check the “Device is online” box in the dialog that appears.
      5.) Go back to Hairless MIDI<->Serial and there should be a new “IAC Bus” option
        in the dropdown. The other MIDI program should also see the IAC bus option.
  
***************************************************************************************/

/***************************************************************************************
  Libraries
***************************************************************************************/
#include <Adafruit_NeoPixel.h>    //WS2812 Communication
#include <MIDIUSB.h>                 //MIDI Communication
#include <EEPROM.h>               //Utilize Storage Memory
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define PIN 6
#define LED 13  


/***************************************************************************************
  Main Program Variable Declarations
***************************************************************************************/
#define FPS 7                   //Frame rate = (1000 / FPS) - 4
int D9 = 9;
unsigned long nextTick = 0;     //Next pattern update time
uint8_t  clockCount = 0;  //96 counts per measure (4/4)
uint8_t  measureStep = 0; //16 steps in LED squence
uint8_t  fullMeasure = 0;
int potentiometerKnob = 0;
#define R 0
#define G 1 
#define B 2

uint8_t  downbeatone = 0;
uint8_t  downbeattwo = 2;
uint8_t  downbeatthree = 4;
uint8_t  downbeatfour = 6;

uint8_t  button = 0; //program button
int  debounce = 0;
int travelingNoteColorR = 0;
int newColorsForMetronomeR = 0;
int oldColorsForMetronomeR = 0;
int fillerBeatColorR = 0;
int downBeatColorR = 0;

//Define default color variables
uint8_t downBeatColor[] = {0,0,255};
uint8_t fillerBeatColor[] = {0,0,0};
uint8_t newColorsForMetronome[] = {255,0,0};
uint8_t oldColorsForMetronome[] = {0,0,255};
uint8_t travelingNoteColor[] = {255,255,255};

/***************************************************************************************
  Parameters for WS2812 / NeoPixels
  Function: Define type of strand and color order.
  // Parameter 1 = number of pixels in strip
  // Parameter 2 = Arduino pin number (most are valid)
  // Parameter 3 = pixel type flags, add together as needed:
  //   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
  //   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
  //   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
  //   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
  //   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
  // IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
  // pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
  // and minimize distance between Arduino and first pixel.  Avoid connecting
  // on a live circuit...if you must, connect GND first.
***************************************************************************************/

Adafruit_NeoPixel strip = Adafruit_NeoPixel(16, PIN, NEO_GRB + NEO_KHZ800);


/***************************************************************************************
  Automated fades for WS based lights
  Function: Control cross fading between light values for
  a smooth effect. Variables and constants are defined.
***************************************************************************************/

#define PWM_MaxPixels 16   //Number of Pixels
#define ALL -1              //Define keyword
#define INVERTED 1          //Define keyword
#define NORMAL 0            //Define keyword

//Define variable structure for each pixel
typedef struct
{
	uint8_t valueSetR;      //R Desired value 
	uint8_t valueActR;      //R Actual value
	uint8_t valueSetG;      //G Desired value
	uint8_t valueActG;      //G Actual value
	uint8_t valueSetB;      //B Desired value
	uint8_t valueActB;      //B Actual value
	uint8_t fadeUpRate;     //Fade up rate in steps
	uint8_t fadeDownRate;   //Fade down rate in steps
}   NeoPixel_PWM_Channel;

//Create an instance of above variables for each channel
NeoPixel_PWM_Channel PWMChannel[PWM_MaxPixels];

// //define Light_Array strucutre shape
// typedef struct
// {
	// uint8_t downBeatColor[2];
	// uint8_t fillerBeatColor[2];
	// uint8_t newColorsForMetronome[2];
	// uint8_t oldColorsForMetronome[2];
	// uint8_t travelingNoteColor[2];	
// }	Light_Array;

// //Create Light_Array max size
// Light_Array LightsDb[2];

// //Fill first Light_Array index
// Light_Array[0].dowBeatColor = {0,0,255};
// Light_Array[0].fillerBeatColor = {138,11,176};
// Light_Array[0].newColorsForMetronome = {255,0,0};
// Light_Array[0].oldColorsForMetronome = {0,0,255};
// Light_Array[0].travelingNoteColor = {255,255,255};

void BlinkLed(byte num)         // Basic blink function
{
    for (byte i=0;i<num;i++){
        digitalWrite(LED, HIGH);
        delay(10);
        digitalWrite(LED, LOW);
    }
}


/***************************************************************************************
  Parameters:
  Function: every midi beat coming from computer/ableton triggers this function
***************************************************************************************/
void midiBeat()
{	
	measureStep++;

	if (fullMeasure == 8){
		fullMeasure = 0;
	}

	if (measureStep > 7){
		measureStep = 0;
		fullMeasure++;
	}
	else {
		setPixel(measureStep - 1, 0,0,0);
	}

	//set color functions

	downBeatFillerBeat(downBeatColor,fillerBeatColor);
	measureSteps(fullMeasure, newColorsForMetronome, oldColorsForMetronome);
	
	// switch(fullMeasure) {
		// case 0:
			// allWhite();
			// break;
		// // case 2:
			// // allWhite();
			// // break;
		// case 4:
			// allWhite();
			// break;
		// // case 6:
			// // allWhite();
			// // break;
	// }
	// Traveling Note setPixel()
	//setPixel(measureStep, travelingNoteColorR,travelingNoteColor[1],travelingNoteColor[2]);
}


/***************************************************************************************
  Setup
***************************************************************************************/
void setup() {

	//Power and Ground for Potentiometer
	pinMode(A1, INPUT_PULLUP);
	pinMode(A2, OUTPUT);
	pinMode(A3, OUTPUT);
	pinMode(D9, INPUT);
	digitalWrite(D9, HIGH);
	digitalWrite(A2, HIGH);
	digitalWrite(A3, LOW);
	

	//WS2812B Setup
	strip.begin();
	strip.show(); // Initialize all pixels to 'off'

	//MIDI Setup
	pinMode(LED, OUTPUT);

	//USB MIDI setup for Hairless MI_PDI Bridge. Increases baud rate. 
	Serial.begin(115200);

	setPixelFade(ALL, 0, 0);

	//Read brightness from potentiometer
	int potentiometerKnob = analogRead(A1);

	//scale from 10-bit to 8-bit
	potentiometerKnob = potentiometerKnob / 4;

	//set brightness
	//strip.setBrightness(potentiometerKnob);
	strip.setBrightness(255);
}


/***************************************************************************************
  Main loop
  Functions:  Watch for MIDI commands. Keep PWM fade rates going as close to the
      desired frame rate as possible. 
***************************************************************************************/
void loop()
{//
	//Check to see if it's time to update the pattern again
	if(nextTick < millis())
	{
		//Add frame rate time to next tick
		nextTick = millis() + FPS;

		//Call main fade calculation task
		PWMControl();
	}

	midiEventPacket_t rx;
	rx = MidiUSB.read();
	
	if (rx.header != 0)                
	{
		// midi message recieved
		switch(rx.byte1)
		{
		  case 0xF8:
		  //clock:248
			clockCount++;
			if (clockCount > 5)
			{
			  midiBeat();
			  clockCount = 0;
			}
			  break;
		  case 0xFA:
		  //start:250
			clockCount = 0;
			fullMeasure = 0;
			measureStep = 0;
			midiBeat();
			  break;
		  case 0xFC:
		  //stop:252
			fullMeasure = 0;
			measureStep = 0;
			clockCount = 0;
			clearAllNotes();
			allRed();//stop color function
			  break;
		  }
	}
	
	if (debounce > 0)
	{
		debounce = debounce - 1;
	}

	Serial.print("digitalPin 9: ");
	Serial.print(digitalRead(9));
	if ((digitalRead(9) == 0)&&(debounce == 0))
	{
		button ++;

		if (button > 5)
		{		  
			button = 0;
		}	
		debounce = 175;
	}
	
	// button = 0;
	Serial.print("; button: ");
	Serial.println(button);
	switch(button)
	{
		case 0:
		{
			//TEAL, dimBlue, dimBlue, dimBlue (w/ red Metronome) (white traveling note)
			
			//Color of the full measure beats
			downBeatColor[R] = 0;
			downBeatColor[G] = 255;
			downBeatColor[B] = 255;
			
			//color of the quarter note beats in the measure
			fillerBeatColor[R] = 0;
			fillerBeatColor[G] = 0;
			fillerBeatColor[B] = 0;
			
			//Color of the LED at the full measure meteronome (past)
			oldColorsForMetronome[R] = downBeatColorR;
			oldColorsForMetronome[G] = downBeatColor[G];
			oldColorsForMetronome[B] = downBeatColor[B];
			
			//Color of the LED at the full measure meteronome (present)
			newColorsForMetronome[R] = 0;
			newColorsForMetronome[G] = 255;
			newColorsForMetronome[B] = 0;
			
			//Color of the LED light traveling on every quarter note
			travelingNoteColor[R] = 255;
			travelingNoteColor[G] = 255;
			travelingNoteColor[B] = 255;
		}	break;
		case 1:
		{
			//BLUE, purple, purple, purple (w/ red Metronome) (white traveling note)
			
			//Color of the full measure beats
			downBeatColor[R] = 0;
			downBeatColor[1] = 0;
			downBeatColor[2] = 255;
			
			//Color of the quarter note beats in the measure
			fillerBeatColor[R] = 10;
			fillerBeatColor[1] = 0;
			fillerBeatColor[2] = 10;
			
			//Color of the LED at the full measure meteronome (downbeat of last measure)
			oldColorsForMetronomeR = downBeatColorR;
			oldColorsForMetronome[1] = downBeatColor[1];
			oldColorsForMetronome[2] = downBeatColor[2];
			
			//Color of the LED at the full measure meteronome (downbeat of current measure)
			newColorsForMetronomeR = 255;
			newColorsForMetronome[1] = 0;
			newColorsForMetronome[2] = 0;
			
			//Color of the LED light traveling on every quarter note
			travelingNoteColorR = 255;
			travelingNoteColor[1] = 255;
			travelingNoteColor[2] = 255;
		}	break;
		case 2:
		{		
			//GREEN, purple, purple, purple (w/ blue Metronome) (white traveling note)
			
			//Color of the full measure beats
			downBeatColorR = 0;
			downBeatColor[1] = 255;
			downBeatColor[2] = 0;
			
			//Color of the quarter note beats in the measure
			fillerBeatColorR = 20;
			fillerBeatColor[1] = 0;
			fillerBeatColor[2] = 20;
			
			//Color of the LED at the full measure meteronome (downbeat of last measure)
			oldColorsForMetronomeR = downBeatColorR;
			oldColorsForMetronome[1] = downBeatColor[1];
			oldColorsForMetronome[2] = downBeatColor[2];
			
			//Color of the LED at the full measure meteronome (downbeat of current measure)
			newColorsForMetronomeR = 0;
			newColorsForMetronome[1] = 0;
			newColorsForMetronome[2] = 255;
			
			//Color of the LED light traveling on every quarter note
			travelingNoteColorR = 255;
			travelingNoteColor[1] = 255;
			travelingNoteColor[2] = 255;
		}	break;
		case 3:
		{		
			//WHITE, off, off, off (w/ red Metronome)(green traveling note)
			
			//Color of the full measure beats
			downBeatColorR = 255;
			downBeatColor[1] = 255;
			downBeatColor[2] = 255;
			
			//Color of the quarter note beats in the measure
			fillerBeatColorR = 0;
			fillerBeatColor[1] = 0;
			fillerBeatColor[2] = 0;
			
			//Color of the LED at the full measure meteronome (downbeat of last measure)
			oldColorsForMetronomeR = downBeatColorR;
			oldColorsForMetronome[1] = downBeatColor[1];
			oldColorsForMetronome[2] = downBeatColor[2];
			
			//Color of the LED at the full measure meteronome (downbeat of current measure)
			newColorsForMetronomeR = 255;
			newColorsForMetronome[1] = 0;
			newColorsForMetronome[2] = 0;
			
			//Color of the LED light traveling on every quarter note
			travelingNoteColorR = 0;
			travelingNoteColor[1] = 255;
			travelingNoteColor[2] = 0;
		}	break;		
		case 4:
		{		
			//WHITE, off, off, off (w/ blue Metronome) (red traveling note)
			
			//Color of the full measure beats
			downBeatColorR = 255;
			downBeatColor[1] = 255;
			downBeatColor[2] = 255;
			
			//Color of the quarter note beats in the measure
			fillerBeatColorR = 0;
			fillerBeatColor[1] = 0;
			fillerBeatColor[2] = 0;
			
			//Color of the LED at the full measure meteronome (downbeat of last measure)
			oldColorsForMetronomeR = downBeatColorR;
			oldColorsForMetronome[1] = downBeatColor[1];
			oldColorsForMetronome[2] = downBeatColor[2];
			
			//Color of the LED at the full measure meteronome (downbeat of current measure)
			newColorsForMetronomeR = 0;
			newColorsForMetronome[1] = 0;
			newColorsForMetronome[2] = 255;
			
			//Color of the LED light traveling on every quarter note
			travelingNoteColorR = 255;
			travelingNoteColor[1] = 0;
			travelingNoteColor[2] = 0;
		}	break; 
		case 5:
		{   
		  //GREEN, off, off, off (w/ purple Metronome) (white traveling note)
		  
		  //Color of the full measure beats
		  downBeatColorR = 0;
		  downBeatColor[1] = 255;
		  downBeatColor[2] = 0;
		  
		  //Color of the quarter note beats in the measure
		  fillerBeatColorR = 0;
		  fillerBeatColor[1] = 0;
		  fillerBeatColor[2] = 0;
		  
		  //Color of the LED at the full measure meteronome (downbeat of last measure)
		  oldColorsForMetronomeR = downBeatColorR;
		  oldColorsForMetronome[1] = downBeatColor[1];
		  oldColorsForMetronome[2] = downBeatColor[2];
		  
		  //Color of the LED at the full measure meteronome (downbeat of current measure)
		  newColorsForMetronomeR = 255;
		  newColorsForMetronome[1] = 0;
		  newColorsForMetronome[2] = 255;
		  
		  //Color of the LED light traveling note on every quarter note
		  travelingNoteColorR = 255;
		  travelingNoteColor[1] = 255;
		  travelingNoteColor[2] = 255;
		} break;
	}

}


/***************************************************************************************
  PWMControl
  Function: Calculate fade times for light values.
  This function should be called 30-120 times per second
  depending on desired refresh rate.
  This will effect the longest fade time:
  255 / N calls per second = Longest fade in seconds
***************************************************************************************/
void PWMControl()
{

  //working memory storage of values
  
  int16_t newValueR;
  int16_t newValueG;
  int16_t newValueB;
  int16_t fadeDirectionR;
  int16_t fadeDirectionG;
  int16_t fadeDirectionB;

  for (uint8_t i = 0; i < PWM_MaxPixels; i++)
  {
    //see if any fade rate is enabled (fade up or down rate is more than 0)
    if (PWMChannel[i].fadeUpRate > 0 || PWMChannel[i].fadeDownRate > 0)
    {
      //Find any differences to fade to (positive or negative fade)
      fadeDirectionR = PWMChannel[i].valueSetR - PWMChannel[i].valueActR;
      fadeDirectionG = PWMChannel[i].valueSetG - PWMChannel[i].valueActG;
      fadeDirectionB = PWMChannel[i].valueSetB - PWMChannel[i].valueActB;
      
      //by default assume we have found the correct value 
      newValueR = PWMChannel[i].valueSetR;
      newValueG = PWMChannel[i].valueSetG;
      newValueB = PWMChannel[i].valueSetB;
      
      //now check to see if otherwise

      //R Channel
      if (fadeDirectionR > 0) //positive fade rate?
      {
        //fade up
        newValueR = PWMChannel[i].valueActR + PWMChannel[i].fadeUpRate;

        //check to see if we overshot
        if (newValueR > PWMChannel[i].valueSetR)
        {
          newValueR = PWMChannel[i].valueSetR; //if overshot go to set value
        }
      }
      else if (fadeDirectionR < 0) //negative fade rate?
      {
        //fade down
        newValueR = PWMChannel[i].valueActR - PWMChannel[i].fadeDownRate;

        //check to see if we overshot
        if (newValueR < PWMChannel[i].valueSetR)
        {
          newValueR = PWMChannel[i].valueSetR; //if overshot go to set value
        }
      }
      PWMChannel[i].valueActR = newValueR; //save setting to data structure

      //G Channel
      if (fadeDirectionG > 0) //positive fade rate?
      {
        //fade up
        newValueG = PWMChannel[i].valueActG + PWMChannel[i].fadeUpRate;

        //check to see if we overshot
        if (newValueG > PWMChannel[i].valueSetG)
        {
          newValueG = PWMChannel[i].valueSetG; //if overshot go to set value
        }
      }
      else if (fadeDirectionG < 0) //negative fade rate?
      {
        //fade down
        newValueG = PWMChannel[i].valueActG - PWMChannel[i].fadeDownRate;

        //check to see if we overshot
        if (newValueG < PWMChannel[i].valueSetG)
        {
          newValueG = PWMChannel[i].valueSetG; //if overshot go to set value
        }
      }
      PWMChannel[i].valueActG = newValueG;  //save setting to data structure

      //B Channel
      if (fadeDirectionB > 0)
      {
        //fade up
        newValueB = PWMChannel[i].valueActB + PWMChannel[i].fadeUpRate;

        //check to see if we overshot
        if (newValueB > PWMChannel[i].valueSetB)
        {
          newValueB = PWMChannel[i].valueSetB;
        }
      }
      else if (fadeDirectionB < 0)
      {
        //fade down
        newValueB = PWMChannel[i].valueActB - PWMChannel[i].fadeDownRate;

        //check to see if we overshot
        if (newValueB < PWMChannel[i].valueSetB)
        {
          newValueB = PWMChannel[i].valueSetB;
        }
      }
      PWMChannel[i].valueActB = newValueB;  
      
    }
    else
    {
      PWMChannel[i].valueActR = PWMChannel[i].valueSetR;
      PWMChannel[i].valueActG = PWMChannel[i].valueSetG;
      PWMChannel[i].valueActB = PWMChannel[i].valueSetB;
    }
    
    strip.setPixelColor(i, PWMChannel[i].valueActR, PWMChannel[i].valueActG, PWMChannel[i].valueActB);
  }  


  //Read brightness from potentiometer
  int knob = analogRead(A1);
  
  //scale from 10-bit to 8-bit
  knob = knob / 4;
  
  //check for difference from previous value
  if ((knob < potentiometerKnob - 4)||(knob > potentiometerKnob + 4))
  {
	//if the difference passes threshold, update value
	potentiometerKnob = knob; 
	//scale from 10-bit to 8-bit
	potentiometerKnob = potentiometerKnob / 4;
  }
	  
  if (potentiometerKnob < 2)
  {
	  potentiometerKnob = 0;
  }



  //set brightness
  strip.setBrightness(potentiometerKnob);
  
  strip.show();
}


/***************************************************************************************
  setPixel
  Function: Used for setting all or one of the PWM
  channels to a new value.
***************************************************************************************/
void setPixel(int8_t pixel, uint8_t rValue, uint8_t gValue, uint8_t bValue)
{
  if (pixel == -1)
  {
    uint8_t i;
    for (i = 0; i < PWM_MaxPixels; i++)
    {
      PWMChannel[i].valueSetR = rValue;
      PWMChannel[i].valueSetG = gValue;
      PWMChannel[i].valueSetB = bValue;
    }
  }


  PWMChannel[pixel].valueSetR = rValue;
  PWMChannel[pixel].valueSetG = gValue;
  PWMChannel[pixel].valueSetB = bValue;
  //else PWMChannel[pin].pwmvalue = value;
}

/***************************************************************************************
  setPixelFade
  Function: Used for setting fade time up and down times
  for PWM channels. Time is represented in steps, the 
  slowest being 1 and the quickest being 128.
  Setting the fade time to 0 disables fading

***************************************************************************************/
void setPixelFade(int8_t pin, uint8_t fadeup, uint8_t fadedown)
{
  if (pin == -1)
  {
    uint8_t i;
    for (i = 0; i < PWM_MaxPixels; i++)
    {
      PWMChannel[i].fadeUpRate = fadeup;
      PWMChannel[i].fadeDownRate = fadedown;  
    }
  }else 
  {
    PWMChannel[pin].fadeUpRate = fadeup;
    PWMChannel[pin].fadeDownRate = fadedown; 
  }
}

/***************************************************************************************
  hsv2rgb
  Function: Calculate RGB values for colors represented
    in Hue, Saturation, and Value (brightness).
***************************************************************************************/
void hsv2rgb(float H, float S, float V, int& R, int& G, int& B)
{
  int var_i;
  float var_1, var_2, var_3, var_h, var_r, var_g, var_b;
  if ( S == 0 )                       //HSV values = 0 รท 1
  {
    R = V * 255;
    G = V * 255;
    B = V * 255;
  }
  else
  {
    var_h = H * 6;
    if ( var_h == 6 ) var_h = 0;      //H must be < 1
    var_i = int( var_h ) ;            //Or ... var_i = floor( var_h )
    var_1 = V * ( 1 - S );
    var_2 = V * ( 1 - S * ( var_h - var_i ) );
    var_3 = V * ( 1 - S * ( 1 - ( var_h - var_i ) ) );

    if ( var_i == 0 ) {
    var_r = V     ;
    var_g = var_3 ;
    var_b = var_1 ;
    }
    else if ( var_i == 1 ) {
    var_r = var_2 ;
    var_g = V     ;
    var_b = var_1 ;
    }
    else if ( var_i == 2 ) {
    var_r = var_1 ;
    var_g = V     ;
    var_b = var_3 ;
    }
    else if ( var_i == 3 ) {
    var_r = var_1 ;
    var_g = var_2 ;
    var_b = V     ;
    }
    else if ( var_i == 4 ) {
    var_r = var_3 ;
    var_g = var_1 ;
    var_b = V     ;
    }
    else                   {
    var_r = V     ;
    var_g = var_1 ;
    var_b = var_2 ;
    }

    //RGB results = 0 รท 255 (Production)
    R = (var_r) * 255;
    G = (var_g) * 255;
    B = (var_b) * 255;
  }
}


