#include <FastLED.h>
const int NUMSTRIPS = 5;
CRGB ledstrips[NUMSTRIPS][150];

#include "sharedTopContent.h"
#include "sharedCustomFunctions.h"

int intersectF2onLED2_ss = 40;

int unusedPixelsLED1_ss = 0;
int unusedPixelsLED2_ss = 30;
int unusedPixelsLED3_ss = 24;
int unusedPixelsLED4_ss = 14;
int unusedPixelsLED5_ss = 0;

// how long is each finger section (use 1-index counting)
int fingerLengths_ss[] = {
	150 - unusedPixelsLED1_ss,
	150 - unusedPixelsLED3_ss,
	150 - unusedPixelsLED4_ss,
	150 - unusedPixelsLED5_ss};

// how long is each filament section (use 1-index counting)
int filamentLengths_ss[] = {
	intersectF2onLED2_ss/2,
	intersectF2onLED2_ss/2,
	(149 - intersectF2onLED2_ss)/2,
	(149 - intersectF2onLED2_ss)/2};

bool isTouchedFinger_ss[] = {false,false,false,false};
int activeToIndexFingerColorB_ss[] = {0,0,0,0};
int activeToIndexFingerColorC_ss[] = {
	fingerLengths_ss[0],
	fingerLengths_ss[1],
	fingerLengths_ss[2],
	fingerLengths_ss[3]};
bool isEmptyFingerColorB_ss[] = {true,true,true,true}; // comparisons could replace this throughout code but this will make it more readable
bool isEmptyFingerColorC_ss[] = {true,true,true,true};
bool isFullFingerColorB_ss[] = {false,false,false,false};
bool isFullFingerColorC_ss[] = {false,false,false,false};

int activeToIndexFilamentColorB_ss[] = {0,0,0,0}; // maybe this should actually go to -1 so it doesn't show 1 ColorB pixel all the time, but this could be fixesd by using the right inequality
int activeToIndexFilamentColorC_ss[] = {
	filamentLengths_ss[0],
	filamentLengths_ss[1],
	filamentLengths_ss[2],
	filamentLengths_ss[3]}; // ColorC uses common indices but starts hidden off the end
bool isEmptyFilamentColorB_ss[] = {true,true,true,true};
bool isEmptyFilamentColorC_ss[] = {true,true,true,true};
bool isFullFilamentColorB_ss[] = {false,false,false,false};
bool isFullFilamentColorC_ss[] = {false,false,false,false};

void setup()
{
	startSerial();
	startWiFi();
	setNameOTA();
	startOTA(); // run function to enable over-the-air updating
	startFastLED();
	startUDP();
	redefineVarsForShortSite();
}

unsigned long lastSendTime = 0;
void loop(){
	handleIncomingUDP();
	handleGrowingShrinking();

	if(millis() - lastSendTime > 150){
	    sendOutStatuses();
	    lastSendTime = millis();
	}

	handleColoringShortSite();

	ArduinoOTA.handle();
	delay(delayval);
}

////////////////////////////////////////
// CUSTOM FUNCTIONS ////////////////////
////////////////////////////////////////

void setNameOTA(){ // NOTE fix the string var handling here and combine to one function that gets passed a string
  ArduinoOTA.setHostname("shortSite"); // NOTE produced an error using string var here
}

void handleGrowingShrinking(){
  // ColorB for all fingers
  for(int i=0; i<4; i++){
	  if (isTouchedFinger_ss[i] && !isFullFingerColorB_ss[i]){ // grow ColorB if the finger is being touched and if it is not full of color B
	    activeToIndexFingerColorB_ss[i]++; // increment the index of ColorB as described above
	  } else if (!isTouchedFinger_ss[i] && !isEmptyFingerColorB_ss[i] && isEmptyFilamentColorB_ss[i]){ // shrink ColorB if the finger is not being touched and if the connected filament is empty of ColorB and if the finger is not empty of ColorB
	    // this loop is generally applied: touchlogic && is this item empty && and is the adjacent color done moving to an extreme (this is not great language, but is a start)
	    activeToIndexFingerColorB_ss[i]--; // decrement the index of ColorB as described above
	  } else {
	    // do not change anything
	  }
	  isEmptyFingerColorB_ss[i] = (activeToIndexFingerColorB_ss[i] == 0); // the finger is empty of ColorB is the index is equal to 0
	  isFullFingerColorB_ss[i] = (activeToIndexFingerColorB_ss[i] == fingerLengths_ss[i]); // the finger is full of ColorB if the index is equal to the length of the finger (maybe add "-1")    
  }

	  // for(int i=0; i<4; i++){
	  // if (isTouchedFinger_ss[i]){ // grow ColorB if the finger is being touched and if it is not full of color B
	  //   activeToIndexFingerColorB_ss[i]++;
	  // }
	  // }

  // ColorB for all filaments
  for(int j=0; j<4; j++){
    if(isTouchedFinger_ss[j] && isFullFingerColorB_ss[j] && !isFullFilamentColorB_ss[j]){ // if the finger is being touched and the finger is full of ColorB and the filament is not full of ColorB
      activeToIndexFilamentColorB_ss[j]++; // increment the index of ColorB as described above
    } else if (!isTouchedFinger_ss[j] && !isEmptyFilamentColorB_ss[j] && isEmptyFilamentColorC_ss[j]){ // if the finger is not being touched and the filament is not empty of ColorB and the filament is empty of ColorC
      activeToIndexFilamentColorB_ss[j]--; // decrement the index of ColorB as described above
    } else {
      // do not change anything
    }
    isEmptyFilamentColorB_ss[j] = (activeToIndexFilamentColorB_ss[j] == 0);
    isFullFilamentColorB_ss[j] = (activeToIndexFilamentColorB_ss[j] == filamentLengths_ss[j]);
  }

  // ColorC for all filaments
  // Sections are (1) between fingers 1 and (2) between fingers 2 and 3
  for(int k=0; k<2; k++){
    if(isFullFilamentColorB_ss[k * 2] && isFullFilamentColorB_ss[k * 2 + 1] && isTouchedFinger_ss[k] && isTouchedFinger_ss[k + 1]){ // if adjacent filaments are both full of ColorB and if both adjacent fingers are being touched
      if(!isFullFilamentColorC_ss[k * 2]){ // if the filament on one side is not full of ColorC then increment the position
        activeToIndexFilamentColorC_ss[k * 2]--; // mathematical decrement here is incrementing ColorC position from end of filament toward the finger
      } 
      if (!isFullFilamentColorC_ss[k * 2 + 1]){ // if the filament on the other side is not full of ColorC then increment the position
        activeToIndexFilamentColorC_ss[k * 2 + 1]--; // mathematical decrement here is incrementing ColorC position from end of filament toward the finger
      }
    } else if(isEmptyFingerColorC_ss[k] && (!isTouchedFinger_ss[k] || !isTouchedFinger_ss[k + 1])){ // if the finger is empty of ColorC and either finger is not being touched and the filament is not empty of ColorC
      if(!isEmptyFilamentColorC_ss[k * 2]){
         activeToIndexFilamentColorC_ss[k * 2]++;
      }
      if(!isEmptyFilamentColorC_ss[k * 2 + 1]){
         activeToIndexFilamentColorC_ss[k * 2 + 1]++;  
      }
    } else {
      // do not change anything
    }
  } 

  // ColorC for all fingers; not special cases but the code is complex here
  if (isTouchedFinger_ss[0] && isTouchedFinger_ss[1] && isFullFilamentColorC_ss[0] && !isFullFingerColorC_ss[0]){ // if finger 1 is touched and finger 2 is touched and filament 1 is full of ColorC and finger 1 is not full of ColorC then increment ColorC
    activeToIndexFingerColorC_ss[0]--; // mathematical decrement is a visual increment for ColorC
  // } else if ((!isTouchedFinger_ss[0] || !isTouchedFinger_ss[1]) && !isEmptyFingerColorC_ss[0]){ // if finger 1 or finger 2 is not touched and finger 1 is not empty of colorC then decrement the index of ColorC on finger 1
  } else if (!isEmptyFingerColorC_ss[0]){
    activeToIndexFingerColorC_ss[0]++;
  } else {
    // do not change anything
  }
  isEmptyFingerColorC_ss[0] = activeToIndexFingerColorC_ss[0] == fingerLengths_ss[0];
  isFullFingerColorC_ss[0] = activeToIndexFingerColorC_ss[0] == 0;

  if (isTouchedFinger_ss[0] && isTouchedFinger_ss[1] && isFullFilamentColorC_ss[1] && !isFullFingerColorC_ss[1]){ // if finger 1 is touched and finger 2 is touched and filament 1 is full of ColorC and finger 1 is not full of ColorC then increment ColorC
    activeToIndexFingerColorC_ss[1]--; // mathematical decrement is a visual increment for ColorC
  // } else if ((!isTouchedFinger_ss[0] || !isTouchedFinger_ss[1]) && !isEmptyFingerColorC_ss[0]){ // if finger 1 or finger 2 is not touched and finger 1 is not empty of colorC then decrement the index of ColorC on finger 1
  } else if (!isEmptyFingerColorC_ss[1]){
    activeToIndexFingerColorC_ss[1]++;
  } else {
    // do not change anything
  }
  isEmptyFingerColorC_ss[1] = activeToIndexFingerColorC_ss[1] == fingerLengths_ss[1];
  isFullFingerColorC_ss[1] = activeToIndexFingerColorC_ss[1] == 0;

  if (isTouchedFinger_ss[2] && isTouchedFinger_ss[3] && isFullFilamentColorC_ss[2] && !isFullFingerColorC_ss[2]){ // if finger 1 is touched and finger 2 is touched and filament 1 is full of ColorC and finger 1 is not full of ColorC then increment ColorC
    activeToIndexFingerColorC_ss[2]--; // mathematical decrement is a visual increment for ColorC
  // } else if ((!isTouchedFinger_ss[0] || !isTouchedFinger_ss[1]) && !isEmptyFingerColorC_ss[0]){ // if finger 1 or finger 2 is not touched and finger 1 is not empty of colorC then decrement the index of ColorC on finger 1
  } else if (!isEmptyFingerColorC_ss[2]){
    activeToIndexFingerColorC_ss[2]++;
  } else {
    // do not change anything
  }
  isEmptyFingerColorC_ss[2] = activeToIndexFingerColorC_ss[2] == fingerLengths_ss[2];
  isFullFingerColorC_ss[2] = activeToIndexFingerColorC_ss[2] == 0;

  if (isTouchedFinger_ss[2] && isTouchedFinger_ss[3] && isFullFilamentColorC_ss[3] && !isFullFingerColorC_ss[3]){ // if finger 1 is touched and finger 2 is touched and filament 1 is full of ColorC and finger 1 is not full of ColorC then increment ColorC
    activeToIndexFingerColorC_ss[3]--; // mathematical decrement is a visual increment for ColorC
  // } else if ((!isTouchedFinger_ss[0] || !isTouchedFinger_ss[1]) && !isEmptyFingerColorC_ss[0]){ // if finger 1 or finger 2 is not touched and finger 1 is not empty of colorC then decrement the index of ColorC on finger 1
  } else if (!isEmptyFingerColorC_ss[3]){
    activeToIndexFingerColorC_ss[3]++;
  } else {
    // do not change anything
  }
  isEmptyFingerColorC_ss[3] = activeToIndexFingerColorC_ss[3] == fingerLengths_ss[3];
  isFullFingerColorC_ss[3] = activeToIndexFingerColorC_ss[3] == 0;
}

void handleIncomingUDP(){
  int packetSize = Udp.parsePacket(); // this variable is redefined here for some reason and moving the code below to its own function destroys the delicate balance of power here
  if (packetSize) {
    // readAndStoreMessage(); // stores in packetBufer
    int n = Udp.read(packetBuffer, packetSize);
    Udp.read(packetBuffer, packetSize);
    packetBuffer[n] = 0;
    Serial.print("Contents: ");
    Serial.print(packetBuffer);
    Serial.print("... at millis()%10000 of ");
    Serial.print(millis()%10000); 
    Serial.println("");
    processTouchCommandShortSite();
    processColorCommand();
    processSystemCommand();
  }
}

void sendOutStatuses(){
	// Udp.beginPacket(addressMedallion,localPort);
	// Udp.write((const uint8_t*)msgIsEmptyFinger3ColorB, packetSize+1);
	// Udp.endPacket();
}

void processTouchCommandShortSite(){
	if(packetBuffer[0] == 'f' && packetBuffer[1] == '6'){ // if it is finger 1
      if(packetBuffer[2] == '1'){ // 1 in the thousands place if touch is sensed; this is a workaround for issues with sending continuous number values
        isTouchedFinger_ss[0] = true;
      } else if (packetBuffer[2] == '0'){
        isTouchedFinger_ss[0] = false;
      }
    }

    // strings from finger 2
    if(packetBuffer[0] == 'f' && packetBuffer[1] == '7'){ // if it is finger 2
      if(packetBuffer[2] == '1'){ // 1 in the thousands place if touch is sensed; this is a workaround for issues with sending continuous number values
        isTouchedFinger_ss[1] = true;
      } else if (packetBuffer[2] == '0'){
        isTouchedFinger_ss[1] = false;
      }
    }

    // strings from finger 3
    if(packetBuffer[0] == 'f' && packetBuffer[1] == '8'){ // if it is finger 3
      if(packetBuffer[2] == '1'){ // 1 in the thousands place if touch is sensed; this is a workaround for issues with sending continuous number values
        isTouchedFinger_ss[2] = true;
      } else if (packetBuffer[2] == '0'){
        isTouchedFinger_ss[2] = false;
      }
    }

    // strings from finger 4
    if(packetBuffer[0] == 'f' && packetBuffer[1] == '9'){ // if it is finger 4
      if(packetBuffer[2] == '1'){ // 1 in the thousands place if touch is sensed; this is a workaround for issues with sending continuous number values
        isTouchedFinger_ss[3] = true;
      } else if (packetBuffer[2] == '0'){
        isTouchedFinger_ss[3] = false;
      }
    }
}

void handleColoringShortSite(){
	breatheYellowGreen();

  // FINGERS MORE EFFICIENT CODING BUT LESS READABLE
  for(int finger=0; finger<4; finger++){ // finger index
    for(int pixel=0; pixel<fingerLengths_ss[finger]; pixel++){ // pixel index
      if (pixel >= activeToIndexFingerColorC_ss[finger]){ // if the current pixel index is greater than the ColorC index then make it ColorC
        finger2stripRGB_ss(finger,pixel,cRed,cGre,cBlu); // make ColorC as described above
      } else if (pixel <= activeToIndexFingerColorB_ss[finger]){ // if the current pixel index is less than the ColorB index then make it ColorB
        finger2stripRGB_ss(finger,pixel,bRed,bGre,bBlu); // make ColorB as described above
      // } else if (pixel > activeToIndexFingerColorB[finger] && pixel < activeToIndexFingerColorC[finger]){ // if the current pixel index is greater than the ColorB index and less than the ColorC index then make the pixel ColorA
      } else {
        finger2stripRGB_ss(finger,pixel,aRed,aGre,aBlu); // make ColorA as described above
      } 
    }
  }

  // FILAMENTS 
  for(int filament=0; filament<4; filament++){ // filament index
    for(int pixel=0; pixel<filamentLengths[filament]; pixel++){ // pixel index
      // the if statement below could have been written more efficiently but it is instead writte to make more sense to a reader, going in order through colors A, B, and C
      if (pixel >= activeToIndexFilamentColorC[filament]){ // if the current pixel index is greater than the ColorC index then make it ColorC
        filament2stripRGB_ss(filament,pixel,cRed,cGre,cBlu); // make ColorC as described above
      } else if (pixel <= activeToIndexFilamentColorB[filament]){ // if the current pixel index is less than the ColorB index then make it ColorB
        filament2stripRGB_ss(filament,pixel,bRed,bGre,bBlu); // make ColorB as described above
      } else {
      // if (pixel > activeToIndexFilamentColorB[filament] && pixel < activeToIndexFilamentColorC[filament]){ // if the current pixel index is greater than the ColorB index and less than the ColorC index then make the pixel ColorA
        filament2stripRGB_ss(filament,pixel,aRed,aGre,aBlu); // make ColorA as described above
      }
    }
  }

	FastLED.show();
}

void redefineVarsForShortSite(){

}

void finger2stripRGB_ss(int finger, int i, int r, int g, int b){ //NOTE this name is not good because it also sets color
	switch(finger){
		// the fingers controlled by longSiteLeft
		case 0: // Finger 1
			ledstrips[0][149 - unusedPixelsLED1_ss - i].setRGB(r,g,b); // 
			break;
		case 1: // Finger 2
			ledstrips[2][149 - unusedPixelsLED3_ss - i].setRGB(r,g,b); // 
			break;
		case 2: // Finger 3
			ledstrips[3][149 - unusedPixelsLED4_ss - i].setRGB(r,g,b); // 
			break;
		case 3: // Finger 4
			ledstrips[4][149 - unusedPixelsLED5_ss - i].setRGB(r,g,b); // 
			break;
		default:
			Serial.print("something went wrong in finger2stripRGB");
			break;
	}
}

void filament2stripRGB_ss(int filament, int i, int r, int g, int b){
	switch(filament){
		// the filaments controlled by longSiteLeft
		case 0:
			ledstrips[1][i].setRGB(r,g,b); // 
			break;
		case 1:
			ledstrips[1][intersectF2onLED2_ss - i].setRGB(r,g,b); //
			break;
		case 2:
			ledstrips[1][i + intersectF2onLED2_ss].setRGB(r,g,b); //
			break;
		case 3:
			ledstrips[1][149 - i].setRGB(r,g,b); //
			break;
		default:
			Serial.print("something went wrong in filament2stripRGB");
			break;
	}
}

void startFastLED(){
	FastLED.addLeds<NEOPIXEL, 26>(ledstrips[0],150); // NOTE tried to use LEDPINS var for some reason
	FastLED.addLeds<NEOPIXEL, 25>(ledstrips[1],150);
	FastLED.addLeds<NEOPIXEL,  4>(ledstrips[2],150);
	FastLED.addLeds<NEOPIXEL, 15>(ledstrips[3],150);
	FastLED.addLeds<NEOPIXEL, 27>(ledstrips[4],150);
}