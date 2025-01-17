//---Тема, в которой обсуждается эта разработка: goo.gl/ijbJho---

// Параметры в структуре ИЗМЕНЕНЫ и перезапишут данные других скетчей при заливке в Ардуино! Если вы обновили данные в дефолтном профиле, измените значение EEPROM_No_Write
// Рампа для низа, Верха и коррекция общей мощности по отклонению температуры платы.

// управление включением/выключением пайки и переключением профилей с РС
// сохранение и загрузка профилей с/на диск PC
// не используется экран, только ПК
// ПИД по измерению dT
// добавлен 3 пид + integral antiwindup для всех 3 пид.

// Защита !!!!!
// 1. Перегрев платы выше max_temperature_pcb (240гр) автоматически останавливает выполнение профиля
// 2. Если этап преднагрева перед стартом профиля длится более max_warmup_duraton (300сек \ 5мин) то выполнение профиля останавливается.
// 3. Автопауза при выполнении профиля - при отклонении температуры более чем max_pcb_delta выполнение профиля приостанавливается на hold_lenght секунд
// 4. Мягкий старт нагревателей 5 секунд 3% мощности перед стартом профиля (можно изменить в переменной)


// Шаги ВИ,НИ, Платы не привязаны друг к другу по времени. Шаг для каждого может быть произвольной длинны до 250 секунд. Максимум 29 шагов (30 точек графика)
// Для подготовки профиля есть специальный файл Excel: Plot_Edit.xlsx


// Словарь команд для ручного управления. Большинство дублируется кнопками в Интерфейсе Port Plotter:

// E - прервать текущее задание = кнопка STOP !!!
// S - Начать выполнение профиля // Если указана темпетарура то нагрев до фикс температуры.
// J - Старт с соответствующей точки профиля если плата горячая.

// H - Включить HOLD - приостанавливаем выполнение профиля и переходим на удержание температуры платы.
// U - Прибавить температуру во время MANUAL HOLD.  При нажатии во время пайки скетч встаёт на паузу автоматически
// D - Убавить температуру во время MANUAL HOLD. При нажатии во время пайки скетч встаёт на паузу автоматически

// N+цифра - сохранить профиль в EEPROM.
// P - загружает профиль из EEPROM в оперативную память и выводит его в COM)
// R - следующий профиль
// L - предыдущий профиль

// M -  M 1 - не используется, M 2,150 - установить температуру НИ 150 и греть до отмены / М 3,150 - установить температуру платы и греть до отмены
// Q - Q 1,100 - установить мощность ВИ 100, /  Q 2,30 установить мощность НИ 30 и греть до отмены.


// TODO

// 1. Вывод всех ошибок в Буфер данных в 2 потока: Статусы, Информация для графиков.
// 2. +\-Отправка данных на пк: во время выполнения профиля: температура, мощности, время по графику, время факт. Включена ли пауза. Синхронизация данных о профиле в начале программы
// 3. +++ Получение данных с ПК: Профиль, команды вкл\выкл\пауза, переключение профилей. данные для отправки генерируются в эксельке.
// 4. +++ Режим удержания температуры платы на хх градусов - Включение паузы вручную в нужный момент
// 5. +++ Режим удержания температуры нагревателя на хх градусов
// 6. Автотюнинг ПИД
// 7. +++ DONE Исправить Перерегулирование ПИД платы на этапе Warmup ++++++++++++++ FIXED AntiWindup
// 8. +++ Исправить активацию автопаузы на этапе остывания (на последних 2 шагах платы отключена автопауза.) FIXED (?)
// 9. +++ Подумать как вносить коррекцию с учетом инерционности нагревателей. // Правильно отстроил ПИД платы
// 10. Метки времени \ температуры для подачи сигналов в профиле
// 11. +++ В режиме паузы ввести возможность корректировать температуру НИ\Платы. Если во время паузы температура платы ушла за макс отклонение, то после отключения паузы - перезапустить профиль.
// 12. +++ Изменить логику HotStart чтобы он явно запускался отдельной кнопкой. По умолчанию отключен. Убрать из профиля.
// 13. +++ Статусы отправляемые через ком (Idle, Hold, Pause, Sound, Error) (@0)
// 14. +++ Исправить логику Автопаузы при слишком быстром нагреве платы.
// 15. +++ Добавить команды управления мощностью нагревателей. (Q 1 = ВИ , Q 2 = НИ).
// 16. Исправить ПИД платы. Сейчас большой перелёт. Продумать алгоритм его настройки
// 17. Исправить логику удержания температуры платы \ НИ при переходе из пайки по профилю к ручному управлению
// 18. Возможность изменить делители для параметров ПИД
// 19. Проверить корректность работы UP\DOWN в разных режимах
// 20. Проверить логику HOTSTART при переходах из IDLE, HOLD_MANUAL, RUN. Возможно стоит использовать прыжок если идет перегрев платы относительно графика.
// 21. Определиться с форматом звуковых сигналов, и их выводом в плоттер \ на буззер
// 22. Привести к одному формату отправку служебной информации плоттером в порт (Serial.print, sprintf (buf,.., s->print(.. )
// 23. +++ Исправлена логика ручного управления температурой HOLD_MANUAL. ВИ принудительно отключается.
// 24. +++ Отключена коррекция температур нагревателей на этапе преднагрева
// 25. Test Mode, в котором показания термопар берутся равными уставке с задержкой на 10 секунд.  
// 26. Добавить паузу между AutoHold

/**  Список статусов
 *
 *   @0 IDLE - режим ожидания
 *   @1 WARMUP - Этап Преднагрева нагревателей и платы до стартовых температур профиля (включает плавный старт и выход на начальные температуры профиля)
 *   @2 RUN - Собственно этап пайки по профилю
 *   @3 HOLD AUTO - включена автопауза
 *   @4 HOLD MANUAL - включена ручная пауза
 *   @5 COMPLETE - профиль выполнен
 *   @6 SIGNAL (Sound type)
 *
 *    Serial.println("@0;");
 */

#include <EEPROM.h>
#include "BSetting.h"
#ifdef UseScreen
 #if defined(__AVR__) 
   #include <UTFT.h>
 #elif defined(ESP8266) || defined(ESP32)
   #include <TFT_CONV.h>
 #endif

#endif

#ifdef Enable_WiFi
  #include <WiFi.h>
  #include <NTPClient.h>
  #include <WiFiUdp.h>
  #include <AsyncTCP.h>
  #include <ESPAsyncWebServer.h>
  #ifdef Enable_WiFi_OTA
  #include <AsyncElegantOTA.h>
  #endif
  #include "Web.h"
  #include "Profile.h"
  #include <ArduinoJson.h>
#endif

#if defined(ESP8266) || defined(ESP32)
  #include "esp_system.h"
  #include "FFat.h"
  #include "FS.h"
  #include <SPI.h>
  #include <dong.wav.h>
  
  #include <Wire.h>
  //#include <Adafruit_MLX90614.h>

  #ifdef Enable_WiFi
  AsyncWebServer server(80);
  AsyncWebSocket ws("/ws"); 
  WiFiUDP ntpUDP;
  NTPClient timeClient(ntpUDP);
  #endif
  
  /*Adafruit_MLX90614 mlx = Adafruit_MLX90614();*/
   
#endif


// Секция переменных ПИД----------------------------------------------

float integra1, integra2, integra3;       // интегральные составляющие ВИ и НИ и платы
float e1, p1, d1, e2, p2, d2, e3, p3, d3; // ошибка регулирования, П-составляющая, Д-составляющая для ВИ, НИ и платы соответственно

int SetPoint_Top;           // Задание по графику для Пид ВИ на текущую секунду
int SetPoint_Bottom;        // Задание по графику для Пид НИ на текущую секунду
int SetPoint_Pcb;           // Задание по графику для Пид ПЛАТЫ на текущую секунду
double Input1;              // Показания термопары ВИ в текущую секунду с фильтрацией
byte Output1;               // Выход ПИД ВИ - мощность на текущую секнду
double Input2;              // Показания термопары НИ в текущую секунду с фильтрацией
byte Output2;               // Выход ПИД НИ - мощность на текущую секнду
double Input3;              // Показания термопары Платы в текущую секунду с фильтрацией
//double Input4;              // Показания термопары Платы в текущую секунду с фильтрацией
float Output3;              // Выход ПИД Платы - коррекция уставки ВИ и НИ на текущую секнду
int tc1, tc2, tc3;          // Показания термопар равны input 1-2-3 целочисленные
int temp_correction_top;    // значение коррекции температуры ВИ регулируемое ПИД платы
int temp_correction_bottom; // значение коррекции температуры НИ регулируемое ПИД платы

int AverageOutput1, AverageOutput2, AverageOutput3 = 0; //  Average output for each heater for 1 second

#define i_min 0.0   // минимум И составляющей
#define i_max 100.0 // максимум И составляющей


// секция алгоритма Брезенхема---Распределение мощности равномерно на 1 секунду----------------
int er1 = 1;
int er2 = 1;
int reg1;
int reg2;
boolean out1;
boolean out2;
//-----------------------------------------------------------


// Секция профиля----------------------------------------------------------
struct pr
{                                  // основные поля профиля
  int time_step_top[30];           // время от старта профиля до шага ВИ //потолок 9 часов
  int temperature_step_top[30];    // температура в НАЧАЛЕ текущего шага ВИ
  int time_step_bottom[30];        // время от старта профиля до шага НИ
  int temperature_step_bottom[30]; // температура в НАЧАЛЕ текущего шага НИ
  int time_step_pcb[10];           // время от старта профиля до шага Платы
  int temperature_step_pcb[10];    // температура в НАЧАЛЕ текущего шага Платы
  // int beep_time[10];            //время через которое подать сигнал
  // int beep_temp[10];            //Температура платы при которой подать сигнал
  byte profile_steps;          // количество шагов ВИ и НИ
  byte table_size;             // размер стола
  byte kp1;                    // пропорциональный коэффициент ВИ
  byte ki1;                    // интегральный коэффициент     ВИ
  byte kd1;                    // дифференциальный коэффициент ВИ
  byte kp2;                    // пропорциональный коэффициент НИ
  byte ki2;                    // интегральный коэффициент     НИ
  byte kd2;                    // дифференциальный коэффициент НИ
  byte kp3;                    // пропорциональный коэффициент Платы
  byte ki3;                    // интегральный коэффициент     Платы
  byte kd3;                    // дифференциальный коэффициент Платы
  byte max_correction_top;     // максимальная коррекция температуры ВИ в обе стороны
  byte max_correction_bottom;  // максимальная коррекция температуры НИ в обе стороны
  byte max_pcb_delta;          // максимальный недогрев Платы после которого включаеся Пауза
  byte hold_lenght;            // длительность паузы в секундах
  byte participation_rate_top; // коэфициент участия ВИ (от 0 - догреваем только низом, до 100 догреваем только верхом)(при отклонении температыры платы от профиля)
  char alias[20];
};

int SizeProfile = sizeof(pr); // длинна поля данных
pr profile;                   // структура для параметров

byte profile_steps_pcb = 10;   // количество шагов профиля Платы // Лучше не менять
byte max_profiles = 6; // Максимальное число профилей  !!!общий размер не должен превышать объем EEPROM!!!

#define ARRAY_SIZE(array) ((sizeof(array))/(sizeof(array[0])))


// Переменные для рассчета шагов по графику-------------------------------------------------------

byte max_correction_total;

byte ParticipationRateBottom;           // коэфициент участия низа. При включении вычисляется автоматом на основе profile.participation_rate_top
byte CurrentStepTop;                    // номер шага профиля ВИ
byte CurrentStepBottom;                 // номер шага профиля НИ
byte CurrentStepPcb;                    // номер шага профиля Платы
byte DurationCurrentStepTop;            // Длительность текущего шага ВИ
byte DurationCurrentStepBottom;         // Длительность текущего шага НИ
byte DurationCurrentStepPcb;            // Длительность текущего шага Платы
byte TimeCurrentStepTop;                // секунд от начала текущего шага ВИ
byte TimeCurrentStepBottom;             // секунд от начала текущего шага НИ
byte TimeCurrentStepPcb;                // секунд от начала текущего шага Платы
int TemperatureDeltaTop;                // дельта температур текущего и следующего шага ВИ
int TemperatureDeltaBottom;             // дельта температур текущего и следующего шага НИ
int TemperatureDeltaPcb;                // дельта температур текущего и следующего шага Платы
int TempSpeedTop;                       // Рассчитанная скорость роста температуры ВИ (может быть положительная и отрицательная) умножена на 100
int TempSpeedBottom;                    // Рассчитанная скорость роста температуры НИ (может быть положительная и отрицательная) умножена на 100
int TempSpeedPcb;                       // Рассчитанная скорость роста температуры Платы (может быть положительная и отрицательная) умножена на 100
unsigned int CurrentProfileSecond;      // Cчетчик времени по профилю
unsigned long CurrentProfileRealSecond; // Какая сейчас секунда профиля с момента запуска (с учетом пауз, прогрева )
int PcbDelta, BottomDelta, TopDelta;    // Отклонение температуры Платы от профиля
int manual_temperature;                 // Переменная для ручной регулировки температуры
int manual_power;                       // Переменная для ручной регулировки мощности

//------------------------------------------------------------------------

// Секция флагов-----------------------------------------------------------
bool manual_temp_changed = false;    // Флаг что мы вручную изменили температуру
bool manual_temperature_pcb = false; // Флаг что мы из Idle рулим темп платы

bool ManualHoldEnable = false; // переменная для ручного включения HOLD (удержания температуры нагревателей)
byte AutoHoldCounter = 0;      // счетчик сколько еще держать Hold
unsigned long LastAutoHold = 0;    // время последнего включения автопаузы 
bool jump_if_pcb_warm = false; // начинать выполнение профиля с ближайшей подходящей точки графика платы (горячий старт)
bool blockflag, blockflagM = false; // Block sending temperatures data from BUF every second to prevent data corruption, and Manual block
//------------------------------------------------------------------------

// Секция переменных общего назначения-------------------------------------


#define SENSOR_SAMPLING_TIME 250  // время чтения температуры и пересчёта ПИД(милисекунды)
#define COM_PORT_SAMPLE_TIME 1000 // время передачи данных на РС

long previousMillis;                // это для счетчиков
unsigned long NextReadTemperatures; // переменная для обновления текущей температуры
byte Secs = 0;                      // Счетчик количества прерываний от детектра ноля. каждые 100 отправляет данные на ПК
long nextRead2 = 0;
byte currentProfile = 1; // номер профиля при включении (в вычислениях используется currentProfile-1)
char itoaTemp[8];       // буфер для преобразования числа в строку
long nextSound = 0;     // переменная для звуковых сигналов


 #if defined(ESP8266) || defined(ESP32)
    char json_buffer[550];

    #ifdef Enable_Bluetooth
      char buf2[32];   //буфер вывода сообщений через сом порт
    #endif

    #ifdef Enable_WiFi
      char buf_WriteFile[100];   //буфер записи в файл
      char buf_WebPlot[100];

      int myInt = 0;
      String JsonTextError;
      StaticJsonDocument<550> jsonDocument;
      StaticJsonDocument<200> json_profile;
      StaticJsonDocument<100> json_profile_settings;

      typedef enum WS_STATUS : byte
      {
        WS_CMD_NULL,
        WS_CMD_DOWN,
        WS_CMD_UP,
        WS_CMD_LEFTc,
        WS_CMD_RIGHTc,
        WS_CMD_SAVE,
        WS_CMD_CANCEL,
        WS_CMD_MANUAL_POWER,
        WS_CMD_MANUAL_TEMP,
        WS_CMD_START,
        WS_CMD_HOTSTART
      } ws_status_t;

      ws_status_t ws_status;

    #endif
 #endif


 #ifdef UseButtons
//состояние кнопок по умолчанию
int upSwitchState = 0;
int downSwitchState = 0;
int rightSwitchState = 0;
int leftSwitchState = 0;
int cancelSwitchState = 0;
int okSwitchState = 0;

//переменные для кнопок
byte Hselect = 0;             //выбор параметра для изменения во время пайки
long ms_button = 0;           //время при котором была нажата кнопка
boolean  button_state = false;//признак нажатой кнопки
boolean  button_long_state = false;//признак длинного нажатия кнопки
boolean  button_state1 = false; //признак нажатой кнопки

#endif

//------------------------------------------------------------------------

// секция ввода/вывода для ПЭВМ-----------------------------------------------
char buf[45];  // буфер вывода температур через сом порт
char buf2[60]; // буфер вывода служебных данных через сом порт вр время выполнения профиля
 
//---------------------------------------------------------------------------

// Cекция для кнопок----------------------------------------------------------

#ifdef UseButtons
          //#define A_PINS_BASE 100 // номер с которого начинается нумерация наших "псевдо-кнопок".
          #define PIN_RIGHT 100
          #define PIN_UP 101
          #define PIN_DOWN 102
          #define PIN_LEFT 103
          #define PIN_SELECT 104
 
          struct A_PIN_DESC{ // определяем  структуру которой будем описывать какое значение мы ожидаем для каждого псевдо-пина
             byte pinNo; // номер пина
             int expectedValue;// ожидаемое значение
          };
          A_PIN_DESC expected_values[]={ // ожидаемые значения для псевдо-кнопок из BSetting.h
            { PIN_UP,SetUP},
            { PIN_DOWN,SetDOWN},
            { PIN_RIGHT,SetRIGHT},
            { PIN_LEFT,SetLEFT},
            { PIN_SELECT,SetSELECT}
          };
          #define A_PINS_COUNT sizeof(expected_values)/sizeof(A_PIN_DESC) // вычисляем сколько у нас всего "псевдо-кнопок" заданно.
           bool digitalReadA(byte pinNo){
   
            for(byte i=0;i<A_PINS_COUNT;i++){ // ищем описание нашего всевдо-пина
              A_PIN_DESC pinDesc=expected_values[i];// берем очередное описание
                if(pinDesc.pinNo==pinNo){ // нашли описание пина?
                    int value=analogRead(ButtonsPin); // производим чтение аналогово входа
                  return (abs(value-pinDesc.expectedValue)<A_POSSIBLE_ABERRATION); // возвращаем HIGH если отклонение от ожидаемого не больше чем на A_POSSIBLE_ABERRATION
               }
           }
           return LOW; // если не нашли описания - считаем что пин у нас LOW
          }
          //Назначаем пины кнопок управления
          int upSwitchPin = PIN_UP;
          int downSwitchPin = PIN_DOWN;
          int rightSwitchPin = PIN_RIGHT;
          int leftSwitchPin = PIN_LEFT;
         // int cancelSwitchPin = PIN_LEFT;
          int okSwitchPin = PIN_SELECT;

          
#endif

// секция переменных для дисплея---------------------------------------------
 
#ifdef UseScreen
  #if defined(__AVR__)
    UTFT myGLCD(CTE40, 38, 39, 40, 41); // стандартные настройки для шилда Arduino MEGA 2560 ILI9486, 16bit
    // Подгружаем шрифты
    extern uint8_t SmallFont[];
    extern uint8_t BigFont[];
    //extern uint8_t BFontRu[];
    // extern uint8_t BigFontRus[];       //Кирилица
    extern uint8_t SevenSegNumFont[];

  #elif defined(ESP8266) || defined(ESP32)
      TFT_CONV myGLCD;
  #endif


// long prev_millis;                      // For rate measurement
// double Input3_1sec,Input3_2sec = 0;   // For rate measurement
 
bool updateScreen = false;                   // флаг обновления экрана

#endif

// these are the different states of the sketch. We call different ones depending on conditions
//  ***** TYPE DEFINITIONS *****
typedef enum REFLOW_STATE : byte
{
  REFLOW_STATE_IDLE,
  REFLOW_STATE_INIT,         // Анализ с какого шага начинать профиль, фиксация температур
  REFLOW_STATE_PRE_HEATER,   // Прогрев ВИ и НИ на мощности PreHeatPower в течение PreHeatTime секунд, ПИД отключены. Чтобы уменьшить нагрузку на управляющие элементы.
  REFLOW_STATE_WARMUP,       // Преднагрев до начальных значений температуры в первом шаге профиля ВИ, НИ, Платы. ПИД включены.
  REFLOW_STATE_RUNNING,      // Выполнение профиля ВИ, НИ, Платы. (объединенные STEP)
  REFLOW_STATE_HOLD_AUTO,    // Приостановка выполнения профиля, фиксация температур ВИ и НИ по графику, чтобы плата догрелась либо остыла (коррекция с учетом отклонения температуры платы продолжается)
  REFLOW_STATE_HOLD_MANUAL,  // Приостановка выполнения профиля вручную, фиксация температуры ПЛАТЫ, если в профиле нет задания для платы то фиксация температуры Низа.
  REFLOW_STATE_POWER_MANUAL, // Режим ручного управления мощностью для этапа тюнинга
  REFLOW_STATE_COMPLETE      // Завершение профиля. Из-за ошибки \ принудительной остановки \ успешного выполнения.
} reflowState_t;

typedef enum REFLOW_STATUS : byte // this is simply to check if reflow should be running or not
{
  REFLOW_STATUS_OFF,
  REFLOW_STATUS_ON
} reflowStatus_t;
reflowStatus_t reflowStatus;
reflowState_t reflowState;

byte prev_reflowState = 0; // для сохранения состояния перед переходом в Hold

// Command Parser area ------------Parser Originally written by Christopher Wang aka Akiba.----------------------
//  ***** TYPE DEFINITIONS *****

#define MAX_CMD_SIZE 160

// command line structure
typedef struct _cmd_t
{
  char *cmd;
  void (*func)(int argc, char **argv);
  struct _cmd_t *next;
} cmd_t;

void cmdInit(Stream *);
void cmdPoll();
void cmdAdd(const char *name, void (*func)(int argc, char **argv));

Stream *cmdGetStream(void);
uint32_t cmdStr2Num(char *str, uint8_t base);

// command line message buffer and pointer
static uint8_t msg[MAX_CMD_SIZE];
static uint8_t *msg_ptr;

// linked list for command table
static cmd_t *cmd_tbl_list, *cmd_tbl;

// text strings for command prompt (stored in flash)

static Stream *stream;

// int led_pin = 50;
// bool led_blink_enb = false;
// int led_blink_delay_time = 1000;
// int pwm_pin = 51;

// END Command Parser area ---------------Parser Originally written by Christopher Wang aka Akiba.---------------------
void beep(int times);
void HOLD(int arg_cnt, char **args);
void DOWN(int arg_cnt, char **args);
void UP(int arg_cnt, char **args);
void LEFTc(int arg_cnt, char **args);
void RIGHTc(int arg_cnt, char **args);
void SAVE(int arg_cnt, char **args);
void CMD_CANCEL(int arg_cnt, char **args);
void BLOCK(int arg_cnt, char **args);
void UNBLOCK(int arg_cnt, char **args);

void update_steps(int arg_cnt, char **args, int *steps_array, byte *steps_count, const char *command);

void PRINT_PROFILE(int arg_cnt, char **args);
void MANUAL_POWER(int arg_cnt, char **args);
void MANUAL_TEMP(int arg_cnt, char **args);
void START(int arg_cnt, char **args);
void HOTSTART(int arg_cnt, char **args);

void update_time_step_top(int arg_cnt, char **args);
void update_temperature_step_top(int arg_cnt, char **args);
void update_time_step_bottom(int arg_cnt, char **args);
void update_temperature_step_bottom(int arg_cnt, char **args);
void update_time_step_pcb(int arg_cnt, char **args);
void update_temperature_step_pcb(int arg_cnt, char **args);

void update_kp1(int arg_cnt, char **args);
void update_ki1(int arg_cnt, char **args);
void update_kd1(int arg_cnt, char **args);
void update_kp2(int arg_cnt, char **args);
void update_ki2(int arg_cnt, char **args);
void update_kd2(int arg_cnt, char **args);
void update_kp3(int arg_cnt, char **args);
void update_ki3(int arg_cnt, char **args);
void update_kd3(int arg_cnt, char **args);
void update_table_size(int arg_cnt, char **args);
void update_max_correction_top(int arg_cnt, char **args);
void update_max_correction_bottom(int arg_cnt, char **args);
void update_max_pcb_delta(int arg_cnt, char **args);
void update_hold_lenght(int arg_cnt, char **args);
void update_participation_rate_top(int arg_cnt, char **args);
void update_profile_steps(int arg_cnt, char **args);
void update_aliasprofile(int arg_cnt, char **args);


#if defined(ESP8266) || defined(ESP32)

  String listFiles(bool ishtml = false);
  #ifdef Enable_WiFi
    String IPlocal;
    void notFound(AsyncWebServerRequest *request);
    void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
  #endif

  #ifdef interruptTimer
    hw_timer_t *Timer0_Cfg = NULL;
  #endif

  void IRAM_ATTR INT0_ISR()
  {
      Dimming();
      //Serial.println("INT0_ISR");
  }

  #ifdef Enable_WiFi

    String DataFile = "log_save.csv";
    File file;

    void StartLogFile(bool start) {

      int file_size = 0;

      /*if (!start) {
      File file2 = SPIFFS.open("/"+DataFile, FILE_READ);
      file_size = file2.size();
      
      if (file_size > 0) {
        String file_name = String(timeClient.getEpochTime());
        SPIFFS.rename("/"+DataFile, "/" + file_name +"_"+ DataFile);
      }
     }
    */

    #ifdef show_diagnostics_com
      Serial.println(listFiles());
    #endif

    File root = FFat.open("/");

    file = FFat.open("/"+DataFile, FILE_READ);
    file_size = file.size();
    file.close();
    if (file_size > 0) {
       String file_name = String(timeClient.getEpochTime());
       FFat.rename("/"+DataFile, "/" + file_name +"_"+ DataFile);
    }

    file = FFat.open("/"+DataFile, FILE_WRITE);
    if(!file){
      #ifdef show_diagnostics_com 
        Serial.println("There was an error opening the file for writing");
      #endif
    } 
    file.close();
  }

  void GenerateJson(int cmd) {

    //cmd
    // 1 - Chart
    // 2 - Meters
    // 3 - Chart + Meters
    // 4 - Status
    // 5 - Time
    // 6 - State + Time
    // 7 - Reload Profile

  
    jsonDocument.clear();
    jsonDocument["cmd"] = (int)cmd;
    jsonDocument["time"] = (int)CurrentProfileSecond;
    jsonDocument["profile_name"] = profile.alias;

    if (cmd == 4 || cmd == 6) {
      if (reflowState == REFLOW_STATE_RUNNING) {
          jsonDocument["status"] = "RUN";
        } else if (reflowState == REFLOW_STATE_IDLE) {
          jsonDocument["status"] = "IDLE";
        } else if (reflowState == REFLOW_STATE_WARMUP || reflowState == REFLOW_STATE_PRE_HEATER) {
          jsonDocument["status"] = "WARM";
        } else if (reflowState == REFLOW_STATE_HOLD_AUTO) {
          jsonDocument["status"] = "PAUSE";
        } else if (reflowState == REFLOW_STATE_HOLD_MANUAL || reflowState == REFLOW_STATE_POWER_MANUAL) {
          jsonDocument["status"] = "MANUAL";
        }
    }

   else {  
    jsonDocument["chart_temp_top"] = (int)tc1;
    jsonDocument["chart_temp_bottom"] = (int)tc2;
    jsonDocument["chart_temp_pcb"] = (int)tc3;
    

    if (Input1 <= -99) {
     jsonDocument["temp_top"] = "ERROR";
    }
    else {
    jsonDocument["temp_top"] = tc1;
    }
    if (Input2 <= -99) {
     jsonDocument["temp_bottom"] = "ERROR";
    }
    else {
    jsonDocument["temp_bottom"] = tc2;
    }
    if (Input3 <= -99) {
     jsonDocument["temp_pcb"] = "ERROR";
    }
    else {
    jsonDocument["temp_pcb"] = tc3;
    }
    
    jsonDocument["power_top"] = Output1;
    jsonDocument["power_bottom"] = Output2;
    jsonDocument["power_pcb"] = Output3;

    jsonDocument["delta_top"] = TopDelta;
    jsonDocument["delta_bottom"] = BottomDelta;
    jsonDocument["delta_pcb"] = PcbDelta;

    jsonDocument["setpoint_top"] = SetPoint_Top;
    jsonDocument["setpoint_bottom"] = SetPoint_Bottom;
    jsonDocument["setpoint_pcb"] = SetPoint_Pcb;
    jsonDocument["reflowState"] = reflowState;
    jsonDocument["TextError"] = JsonTextError;
   }

    serializeJsonPretty(jsonDocument, json_buffer);
    ws.textAll(json_buffer);

    if (cmd == 2) 
     //if (reflowState >= REFLOW_STATE_PRE_HEATER)
     if ((reflowState >= REFLOW_STATE_RUNNING ) and (reflowState < REFLOW_STATE_COMPLETE))
     {
      file = FFat.open("/"+DataFile, FILE_APPEND);
      sprintf (buf_WebPlot, "%d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d", (int)CurrentProfileSecond, (int)tc1, (int)tc2, (int)tc3, (Output1), (Output2), (Output3), (int)TopDelta, (int)BottomDelta, (int)PcbDelta );
      file.println(buf_WebPlot);
      file.close();
    }
  }


 void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    String message = (char*)data;

    if (strcmp((char*)data, "getData") == 0) {
      GenerateJson(2);
    }
    if (strcmp((char*)data, "START") == 0) {
      ws_status = WS_CMD_START;
    }
    if (strcmp((char*)data, "CANCEL") == 0) {
      ws_status = WS_CMD_CANCEL;
    }
     if (strcmp((char*)data, "HOT_START") == 0) {
      ws_status = WS_CMD_HOTSTART;
    }
    if (strcmp((char*)data, "LEFT") == 0) {
      ws_status = WS_CMD_LEFTc;
    }
    if (strcmp((char*)data, "RIGHT") == 0) {
      ws_status = WS_CMD_RIGHTc;
    }
     
    if (strcmp((char*)data, "UP") == 0) {
       if (reflowState == REFLOW_STATE_IDLE) {
        char* tempargs[3] = {"1","2","150"}; // Передаем в функцию MANUAL_TEMP три аргумента
        MANUAL_TEMP(3, tempargs);
       }
       else {
         UP(0,0);//вызов функции UP
       }
    }

    if (strcmp((char*)data, "DOWN") == 0) {
       if (reflowState == REFLOW_STATE_IDLE) {
        char* tempargs[3] = {"1","2","100"}; // Передаем в функцию MANUAL_TEMP три аргумента
        MANUAL_TEMP(3, tempargs);
       }
       else {
         DOWN(0,0);//вызов функции UP
       }
    }
  }
 }

 void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      #ifdef show_diagnostics_com
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      #endif
      break;
    case WS_EVT_DISCONNECT:
      #ifdef show_diagnostics_com
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
      #endif
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
 }
 #endif

#endif

void printProfileNumber()
{
  blockflag=true;
  Serial.print("!profile: ");
  Serial.print(currentProfile);
  Serial.println(";");
  Serial.print("!aliasprofile: ");
  Serial.print(profile.alias);
  Serial.println(";");
  blockflag=false;

#ifdef UseScreen

      myGLCD.setFont(BigFont);
      myGLCD.setColor(VGA_WHITE);
      myGLCD.print("                    ",180, 300); // Clear profile name space
      myGLCD.print(profile.alias,180, 300);

#endif

}


void loadProfile() // this function loads whichever profile currentProfile variable is set to  TODO исправить под новый формат профиля
{

  EEPROM.get((currentProfile - 1) * SizeProfile, profile); // читаем профиль с EEPROM в память

  ParticipationRateBottom = 100 - profile.participation_rate_top; // считаем коэфициент участия низа
  max_correction_total = profile.max_correction_top + profile.max_correction_bottom;

  delay(100);
  
  // CreateJson
  json_profile.clear();
  JsonObject nested = json_profile.createNestedObject();
  nested["profile_size"] = (int)ARRAY_SIZE(profile.time_step_top);
  nested["name"] = "Profile Temp Top";
  nested["type"] = "line";
  nested["color"] = "#C74A20";
  JsonObject nested5 = nested.createNestedObject("marker");
  nested5["symbol"] = "square";
  JsonArray payload_values = nested.createNestedArray("data");

  JsonObject nested2 = json_profile.createNestedObject();
  nested2["profile_size"] = (int)ARRAY_SIZE(profile.temperature_step_top);
  nested2["name"] = "Profile Temp Bottom";
  nested2["type"] = "line";
  nested2["color"] = "#7F32C7";
  nested5["symbol"] = "square";
  JsonArray payload_values2 = nested2.createNestedArray("data");

  for (int i = 0; i < (int)profile.profile_steps; i++)
  {
      JsonArray arr_tmp = payload_values.createNestedArray();
      arr_tmp.add((int)profile.time_step_top[i]);
      arr_tmp.add((int)profile.temperature_step_top[i]); 
    
      JsonArray arr_tmp2 = payload_values2.createNestedArray();
      arr_tmp2.add((int)profile.time_step_bottom[i]);
      arr_tmp2.add((int)profile.temperature_step_bottom[i]); 
  }
  
  JsonObject nested3 = json_profile.createNestedObject();
  nested3["profile_size"] = (int)ARRAY_SIZE(profile.time_step_pcb);
  nested3["name"] = "Profile Temp PCB";
  nested3["type"] = "line";
  nested3["color"] = "#699a32";
  nested5["symbol"] = "square";

  JsonArray payload_values3 = nested3.createNestedArray("data");
  for (int i = 0; i < (int)profile_steps_pcb; i++)
  {
     JsonArray arr_tmp3 = payload_values3.createNestedArray();
    arr_tmp3.add((int)profile.time_step_pcb[i]);
    arr_tmp3.add((int)profile.temperature_step_pcb[i]); 
  }
   
  JsonObject nested4 = json_profile.createNestedObject();
  nested4["name"] = "Temp Top";
  nested4["type"] = "line";
  nested4["color"] = "#F08922";

  JsonObject nested6 = json_profile.createNestedObject();
  nested6["name"] = "Temp Bottom";
  nested6["type"] = "line";
  nested6["color"] = "#C837F0";

 JsonObject nested7 = json_profile.createNestedObject();
  nested7["name"] = "Temp PCB";
  nested7["type"] = "line";
  nested7["color"] = "#54F06C";
/////////////////

 JsonObject nested8 = json_profile.createNestedObject();
  nested8["name"] = "Power Top";
  nested8["type"] = "line";
  nested8["color"] = "#ED1A1A";
  nested8["visible"] = false;

 JsonObject nested9 = json_profile.createNestedObject();
  nested9["name"] = "Power Bottom";
  nested9["type"] = "line";
  nested9["color"] = "#6030F0";
  nested9["visible"] = false;

 JsonObject nested10 = json_profile.createNestedObject();
  nested10["name"] = "Power PCB";
  nested10["type"] = "line";
  nested10["color"] = "#EDE255";
  nested10["visible"] = false;

 ///////

 JsonObject nested11 = json_profile.createNestedObject();
  nested11["name"] = "Diff Top";
  nested11["type"] = "line";
  nested11["color"] = "#F76E1B";
  nested11["visible"] = false;

 JsonObject nested12 = json_profile.createNestedObject();
  nested12["name"] = "Diff Bottom";
  nested12["type"] = "line";
  nested12["color"] = "#B231F7";
  nested12["visible"] = false;

 JsonObject nested13 = json_profile.createNestedObject();
  nested13["name"] = "Diff PCB";
  nested13["type"] = "line";
  nested13["color"] = "#75F759";
  nested13["visible"] = false;
//

  byte profile_steps;          // количество шагов ВИ и НИ
  byte table_size;             // размер стола
  byte kp1;                    // пропорциональный коэффициент ВИ
  byte ki1;                    // интегральный коэффициент     ВИ
  byte kd1;                    // дифференциальный коэффициент ВИ
  byte kp2;                    // пропорциональный коэффициент НИ
  byte ki2;                    // интегральный коэффициент     НИ
  byte kd2;                    // дифференциальный коэффициент НИ
  byte kp3;                    // пропорциональный коэффициент Платы
  byte ki3;                    // интегральный коэффициент     Платы
  byte kd3;                    // дифференциальный коэффициент Платы
  byte max_correction_top;     // максимальная коррекция температуры ВИ в обе стороны
  byte max_correction_bottom;  // максимальная коррекция температуры НИ в обе стороны
  byte max_pcb_delta;          // максимальный недогрев Платы после которого включаеся Пауза
  byte hold_lenght;            // длительность паузы в секундах
  byte participation_rate_top; // коэфициент участия ВИ (от 0 - догреваем только низом, до 100 догреваем только верхом)(при отклонении температыры платы от профиля)
  char alias[20];

  json_profile_settings.clear();
  json_profile_settings["currentProfile"] = (int)currentProfile;
  json_profile_settings["profile_steps"] = (int)profile.profile_steps;
  json_profile_settings["table_size"] = (int)profile.table_size;
  json_profile_settings["kp1"] = (int)profile.kp1;
  json_profile_settings["ki1"] = (int)profile.ki1;
  json_profile_settings["kd1"] = (int)profile.kd1;
  json_profile_settings["kp2"] = (int)profile.kp2;
  json_profile_settings["ki2"] = (int)profile.ki2;
  json_profile_settings["kd2"] = (int)profile.kd2;
  json_profile_settings["kp3"] = (int)profile.kp3;
  json_profile_settings["ki3"] = (int)profile.ki3;
  json_profile_settings["kd3"] = (int)profile.kd3;
  json_profile_settings["max_correction_top"] = (int)profile.max_correction_top;
  json_profile_settings["max_correction_bottom"] = (int)profile.max_correction_bottom;
  json_profile_settings["max_pcb_delta"] = (int)profile.max_pcb_delta;
  json_profile_settings["hold_lenght"] = (int)profile.hold_lenght;
  json_profile_settings["participation_rate_top"] = (int)profile.participation_rate_top;
  json_profile_settings["alias"] = profile.alias;

  //serializeJsonPretty(json_profile, Serial);
  //Serial.println();

  printProfileNumber();
  // aliasprofile();
  GenerateJson(7);

  return;
}

void SaveProfile()
{ //  сохранение текущего профиля в позицию currentProfile
  EEPROM.put((currentProfile - 1) * SizeProfile, profile);
  #if defined(ESP8266) || defined(ESP32)
      EEPROM.commit();
  #endif
}

// Screen data update Functions-----------------------------------------------

#ifdef UseScreen

void LcdDrawLayout() { // Отрисовка сетки экрана с надписями
        myGLCD.clrScr();
        myGLCD.setFont(BigFont);
        myGLCD.setColor(VGA_SILVER);

      //  myGLCD.print("`/c",175, 133);
      //  myGLCD.print("`/c",175, 245);
 

        myGLCD.setColor(VGA_YELLOW);    // "вкладка" 
        myGLCD.drawLine(195,108,210,78); //   для
        myGLCD.drawLine(210,78,475,78);  // мощности
        myGLCD.drawLine(475,78,475,108); //  верха
        myGLCD.drawLine(195,106,475,106);//

        myGLCD.setColor(VGA_AQUA);    // "вкладка" 
        myGLCD.drawLine(195,221,210,191); //   для
        myGLCD.drawLine(210,191,475,191);  // мощности
        myGLCD.drawLine(475,191,475,221); //   низа
        myGLCD.drawLine(197,219,475,219);//
        myGLCD.setColor(30,30,30);
       // myGLCD.print("График Температуры",90, 20);
        myGLCD.setColor(VGA_YELLOW);
        myGLCD.print("Power         %",230, 84);
        myGLCD.setColor(VGA_AQUA);
        myGLCD.print("Power         %",230, 198);

        myGLCD.setColor(VGA_LIME);
        myGLCD.drawRoundRect(3,1,478,71);
        myGLCD.setColor(VGA_RED);
        myGLCD.drawRoundRect(3,108,478,179);
        myGLCD.setColor(VGA_FUCHSIA);
        myGLCD.drawRoundRect(3,221,478,291);
        

       // myGLCD.setColor(40,40,40);      // верхний график 
       // myGLCD.drawRect(1,1,479,72);    //температуры
        //myGLCD.drawRect(224,221,478,291);
        myGLCD.setColor(VGA_LIME);
        myGLCD.drawLine(224,1,224,71); //
        myGLCD.setColor(VGA_RED);
        myGLCD.drawLine(224,108,224,179); //разделитель
        myGLCD.setColor(VGA_FUCHSIA);
        myGLCD.drawLine(224,221,224,291); //
        
        myGLCD.setColor(VGA_LIME);
        myGLCD.print("PCB ->",5, 7);
        myGLCD.print("`",155, 7);

       // myGLCD.print("Rate=",5, 25);
      //  myGLCD.print("`/c",175, 25);
        myGLCD.setColor(VGA_RED);
        myGLCD.print("TOP ->",5, 115);
        myGLCD.print("`",155, 115);    // знак градуса
        myGLCD.setColor(VGA_FUCHSIA);
        myGLCD.print("BOTT->",5, 227);
        myGLCD.print("`",155, 227);

        myGLCD.setColor(VGA_RED);
        myGLCD.print("`",450, 123); // Знак градуса возле температуры ВИ
        myGLCD.setColor(VGA_FUCHSIA);
        myGLCD.print("`",450, 235); // Знак градуса возле температуры НИ  
        myGLCD.setColor(VGA_LIME);
        myGLCD.print("`",450, 15);  // Знак градуса возле температуры Платы 
 
      //  myGLCD.print("Rate=",5, 133);
      //  myGLCD.print("Rate=",5, 245);
        updateScreen = false;
       
}

void LcdDrawState()
  {
    //myGLCD.setColor(VGA_BLACK);
    //myGLCD.fillRect(0, 0, 480, 20);
    myGLCD.setColor(VGA_AQUA);
    myGLCD.setFont(BigFont);

if (reflowState == REFLOW_STATE_RUNNING) {
  myGLCD.print("RUN        ", 5, 300);
} else if (reflowState == REFLOW_STATE_IDLE) {
  myGLCD.print("IDLE       ", 5, 300); // Spaces to clean old counters
} else if (reflowState == REFLOW_STATE_WARMUP || reflowState == REFLOW_STATE_PRE_HEATER) {
  myGLCD.print("WARM  ", 5, 300);
} else if (reflowState == REFLOW_STATE_HOLD_AUTO) {
  myGLCD.print("PAUSE      ", 5, 300);
} else if (reflowState == REFLOW_STATE_HOLD_MANUAL || reflowState == REFLOW_STATE_POWER_MANUAL) {
  myGLCD.setColor(VGA_YELLOW);
  myGLCD.print("MANUAL", 5, 300);
}

  }

void LcdUpdateMeters() {  // Update measurment numbers on the screen (temperature, power rate, time)
  if(updateScreen){  

    // Big Temperatures
        if (Input1 <= -99) {
          myGLCD.setColor(VGA_BLACK);
          myGLCD.drawRoundRect(340,100,460,180);
          myGLCD.setFont(BigFont);
          myGLCD.setColor(VGA_RED);
          myGLCD.print("ERROR",360, 140);
        } else {
          myGLCD.setFont(SevenSegNumFont);
          myGLCD.setColor(VGA_RED);
          myGLCD.printNumI(tc1,345, 120,3,'0');
        }
        if (Input2<= -99) {
          myGLCD.setFont(BigFont);
          myGLCD.setColor(VGA_RED);
          myGLCD.print("ERROR",360, 250);
        } else {
          myGLCD.setFont(SevenSegNumFont);
          myGLCD.setColor(VGA_FUCHSIA);
          myGLCD.printNumI(tc2,345, 232,3,'0');
        }
        if (Input3 <= -99) {
          myGLCD.setFont(BigFont);
          myGLCD.setColor(VGA_RED);
          myGLCD.print("ERROR",360, 32);
        } else {
          myGLCD.setFont(SevenSegNumFont);
          myGLCD.setColor(VGA_LIME);              // Lime потому что лучше всего видно под углом
          myGLCD.printNumI(tc3,345, 12,3,'0');
        }
    // Power rate
        myGLCD.setFont(BigFont);
        myGLCD.setColor(VGA_SILVER);
        
        //myGLCD.printNumI(Output3, 365, 7,3,'0'); // Power PCB
        myGLCD.setColor(VGA_YELLOW);
        myGLCD.printNumI(Output1, 365, 84,3,' ');  // Power TOP
        myGLCD.setColor(VGA_AQUA);
        myGLCD.printNumI(Output2, 365, 198,3,' '); // Power Bottom
      

    // Setpoints in degree, without correction 
          myGLCD.setColor(VGA_LIME);
          myGLCD.printNumI(SetPoint_Pcb,100, 7,3,' ');
          myGLCD.setColor(VGA_RED);
          myGLCD.printNumI(SetPoint_Top,100, 115,3,' ');
          myGLCD.setColor(VGA_FUCHSIA);
          myGLCD.printNumI(SetPoint_Bottom,100, 227,3,' ');
    
    // Heating Rate
        //  myGLCD.printNumF((Input1 - Input_f1) * 1000 / (millis()-prev_millis),2,92,133,',',5);
        //  myGLCD.printNumF((Input2 - Input_f2) * 1000 / (millis()-prev_millis),2,92,245,',',5);

        //myGLCD.printNumF((Input3 - Input3_2sec) * 500 / (millis()-prev_millis),2,92,25,',',5);  // Rate PCB
       
       // prev_millis = millis();
        //Input3_2sec = Input3_1sec;
       // Input3_1sec = Input3;



      updateScreen=false;}
}

void LcdPrintTimer(){  // Print Timer on the screen, without waiting for update
        myGLCD.setFont(BigFont);
        myGLCD.setColor(VGA_YELLOW);
        myGLCD.printNumI(CurrentProfileRealSecond, 85, 300, 3,' ');
}


#endif

#ifdef UseButtons
void ButtonsReadState(){
 //Считываем состояние кнопок управления для резистивной клавиатуры
  upSwitchState = digitalReadA(upSwitchPin);
  downSwitchState = digitalReadA(downSwitchPin);
  rightSwitchState = digitalReadA(rightSwitchPin);
    leftSwitchState = digitalReadA(leftSwitchPin);
   // cancelSwitchState = digitalReadA(cancelSwitchPin);
    okSwitchState = digitalReadA(okSwitchPin);

     if (upSwitchState == HIGH && ( millis() - ms_button)>250)//if UP switch is pressed go Run manual heat
          {
            ms_button =  millis();
            if (reflowState == REFLOW_STATE_IDLE)
            { 
              const char* tempargs[3] = {"1","2","150"}; // Передаем в функцию MANUAL_TEMP три аргумента
              MANUAL_TEMP(3, tempargs);
            }
            else{
              UP(0,0);//вызов функции UP
            }
          }
      if (downSwitchState == HIGH && ( millis() - ms_button)>250)//if DOWN switch is pressed go manual heat
          {
            ms_button =  millis();
            if (reflowState == REFLOW_STATE_IDLE)
            { 
              const char* tempargs[3] = {"1","2","100"}; // Передаем в функцию MANUAL_TEMP три аргумента
              MANUAL_TEMP(3, tempargs);
            }
            DOWN(0,0);//вызов функции DOWN
          }

    if(reflowState == REFLOW_STATE_IDLE) {
    if (rightSwitchState == HIGH && ( millis() - ms_button)>500)//if RIGHT switch is pressed go to next profile
        {
          ms_button =  millis();
          RIGHTc(0,0); //вызов функции RIGHT
          PRINT_PROFILE(0,0); // Sync graph with plotter
        }
    if (leftSwitchState == HIGH && ( millis() - ms_button)>500)//if LEFT switch is pressed go to previous profile
        {
          ms_button =  millis();
          LEFTc(0,0); //вызов функции LEFT
          PRINT_PROFILE(0,0); // Sync graph with plotter
        }
    }

    if (okSwitchState == HIGH && ( millis() - ms_button)>500)//if OK switch is pressed go to previous profile
        {
    if (reflowState == REFLOW_STATE_IDLE && (millis() - ms_button) > 1000) // If pcb hotter than start profile temperature use Hotstart
    {
      ms_button = millis();
      if (tc3 > profile.temperature_step_pcb[0])
      {
        HOTSTART(0, 0);
      }
      else
      {
        START(0, 0);
      }
    }
    else if ((millis() - ms_button) > 600) // Обработка нажатия отмены.
    {
      ms_button = millis();
      CMD_CANCEL(0, 0);
    }
   }
}
#endif

void setup()
{
  blockflag=true; // block temp sending before profile initialization  
  
  ///myGLCD.InitLCD();
  //myGLCD.clrScr();
  //myGLCD.setFont(BigFont);
  //myGLCD.setFontAlt(BFontRu);

  Serial.begin(115200);

#ifdef show_diagnostics_com
  // Serial.begin(9600);
  Serial.println("Setup START"); // DIAGNOSTICS
#endif

#if defined(ESP8266) || defined(ESP32)
  #define EEPROM_SIZE 4096
  EEPROM.begin(EEPROM_SIZE);

    // check file system
  if (!FFat.begin()) {
      #ifdef show_diagnostics_com
        Serial.println("formating file system");
      #endif
      FFat.format();
      FFat.begin();
  }
  

#endif

#if defined(ESP8266) || defined(ESP32)
 #ifdef Enable_WiFi
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      #ifdef show_diagnostics_com
        Serial.print(".");
      #endif
    }
    #ifdef show_diagnostics_com
      Serial.println("");
      Serial.print("Connected to ");
      Serial.println(ssid);
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
    #endif

    IPAddress ip = WiFi.localIP();
    IPlocal = ip.toString();
    //myGLCD.print(&IPlocal[0], 100, 256);
     
    timeClient.begin();
    timeClient.setTimeOffset(10800); //GMT+3
    timeClient.update();
    #ifdef show_diagnostics_com
      Serial.println(timeClient.getFormattedTime()); //Выводим время  
    #endif
     
    // if url isn't found
    server.onNotFound(notFound);

    // run handleUpload function when any file is uploaded
    server.onFileUpload(handleUpload);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
     request->send(200, "text/html", index_html);
    });
    
    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
     request->send(200, "text/css", style_html);
    });

    server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request) {
     request->send(200, "text/javascript", script_html);
    });

    server.on("/profile", HTTP_GET, [](AsyncWebServerRequest *request) {
     request->send(200, "text/html", profile_html);
    });

    server.on("/log_save.csv", HTTP_GET, [](AsyncWebServerRequest *request){
     request->send(FFat, "/"+DataFile);
    });

    server.on("/profile.json", HTTP_GET, [](AsyncWebServerRequest *request){
     String responceBody;
     ArduinoJson::serializeJson(json_profile, responceBody);
     request->send(200, "application/json", responceBody);
    });

    server.on("/profile_settings.json", HTTP_GET, [](AsyncWebServerRequest *request){
     String responceBody;
     ArduinoJson::serializeJson(json_profile_settings, responceBody);
     request->send(200, "application/json", responceBody);
    });

    server.on("/dong.wav", HTTP_GET, [](AsyncWebServerRequest *request){
     //request->send(200, "audio/wav", sound1, 55);
     request->send_P(200, "audio/wav", sound1, 45616);
    });
   
    server.on("/listfiles", HTTP_GET, [](AsyncWebServerRequest * request) {
     String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
     request->send(200, "text/plain", listFiles(true));
    });

    server.on("/file", HTTP_GET, [](AsyncWebServerRequest * request) {
     String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
     //logmessage += " Auth: Success";

      if (request->hasParam("name") && request->hasParam("action")) {
        const char *fileName = request->getParam("name")->value().c_str();
        const char *fileAction = request->getParam("action")->value().c_str();

        logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url() + "?name=" + String(fileName) + "&action=" + String(fileAction);

        if (!FFat.exists(fileName)) {
          #ifdef show_diagnostics_com
            Serial.println(logmessage + " ERROR: file does not exist");
          #endif
          request->send(400, "text/plain", "ERROR: file does not exist");
        } else {
          #ifdef show_diagnostics_com
            Serial.println(logmessage + " file exists");
          #endif
          if (strcmp(fileAction, "download") == 0) {
            logmessage += " downloaded";
            request->send(FFat, fileName, "application/octet-stream");
          } else if (strcmp(fileAction, "delete") == 0) {
            logmessage += " deleted";
            FFat.remove(fileName);
            request->send(200, "text/plain", "Deleted File: " + String(fileName));
          }
          else if (strcmp(fileAction, "write") == 0) {
            request->send(200, "text/plain", "");
          } else {
            logmessage += " ERROR: invalid action param supplied";
            request->send(400, "text/plain", "ERROR: invalid action param supplied");
          }
          #ifdef show_diagnostics_com
            Serial.println(logmessage);
          #endif
        }
      } else {
        request->send(400, "text/plain", "ERROR: name and action params required");
      }
    });

    server.on("/SaveProfile", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
       
     DynamicJsonDocument jsonDoc(2048);

     DeserializationError error = deserializeJson(jsonDoc, data, len);
     
     if (error) {
      #ifdef show_diagnostics_com
        Serial.print("JSON parse error: ");
        Serial.println(error.c_str());
      #endif
      request->send(400, "text/plain", "Failed to parse JSON");
      return;
     }
      
     for(int i=0; i < jsonDoc["time_step_top"].size(); i++ ){
       profile.time_step_top[i] = jsonDoc["time_step_top"][i];
       profile.temperature_step_top[i] = jsonDoc["temperature_step_top"][i];
       profile.time_step_bottom[i] = jsonDoc["time_step_bottom"][i];
       profile.temperature_step_bottom[i] = jsonDoc["temperature_step_bottom"][i];
     }
      for(int i=0; i < jsonDoc["time_step_pcb"].size(); i++ ){
       profile.time_step_pcb[i] = jsonDoc["time_step_pcb"][i];
       profile.temperature_step_pcb[i] = jsonDoc["temperature_step_pcb"][i];
     }

     profile.profile_steps = jsonDoc["setings"]["profile_steps"].as<int>();
     profile.table_size = jsonDoc["setings"]["table_size"].as<int>();
     profile.kp1 = jsonDoc["setings"]["kp1"].as<int>();
     profile.ki1 = jsonDoc["setings"]["ki1"].as<int>();
     profile.kd1 = jsonDoc["setings"]["kd1"].as<int>();
     profile.kp2 = jsonDoc["setings"]["kp2"].as<int>();
     profile.ki2 = jsonDoc["setings"]["ki2"].as<int>();
     profile.kd2 = jsonDoc["setings"]["kd2"].as<int>();
     profile.kp3 = jsonDoc["setings"]["kp3"].as<int>();
     profile.ki3 = jsonDoc["setings"]["ki3"].as<int>();
     profile.kd3 = jsonDoc["setings"]["kd3"].as<int>();
     profile.max_correction_top = jsonDoc["setings"]["max_correction_top"].as<int>();
     profile.max_correction_bottom = jsonDoc["setings"]["max_correction_bottom"].as<int>();
     profile.max_pcb_delta = jsonDoc["setings"]["max_pcb_delta"].as<int>();
     profile.hold_lenght = jsonDoc["setings"]["hold_lenght"].as<int>();
     profile.participation_rate_top = jsonDoc["setings"]["participation_rate_top"].as<int>();

     String alias_tmp = jsonDoc["setings"]["alias"].as<String>();
     alias_tmp.toCharArray(profile.alias, 20);

     SaveProfile();
     loadProfile();
     PRINT_PROFILE(0,0); // выводим профиль в порт
     #ifdef show_diagnostics_com
       Serial.println("Profile Saved");
     #endif
     request->send(200, "text/plain", "Save profile successfully");
    });


    ws.onEvent(onEvent);
    server.addHandler(&ws);
    #ifdef Enable_WiFi_OTA
    AsyncElegantOTA.begin(&server);
    #endif
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    server.begin();

    StartLogFile(true);

    JsonTextError = "";
  #endif


#endif

  // setup reley pins as outputs
  pinMode(P1_PIN, OUTPUT);
  pinMode(P2_PIN, OUTPUT);
  pinMode(P3_PIN, OUTPUT);
  pinMode(P4_PIN, OUTPUT);
  pinMode(thermoCLK1, OUTPUT);
  pinMode(thermoCLK2, OUTPUT);
  pinMode(thermoCLK3, OUTPUT);
  pinMode(thermoDO1, INPUT);
  pinMode(thermoDO2, INPUT);
  pinMode(thermoDO3, INPUT);
  pinMode(thermoCS1, OUTPUT);
  pinMode(thermoCS2, OUTPUT);
  pinMode(thermoCS3, OUTPUT);
  digitalWrite(thermoCS1, HIGH);
  digitalWrite(thermoCS2, HIGH);
  digitalWrite(thermoCS3, HIGH);
  //pinMode(cancelSwitchPin, INPUT_PULLUP); // подключен подтягивающий резистор
  pinMode(RelayPin1, OUTPUT);
  pinMode(RelayPin2, OUTPUT);
  
  #if defined(ESP8266) || defined(ESP32)  
    pinMode(ZCC_PIN, INPUT);
  #endif

  initEeprom();


  NextReadTemperatures = millis();
    

    #if defined(__AVR__)
       attachInterrupt(ZCC_PIN, Dimming, RISING); // настроить порт прерывания 0 = 2й или 1 = 3й цифровой пин
    #elif defined(ESP8266) || defined(ESP32)  
      #ifndef interruptTimer
        attachInterrupt(ZCC_PIN, INT0_ISR, RISING); //
      #else
        Timer0_Cfg = timerBegin(1000000); // Set timer frequency to 1Mhz
        timerAttachInterrupt(Timer0_Cfg, &INT0_ISR);
        timerAlarm(Timer0_Cfg, 100000, true, 0); // 100ms period
        timerStop(Timer0_Cfg);
      #endif
    #endif  
 

  // LIST OF COMMANDS Alias-----cmdAdd("receive_from_com", execute_command_name);---receive_from_com - текст полученный из COM -------execute_command_name - имя функции которую выполнить------------------------
  cmdInit(&Serial);

  cmdAdd("time_step_top", update_time_step_top);
  cmdAdd("temperature_step_top", update_temperature_step_top);
  cmdAdd("time_step_bottom", update_time_step_bottom);
  cmdAdd("temperature_step_bottom", update_temperature_step_bottom);
  cmdAdd("time_step_pcb", update_time_step_pcb);
  cmdAdd("temperature_step_pcb", update_temperature_step_pcb);

  cmdAdd("kp1", update_kp1);
  cmdAdd("ki1", update_ki1);
  cmdAdd("kd1", update_kd1);
  cmdAdd("kp2", update_kp2);
  cmdAdd("ki2", update_ki2);
  cmdAdd("kd2", update_kd2);
  cmdAdd("kp3", update_kp3);
  cmdAdd("ki3", update_ki3);
  cmdAdd("kd3", update_kd3);
  cmdAdd("profile_steps", update_profile_steps);
  cmdAdd("table_size", update_table_size);
  cmdAdd("max_correction_top", update_max_correction_top);
  cmdAdd("max_correction_bottom", update_max_correction_bottom);
  cmdAdd("max_pcb_delta", update_max_pcb_delta);
  cmdAdd("hold_lenght", update_hold_lenght);
  cmdAdd("participation_rate_top", update_participation_rate_top);
  cmdAdd("aliasprofile", update_aliasprofile);

  cmdAdd("U", UP);
  cmdAdd("D", DOWN);
  cmdAdd("L", LEFTc);         // Renamed because of UTFT_Requirements
  cmdAdd("R", RIGHTc);        // Renamed because of UTFT_Requirements
  cmdAdd("N", SAVE);          // Save profile from memory to position
  cmdAdd("E", CMD_CANCEL);        // Stop executing profile
  cmdAdd("H", HOLD);          // Pause executing profile and hold current PCB temperature
  cmdAdd("S", START);         // OK // Start profile execution
  cmdAdd("J", HOTSTART);      // HotStart (J) // Start profile execution
  cmdAdd("M", MANUAL_TEMP);   // Manual Temp adjustment
  cmdAdd("Q", MANUAL_POWER);  // Manual Power adjustment
  cmdAdd("P", PRINT_PROFILE); // Print profile to COM
  cmdAdd("X", BLOCK);         // Edit profile mode - no upstream data from station
  cmdAdd("W", UNBLOCK);       // Exit edit profile mode

  // End LIST OF COMMANDS Alias---------------------------------------------------------------------------------------

  delay(100);
  Serial.println("$#;"); // Clear old plottables
  Serial.println("$0 0 0 0 0 0 0 0 0;");// Hack to draw 9 plottables in plotter to initialize graph correctly

  #ifdef UseScreen
  myGLCD.InitLCD();
  myGLCD.clrScr();
  myGLCD.setFont(BigFont);
  
  LcdDrawLayout();
  LcdDrawState();
  #endif
  #ifdef Enable_WiFi
      GenerateJson(4);
  #endif

  blockflag=false; // unblock temp sending after profile initialization

  loadProfile(); // вызов функции loadProfile для загрузки данных профиля из eeprom
  PRINT_PROFILE(0,0); // выводим профиль в порт
  

  Serial.println("@0;"); // статус IDLE
 
  
  reflowState = REFLOW_STATE_IDLE;

#ifdef show_diagnostics_com
  Serial.println("Setup END"); // DIAGNOSTICS
#endif

/*
 if (!mlx.begin()) {
   #ifdef show_diagnostics_com
    Serial.println("Error connecting to MLX sensor. Check wiring.");
   #endif
  };
  #ifdef show_diagnostics_com
   Serial.print("Emissivity = "); Serial.println(mlx.readEmissivity());
   Serial.print("Ambient = "); Serial.print(mlx.readAmbientTempC());
   Serial.print("*C\tObject = "); Serial.print(mlx.readObjectTempC()); Serial.println("*C");
  #endif
*/

}



const char EEPROM_No_Write = 's'; // <----------------- Изменить значение на любое другое, чтобы обновить профиль в EEPROM при заливке скетча

void initEeprom() // Запись в 0 позицию профиля по умолчанию
{
#ifdef show_diagnostics_com
  Serial.println("EEPROM Init START"); // DIAGNOSTICS
#endif

  if (EEPROM.read(max_profiles * SizeProfile + 1) == EEPROM_No_Write) // записываем любое значение, при включении проверяем его, если совпадает по адресу, то профиль не перезаписываем.
  {
#ifdef show_diagnostics_com
    Serial.println("EEPROM Init Skipped"); // DIAGNOSTICS
#endif                                     // чтото только для уже инициализированой памяти
  }
  else
  {
    // профиль 1 обновлен до профиля ВИ керамика 450вт, ни Алюминиевая плита:
    static int rom1[] = {// ДАННЫЕ для шагов НИЖЕ МОЖНО СФОРМИРОВАТЬ В EXCELL файле ПОДГОТОВКА ПРОФИЛЯ.xls
                         // 30 время от старта профиля до шага ВИ
                         0, 10, 40, 60, 80, 100, 120, 140, 160, 180, 200, 220, 240, 260, 280, 300, 320, 340, 360, 380, 400, 420, 440, 457, 593, 593, 593, 0, 0, 0,
                         // 30 температур по шагам ВИ
                         84, 84, 102, 112, 122, 131, 141, 151, 161, 165, 170, 175, 185, 202, 226, 251, 273, 293, 311, 328, 341, 349, 353, 345, 95, 95, 95, 0, 0, 0,
                         // 30 время от старта профиля до шага НИ
                         0, 4, 34, 54, 74, 94, 114, 134, 154, 174, 194, 214,234,254,274,294,314,334,354,374,394, 414, 434, 454, 474, 494, 593,  0, 0, 0,
                         // 30 температур по шагам НИ
                         220, 220, 270, 280, 290, 285, 280, 275, 270, 265, 255, 245, 234, 246, 266, 273, 274, 275, 274, 273, 271, 269, 267, 259, 219, 166, 67, 0, 0, 0,
                         // 10 время от старта профиля до шага Платы
                         0,4,89,159,212,252,420,442,457,593,
                         // 10 температур по шагам Платы
                         79,79,122,148,161,165,225,231,231,89,

    };
    static byte rom2[] = {
        // кол-во шагов, размер стола
        27,
        3,
        // PID верха,
        6,
        40,
        60,
        // PID низа,
        25,
        0,
        0,
        // PID Платы,
        15,
        0,
        0,
        // максимальная коррекция температуры ВИ, НИ
        35,
        80,
        // максимальное отклонение температуры Платы после которого включаеся Авто Пауза (0-250)
        3,
        // длительность паузы в секундах
        5,
        // коэфициент участия ВИ (0 - догреваем только низом\ 100 - только верхом)
        30,

    };

    static char aliast[20] = "Default Profile";
    #if defined(__AVR__)
      EEPROM.put(0, rom1); // TODO добавить смещение записи
      EEPROM.put(280, rom2);
      EEPROM.put(296, aliast);
    #elif defined(ESP8266) || defined(ESP32)
      EEPROM.put(0, rom1); // TODO добавить смещение записи
      EEPROM.put(560, rom2);
      EEPROM.put(576, aliast);

      
    #endif  

    EEPROM.write(max_profiles * SizeProfile + 1, EEPROM_No_Write); // Запись 
    #if defined(ESP8266) || defined(ESP32)
      EEPROM.commit();
    
    // Перезапись наименований профилей
    static char aliast_tmp[20] = "";
    for (int i = 2; i <= max_profiles; i++) {    
      sprintf(aliast_tmp, "Empty Profile %i", i-1);
      EEPROM.put((i * SizeProfile)-ARRAY_SIZE(profile.alias), aliast_tmp);
      EEPROM.commit();
    }

    #endif
    // for (int i = SizeProfile+1; i < max_profiles * SizeProfile; i++){EEPROM.write(i, 0);}

#ifdef show_diagnostics_com
    Serial.print("EEPROM Profile 1 Updated. Size ");
    Serial.println(SizeProfile); // DIAGNOSTICS
#endif
  }
}

void loop()
{

  #if defined(ESP8266) || defined(ESP32)
  #ifdef Enable_WiFi
    #ifdef Enable_WiFi_OTA
    AsyncElegantOTA.loop();
    #endif
    switch (ws_status)
    {
      case WS_CMD_NULL:
        ws_status = WS_CMD_NULL;
      break;
      case WS_CMD_START:
        ws_status = WS_CMD_NULL;
        START(0,0);
      break;
      case WS_CMD_CANCEL:
        ws_status = WS_CMD_NULL;
        CMD_CANCEL(0,0);
      break;
      case WS_CMD_HOTSTART:
        ws_status = WS_CMD_NULL;
        HOTSTART(0,0);
      break;
      case WS_CMD_LEFTc:
        ws_status = WS_CMD_NULL;
        LEFTc(0,0); //вызов функции LEFT
        PRINT_PROFILE(0,0); // Sync graph with plotter
      break;
      case WS_CMD_RIGHTc:
        ws_status = WS_CMD_NULL;
        RIGHTc(0,0); //вызов функции RIGHT
        PRINT_PROFILE(0,0); // Sync graph with plotter
      break;
    }
  #endif
  #endif  


  cmdPoll(); // Получить команды из ком порта если есть

 

  #ifdef UseButtons
  ButtonsReadState();
  #endif

  
  unsigned long currentMillis = millis();
 
  switch (reflowState)
  {
  case REFLOW_STATE_IDLE:
 
    // блок обработки данных с ПЭВМ--------------------TODO Переписать прием-----------------------------------

    if (millis() > NextReadTemperatures)
    {
      // Read thermocouples next sampling period
      NextReadTemperatures = millis() + SENSOR_SAMPLING_TIME;
      readAllTemperatures();
    }

    if (millis() > nextRead2) // TODO переписать отправку данных на ПК
    {
      nextRead2 = millis() + COM_PORT_SAMPLE_TIME;
      sprintf(buf, "$%03d %03d %03d %03d %03d %03d %03d %03d %03d;", (Output1), (Output2), (int)(Output3), tc1, tc2, tc3, TopDelta, BottomDelta, PcbDelta); // Send temperatures to com
#ifdef UseScreen
      LcdUpdateMeters();
#endif
#ifdef Enable_WiFi
      //JsonUpdateMeters();
      GenerateJson(2);
#endif
     // DrawScreen(); // update screen if needed
    }

    break; // REFLOW_STATE_IDLE

  case REFLOW_STATE_INIT: // выбираем точку на графике, которая соответствует текущей температуре Платы
    previousMillis = millis();

    // сбрасываем переменные
    Output1 = 0;
    Output2 = 0;
    Output3 = 0;
    Serial.println("$#;"); // Команда - очистить график в Порт плоттере
    beep(1);               // Send sound Command to com

#ifdef show_diagnostics_com
    Serial.println("REFLOW_STATE_INIT");
#endif

    integra1 = 0;
    integra2 = 0;
    integra3 = 0;

    p1 = 0;
    p2 = 0;
    p3 = 0;

    d1 = 0;
    d2 = 0;
    d3 = 0;

    DurationCurrentStepTop = 0;
    DurationCurrentStepBottom = 0;
    DurationCurrentStepPcb = 0;

    TemperatureDeltaTop = 0;
    TemperatureDeltaBottom = 0;
    TemperatureDeltaPcb = 0;

    TempSpeedTop = 0;
    TempSpeedBottom = 0;
    TempSpeedPcb = 0;

    TimeCurrentStepTop = 0;
    TimeCurrentStepBottom = 0;
    TimeCurrentStepPcb = 0;

    CurrentStepTop = 0;
    CurrentStepBottom = 0;
    CurrentStepPcb = 0;

    CurrentProfileSecond = 0;
    CurrentProfileRealSecond = 0;
    LastAutoHold = 0;

    #if defined(ESP8266) || defined(ESP32) 
      #ifdef interruptTimer 
          timerStart(Timer0_Cfg);
          timerWrite(Timer0_Cfg, 0);
      #endif
    #endif  

     // фиксируем размер стола
    if (profile.table_size == 1)
    {
      digitalWrite(P1_PIN, HIGH);
    }
    if (profile.table_size == 2)
    {
      digitalWrite(P1_PIN, HIGH);
      digitalWrite(P2_PIN, HIGH);
    }
    if (profile.table_size == 3)
    {
      digitalWrite(P1_PIN, HIGH);
      digitalWrite(P2_PIN, HIGH);
      digitalWrite(P3_PIN, HIGH);
    }

    // выбираем точку на графике, которая соответствует текущей температуре Платы

    if (jump_if_pcb_warm) // Если включен "Горячий старт" рассчитываем уставки для ВИ и НИ основываясь на текущей температуре платы
    {
#ifdef show_diagnostics_com
      Serial.println("STATE_INIT HOT_Start calc\r\n"); // DIAGNOSTICS
#endif
      if (profile.temperature_step_pcb[0] > 0)
      {
        while (tc3 >= profile.temperature_step_pcb[CurrentStepPcb + 1]) // Прибавляем 1 шаг если температура платы сейчас ниже конечной температуры шага
        {
          CurrentStepPcb++;
#ifdef show_diagnostics_com
          Serial.println(CurrentStepPcb);          // DIAGNOSTICS
#endif
          if (CurrentStepPcb >= profile_steps_pcb) // если проверили все шаги, но температура платы сейчас выше максимальной температуры в профиле
          {
            Serial.print("ERROR Pcb Tempereture too high\r\n");
            #ifdef Enable_WiFi 
               JsonTextError = "ERROR Pcb Tempereture too high"; 
            #endif
            reflowState = REFLOW_STATE_COMPLETE;
          }
        }
#ifdef show_diagnostics_com
        Serial.print("Calculated CurrentStepPcb:\r\n");
        Serial.println(CurrentStepPcb); // DIAGNOSTICS
#endif
        if (profile.temperature_step_pcb[CurrentStepPcb] != profile.temperature_step_pcb[CurrentStepPcb + 1])
        {
          DurationCurrentStepPcb = profile.time_step_pcb[CurrentStepPcb + 1] - profile.time_step_pcb[CurrentStepPcb];
          TemperatureDeltaPcb = profile.temperature_step_pcb[CurrentStepPcb + 1] - profile.temperature_step_pcb[CurrentStepPcb];
          TempSpeedPcb = TemperatureDeltaPcb * 100 / DurationCurrentStepPcb;                               // определяем скорость роста температуры на выбранном шаге
          TimeCurrentStepPcb = (tc3 - profile.temperature_step_pcb[CurrentStepPcb]) * 100L / TempSpeedPcb; // Домножаем на 100L, т.к делим на TempSpeedPcb которая уже умножена на 100
#ifdef show_diagnostics_com
          Serial.print("Calculated DurationCurrentStepPcb:\r\n"); // DIAGNOSTICS
          Serial.println(DurationCurrentStepPcb);                 // DIAGNOSTICS
          Serial.print("Calculated TemperatureDeltaPcb:\r\n");    // DIAGNOSTICS
          Serial.println(TemperatureDeltaPcb);                    // DIAGNOSTICS
          Serial.print("Calculated TempSpeedPcb:\r\n");           // DIAGNOSTICS
          Serial.println(TempSpeedPcb);                           // DIAGNOSTICS
          Serial.print("Calculated TimeCurrentStepPcb:\r\n");     // DIAGNOSTICS
          Serial.println(TimeCurrentStepPcb);                     // DIAGNOSTICS
#endif
        }
        // Вычисляем сколько секунд должно было пройти по профилю платы до текущей температуры
        CurrentProfileSecond = profile.time_step_pcb[CurrentStepPcb] + TimeCurrentStepPcb; // секунд от начала профиля до текущего шага платы + секунд от начала текущего шага до текущей температуры
        SetPoint_Pcb = profile.temperature_step_pcb[CurrentStepPcb] + TimeCurrentStepPcb * TempSpeedPcb / 100L;
#ifdef show_diagnostics_com
        Serial.print("Choosen CurrentProfileSecond:\r\n"); // DIAGNOSTICS
        Serial.println(CurrentProfileSecond);              // DIAGNOSTICS
        Serial.print("Choosen SetPoint_Pcb:\r\n");         // DIAGNOSTICS
        Serial.println(SetPoint_Pcb);                      // DIAGNOSTICS
#endif
      }

      if (profile.temperature_step_top[0] > 0) // определяем какую температуру ВИ выбрать (только если включен горячий страт)
      {
        while (CurrentProfileSecond >= profile.time_step_top[CurrentStepTop + 1]) // вычисляем с какого шага ВИ начинать
        {
          CurrentStepTop++;
#ifdef show_diagnostics_com
          Serial.println(CurrentStepTop); // DIAGNOSTICS
#endif
          if (CurrentStepTop >= profile.profile_steps) //
          {
            Serial.print("ERROR Top Steps Overcount\r\n");
            #ifdef Enable_WiFi 
               JsonTextError = "ERROR Top Steps Overcount"; 
            #endif
            reflowState = REFLOW_STATE_COMPLETE;
            break;
          }
        }
        DurationCurrentStepTop = profile.time_step_top[CurrentStepTop + 1] - profile.time_step_top[CurrentStepTop];
        TemperatureDeltaTop = profile.temperature_step_top[CurrentStepTop + 1] - profile.temperature_step_top[CurrentStepTop];
        TempSpeedTop = TemperatureDeltaTop * 100 / DurationCurrentStepTop;
        TimeCurrentStepTop = CurrentProfileSecond - profile.time_step_top[CurrentStepTop];
        SetPoint_Top = profile.temperature_step_top[CurrentStepTop] + TimeCurrentStepTop * TempSpeedTop / 100L; // Находим значение температуры, которое выдать в задание преднагрева. Найти секунду от начала шага -> умножть разницу на гр\сек в текущем шаге
      }
      if (profile.temperature_step_bottom[0] > 0) // определяем какую температуру ВИ выбрать (только если включен горячий страт)
      {
        while (CurrentProfileSecond >= profile.time_step_bottom[CurrentStepBottom + 1]) // вычисляем с какого шшшага ВИ начинать
        {
          CurrentStepBottom++;
#ifdef show_diagnostics_com
          Serial.println(CurrentStepBottom); // DIAGNOSTICS
#endif
          if (CurrentStepBottom >= profile.profile_steps) //
          {
            Serial.print("ERROR Bottom Steps Overcount\r\n");
            #ifdef Enable_WiFi 
               JsonTextError = "ERROR Bottom Steps Overcount"; 
            #endif
            reflowState = REFLOW_STATE_COMPLETE;
            break;
          }
        }
        DurationCurrentStepBottom = profile.time_step_bottom[CurrentStepBottom + 1] - profile.time_step_bottom[CurrentStepBottom];
        TemperatureDeltaBottom = profile.temperature_step_bottom[CurrentStepBottom + 1] - profile.temperature_step_bottom[CurrentStepBottom];
        TempSpeedBottom = TemperatureDeltaBottom * 100 / DurationCurrentStepBottom;
        TimeCurrentStepBottom = CurrentProfileSecond - profile.time_step_bottom[CurrentStepBottom];
        SetPoint_Bottom = profile.temperature_step_bottom[CurrentStepBottom] + TimeCurrentStepBottom * TempSpeedBottom / 100L;
      }
      if (CurrentProfileSecond == 0)
      {
        DurationCurrentStepPcb = profile.time_step_pcb[CurrentStepPcb + 1] - profile.time_step_pcb[CurrentStepPcb];
        DurationCurrentStepTop = profile.time_step_top[CurrentStepTop + 1] - profile.time_step_top[CurrentStepTop];
        DurationCurrentStepBottom = profile.time_step_bottom[CurrentStepBottom + 1] - profile.time_step_bottom[CurrentStepBottom];
      }
    } // конец логики горячего старта

    else // если горячий старт не включен, выставляем задание для ПИД равное температуре первой секунды первого шага ВИ, НИ и Платы.
    {
#ifdef show_diagnostics_com
      Serial.println("STATE_INIT COLD calc\r\n"); // DIAGNOSTICS
#endif
      SetPoint_Pcb = profile.temperature_step_pcb[0];
      SetPoint_Top = profile.temperature_step_top[0];
      SetPoint_Bottom = profile.temperature_step_bottom[0];

#ifdef show_diagnostics_com
      Serial.print("Calculated SetPoint_Pcb:\r\n");    // DIAGNOSTICS
      Serial.println(SetPoint_Pcb);                    // DIAGNOSTICS
      Serial.print("Calculated SetPoint_Top:\r\n");    // DIAGNOSTICS
      Serial.println(SetPoint_Top);                    // DIAGNOSTICS
      Serial.print("Calculated SetPoint_Bottom:\r\n"); // DIAGNOSTICS
      Serial.println(SetPoint_Bottom);                 // DIAGNOSTICS
#endif
    }

    // на выходе - номера шагов профиля ВИ, НИ, Платы (CurrentStepTop|Bottom|Pcb), а также количество секунд от их начала (TimeCurrentStepTop|Bottom|Pcb).
    // если температуры ВИ, НИ меньше 70 градусов -> preheat иначе warmup

#ifdef show_diagnostics_com
    Serial.print("Calculated CurrentStepPcb:\r\n");    // DIAGNOSTICS
    Serial.println(CurrentStepPcb);                    // DIAGNOSTICS
    Serial.print("Calculated CurrentStepTop:\r\n");    // DIAGNOSTICS
    Serial.println(CurrentStepTop);                    // DIAGNOSTICS
    Serial.print("Calculated CurrentStepBottom:\r\n"); // DIAGNOSTICS
    Serial.println(CurrentStepBottom);                 // DIAGNOSTICS
#endif
    if (tc2 < 70) // Если низ меньше 70 градусов - включаем плавный старт N секунд на малой мощности
    {

      reflowState = REFLOW_STATE_PRE_HEATER;
      Serial.println("@1;"); // отправить статус что этап Warmup
#ifdef UseScreen
      LcdDrawState();
#endif
#ifdef Enable_WiFi
      GenerateJson(4);
#endif
      Serial.println("STATE_PRE_HEATER\r\n"); // DIAGNOSTICS
      //  beep(1);
    }
    else
    {
      reflowStatus = REFLOW_STATUS_ON;
      reflowState = REFLOW_STATE_WARMUP;
      Serial.println("@1;");
#ifdef UseScreen
      LcdDrawState();
#endif
#ifdef Enable_WiFi
      GenerateJson(4);
#endif
      Serial.println("STATE_WARMUP");
      //  beep(1);
    }
    break;

  case REFLOW_STATE_PRE_HEATER:
    if (currentMillis - previousMillis > 1000)
    {
#ifdef show_diagnostics_com
      Serial.println("STATE_PRE_HEATER\r\n"); // DIAGNOSTICS
#endif
      previousMillis = currentMillis;
      CurrentProfileRealSecond++;
      Output2 = PreHeatPower;
      Serial.println("@1;"); // Send all time that we are in Warmup mode. To draw point only in the beginning of the Graph

#ifdef UseScreen
      LcdDrawState();
      LcdPrintTimer();
#endif
#ifdef Enable_WiFi
      GenerateJson(6);
#endif

      if (profile.temperature_step_top[CurrentStepTop + 1] > 1) // Если включаем верх в профиле то прогреваем и его
      {
        Output1 = PreHeatPower;
      }
      Output3 = 0;
      if (CurrentProfileRealSecond >= PreHeatTime) // Если прошло N секунд
      {
        Output1 = 0;
        Output2 = 0;
        Serial.println("STATE_WARMUP");
        reflowStatus = REFLOW_STATUS_ON;
        reflowState = REFLOW_STATE_WARMUP;
        // beep(1);
      }
    }
    break;

  // преднагрев ВИ, НИ и платы до значений первого шага / секунды на которую перепрыгиваем. На этом этапе поградусная выдача задания для ПИД (Рампа) ОТКЛЮЧЕНА. ВИ и НИ греюся максимально быстро.
  // Проверяем только факт нагрева Платы и НИ до нужных температур. Целевые температуры определены на шаге REFLOW_STATE_INIT
  case REFLOW_STATE_WARMUP:
    if (currentMillis - previousMillis > 1000)
    {    
#ifdef show_diagnostics_com  
      Serial.println("STATE_WARMUP"); // DIAGNOSTICS
#endif
      Serial.println("@1;"); // Need to send evey second for propper graph plotting
      //  LcdDrawState(); // No need to redraw state on LCD every second
      //Serial.println("!second: 0;");           // DIAGNOSTICS
#ifdef UseScreen
      LcdPrintTimer();
#endif
#ifdef Enable_WiFi
      GenerateJson(5);
#endif

      previousMillis = currentMillis;
      CurrentProfileRealSecond++;
      

      // Ждем когда показания термопар выйдут на нужные значения переход на Running
      if (tc3 >= (SetPoint_Pcb - profile.max_pcb_delta / 4)) // Если термопара Платы показывает значение больше чем половина  (задание - макс отклонение)
      {
        if (tc2 >= (SetPoint_Bottom - profile.max_correction_bottom)) // Если термопара Низа показывает значение больше чем (задание - макс коррекция)
        {
          reflowState = REFLOW_STATE_RUNNING;
          Serial.println("@2;");
#ifdef UseScreen
          LcdDrawState();
#endif
#ifdef Enable_WiFi
      GenerateJson(4);
#endif
          Serial.println("STATE_RUNNING;"); // DIAGNOSTICS
          beep(2);
        }
      }
      if (CurrentProfileRealSecond > max_warmup_duraton) // Если этап преднагрева затянулся больше чем на 4 минуты то отменяем профиль.
      {
        Serial.println("ERROR Warmup over 5 min\r\n"); // DIAGNOSTICS
        #ifdef Enable_WiFi 
               JsonTextError = "ERROR Warmup over 5 min"; 
        #endif
        reflowState = REFLOW_STATE_COMPLETE;
      }
    }
    break;

  case REFLOW_STATE_RUNNING: // На входе номер шага НИ, ВИ, Платы, скорости роста температур, время внутри текущего шага, текущая секунда профиля.
    if (currentMillis - previousMillis > 1000)
    {
      previousMillis = currentMillis;

      // Логика для графика Платы
      if ((profile.time_step_pcb[CurrentStepPcb] > 0) || (profile.temperature_step_pcb[CurrentStepPcb] > 0)) // Если время шага и температура по профилю в начале текущего шага > 0
      {
        if (CurrentProfileSecond >= profile.time_step_pcb[CurrentStepPcb + 1]) // if (CurrentProfileSecond==0 ||  если счетчик времени по профилю 0 или >= времени начала следующего шага
        {                                                                      // переходим на следующий шаг профиля, считаем продолжительность, изменение температуры за шаг, скорость роста температуры
          CurrentStepPcb++;
          DurationCurrentStepPcb = profile.time_step_pcb[CurrentStepPcb + 1] - profile.time_step_pcb[CurrentStepPcb];
          TemperatureDeltaPcb = profile.temperature_step_pcb[CurrentStepPcb + 1] - profile.temperature_step_pcb[CurrentStepPcb];
          TempSpeedPcb = TemperatureDeltaPcb * 100 / DurationCurrentStepPcb;
          TimeCurrentStepPcb = 0; // определяем скорость роста температуры на текущем шаге
          if (TemperatureDeltaPcb < -10)
            beep(10); // Если дельта температур < -10 градусов, то даем сигнал что профиль все.
        }
        
        SetPoint_Pcb = profile.temperature_step_pcb[CurrentStepPcb] + TimeCurrentStepPcb * TempSpeedPcb / 100; // задание для ПИД = начальная температура шага + секунд от начала текущего шага * рост температуры в секунду (ошибка не накопится)
        TimeCurrentStepPcb++;                                                                                  // прибавляем +1 счетчику секунд от начала шага
      }
      else
      {
        SetPoint_Pcb = 0;
      }

      // Логика для графика НИ
      if (profile.time_step_bottom[CurrentStepBottom] > 0 || profile.temperature_step_bottom[CurrentStepBottom] > 0) // Если время шага и температура по профилю в начале текущего шага > 0
      {
        if (CurrentProfileSecond >= profile.time_step_bottom[CurrentStepBottom + 1]) // если счетчик времени по профилю >= времени начала следующего шага
        {                                                                            // переходим на следующий шаг профиля, считаем продолжительность, изменение температуры за шаг, скорость роста температуры
          CurrentStepBottom++;
          if (CurrentStepBottom == profile.profile_steps) // Если достигли последнего шага низа завершаем профиль
          {
            reflowState = REFLOW_STATE_COMPLETE;
            Serial.print("TX OK Last Bottom Step Complete\r\n");
            break;
          }
          DurationCurrentStepBottom = profile.time_step_bottom[CurrentStepBottom + 1] - profile.time_step_bottom[CurrentStepBottom];
          TemperatureDeltaBottom = profile.temperature_step_bottom[CurrentStepBottom + 1] - profile.temperature_step_bottom[CurrentStepBottom];
          TempSpeedBottom = TemperatureDeltaBottom * 100 / DurationCurrentStepBottom;
          TimeCurrentStepBottom = 0; // определяем скорость роста температуры на текущем шаге
        }
        SetPoint_Bottom = profile.temperature_step_bottom[CurrentStepBottom] + TimeCurrentStepBottom * TempSpeedBottom / (uint8_t)100; // задание для ПИД = начальная температура шага + секунд от начала текущего шага * рост температуры в секунду
        TimeCurrentStepBottom++;                                                                                                       // прибавляем +1 счетчику секунд от начала шага
      }
      else
      {
        SetPoint_Bottom = 0;
        Serial.print("SetPoint_Bottom: 0\r\n"); // DIAGNOSTICS
      }

      // Логика для графика ВИ
      if (profile.time_step_top[CurrentStepTop] > 0 || profile.temperature_step_top[CurrentStepTop] > 0) // Если время шага и температура по профилю в начале текущего шага > 1
      {
        if (CurrentProfileSecond >= profile.time_step_top[CurrentStepTop + 1]) // если счетчик времени по профилю >= времени начала следующего шага
        {                                                                      // переходим на следующий шаг профиля, считаем продолжительность, изменение температуры за шаг, скорость роста температуры
          CurrentStepTop++;
          DurationCurrentStepTop = profile.time_step_top[CurrentStepTop + 1] - profile.time_step_top[CurrentStepTop];
          TemperatureDeltaTop = profile.temperature_step_top[CurrentStepTop + 1] - profile.temperature_step_top[CurrentStepTop];
          TempSpeedTop = TemperatureDeltaTop * 100 / DurationCurrentStepTop;
          TimeCurrentStepTop = 0; // определяем скорость роста температуры на текущем шаге
        }
        SetPoint_Top = profile.temperature_step_top[CurrentStepTop] + TimeCurrentStepTop * TempSpeedTop / (uint8_t)100; // задание для ПИД = начальная температура шага + секунд от начала текущего шага * рост температуры в секунду
        TimeCurrentStepTop++;                                                                                           // прибавляем +1 счетчику секунд от начала шага
      }
      else
      {
        SetPoint_Top = 0;
      }

      if (SetPoint_Pcb == 0 && SetPoint_Bottom == 0) // если задание для ПИД платы и для низа по нолям -> закончить профиль (т.к. отдельно верхом мы не паяем)
      {
        reflowState = REFLOW_STATE_COMPLETE;
      }
      

#ifdef UseScreen
      LcdPrintTimer();
#endif
#ifdef Enable_WiFi
      GenerateJson(5);
#endif

      // Логика срабатывания HOLD
      if ((SetPoint_Pcb - tc3 > profile.max_pcb_delta) && ((CurrentProfileRealSecond - LastAutoHold) > profile.hold_lenght*2 )) // если температура платы не успевает за графиком больше чем на max_pcb_delta - переходим в режим HOLD (удержание температуры нагревателей, ждем пока плата догреется)
      {
        if (CurrentStepPcb < (profile_steps_pcb - 3)) // на последних 3 шагах когда идет финальная полка и охлаждение чтобы не вставало на автопаузу
        {
          if (AutoHoldCounter == 0) // DIAGNOSTICS(добавить =) выставляем счетчик по новой только если отсчитали предыдущий цикл полностью. Если за эти N секунд температура не вышла в диспазон допустимого отклонения - Можно включать HOLD по новой
          {
            AutoHoldCounter = profile.hold_lenght;
            LastAutoHold = CurrentProfileRealSecond;
          }
          reflowState = REFLOW_STATE_HOLD_AUTO;
          
          Serial.println("@3;");
#ifdef UseScreen
          LcdDrawState();
#endif
#ifdef Enable_WiFi
      GenerateJson(4);
#endif
        }
        if (SetPoint_Pcb - tc3 < -profile.max_pcb_delta) // если температура платы перелетает уставку больше чем на max_pcb_delta - вопим что слишком большая мощность, но продолжаем паять.
        {
          Serial.println("WARNING Profile to powerfull");
          beep(7);
        }
      }

      CurrentProfileSecond++;
      CurrentProfileRealSecond++;
 
      itoa(CurrentProfileSecond,itoaTemp,10);
      strcat(buf2, "!second: ");           // DIAGNOSTICS // !IMPORTANT for graph plotting
      strcat(buf2, itoaTemp);   // DIAGNOSTICS
      strcat(buf2, ";    ");
      strcat(buf2, " ");

      itoa(CurrentProfileRealSecond,itoaTemp,10);
      strcat(buf2, " RealSec: ");               // DIAGNOSTICS
      strcat(buf2, itoaTemp); // DIAGNOSTICS
      strcat(buf2, "\r\n");

#ifdef UseScreen
      
#endif      

    }
    break;

  case REFLOW_STATE_HOLD_AUTO: // попадаем в HOLD, задания для НИ, ВИ, и платы по графику остаются как в последнюю секунду. Только ПИД платы вносит небольшие коррективы в температуры ВИ и НИ .
    if (currentMillis - previousMillis > 1000)
    {
 #ifdef show_diagnostics_com  
      Serial.println("HOLD_AUTO\r\n"); // DIAGNOSTICS
 #endif
      // beep(1);  // Send sound Command to com every Second
      previousMillis = currentMillis;
      CurrentProfileRealSecond = CurrentProfileRealSecond + 1; // продолжаем считать полное время выполнения
      if (AutoHoldCounter > 0)
      {
       itoa(CurrentProfileSecond,itoaTemp,10); // Complex output via buffer to prevent com port overflow
      strcat(buf2, "!second: ");           // DIAGNOSTICS // !IMPORTANT for graph plotting
      strcat(buf2, itoaTemp);   // DIAGNOSTICS
      strcat(buf2, ";    ");

      itoa(CurrentProfileRealSecond,itoaTemp,10);
      strcat(buf2, "RealSec: ");               // DIAGNOSTICS
      strcat(buf2, itoaTemp); // DIAGNOSTICS
      strcat(buf2, "    ");

        
        strcat(buf2, "AutoHoldCount: "); // DIAGNOSTICS
        itoa(AutoHoldCounter,itoaTemp,10);
        strcat(buf2, itoaTemp);     // DIAGNOSTICS
        strcat(buf2, "\r\n");

#ifdef UseScreen                              // Draw countdown on screen
      //  myGLCD.setColor(255, 255, 255);
      //  myGLCD.fillRect(5, 300, 100, 320);
        myGLCD.setFont(BigFont);
        myGLCD.setColor(VGA_SILVER);
        myGLCD.printNumI(AutoHoldCounter, 85, 300,4,' ');
#endif
      AutoHoldCounter--;


      }
      else
      {
        reflowState = REFLOW_STATE_RUNNING;
        Serial.println("@2;");
#ifdef UseScreen
        LcdDrawState();
#endif
#ifdef Enable_WiFi
      GenerateJson(4);
#endif
        // beep(2); // Send sound Command to com when stop hold
      }
    }
    break;

  case REFLOW_STATE_HOLD_MANUAL:
    if (currentMillis - previousMillis > 1000) // 1 раз в секунду проверяем не нажали ли отмену \ отключение HOLD
    {
      if (prev_reflowState != REFLOW_STATE_IDLE && manual_temp_changed || manual_temperature_pcb)
      { // управление уставкой платы если перешли в паузу из профиля
        SetPoint_Pcb = manual_temperature;
      //  Serial.print("HOLD_MANUAL ENABLE Setpoint PCB Update ");
      //  Serial.println(SetPoint_Pcb);
      }
      else
      { // управление уставкой НИ если перешли в паузу из IDLE
        if (manual_temp_changed)
        {
          SetPoint_Bottom = manual_temperature;
       //   Serial.print("HOLD_MANUAL ENABLE Setpoint BOTTOM Update ");
       //   Serial.println(SetPoint_Bottom);
        }
      }
      Output1 = 0;
      manual_temp_changed = false;
      //manual_temperature_pcb = false;
      previousMillis = currentMillis;
      CurrentProfileRealSecond = CurrentProfileRealSecond + 1;
      if (CurrentProfileRealSecond % 60 == 0)
      {
        beep(1); // Если реальное время профиля кратно 60 скундам - сделать Бип
      }
      Serial.print("HOLD_MANUAL ENABLE  "); // DIAGNOSTICS
    }
    break;

  case REFLOW_STATE_POWER_MANUAL:              ////// Can be started only from IDLE.
    if (currentMillis - previousMillis > 1000) // 1 раз в секунду проверяем не нажали ли отмену
    {

      // manual_power_changed = false;
      previousMillis = currentMillis;
      CurrentProfileRealSecond = CurrentProfileRealSecond + 1;
      if (CurrentProfileRealSecond % 60 == 0)
      {
        beep(1); // Если реальное время профиля кратно 60 скундам - сделать Бип
        Serial.print("MANUAL_POWER: Top ");
        Serial.print(Output1);
        Serial.print("   Bottom: ");
        Serial.println(Output2);
      }
      
      // Serial.print("MANUAL_POWER ENABLE  "); //DIAGNOSTICS
    }
    break;

  // завершение пайки
  case REFLOW_STATE_COMPLETE:

    #if defined(ESP8266) || defined(ESP32)  
      #ifdef interruptTimer
          timerStop(Timer0_Cfg);
      #endif
    #endif  

    digitalWrite(P1_PIN, LOW);
    digitalWrite(P2_PIN, LOW);
    digitalWrite(P3_PIN, LOW);
    digitalWrite(P4_PIN, LOW);

   

    //  Serial.println("TXStop");
    Serial.println("REFLOW_STATE_COMPLETE");
    Serial.println("@5;");
    reflowStatus = REFLOW_STATUS_OFF;

    Output1 = 0;
    Output2 = 0;
    Output3 = 0;
    SetPoint_Top = 0;
    temp_correction_top = 0;
    SetPoint_Bottom = 0;
    temp_correction_bottom = 0;
    SetPoint_Pcb = 0;
    TopDelta = 0;
    BottomDelta = 0;
    PcbDelta = 0;
    manual_temperature = 0;
    manual_power = 0;

    beep(3);

    reflowState = REFLOW_STATE_IDLE;
    


    Serial.println("@0;");
#ifdef UseScreen
    LcdDrawState();
#endif
#ifdef Enable_WiFi
      GenerateJson(4);
#endif
    break;
  } // End of Switch

  // включение нагревателей
  if (reflowStatus == REFLOW_STATUS_ON)
  {

    if (millis() > NextReadTemperatures)
    {
      NextReadTemperatures = millis() + SENSOR_SAMPLING_TIME;
      readAllTemperatures();
  

      if (reflowState != REFLOW_STATE_PRE_HEATER && reflowState != REFLOW_STATE_POWER_MANUAL) // не вычисляем PID в режиме преднагрева и ручного управления мощностью
      {
        //   Output3=(SetPoint_Pcb == 0) ? 0 : Pid3(Input3,SetPoint_Pcb,profile.kp3,profile.ki3,profile.kd3);         // если задание для ПИД платы 0 - не считаем его (не корректируем температуру)
        //   Output3=(SetPoint_Pcb == 0) ? 0 :Output3*0.92+0.08*Pid3(Input3,SetPoint_Pcb,profile.kp3,profile.ki3,profile.kd3);         // Сглаживание на 10 секунд 0.975 * 0.025//TODO Определится какой ПИД используем и какие настройки фильтрации значений выбрать.
        //Serial.println("PID Calculation");
        
        // Output3 = (SetPoint_Pcb == 0) ? 0 : PidTEST4(Input3, SetPoint_Pcb, profile.kp3, profile.ki3, profile.kd3); //
        Output3 = (SetPoint_Pcb == 0) ? 0 : Pid3(Input3, SetPoint_Pcb, profile.kp3, profile.ki3, profile.kd3); //

        if (reflowState == REFLOW_STATE_WARMUP) {Output3 = 0;} // Won't correct temperature during Warmup period

        temp_correction_top = Output3 * profile.participation_rate_top / (uint8_t)100;                                               // вычисляем на сколько градусов скорректировать значние по графику
        temp_correction_top = (temp_correction_top < profile.max_correction_top) ? temp_correction_top : profile.max_correction_top; // проверяем не превышает ли макс значение коррекции

        temp_correction_bottom = Output3 * ParticipationRateBottom / (uint8_t)100;                                                                  // ParticipationRateBottom без profile.
        temp_correction_bottom = (temp_correction_bottom < profile.max_correction_bottom) ? temp_correction_bottom : profile.max_correction_bottom; // проверяем не превышает ли макс значение коррекции

        Output2 = PidTEST2(Input2, temp_correction_bottom + SetPoint_Bottom, profile.kp2, profile.ki2, profile.kd2);
        Output1 = (profile.temperature_step_top[CurrentStepTop] == 0) ? 0 : Pid1(Input1, temp_correction_top + SetPoint_Top, profile.kp1, profile.ki1, profile.kd1); // если для ВИ есть задание по графику, считаем ПИД верха

        // Вывод информации об отклонении температур от уставки, смещены на 50точек вниз, для удобства просмотра на 1 графике
        TopDelta = (tc1 - SetPoint_Top - temp_correction_top) - 50;
        BottomDelta = (tc2 - SetPoint_Bottom - temp_correction_bottom) - 50;

        PcbDelta = (tc3 - SetPoint_Pcb) - 50;
      }

      AverageOutput1 = AverageOutput1 + Output1;
      AverageOutput2 = AverageOutput2 + Output2;
      AverageOutput3 = AverageOutput3 + Output3;
      
      sprintf(buf, "$%03d %03d %03d %03d %03d %03d %03d %03d %03d;", (int)(AverageOutput1/4), (int)(AverageOutput2/4), (int)(AverageOutput3/4), tc1, tc2, tc3, TopDelta, BottomDelta, PcbDelta); // Send Tempereture, Power and Delta to PC

      #ifdef Enable_WiFi
        GenerateJson(2);
      #endif
      #ifdef UseScreen
        LcdUpdateMeters();
      #endif
      ///#ifdef Enable_WiFi
      //  JsonUpdateMeters();
      //#endif


      //  sprintf (buf, "OK%03d%03d%03d%03d%03d%03d%03d\r\n", (Output1), (Output2), (Output3), tc1, tc2, tc3, (CurrentProfileRealSecond)); // TODO исправить формат отправки для графика ПК
      // sprintf (buf, "$%03d %03d %03d %03d %03d %03d %03d %03d %03d;", (Output1), (Output2), (int)(Output3), tc1, tc2, tc3, (TopDelta), (BottomDelta), (PcbDelta)); // TODO исправить формат отправки для графика ПК
       // sprintf (buf, "$%03d %03d %03d %03d %03d %03d %03d %03d %03d;", (Output1), (Output2), (int)(Output3), tc1, tc2, tc3, (int)(p2), (int)(integra2), (int)(d2));  //DIAGNOSTICS


    }
  if (tc3 >= max_temperature_pcb) // если температура платы достигла предела (250гр например) прервать выполнение профиля
  {
    reflowStatus = REFLOW_STATUS_OFF;
    reflowState = REFLOW_STATE_COMPLETE;
    Serial.println("ERROR PCB Overheat");
    #ifdef Enable_WiFi 
               JsonTextError = "ERROR PCB Overheat"; 
    #endif
    beep(3);
  }
  }
}

void readAllTemperatures()
{    

 #ifdef test_mode  
      Input1 = Input1 * 0.95 + 0.05 * (SetPoint_Top); //симуляция замера температуры с задержкой на 10 измерений
      Input2 = Input2 * 0.95 + 0.05 * (SetPoint_Bottom); //симуляция замера температуры с задержкой на 10 измерений
      Input3 = Input3 * 0.95 + 0.05 * (SetPoint_Pcb); //симуляция замера температуры с задержкой на 10 измерений
 
 #else
      Input1 = Input1 * 0.6 + 0.4 * (max6675_read_temp(thermoCLK1, thermoCS1, thermoDO1)); // сглаживание температуры на 12 измерений
      Input2 = Input2 * 0.6 + 0.4 * (max6675_read_temp(thermoCLK2, thermoCS2, thermoDO2)); // сглаживание температуры на 12 измерений
      Input3 = Input3 * 0.4 + 0.6 * (max6675_read_temp(thermoCLK3, thermoCS3, thermoDO3)); // сглаживание температуры на 8 измерений
      //Input4 = Input3 * 0.4 + 0.6 * (mlx.readObjectTempC()); // сглаживание температуры на 8 измерений
#endif
      tc1 = Input1;
      tc2 = Input2;
      tc3 = Input3;
      
      if (Input1 <= -99) { 
        Serial.println("ERROR Thermocouple Upper Heater"); 
        #ifdef Enable_WiFi 
          JsonTextError = "ERROR Thermocouple Upper Heater"; 
        #endif  
        }
      if (Input2 <= -99) { 
        Serial.println("ERROR Thermocouple Bottom Heater"); 
        #ifdef Enable_WiFi
          JsonTextError = "ERROR Thermocouple Bottom Heater"; 
        #endif 
        }
      if (Input3 <= -99) { 
        Serial.println("ERROR Thermocouple PCB"); 
        #ifdef Enable_WiFi 
           JsonTextError = "ERROR Thermocouple PCB"; 
        #endif
        }

      }


void Dimming() // Вызывается по прерыванию от детектора ноля 100 раз в секунду
{
  OutPWR_TOP();
  OutPWR_BOTTOM();

  if (Secs >= 100) // каждые 100 прерываний отправляем данные через serial
  {
    if (!blockflag & !blockflagM)
    {
      //Serial.println(buf);
    }
    AverageOutput1 = AverageOutput2 = AverageOutput3 = 0;
    Secs = 1; // Начинаем считать с 1, т.к. в первый шаг мы только присваиваем значение, но не прибавляем.
  }
  else
  {
    if (Secs < 95 &&  Secs > 3 && buf2[0] != '\0') // if buf2 is not empty - send it to Serial
    {
      //Serial.print(buf2);
      buf2[0] = '\0';
    } 
    Secs++;
  }
#ifdef UseScreen
  if (Secs == 50 ) // на середине секунды обновляем данные на экране
    {updateScreen=true;}
#endif 

}

void OutPWR_TOP()
{
  reg1 = Output1 + er1; // pwr- задание выходной мощности в %,в текущем шаге профиля, er- ошибка округления
  if (reg1 < 50)
  {
    out1 = LOW;
    er1 = reg1; // reg- переменная для расчетов
  }
  else
  {
    out1 = HIGH;
    er1 = reg1 - 100;
  }
  digitalWrite(RelayPin1, out1); // пин через который осуществляется дискретное управление
}

void OutPWR_BOTTOM()
{
  reg2 = Output2 + er2; // pwr- задание выходной мощности в %, er- ошибка округления
  if (reg2 < 50)
  {
    out2 = LOW;
    er2 = reg2; // reg- переменная для расчетов
  }
  else
  {
    out2 = HIGH;
    er2 = reg2 - 100;
  }
  digitalWrite(RelayPin2, out2); // пин через который осуществляется дискретное управление
}

byte Pid1(double temp, int ust, byte kP, byte kI, byte kD)
{
  static byte out = 0;
  static float ed = 0;
  e1 = (ust - temp);   // ошибка регулирования
  p1 = (kP * e1); // П составляющая
  integra1 = ((out > 99) && (e1 * integra1 >= 0)) ? integra1 : (integra1 < i_min) ? i_min
                                                           : (integra1 > i_max)   ? i_max
                                                                                  : integra1 + (kI * e1) / 1000.0; // И составляющая
  d1 = kD * (temp - ed);                                                                                          // Д составляющая
  ed = temp;
  out = (p1 + integra1 - d1 >= 100) ? 100 : (p1 + integra1 - d1 <= 0) ? 0
                                                                      : p1 + integra1 - d1; //  Выход ПИД от 0 до 100.
  return out;
}

byte Pid2(double temp, int ust, byte kP, byte kI, byte kd)
{
  byte out = 0;
  static float ed = 0;
  e2 = (ust - temp); // ошибка регулирования
  p2 = (kP * e2);    // П составляющая
  integra2 = (integra2 < i_min) ? i_min : (integra2 > i_max) ? i_max
                                                             : integra2 + (kI * e2) / 1000.0; // И составляющая
  d2 = kd * (temp - ed);                                                                      // Д составляющая
  ed = temp;
  out = (p2 + integra2 - d2 >= 100) ? 100 : (p2 + integra2 - d2 <= 0) ? 0
                                                                      : p2 + integra2 - d2; //  Выход ПИД от 0 до 100.
  return out;
}

int8_t Pid3(double temp, int ust, byte kP, byte kI, byte kd) // Темп фактически, темп должно быть, коэфициенты пид, результат - значение коррекции для ПИД ВИ и НИ в градусах (от -100 до 100)
{
  int8_t out = 0;
  static float ed = 0;
  e3 = e3 * 0.75 + 0.25 * (ust - temp); // ошибка регулирования Сглаживание на 11 шагов
  p3 = (kP * e3);    // П составляющая
  integra3 = (integra3 < i_min) ? i_min : (integra3 > i_max) ? i_max
                                                             : integra3 + (kI * e3) / 1000.0; // И составляющая
  d3 = kd * (temp - ed);                                                                      // Д составляющая
  ed = temp;
  out = (p3 + integra3 - d3 > 100) ? 100 : (p3 + integra3 - d3 < -100) ? -100
                                                                       : p3 + integra3 - d3; // Ограничиваем выхо��ные значения
  return out;
}

int8_t PidTEST(double temp, int ust, byte kP, byte kI, byte kd) // Сглаживание дифференциальной составляющей на 4 шага
{
  int8_t out = 0;
  static float ed = 0;
  e3 = e3 * 0.75 + 0.25 * (ust - temp); // ошибка регулирования
  p3 = (kP * e3);                       // П составляющая
  integra3 = (integra3 < i_min) ? i_min : (integra3 > i_max) ? i_max
                                                             : integra3 + (kI * e3) / 1000.0; // И составляющая
  // d3 = d3 * 0.75 + 0.25 * kd * (e3 - ed); //Д составляющая
  d3 = kd * (e3 - ed);
  ed = e3;
  out = (p3 + integra3 + d3 > 100) ? 100 : (p3 + integra3 + d3 < -100) ? -100
                                                                       : p3 + integra3 + d3; // Ограничиваем выходные значения
#ifdef show_diagnostics_com_deep
  Serial.println(p3);       // DIAGNOSTICS
  Serial.println(integra3); // DIAGNOSTICS
  Serial.println(d3);       // DIAGNOSTICS
#endif
  return out;
}

byte PidTEST2(double temp, int ust, byte kP, byte kI, byte kD) // Сглаживание дифференциальной составляющей на 4 шага
{
  static byte out;
  static int prev_ust;
  static byte lastOut; // предыдущее значение мощности
  static float integra_coef = 10000.0; // Делитель интегральной составляющей
  //int ust_delta = prev_ust - ust;
  static byte buff_position;
  static const byte buff_depth = 4; //  // глубина буфера 4 = 1 секунда, 8 = 2 секунды... MAX 250, увеличение кушает память.
  static float ed[buff_depth];      // массив предыдущих измерений ошибки (виден только внутри функции пид)
  e2 = (ust - temp);                // ошибка регулирования Сглаживание на 4 шага
  p2 = (kP * e2) ;              // П составляющая
  integra2 = ((out > 99) && (e2 * integra2 >= 0)) ? integra2 : (integra2 < i_min) ? i_min
                                                           : (integra2 > i_max)   ? i_max
                                                                                  : integra2 + (kI * e2) / integra_coef; // И составляющая
  //integra2 = (ust_delta == 0) ? integra2 : integra2 - (kI * ust_delta) / integra_coef;
  d2 = kD * (temp - ed[buff_position]);
  ed[buff_position] = temp;
  buff_position = (buff_position < buff_depth - 1) ? buff_position + 1 : 0;
  out = (p2 + integra2 - d2 >= 100) ? 100 : (p2 + integra2 - d2 <= 0) ? 0
                                                                      : p2 + integra2 - d2; // Ограничиваем выходные значения
  prev_ust = ust;
  lastOut = out;
  return out;
}

/*!
int8_t PidTEST4(double temp, int ust, byte kP, byte kI, byte kD) // Сглаживание дифференциальной составляющей на 4 шага
{
  int8_t out = 0;
  static byte lastOut; // предыдущее значение мощности
  byte kF = kD;        // Значение множителя К для Сопротивления изменению температуры (f = friction)
  float f3 = 0;
  static const byte buff_depth = 4; //  // глубина буфера 4 = 1 секунда, 8 = 2 секунды... MAX 250, увеличение кушает память.
  static float ed[buff_depth];      // массив предыдущих измерений ошибки (виден только внутри функции пид)
  static float ef[buff_depth];      // массив предыдущих измерений температуры (виден только внутри функции пид)
  static byte buff_position;
  e3 = e3 * 0.75 + 0.25 * (ust - temp); // ошибка регулирования Сглаживание на 11 шагов
  p3 = (kP * e3);                       // П составляющая
  // integra3 = (integra3 < -100) ? -100 : (integra3 > 100) ? 100 : integra3 + (kI * e3)/1000.0; //И составляющая
  integra3 = ((lastOut > max_correction_total - 1) && (e3 * integra3 >= 0)) ? integra3 : (integra3 < -max_correction_total) ? -max_correction_total
                                                                                     : (integra3 > max_correction_total)    ? max_correction_total
                                                                                                                            : integra3 + (kI * e3) / 1000.0;
  d3 = kD * (e3 - ed[buff_position]);                                       // Д составляющая // сравниваем со значением ошибки N шагов назад, если ошибка уменьшилась уменьшаем ПИД
  f3 = kF * (temp - ef[buff_position]);                                     // скорость изменения температуры, если быстро растет, гасим пид, если быстро падает - подпинываем
  ed[buff_position] = e3;                                                   // Запись ошибки buff_depth шагов назад
  ef[buff_position] = temp;                                                 // Запись температуры buff_depth шагов назад
  buff_position = (buff_position < buff_depth - 1) ? buff_position + 1 : 0; // если достигли последнего значения буфера начинаем писать с 0
#ifdef show_diagnostics_com_deep
  Serial.println(p3);       // DIAGNOSTICS
  Serial.println(integra3); // DIAGNOSTICS
  Serial.println(d3);       // DIAGNOSTICS
  Serial.println(f3);
#endif
  out = (p3 + integra3 + d3 - f3 > max_correction_total) ? max_correction_total : (p3 + integra3 + d3 - f3 < -max_correction_total) ? -max_correction_total
                                                                                                                                    : p3 + integra3 + d3 - f3; // Ограничиваем выходные значения -100 __ 100
  lastOut = out;
  return out;
}

*/

#if defined(__AVR__)

double max6675_read_temp(int ck, int cs, int so)
{
  char i;
  int tmp = 0;
  digitalWrite(cs, LOW); // cs = 0;                            // Stop a conversion in progress
  asm volatile(" nop"
               "\n\t");
  for (i = 15; i >= 0; i--)
  {
    digitalWrite(ck, HIGH);
    asm volatile(" nop"
                 "\n\t");
    if (digitalRead(so))
      tmp |= (1 << i);
    digitalWrite(ck, LOW);
    asm volatile(" nop"
                 "\n\t");
  }
  digitalWrite(cs, HIGH);
  if (tmp & 0x4)
  {
    return -100;
  }
  else
    return ((tmp >> 3)) * 0.25;
}

#elif defined(ESP8266) || defined(ESP32)
  
double max6675_read_temp(int CLK, int iCS, int DO) {
  int d = 0;
  digitalWrite(iCS, LOW);  //delay(100); 
  asm volatile (" nop"  "\n\t" );
  for (int8_t i=15; i >= 0; i--) {
    digitalWrite(CLK, HIGH);    //delay(1);
    asm volatile ( " nop"  "\n\t" );//    d = (d << 1) | digitalRead(iDO);  
    if ( digitalRead(DO))   d |= (1 << i);
    digitalWrite(CLK, LOW);
    asm volatile ( " nop"  "\n\t" );    //delay(1);
    }
  digitalWrite(iCS, HIGH);
  if (d & 0x4) return(-100);     //return -100;
    else  return ((d >> 3) * 0.25);
}

#endif

/**************************************************************************/
/*!
    Generate the main command prompt
*/
/**************************************************************************/
//void cmd_display()
//{
  //char buf[180];
//}

/**************************************************************************/
/*!
    Parse the command line. This function tokenizes the command input, then
    searches for the command table entry associated with the commmand. Once found,
    it will jump to the corresponding function.
*/
/**************************************************************************/
void cmd_parse(char *cmd)
{
  static uint8_t argc;

  uint8_t i = 0;

#define max_arguments 40
  char *argv[max_arguments]; // Max number of variables in recieved command line
  cmd_t *cmd_entry;

  // parse the command line statement and break it up into delimited
  // strings. the array of strings will be saved in the argv array.
  argv[i] = strtok(cmd, " "); 
  do
  {
    // argv[++i] = strtok(NULL, " ");
    argv[++i] = strtok(NULL, ",");
  } while ((i < max_arguments) && (argv[i] != NULL));
  if (!argv[0])
    return; // if we found that 0 element = 0 than we have no command. no need to react.

  // save off the number of arguments for the particular command.
  argc = i;

  // parse the command table for valid command. used argv[0] which is the
  // actual command name typed in at the prompt

  for (cmd_entry = cmd_tbl; cmd_entry != NULL; cmd_entry = cmd_entry->next)
  {

    if (!strcmp(argv[0], cmd_entry->cmd))
    {
      cmd_entry->func(argc, argv);
    //  cmd_display(); // Echoe commands
      return;
    }
    if (cmd_entry == NULL)
    {
    //  cmd_display(); // Echoe commands
      return;
    }
  }

  // command not recognized. print message and re-generate prompt.

  stream->print("CMD Unknown: ");
  stream->println(argv[0]);
}

/**************************************************************************/
/*!
    This function processes the individual characters typed into the command
    prompt. It saves them off into the message buffer unless its a "backspace"
    or "enter" key.
*/
/**************************************************************************/
void cmd_handler()
{
  unsigned char c = stream->read();

  switch (c)
  {
  case ';':
    *msg_ptr = '\0';
    cmd_parse((char *)msg);
    msg_ptr = msg;

    break;
  case '\r':

    // ignore return characters. they usually come in pairs
    // with the \r characters we use for newline detection.

    break;

  case '\n':
    // terminate the msg and reset the msg ptr. then send
    // it to the handler for processing.
    *msg_ptr = '\0';
    cmd_parse((char *)msg);
    msg_ptr = msg;

    break;

  default:
    // normal character entered. add it to the buffer

    *msg_ptr++ = c;
    break;
  }
}

/**************************************************************************/
/*!
    This function should be set inside the main loop. It needs to be called
    constantly to check if there is any available input at the command prompt.
*/
/**************************************************************************/
void cmdPoll()
{
  while (stream->available())
  {
    cmd_handler();
  }
}

/**************************************************************************/
/*!
    Initialize the command line interface. This sets the terminal speed and
    and initializes things.
*/
/**************************************************************************/
void cmdInit(Stream *str)
{
  stream = str;
  // init the msg ptr
  msg_ptr = msg;

  // init the command table
  cmd_tbl_list = NULL;
}

/**************************************************************************/
/*!
    Add a command to the command table. The commands should be added in
    at the setup() portion of the sketch.
*/
/**************************************************************************/
void cmdAdd(const char *name, void (*func)(int argc, char **argv))
{
  // alloc memory for command struct
  cmd_tbl = (cmd_t *)malloc(sizeof(cmd_t));

  // alloc memory for command name
  char *cmd_name = (char *)malloc(strlen(name) + 1);

  // copy command name
  strcpy(cmd_name, name);

  // terminate the command name
  cmd_name[strlen(name)] = '\0';

  // fill out structure
  cmd_tbl->cmd = cmd_name;
  cmd_tbl->func = func;
  cmd_tbl->next = cmd_tbl_list;
  cmd_tbl_list = cmd_tbl;
}

/**************************************************************************/
/*!
    Get a pointer to the stream used by the interpreter. This allows
    commands to use the same communication channel as the interpreter
    without tracking it in the main program.
*/
/**************************************************************************/
Stream *cmdGetStream(void)
{
  return stream;
}

/**************************************************************************/
/*!
    Convert a string to a number. The base must be specified, ie: "32" is a
    different value in base 10 (decimal) and base 16 (hexadecimal).
*/
/**************************************************************************/
uint32_t cmdStr2Num(char *str, uint8_t base) {
  return strtol(str, NULL, base);
}

// Array Variables Update Area -------------------------------------------

void update_steps(int arg_cnt, char **args, int *steps_array, byte *steps_count, const char *command)
{
  Stream *s = cmdGetStream();
  for (int i = 1; i < arg_cnt; i++)
  {
    steps_array[i - 1] = atoi(args[i]);
  }

  s->print(command);
  s->print(": ");
  for (int i = 0; i < (int)steps_count; i++)
  {
    s->print(steps_array[i]);
    s->print(",");
  }
  s->println(";");
}

void update_time_step_top(int arg_cnt, char **args)
{
  update_steps(arg_cnt, args, profile.time_step_top, (byte *)ARRAY_SIZE(profile.time_step_top), "!time_step_top");
}

void update_temperature_step_top(int arg_cnt, char **args)
{
  update_steps(arg_cnt, args, profile.temperature_step_top, (byte *)ARRAY_SIZE(profile.temperature_step_top), "!temperature_step_top");
}

void update_time_step_bottom(int arg_cnt, char **args)
{
  update_steps(arg_cnt, args, profile.time_step_bottom, (byte *)ARRAY_SIZE(profile.time_step_bottom), "!time_step_bottom");
}

void update_temperature_step_bottom(int arg_cnt, char **args)
{
  update_steps(arg_cnt, args, profile.temperature_step_bottom, (byte *)ARRAY_SIZE(profile.temperature_step_bottom), "!temperature_step_bottom");
}

void update_time_step_pcb(int arg_cnt, char **args)
{
  update_steps(arg_cnt, args, profile.time_step_pcb, (byte *)ARRAY_SIZE(profile.time_step_pcb),  "!time_step_pcb");
}

void update_temperature_step_pcb(int arg_cnt, char **args)
{
  update_steps(arg_cnt, args, profile.temperature_step_pcb, (byte *)ARRAY_SIZE(profile.temperature_step_pcb), "!temperature_step_pcb");
}



// Single Variables Update Area -------------------------------------------
void update_aliasprofile(int arg_cnt, char **args)
{
  Stream *s = cmdGetStream();
  s->print("!aliasprofile: ");
  if (arg_cnt > 1)
  {
    strcpy(profile.alias, args[1]);
  }
  s->print(profile.alias);
  s->println(";");
}

void update_parameter(int arg_cnt, char **args, byte *parameter, const char *command)
{
    Stream *s = cmdGetStream();
    s->print(command);
    s->print(": ");
    if (arg_cnt > 1)
    {
        *parameter = atoi(args[1]);
    }
    s->print(*parameter);
    s->println(";");
}

void update_profile_steps(int arg_cnt, char **args)
{
    update_parameter(arg_cnt, args, &profile.profile_steps, "!profile_steps");
}

void update_table_size(int arg_cnt, char **args)
{
    update_parameter(arg_cnt, args, &profile.table_size, "!table_size");
}

void update_kp1(int arg_cnt, char **args)
{
    update_parameter(arg_cnt, args, &profile.kp1, "!kp1");
}

void update_ki1(int arg_cnt, char **args)
{
    update_parameter(arg_cnt, args, &profile.ki1, "!ki1");
}

void update_kd1(int arg_cnt, char **args)
{
    update_parameter(arg_cnt, args, &profile.kd1, "!kd1");
}

void update_kp2(int arg_cnt, char **args)
{
    update_parameter(arg_cnt, args, &profile.kp2, "!kp2");
}

void update_ki2(int arg_cnt, char **args)
{
    update_parameter(arg_cnt, args, &profile.ki2, "!ki2");
}

void update_kd2(int arg_cnt, char **args)
{
    update_parameter(arg_cnt, args, &profile.kd2, "!kd2");
}

void update_kp3(int arg_cnt, char **args)
{
    update_parameter(arg_cnt, args, &profile.kp3, "!kp3");
}

void update_ki3(int arg_cnt, char **args)
{
    update_parameter(arg_cnt, args, &profile.ki3, "!ki3");
}

void update_kd3(int arg_cnt, char **args)
{
    update_parameter(arg_cnt, args, &profile.kd3, "!kd3");
}

void update_max_correction_top(int arg_cnt, char **args)
{
    update_parameter(arg_cnt, args, &profile.max_correction_top, "!max_correction_top");
}

void update_max_correction_bottom(int arg_cnt, char **args)
{
    update_parameter(arg_cnt, args, &profile.max_correction_bottom, "!max_correction_bottom");
}

void update_max_pcb_delta(int arg_cnt, char **args)
{
    update_parameter(arg_cnt, args, &profile.max_pcb_delta, "!max_pcb_delta");
}

void update_hold_lenght(int arg_cnt, char **args)
{
    update_parameter(arg_cnt, args, &profile.hold_lenght, "!hold_lenght");
}

void update_participation_rate_top(int arg_cnt, char **args)
{
    update_parameter(arg_cnt, args, &profile.participation_rate_top, "!participation_rate_top");
}

// Statiion commands area ----------------------------------------------------------

void SAVE(int arg_cnt, char **args)
{
  Stream *s = cmdGetStream();
  if (arg_cnt <= 1)
  {
    printProfileNumber();
  }
  else
  {
    if (atoi(args[1]) <= 0 || atoi(args[1]) > max_profiles)
    {
      s->print("Profile number over limit");
    }
    else
    {
      currentProfile = atoi(args[1]);
      SaveProfile();
      s->print("Profile ");
      s->print(currentProfile);
      s->println(" updated");
      printProfileNumber();
      s->println("\r\n");
      s->println("!transfer_finished: 1;");  // Added to update profile graph on PC after profile transfer 
      s->println("\r\n");
       // Print profile Number
    }
  }
}

void UP(int arg_cnt, char **args) // Adjust PCB temperature up if called during profile running. BOTTOM temperature if called from IDLE
{
  Stream *s = cmdGetStream();

  if (arg_cnt > 1)
    s->print("UP sould have 0 params");
  else
  {
    if (reflowState == REFLOW_STATE_RUNNING || reflowState == REFLOW_STATE_HOLD_AUTO || reflowState == REFLOW_STATE_HOLD_MANUAL)
    {
      manual_temperature = (reflowState != REFLOW_STATE_HOLD_MANUAL) ? ((SetPoint_Pcb == 0) ? Input3 : SetPoint_Pcb) : manual_temperature + 2; // if it's first call during profile run - fix PCB current temp as setpoint
      manual_temp_changed = true;
      s->print("!set_temp: ");
      s->print(manual_temperature);
      s->println(";");
      if (reflowState != REFLOW_STATE_HOLD_MANUAL){HOLD(0, 0);} //если мы не в режиме ручной паузы - включить её
    }
  }
}

void DOWN(int arg_cnt, char **args)
{
  Stream *s = cmdGetStream();

  if (arg_cnt > 1)
    s->print("DOWN sould have 0 params");
  else
  {
    if (reflowState == REFLOW_STATE_RUNNING || reflowState == REFLOW_STATE_HOLD_AUTO || reflowState == REFLOW_STATE_HOLD_MANUAL)
    {
      manual_temperature = (reflowState != REFLOW_STATE_HOLD_MANUAL) ? ((SetPoint_Pcb == 0) ? Input3 : SetPoint_Pcb) : manual_temperature - 2;
      manual_temp_changed = true;
      s->print("!set_temp: ");
      s->print(manual_temperature);
      s->println(";");
      if (reflowState != REFLOW_STATE_HOLD_MANUAL){HOLD(0, 0);}
    }
  }
}

void RIGHTc(int arg_cnt, char **args) // Switch to next profile in IDLE
{
  Stream *s = cmdGetStream();

  if (arg_cnt > 1)
    s->print("RIGHT sould have 0 params");
  else
  {
    if (reflowState == REFLOW_STATE_IDLE)
    {
      currentProfile = currentProfile + 1;
      if (currentProfile > max_profiles)
        currentProfile = 1;
      loadProfile();
    }
  }
}

void LEFTc(int arg_cnt, char **args) // Switch to prev profile in IDLE
{
  Stream *s = cmdGetStream();

  if (arg_cnt > 1)
    s->print("LEFT sould have 0 params");
  else
  {
    if (reflowState == REFLOW_STATE_IDLE)
    {
      currentProfile = currentProfile - 1;
      if (currentProfile <= 0)
        currentProfile = max_profiles;
      loadProfile();
    }
  }
}

void CMD_CANCEL(int arg_cnt, char **args)
{
  Stream *s = cmdGetStream();

  if (arg_cnt > 1)
    s->print("CANCEL sould have 0 params");
  else
  {
    if (reflowStatus == REFLOW_STATUS_ON || reflowState == REFLOW_STATE_PRE_HEATER)
    {
      Serial.println("STOP");
      reflowState = REFLOW_STATE_COMPLETE; // Обработка команды Отмены выполнения
    }
  }
}

void BLOCK(int arg_cnt, char **args)        // Prevent interruptions during settings transfer
{
  Stream *s = cmdGetStream();
  if (arg_cnt > 1)
    s->print("BLOCK sould have 0 params");
  else
  {
    if (reflowState == REFLOW_STATE_IDLE)
    {
      Serial.println("Settings mode enable");
      blockflagM = true;
      
      #ifndef interruptTimer
        detachInterrupt(ZCC_PIN); // Additional transfer problem prevention
      #else
        #if defined(ESP8266) || defined(ESP32)
          timerStop(Timer0_Cfg);
        #endif
      #endif  
    }
  }
}

void UNBLOCK(int arg_cnt, char **args)
{
  Stream *s = cmdGetStream();
  if (arg_cnt > 1)
    s->print("UNBLOCK sould have 0 params");
  else
  {
    if (reflowState == REFLOW_STATE_IDLE)
    {
      Serial.println("Settings mode disable");
      blockflagM = false;

      #if defined(__AVR__)
         attachInterrupt(ZCC_PIN, Dimming, RISING);  // Additional transfer problem prevention
      #elif defined(ESP8266) || defined(ESP32)
         #ifndef interruptTimer
           attachInterrupt(ZCC_PIN, INT0_ISR, RISING);  // Additional transfer problem prevention
         #else
           timerStart(Timer0_Cfg);
           timerWrite(Timer0_Cfg, 0);
         #endif
      #endif 
    }
  }
}

void PRINT_PROFILE(int arg_cnt, char **args) // отправка профиля в КОМ Костыль))
{
  // printProfileNumber(); // печатает номер профиля не нужно, т.к. до этого вызывается loadProfile которая уже печатает номер профиля
  blockflag=true; // потому что print Profile снимает blockflag
  update_time_step_top(0, 0);
  update_temperature_step_top(0, 0);
  update_time_step_bottom(0, 0);
  update_temperature_step_bottom(0, 0);
  update_time_step_pcb(0, 0);
  update_temperature_step_pcb(0, 0);
  update_kp1(0, 0);
  update_ki1(0, 0);
  update_kd1(0, 0);
  update_kp2(0, 0);
  update_ki2(0, 0);
  update_kd2(0, 0);
  update_kp3(0, 0);
  update_ki3(0, 0);
  update_kd3(0, 0);
  update_profile_steps(0, 0);
  update_table_size(0, 0);
  update_max_correction_top(0, 0);
  update_max_correction_bottom(0, 0);
  update_max_pcb_delta(0, 0);
  update_hold_lenght(0, 0);
  update_participation_rate_top(0, 0);
  //update_aliasprofile(0, 0); // we already recieve profile alias from printProfileNumber()
  Serial.println("\r\n");
  Serial.println("!transfer_finished: 1;");
  Serial.println("\r\n");
    // added empty rows to prevent segfault when temperatures arrive right before transfer finished ";" sign
  blockflag=false;
  return;
}

void HOLD(int arg_cnt, char **args) // Ручная пауза выполнения профиля
{
  Stream *s = cmdGetStream();
  if (arg_cnt > 1)
    s->print("HOLD sould have 0 params");
  else
  {
    if (reflowStatus == REFLOW_STATUS_ON)
    {
      if (reflowState != REFLOW_STATE_HOLD_MANUAL) // Если не в Hold - перейти в него
      {
        manual_temperature = (SetPoint_Pcb == 0) ? Input3 : SetPoint_Pcb; // если задание платы = 0 (в профиле нет графика для платы) то задаём уставку равной текущей температуре платы
                                                                          // manual_temperature = SetPoint_Pcb;
        s->print("manual temp: ");
        s->print(manual_temperature);
        prev_reflowState = reflowState;
        reflowState = REFLOW_STATE_HOLD_MANUAL;
        Serial.println("@4;");
#ifdef UseScreen
        LcdDrawState();
#endif
#ifdef Enable_WiFi
      GenerateJson(4);
#endif     
      }
      else // если уже в HOLD и еще раз пришла команда HOLD - выходим из удержания
      {
        if (!manual_temp_changed)
        {
          if (prev_reflowState == REFLOW_STATE_RUNNING && abs(tc3 - SetPoint_Pcb) > profile.max_pcb_delta) // если предыдущее состояние RUNNING и температура платы ушла больше чем на макс отклонение температуры - стартуем профиль на горячую
          {
            jump_if_pcb_warm = true;
            reflowState = REFLOW_STATE_INIT;
          }
          else
          {
            reflowState = (reflowState_t)prev_reflowState;
          }
          Serial.println("HOLD_MANUAL Disabled");
        }
      }
    }
  }
}

void START(int arg_cnt, char **args)
{
  #ifdef Enable_WiFi
     JsonTextError = "";
  #endif

  Stream *s = cmdGetStream();

  if (arg_cnt > 1)
    s->print("START sould have 0 params");
  else
  {
    if (reflowState == REFLOW_STATE_IDLE)
    {
      #ifdef Enable_WiFi
         StartLogFile(false);
      #endif
      jump_if_pcb_warm = false;
      Serial.println("Profile Start");
      reflowStatus = REFLOW_STATUS_OFF;
      reflowState = REFLOW_STATE_INIT;
    }
  }
}

void HOTSTART(int arg_cnt, char **args) // Может быть вызвана в любой момент.
{
  Stream *s = cmdGetStream();

  if (arg_cnt > 1)
    s->print("HOTSTART sould have 0 params");
  else
  {
    #ifdef Enable_WiFi
         StartLogFile(false);
    #endif
    jump_if_pcb_warm = true;
    Serial.println("Profile HotStart");
    reflowStatus = REFLOW_STATUS_OFF;
    reflowState = REFLOW_STATE_INIT;
  }
}


void MANUAL_TEMP(int arg_cnt, char **args)
{
  Stream *s = cmdGetStream();

  if (arg_cnt < 3)
    s->println("MANUAL_TEMP sould have 2 params");
  else
  {
    if (reflowState == REFLOW_STATE_IDLE || REFLOW_STATE_HOLD_MANUAL)
    {
      manual_temperature = cmdStr2Num(args[2], 10);
      manual_temp_changed = true;
      s->print("manual_temperature: ");
      s->print(manual_temperature);
      s->println(";");

      s->print("!set_temp: ");
      s->print(manual_temperature);
      s->println(";");

      prev_reflowState = reflowState;
      reflowStatus = REFLOW_STATUS_ON;
      reflowState = REFLOW_STATE_HOLD_MANUAL;
      Serial.println("@4;");
#ifdef UseScreen
      LcdDrawState();
#endif
#ifdef Enable_WiFi
      GenerateJson(4);
#endif
     #if defined(ESP8266) || defined(ESP32)  
       #ifdef interruptTimer 
          timerStart(Timer0_Cfg);
          timerWrite(Timer0_Cfg, 0);
       #endif
      #endif  

      if (profile.table_size == 1)
      {
        digitalWrite(P1_PIN, HIGH);
      }
      if (profile.table_size == 2)
      {
        digitalWrite(P1_PIN, HIGH);
        digitalWrite(P2_PIN, HIGH);
      }
      if (profile.table_size == 3)
      {
        digitalWrite(P1_PIN, HIGH);
        digitalWrite(P2_PIN, HIGH);
        digitalWrite(P3_PIN, HIGH);
      }

  

      switch (cmdStr2Num(args[1], 10)) // Определяем чем будем управлять
      {
      case 2: // Греть НИ до указанной температуры
        SetPoint_Bottom = manual_temperature;
        manual_temp_changed = true;
        s->print("ManualControl_Bottom: ");
        s->println(manual_temperature);
        break;

      case 3: // Греть плату при помощи НИ до указанной температуры
        SetPoint_Pcb = manual_temperature;
        SetPoint_Bottom = manual_temperature * 1,5;        // Уставка НИ = 1,5 уставки платы
        manual_temperature_pcb = true;
        profile.max_correction_bottom = manual_temperature; // коррекция НИ = уставки платы
        s->print("ManualControl_PCB: ");
        s->println(manual_temperature);
        break;

      default:
        s->println("ManualControl Wrong param");
        break;
      }
    }
    else
    {
      s->print("Wrong state. Cant set MANUAL TEMP");
      // break;
    }
  }
}

void MANUAL_POWER(int arg_cnt, char **args)
{
  Stream *s = cmdGetStream();

  if (arg_cnt < 3)
    s->println("MANUAL_POWER sould have 2 params");
  else
  {
    if (reflowState == REFLOW_STATE_IDLE || reflowState == REFLOW_STATE_POWER_MANUAL) // Can start only from IDLE and adjust from POWER_MANUAL
    {
      manual_power = cmdStr2Num(args[2], 10);
      s->print("manual_power ");
      s->print(manual_power);
      s->println(";");

      s->print("!Set Power ");
      s->print(manual_power);
      s->println(";");

      prev_reflowState = reflowState;
      reflowStatus = REFLOW_STATUS_ON;
      reflowState = REFLOW_STATE_POWER_MANUAL;
      Serial.println("@4;");
#ifdef UseScreen
      LcdDrawState();
#endif
#ifdef Enable_WiFi
      GenerateJson(4);
#endif
 
      #if defined(ESP8266) || defined(ESP32)  
        #ifdef interruptTimer 
          timerStart(Timer0_Cfg);
          timerWrite(Timer0_Cfg, 0);
        #endif
      #endif  

      if (profile.table_size == 1)
      {
        digitalWrite(P1_PIN, HIGH);
      }
      if (profile.table_size == 2)
      {
        digitalWrite(P1_PIN, HIGH);
        digitalWrite(P2_PIN, HIGH);
      }
      if (profile.table_size == 3)
      {
        digitalWrite(P1_PIN, HIGH);
        digitalWrite(P2_PIN, HIGH);
        digitalWrite(P3_PIN, HIGH);
      }

    

      switch (cmdStr2Num(args[1], 10))
      {
      case 1: // Греть ВИ указанной мощностью
        Output1 = constrain(manual_power, 0, 100);
        s->print("ManualControl_POWER_Top: ");
        s->println(Output1);
        break;

      case 2: // Греть НИ до указанной мощностью
        Output2 = constrain(manual_power, 0, 100);
        s->print("ManualControl_POWER_Bottom: ");
        s->println(Output2);
        break;

      default:
        s->println("ManualControl. Wrong parameter");
        break;
      }
    }
    else
    {
      s->print("Wrong state. Cant set MANUAL Power.");
      // break;
    }
  }
}

void beep(int times)
{
  int t = 0;
  do
  
  {
    if (millis() > nextSound)
    {
      // Read thermocouples next sampling period
      nextSound = millis() + 400;
    Serial.println("$@;");
    t++;
    }
  } while (t < times);

#ifdef Enable_WiFi
  char jq[25];
  sprintf(jq, "{\"beep\":%d}", (int)t);
  ws.textAll(jq);
#endif
}

#if defined(ESP8266) || defined(ESP32)
// Make size of files human readable
// source: https://github.com/CelliesProjects/minimalUploadAuthESP32
String humanReadableSize(const size_t bytes) {
  if (bytes < 1024) return String(bytes) + " B";
  else if (bytes < (1024 * 1024)) return String(bytes / 1024.0) + " KB";
  else if (bytes < (1024 * 1024 * 1024)) return String(bytes / 1024.0 / 1024.0) + " MB";
  else return String(bytes / 1024.0 / 1024.0 / 1024.0) + " GB";
}

#ifdef Enable_WiFi

// list all of the files, if ishtml=true, return html rather than simple text
String listFiles(bool ishtml) {

  String returnText = "<p>Free: "+ String(humanReadableSize((FFat.totalBytes() - FFat.usedBytes()))) + " / Total: "+ String(humanReadableSize(FFat.totalBytes()))+"</p>";
  #ifdef show_diagnostics_com
    Serial.println("Listing files stored on FFat");
  #endif
  File root = FFat.open("/");
  File foundfile = root.openNextFile();
  if (ishtml) {
    returnText += "<table><tr><th align='left'>Name</th><th align='left'>Size</th><th></th><th></th></tr>";
  }
  while (foundfile) {
    if (ishtml) {
      returnText += "<tr align='left'><td>" + String(foundfile.name()) + "</td><td>" + humanReadableSize(foundfile.size()) + "</td>";
      returnText += "<td><button onclick=\"fileButton(\'" + String(foundfile.name()) + "\', \'download\')\">Download</button>";
      returnText += "<td><button onclick=\"fileButton(\'" + String(foundfile.name()) + "\', \'delete\')\">Delete</button>";
      returnText += "<td><button onclick=\"fileButton(\'" + String(foundfile.name()) + "\', \'load\')\">Load graph</button></tr>";
    } else {
      returnText += "File: " + String(foundfile.name()) + " Size: " + humanReadableSize(foundfile.size()) + "\n";
    }
    foundfile = root.openNextFile();
  }
  if (ishtml) {
    returnText += "</table>";
  }
  root.close();
  foundfile.close();
  return returnText;
}

void notFound(AsyncWebServerRequest *request) {
  String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
  #ifdef show_diagnostics_com
    Serial.println(logmessage);
  #endif  
  request->send(404, "text/plain", "Not found");
}

void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  // make sure authenticated before allowing upload
    String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
    #ifdef show_diagnostics_com
      Serial.println(logmessage);
    #endif

    if (!index) {
      logmessage = "Upload Start: " + String(filename);
      // open the file on first call and store the file handle in the request object
      request->_tempFile = FFat.open("/" + filename, "w");
      #ifdef show_diagnostics_com
        Serial.println(logmessage);
      #endif
    }

    if (len) {
      // stream the incoming chunk to the opened file
      request->_tempFile.write(data, len);
      logmessage = "Writing file: " + String(filename) + " index=" + String(index) + " len=" + String(len);
      #ifdef show_diagnostics_com
        Serial.println(logmessage);
      #endif
    }

    if (final) {
      logmessage = "Upload Complete: " + String(filename) + ",size: " + String(index + len);
      // close the file handle as the upload is now done
      request->_tempFile.close();
      #ifdef show_diagnostics_com
        Serial.println(logmessage);
      #endif
      request->redirect("/");
    }
  
}

#endif
#endif

