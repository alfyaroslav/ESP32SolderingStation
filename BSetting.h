 #define UseScreen 1 //  раскомментировать если используется экран
 //#define UseButtons 1 //  раскомментировать если используются кнопки
 //#define Set_Touch 

// Выводить ли диагностические данные в СОМ---- Раскомментировать \ закомментировать при необходимости------------

#define show_diagnostics_com true;
// #define show_diagnostics_com_deep false;
// #define test_mode true; // Включает режим тестирования. Показания термопар берутся равными уставке с задержкой на 10 секунд. 

#if defined(ESP8266) || defined(ESP32)
#define Enable_WiFi
//#define Enable_WiFi_OTA
//#define Enable_Bluetooth

const char* ssid = "ALF";
const char* password = "";
#endif

// Переменные которые можно настраивать под себя--------------------------------------------

const int max_temperature_pcb = 240; // Предел температуры платы. Если термопара покажет больше профиль автоматически будет остановлен.
const int max_warmup_duraton = 300;  // Максимальная длительность выхода на стартовые температуры профиля.
const byte PreHeatPower = 3;         // мощность на этапе преднагрева НИ в процентах
const byte PreHeatTime = 5;          // время преднагрева НИ в Секундах
//const byte max_profiles = 6; 
//const byte profile_steps_pcb = 10;

//-----------------------------------------------------------
// Секция пинов подключеня----------------------------------------
#if defined(__AVR__)

    #define RelayPin1 7 // назначаем пин SSR "ВЕРХНЕГО" нагревателя
    #define RelayPin2 6 // назначаем пин SSR  НИЖНЕГО" нагревателя

    #define ZCC_PIN 0 // пин ZCC 0 = 2й или 1 = 3й цифровой пин

    // Выходы реле
    #define P1_PIN 9           // назначаем пин реле 1
    #define P2_PIN 10          // назначаем пин реле 2
    #define P3_PIN 11          // назначаем пин реле 3
    #define P4_PIN 12          // назначаем пин реле 4

    byte buzzerPin = 8;        // пин пищалки
    byte cancelSwitchPin = 21; // пин кнопки отмена или назад

    // назначаем пины усилителя термопары MAX6675 "ВЕРХНЕГО" нагревателя   clk=sck cs=cs do=so
    byte thermoCLK1 = 14; //=sck
    byte thermoCS1 = 15;  //=cs //separately
    byte thermoDO1 = 16;  //=so

    // назначаем пины усилителя термопары MAX6675 "НИЖНЕГО" нагревателя clk=sck cs=cs do=so
    byte thermoCLK2 = 14; //=sck
    byte thermoCS2 = 17;  //=cs //separately
    byte thermoDO2 = 16;  //=so

    // назначаем пины усилителя термопары MAX6675 "ПЛАТЫ" clk=sck cs=cs do=so
    byte thermoCLK3 = 14; //=sck
    byte thermoCS3 = 19;  //=cs //separately
    byte thermoDO3 = 16;  //=so
#elif defined(ESP8266) || defined(ESP32)

    #define RelayPin1 6 // назначаем пин SSR "ВЕРХНЕГО" нагревателя
    #define RelayPin2 7 // назначаем пин SSR  НИЖНЕГО" нагревателя

    //#define ZCC_PIN 0 // пин ZCC 0 = 2й или 1 = 3й цифровой пин
    #define ZCC_PIN 1 // пин ZCC 0 = 2й или 1 = 3й цифровой пин
    //#define interruptTimer // пререрывание по таймеру

    // Выходы реле
    #define P1_PIN 15           // назначаем пин реле 1
    #define P2_PIN 16         // назначаем пин реле 2
    #define P3_PIN 17         // назначаем пин реле 3
    #define P4_PIN 18          // назначаем пин реле 4

    byte buzzerPin = 14;        // пин пищалки
    //byte cancelSwitchPin = 21; // пин кнопки отмена или назад

    // назначаем пины усилителя термопары MAX6675 "ВЕРХНЕГО" нагревателя   clk=sck cs=cs do=so
    byte thermoCLK1 = 13; //=sck
    byte thermoCS1 = 5;  //=cs //separately
    byte thermoDO1 = 12;  //=so

    // назначаем пины усилителя термопары MAX6675 "НИЖНЕГО" нагревателя clk=sck cs=cs do=so
    byte thermoCLK2 = 13; //=sck
    byte thermoCS2 = 4;  //=cs //separately
    byte thermoDO2 = 12;  //=so

    // назначаем пины усилителя термопары MAX6675 "ПЛАТЫ" clk=sck cs=cs do=so
    byte thermoCLK3 = 13; //=sck
    byte thermoCS3 = 10;  //=cs //separately
    byte thermoDO3 = 12;  //=so
#endif
//-----------------------------------------------------------

//-Секция настройки экрана------------------------------------------------
// UTFT myGLCD(CTE40, 38, 39, 40, 41); // Изменять в основном скетче, строка 279 // стандартные настройки для шилда Arduino MEGA 2560 ILI9486, 16bit

/*  Используется библиотека UTFT_Rus_W1

Если необходимо изменить ориентацию экрана, то нужно отредактировать файл:
...\Documents\Arduino\libraries\UTFT_Rus_W1\tft_drivers\ВАШ ДРАЙВЕР ДИСПЛЕЯ\initlcd.h

45  LCD_Write_DATA(0x42);		// 0x22 = Rotate 0degree, 0x42 = Rotate display 180 deg.

*/

//------------------------------------------------------
// ожидаемые значения для псевдо-кнопок (по умолчанию для китайской клавиатуры)
#ifdef UseButtons

#define SetUP 145      //353
#define SetDOWN 331    //711
#define SetRIGHT 508  // 
#define SetLEFT 0     //569
#define SetSELECT 744 //4ё

#define ButtonsPin A0    // пин подключения аналоговой клавиатуры
#define A_POSSIBLE_ABERRATION 10 // допустимое отклонение analogRead от ожидаемого значения, при котором псевдо кнопка считается нажатой

#endif
//------------------------------------------------------

/*/ Код скетча для проверки номиналов кнопок:

Залить в дуину, подключить кнопки к питанию, пину A1 и массе, открыть монитор порта и нажимать кнопки.

byte key(){  
  int val = analogRead(0);
    if (val < 50) return 1;
    else if (val < 150) return 2;
    else if (val < 350) return 5;
    else if (val < 500) return 4;
    else if (val < 800) return 3;
    else return 0;  
}

// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 115200 bits per second:
  Serial.begin(115200);
}

// the loop routine runs over and over again forever:
void loop() {
  // read the input on analog pin 0:
  int sensorValue = analogRead(A1);
  // print out the value you read:
  Serial.print(key());
  Serial.print(" ");
  Serial.println(sensorValue);
  delay(300);        // delay in between reads for stability
}

//------------------------------------------------------
*/