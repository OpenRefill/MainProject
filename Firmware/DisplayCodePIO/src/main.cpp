#include <Arduino.h>
#include <U8g2lib.h>
#include <SPI.h>
#include <Wire.h>

#include "dxconfigmain.h"
#include "dxchars.h"

#define I2C_DEV_ADDR 0x56

uint8_t draw_state = 0;
int activeDispenser = 0; //1 to 4
uint8_t display_current_number = 0; //0 to 3

U8G2_ST7920_128X64_F_SW_SPI display1(U8G2_R0, /* clock=*/15 /* A4 */, /* data=*/16 /* A2 */, /* CS=*/17 /* A3 */, /* reset=*/U8X8_PIN_NONE);
U8G2_ST7920_128X64_F_SW_SPI display2(U8G2_R0, /* clock=*/13 /* A4 */, /* data=*/16 /* A2 */, /* CS=*/17 /* A3 */, /* reset=*/U8X8_PIN_NONE);
U8G2_ST7920_128X64_F_SW_SPI display3(U8G2_R0, /* clock=*/14 /* A4 */, /* data=*/16 /* A2 */, /* CS=*/17 /* A3 */, /* reset=*/U8X8_PIN_NONE);
U8G2_ST7920_128X64_F_SW_SPI display4(U8G2_R0, /* clock=*/18 /* A4 */ ,/* data=*/16 /* A2 */, /* CS=*/17 /* A3 */, /* reset=*/U8X8_PIN_NONE);



U8G2_ST7920_128X64_F_SW_SPI *dispArray[NUM_DISPLAYS] = {&display1,&display2,&display3,&display4};
U8G2_ST7920_128X64_F_SW_SPI *dispCurrent = dispArray[0];

void onRequest() // Anwsers to Master's "requestFrom"
{
   // SerialMon.println("Request received. Sending buffered data."); //The sending happens on the background
}

void onReceive(int len) // Anwsers to Master's "transmissions"
{
   // The way msg have been contstructed is DispenserIndex:MSG:QTY
   String requestResponse = ""; // to generate the request reply content
   String msg_str = "";   // to save Master's message

   // char msg_char[len];
   // int msgIndex = 0;
   while (Wire.available()) // If there are bytes through I2C
   {
      msg_str.concat((char)Wire.read()); // make a string out of the bytes
   }

   // msg_str.toCharArray(msg_char, len+1);
   
   Serial.print("Master Message: ");
   Serial.println(msg_str);

   // Serial.print("First char of Master Message: ");
   // Serial.println(msg_char[0]);
   
   // if (masterMessage == "1" || masterMessage == "2" || masterMessage == "3" || masterMessage == "4") 
   if (msg_str.length() == 1 && msg_str >= "1" && msg_str <= "4") 
   {
               uint8_t disp_number = msg_str[0] - '0';  // Convert the string to uint8_t
               setActiveDisplay(disp_number);
   }
   //S:XX (send the savings value for the savings screen)
   else if (msg_str[0] == 'S')                      showYouSaved(msg_str); 
   //N:XX:XXX: (number changes on UI menu, selecting amount. Order is N:<price>:<volume>:)
   else if (msg_str[0] == 'N')                      showChangedSelection(msg_str); 
   //D:XX:XX: (dispensing progress updates. Order is D:<currentAmount>:<totalAmount>:)
   else if (msg_str[0] == 'D')                      showProgress(msg_str);
   //show text values
   else if (msg_str == "Press Green")     showPressGreen();
   else if (msg_str == "Push Bottle")     showPushBottle();
   else if (msg_str == "Cancel")          showCancel();
   else if (msg_str == "Printing")        showPrinting();
   else if (msg_str == "Finished")        showFinished();
}

void resetDisplays()
{
   for (size_t i = 0; i < NUM_DISPLAYS; i++)
   {
      resetDisplayX(i);
   }
}

void resetDisplayX(uint8_t display_number)
{
   dispArray[display_number]->clearBuffer();
   dispArray[display_number]->drawXBMP(0, 0, PushBottle_width, PushBottle_height, PushAnyBtn);
   dispArray[display_number]->sendBuffer();
}

//dx_number is from 1,2,3,4, while display number 0..3
void setActiveDisplay(uint8_t dx_number)
{
   if (dx_number <= 0 || dx_number > NUM_DISPLAYS){
      Serial.printf("set-active: error display number: %d\n",dx_number);
   }
   else{
      Serial.printf("set-active: Init %d\n",dx_number);
      activeDispenser = dx_number;
      display_current_number = dx_number-1;
      dispCurrent = dispArray[display_current_number];
      dispCurrent->begin();
      dispCurrent->clearBuffer();
      dispCurrent->drawXBMP(0, 0, takka_colon_width, takka_colon_height, takkaColon);
      dispCurrent->drawXBMP(0,32,ML_width, ML_height, ML);
      dispCurrent->sendBuffer();
   }
}

void showYouSaved(String msg)
{
   // S:XX
   uint8_t first_num, second_num, third_num;
   uint8_t num_found = getDigitsFromString(msg,2,2,first_num,second_num,third_num);

   dispCurrent->clearBuffer();
   dispCurrent->drawXBMP(0, 0, YouSaved_width, YouSaved_height, YouSaved);
   
   if (num_found == 2)
   {
      dispCurrent->drawXBMP(0, YouSaved_height, NumerialWidth, NumerialHeight, numerials_32[first_num]);
      dispCurrent->drawXBMP(NumerialWidth, YouSaved_height, NumerialWidth, NumerialHeight, numerials_32[second_num]);
   }         
   else if (num_found == 1)
   {
      dispCurrent->drawXBMP(NumerialWidth, YouSaved_height, NumerialWidth, NumerialHeight, numerials_32[first_num]);
   }
   
   dispCurrent->sendBuffer();
   delay(10000);

   dispCurrent->clearBuffer();
   dispCurrent->sendBuffer();
   delay(1000);

   resetDisplayX(display_current_number);
}

void showChangedSelection(String msg)
{
   //N:XX:XXX: (number changes on UI menu, selecting amount. Order is N:<price>:<volume>:)
   // 0 1 2 3 4 5 6 7 8
   // N : X X : X X X :
   Serial.println("Number change");
   Serial.print(msg);
   //Serial.print(msg[5]);

   uint8_t pn1,pn2,pn3;
   uint8_t vn1,vn2,vn3;
   
   uint8_t num_found_price = getDigitsFromString(msg,2,2,pn1,pn2,pn3);
   uint8_t num_found_vol = getDigitsFromString(msg,5,3,vn1,vn2,vn3);

   dispCurrent->clearBuffer();
   dispCurrent->drawXBMP(0, 0, takka_colon_width, takka_colon_height, takkaColon);

   //changed price
   if (num_found_price > 0)   dispCurrent->drawXBMP(takka_colon_width, 0, NumerialWidth, NumerialHeight, numerials_32[pn1]);
   if (num_found_price > 1)   dispCurrent->drawXBMP(takka_colon_width + NumerialWidth, 0, NumerialWidth, NumerialHeight, numerials_32[pn2]);
   
   //changed volume
   dispCurrent->drawXBMP(0,32,ML_width, ML_height, ML);
   if  (num_found_vol < 3){
      dispCurrent->drawXBMP(ML_width,32,NumerialWidth, NumerialHeight, numerials_32[vn1]);
      if  (num_found_vol == 2) dispCurrent->drawXBMP(ML_width+NumerialWidth,32,NumerialWidth, NumerialHeight, numerials_32[vn2]);
   }
   else{ //assume ==3
         dispCurrent->drawXBMP(ML_width,32,16, NumerialHeight, numerials_16[vn1]);
         dispCurrent->drawXBMP(ML_width+16,32,16, NumerialHeight, numerials_16[vn2]);
         dispCurrent->drawXBMP(ML_width+16+16,32,16, NumerialHeight, numerials_16[vn3]); // change
   }
   dispCurrent->sendBuffer();
}

void showProgress(String msg)
{
   //D:XX:XX: (dispensing progress updates. Order is D:<currentAmount>:<totalAmount>:)
   Serial.println("Progress");
   uint8_t num_found_1st_val, num_found_2nd_val;
   uint8_t v1_n1,v1_n2,v1_n3;
   uint8_t v2_n1,v2_n2,v2_n3;

   num_found_1st_val = getDigitsFromString(msg,2,2,v1_n1,v1_n2,v1_n3);
   num_found_2nd_val = getDigitsFromString(msg,5,2,v2_n1,v2_n2,v2_n3);

   dispCurrent->clearBuffer();
   dispCurrent->drawXBMP(16, 0, 96, 32, Dispensing);
   if (num_found_1st_val > 0) dispCurrent->drawXBMP(0, 32, 16, 32, numerials_16[v1_n1]);
   if (num_found_1st_val > 1) dispCurrent->drawXBMP(16, 32, 16, 32, numerials_16[v1_n2]);

   dispCurrent->drawXBMP(32, 32, 16, 32, div_bits);

   if (num_found_2nd_val == 1) dispCurrent->drawXBMP(48, 32, 16, 32, numerials_16[v2_n2]);
   if (num_found_2nd_val == 2){
      dispCurrent->drawXBMP(48, 32, 16, 32, numerials_16[v2_n2]);
      dispCurrent->drawXBMP(48 + 16, 32, 16, 32, numerials_16[v2_n2]);
   }

   dispCurrent->drawXBMP(48 + 32, 32, 48, 32, takka_bits);
   dispCurrent->sendBuffer();
    
}

void showPressGreen()
{
   dispCurrent->clearBuffer();
   dispCurrent->drawXBMP(0, 0, PressGreen_width, PressGreen_height, PressGreen);
   dispCurrent->sendBuffer();
}

void showPushBottle()
{
   dispCurrent->clearBuffer();
   dispCurrent->drawXBMP(0, 0, PushBottle_width, PushBottle_height, PushBottle);
   dispCurrent->sendBuffer();

}

void showCancel()
{
   dispCurrent->clearBuffer();
   dispCurrent->drawXBMP(0, 0, RedCancel_width, RedCancel_height, RedCancel);
   dispCurrent->sendBuffer();
}

void showPrinting()
{
   dispCurrent->clearBuffer();
   dispCurrent->drawXBMP(0, 0, 128, 64, Printing);
   dispCurrent->sendBuffer();
}

void showFinished()
{
   dispCurrent->clearBuffer();
   dispCurrent->sendBuffer();
   delay(1000);
   resetDisplayX(display_current_number);
}

uint8_t getDigitsFromString(String msg, uint8_t start_pos, uint8_t num_digits, uint8_t &val1, uint8_t &val2, uint8_t &val3)
{
  uint8_t digits_found = 0;
  val1 = val2 = val3 = 0;
  unsigned int msg_len = msg.length();

  for (uint8_t i = start_pos; i < msg_len && digits_found < num_digits; i++) {
    char c = msg[i];
    if (isdigit(c)) {
      uint8_t val = c - '0';
      switch (digits_found) { 
        case 0: val1 = val; break;
        case 1: val2 = val; break;
        case 2: val3 = val; break;
      }
      digits_found++;
    }
    else break;
  }

  return digits_found;
}

/* --- Arduino setup and loop Functions --- */
void setup(void)
{
   Serial.begin(115200);
   
   Wire.onReceive(onReceive);
   Wire.onRequest(onRequest);
   Wire.begin((uint8_t)I2C_DEV_ADDR); // Starting Wire as Slave

   for (size_t i = 0; i < NUM_DISPLAYS; i++)
   {
      dispArray[i]->begin();
      resetDisplayX(i);
   }
}

void loop(void)
{
}
