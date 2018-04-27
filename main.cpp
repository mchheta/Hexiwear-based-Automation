#include "mbed.h"
#include "Hexi_KW40Z.h"
#include "Hexi_OLED_SSD1351.h"
#include "OLED_types.h"
#include "OpenSans_Font.h"
#include "string.h"
#include "MAX30101.h"
#include "HTU21D.h"
#include "TSL2561.h"
#define LED_ON      0
#define LED_OFF     1

void UpdateSensorData(void);
void StartHaptic(void);
void StopHaptic(void const *n);
void txTask(void);
void Heart(void);
void sendData();


DigitalOut redLed(LED1,1);
DigitalOut greenLed(LED2,1);
DigitalOut blueLed(LED3,1);


DigitalOut haptic(PTB9);
typedef unsigned char byte;
uint8_t cmd_get_sensor[3];
DigitalIn LD0(PTA29);
DigitalIn oth (PTB12, PullDown);
Serial pc(USBTX, USBRX);// Initialize Serial port
Serial uart(PTD3, PTD2);
MAX30101 heart(PTB1, PTB0, 0x57);
HTU21D temphumid(PTB1,PTB0); // HTU21D Sensor
I2C i2c(PTB1,PTB0);    // SDA, SCL
TSL2561 ambl(i2c);   
/* Define timer for haptic feedback */
RtosTimer hapticTimer(StopHaptic, osTimerOnce);

/* Instantiate the Hexi KW40Z Driver (UART TX, UART RX) */ 
KW40Z kw40z_device(PTE24, PTE25);

/* Instantiate the SSD1351 OLED Driver */ 
SSD1351 oled(PTB22,PTB21,PTC13,PTB20,PTE6, PTD15); /* (MOSI,SCLK,POWER,CS,RST,DC) */

/*Create a Thread to handle sending BLE Sensor Data */ 
Thread txThread;

 /* Text Buffer */ 
char text[20]; 

float data;
int choice;
int count=0;
uint8_t Fifoptr[8];
int sample_ftemp;
int sample_humid;
uint8_t ser;
uint8_t ch;
/****************************Call Back Functions*******************************/
void ButtonRight(void)
{
    StartHaptic();
    kw40z_device.ToggleAdvertisementMode();
}

void ButtonLeft(void)
{
    StartHaptic();
    kw40z_device.ToggleAdvertisementMode();
}

void PassKey(void)
{
    StartHaptic();
    strcpy((char *) text,"PAIR CODE");
    oled.TextBox((uint8_t *)text,0,25,95,18);
  
    /* Display Bond Pass Key in a 95px by 18px textbox at x=0,y=40 */
    sprintf(text,"%d", kw40z_device.GetPassKey());
    oled.TextBox((uint8_t *)text,0,40,95,18);
}

/***********************End of Call Back Functions*****************************/

/********************************Main******************************************/

int main()
{
        pc.baud(115200);
        uart.baud(9600);
        heart.setLED1_PA(0x0C);
        heart.setLED2_PA(0x0D);
        heart.setLED3_PA(0x0E);
        pc.printf("\r\nWelcome to Hexiwear\r\n");  
        wait(0.2);  
        pc.printf("\r\nPlace your finger on the Sensor\r\n");
        wait(0.3);
        kw40z_device.attach_buttonLeft(&ButtonLeft);
            kw40z_device.attach_buttonRight(&ButtonRight);
            kw40z_device.attach_passkey(&PassKey);

            /* Turn on the backlight of the OLED Display */
            oled.DimScreenOFF();
    
            /* Fills the screen with solid black */         
            oled.FillScreen(COLOR_BLACK);
            while (true) 
            {  
            if(uart.writeable()){
            cmd_get_sensor[0]=temphumid.sample_ftemp();
            pc.printf("Temperature: %d F\n\r", cmd_get_sensor[0]);
            
            
            cmd_get_sensor[1] = ambl.lux();
            pc.printf("Illuminance: %+7.2f [Lux]\r\n", ambl.lux());
            
            
            heart.setFIFO_CONFIG(0x08);
            heart.setMODE_CONFIG(0x02);
            heart.setTEMP_EN();
            heart.setFIFO_RD_PTR(0x06);
            heart.setFIFO_WR_PTR(0x04);
            heart.setFIFO_DATA(0x07);
            cmd_get_sensor[2] = heart.readFIFO()/800; 
            pc.printf("BPM: %d\r\n\r\n",cmd_get_sensor[2]);
            
            pc.printf("\r\n\r\n\r\n");
            pc.printf("-------------------------------------------------------------\r\n");
            /* Register callbacks to application functions */
            

            /* Get OLED Class Default Text Properties */
            oled_text_properties_t textProperties = {0};
            oled.GetTextProperties(&textProperties);    
         
 
    
    
            textProperties.fontColor = COLOR_WHITE;
            textProperties.alignParam = OLED_TEXT_ALIGN_RIGHT;
            oled.SetTextProperties(&textProperties);  
            strcpy((char *) text,"Heart Rate: ");
            oled.Label((uint8_t *)text,5,5);      
      
        /* Format the value */
            sprintf(text,"%i",heart.readFIFO()/800);
        /* Display time reading in 35px by 15px textbox at(x=55, y=40) */
            oled.TextBox((uint8_t *)text,70,5,15,15);
    
            textProperties.fontColor = COLOR_WHITE;
            textProperties.alignParam = OLED_TEXT_ALIGN_RIGHT;
            oled.SetTextProperties(&textProperties);  
            strcpy((char *) text,"Body Temp: ");
            oled.Label((uint8_t *)text,5,35);      
      
        /* Format the value */
            heart.setTEMP_EN();
            sprintf(text,"%i",(int)temphumid.sample_ftemp());
        /* Display time reading in 35px by 15px textbox at(x=55, y=40) */
            oled.TextBox((uint8_t *)text,70,35,15,15);
    
    
            textProperties.fontColor = COLOR_WHITE;
            textProperties.alignParam = OLED_TEXT_ALIGN_RIGHT;
            oled.SetTextProperties(&textProperties);  
            strcpy((char *) text,"Light: ");
            oled.Label((uint8_t *)text,5,55);      
      
        /* Format the value */
            heart.setTEMP_EN();
            sprintf(text,"%i",cmd_get_sensor[1]);
        /* Display time reading in 35px by 15px textbox at(x=55, y=40) */
            oled.TextBox((uint8_t *)text,70,55,15,15);
    
        /* Display Label at x=22,y=80 */ 
        strcpy((char *) text,"Tap Below");
        oled.Label((uint8_t *)text,22,80);
                
        /*Notify Hexiwear App that it is running Sensor Tag mode*/
        kw40z_device.SendSetApplicationMode(GUI_CURRENT_APP_SENSOR_TAG);
        
        /*Send Humidity at 90% */
        kw40z_device.SendHumidity(heart.readFIFO()/8);
        heart.setTEMP_EN();
        /*Send Temperature at 25 degrees Celsius */
        kw40z_device.SendTemperature(heart.getTEMP()*100);
        blueLed = !kw40z_device.GetAdvertisementMode(); /*Indicate BLE Advertisment Mode*/   
        sendData();
        
        //Thread::wait(50);
        //wait(1);
        }
    }
}

/******************************End of Main*************************************/

void txTask(void){
   
   while (true) 
   {
        //UpdateSensorData();
        
        /*Notify Hexiwear App that it is running Sensor Tag mode*/
        kw40z_device.SendSetApplicationMode(GUI_CURRENT_APP_SENSOR_TAG);
        
        kw40z_device.SendHumidity(heart.readFIFO()/7);
        heart.setTEMP_EN();
        /*Send Body Temperature in degrees Celsius */
        kw40z_device.SendTemperature(heart.getTEMP()*100);    
    }
}


void StartHaptic(void)  {
    hapticTimer.start(50);
    haptic = 1;
}

void StopHaptic(void const *n) {
    haptic = 0;
    hapticTimer.stop(); }
void sendData(){
        uart.putc(cmd_get_sensor[0]);
        //wait(3);
        uart.putc(ambl.lux()*10);
        //wait(3);
        uart.putc(cmd_get_sensor[2]);
        wait(1);
        
    }