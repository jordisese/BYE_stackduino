/*////////////////////////////////////////////////////////////////////////////////////////////////////////
//  BASADO EN PARTE EN STACKDUINO 1 (G1.0)                                                              //
//                                                                                                      //
//  A sketch to drive an Arduino compatible MacroPhotography Focus Stacking Controller                  //
//  http://reallysmall.github.io/Stackduino/                                                            //
//  (G1) Code clean-up and additional functions by GigiG, May 2016                                      //
//  (https://sites.google.com/site/gigimysite/home/macro-rail)                                          //
//  Adaptado por Jordi Sesé a las manías y uso de José A. Soldevilla Feb.2020                           //
//                                                                                                      //  
////////////////////////////////////////////////////////////////////////////////////////////////////////*/

/*////////////////////////////////////////////////////////////////////////////////////////////////////////
//  LIBRARY DEPENDANCIES                                                                                //         
////////////////////////////////////////////////////////////////////////////////////////////////////////*/
#include <EEPROM.h>

// -------
// para deshabilitar la wifi del todo
#ifdef ESP8266
  #include "ESP8266WiFi.h"
#endif
boolean do_overclock =false; // switch 80/160 on ESP8266 when in FAST mode
//-------

//#define disableJoystick // el analogRead coge valores aleatorios si no está conectado

#define xAnalogPin A0

#include <LCD.h>
#include <LiquidCrystal_I2C.h>


  #define I2C_ADDR        0x27  // Define I2C Address for the PCF8574T 
  #define I2C_ADDR_ALT    0x3F  // OJO, solamente una de las dos!!!!
  
  //---(Following are the PCF8574 pin assignments to LCD connections )----
  // This are different than earlier/different I2C LCD displays
  #define BACKLIGHT_PIN  3
  #define En_pin  2
  #define Rw_pin  1
  #define Rs_pin  0
  #define D4_pin  4
  #define D5_pin  5
  #define D6_pin  6
  #define D7_pin  7
  #define  LED_OFF  1
  #define  LED_ON  0

LiquidCrystal_I2C  lcd(I2C_ADDR,En_pin,Rw_pin,Rs_pin,D4_pin,D5_pin,D6_pin,D7_pin);

#include "Adafruit_MCP23017.h"
Adafruit_MCP23017 mcp;

/*////////////////////////////////////////////////////////////////////////////////////////////////////////
//  ARDUINO PIN ASSIGNMENTS                                                                             //
////////////////////////////////////////////////////////////////////////////////////////////////////////*/

//#define ENC_A D6 //A0			// rotary encoder (A0 - pin 14)
//#define ENC_B D7 //A1			// rotary encoder (A1 - pin 15)
//#define ENC_PORT PINC	// rotary encoder (a port not a pin)
#include <RotaryEncoder.h>
int encoder0PinA = D6;
int encoder0PinB = D7;
int encoderButton = D5;
int encoderPos = 0;
RotaryEncoder encoder(encoder0PinA, encoder0PinB);

// conexion directa - interrupciones
const byte main_button = D8; //2;			// start/stop stack button (for attachInterrupt must be 2||3)
const byte encoder_button = D5; // 3;		// select/unselect menu item button (for attachInterrupt must be 2||3)

// ojo: a traves del extensor (B5=13)
const byte set_direction =  13;// 16;		// (A2) stepper motor direction
// directo!!!!
const byte do_step = D4; // 

// --- ATENCION!!!! pendientes de asignar
// ojo: a traves del extensor (A3=3)
const byte focus = 4; //11;					// send an autofocus signal to the camera
// ojo: a traves del extensor (A4=4)
const byte shutter = 3; //13;				// send a shutter signal to the camera
// ---
// ojo: a traves del extensor (A0=0)
const byte forward_control = 0; // 9;		// for manual positioning
// ojo: a traves del extensor (B7=15)
const byte enable = 15; // 18;					// (A4) enable and disable easydriver when not stepping to reduce heat and power consumption
// ojo: de momento directo, hay que pasarlo a traves del extensor
//const byte limit_switches = D0; // 19;		// (A5) limit switches to stop stepping if end of travel on rail is reached at either end
// ojo: a traves del extensor (A1=1)
const byte limit_switches = 1;    // limit switches to stop stepping if end of travel on rail is reached at either end
// ojo: a traves del extensor (B0=8)
const byte backward_control = 8; //10;	// for manual positioning
// ojo: a traves del extensor (B1=9)
const byte fast_manual = 9;			// fast speed for manual positioning when pressed togheter with fwd/bwd control
// ojo: a traves del extensor (B6=14)
const byte fast_led = 14;       // (A3) led indicating fast/normal speed
bool fast_on = false;
bool was_fast_on = false;
bool force_dir_write = false; // write direction pin to stepper every time (DEBUG!)

// ojo: a traves del extensor (A2=2)
const byte stacking = 2;// 5;				// LED that turns on during stacking phase

// ojo: a traves del extensor (A5=5 y A6=6)
const byte photo_button = 5; // botón para tomar una foto
const byte photo_led = 6; // LED that turns on when taking photos
// M0, M1 y M2 a traves del extensor (B2=10, B3=11 y B4=12) - TO DO!!!!
const byte pin_M0 = 10;
const byte pin_M1 = 11;
const byte pin_M2 = 12;

/*////////////////////////////////////////////////////////////////////////////////////////////////////////
//  DEFAULT SETTINGS (some initialized in loadDefault function below)                                   // 
////////////////////////////////////////////////////////////////////////////////////////////////////////*/
int bracket;								// number of images to bracket per focus slice
boolean disable_easydriver = true;	// whether to disable easydriver betweem steps to save power and heat
boolean disable_on_stacking = false; // hacer enable/disable para cada movimiento o dejarlo siempre en enable durante el stacking
boolean stacking_now = false;

int encoder_counter = 0;				// count pulses from encoder
int encoder_pos = 1;						// which menu item to display when turning rotary encoder
int last_encoder_pos = 0;
boolean forward = true;					// store the current direction of travel - used when a limit switch is hit because
												// we can't digitalRead the direction pin as it is set as an OUTPUT
int mirror_lockup;						// set to 2 to enable mirror lockup
int mirror_delay = 2000;				// sets the delay between mirror up and shutter (milliseconds)
int return_to_start;						// whether camera/ subject is returned to starting position at end of stack - set to 2 for yes
int exp_time;								// delay needed for camera to process focus and shutter signals (milliseconds)
int exp_post;								// default time to wait for camera to take picture (milliseconds)
int slice_counter = 0;					// count of number of pictures taken so far
int slice_depth;							// default number of microns stepper motor should make between pictures
int slices;									// default total number of pictures to take
int stepper_speed;						// delay in microseconds between motor steps, governing motor speed in stack motions
int stepper_speed_manual;				// delay in microseconds between motor steps, governing motor speed in manual motions and returning
int fast_manual_speed;					// delay in microseconds between motor steps, governing motor speed in fast manual motions
int unit_of_measure;						// whether to use microns, mm or cm when making steps
int unit_of_measure_multiplier = 1;	// multiplier to factor into step signals (1, 1000 or 10000) depending on unit of measure used
boolean update_screen = true;			// whether to print to the lcd with new or updated information
int motor_post_delay;					// optionally set a delay to allow vibrations from stage movement to settle (milliseconds)
int bracket_delay = 1000;				// delay between bracketed shots (milliseconds)
// OJO!!!!!!!
float gear = 1.28;						// a float to tune the distance travelled based on your setup, see note below:
// The 'gear' value depends on stepper's number of steps per full rotation, the distance traveled by your carriage by one stepper's
// shaft rotation (in um) and the EasyDriver's microstepping value.
// In my case (200 steps per rev stepper, M8x1.25 lead screw and 8 microstepping ), gear is (200/1250)*8 = 1.28
int save_settings = 2;					// set to 1 by menu to save all current variable values
int load_defaults = 2;					// set to 1 by menu to load variable default values
int load_lastSaved = 2;					// set to 1 by menu to load last saved values
int test_shot = 2;						// set to 1 by menu to shoot a camera test shot
long microsteps;							// number of motor microsteps for each slice
int backslash_slice = 10;    // micropasos extra para compensar el backslash
int backslash_adjust = 0; // pasos que se pierden al cambiar de sentido
int fast_increment = 10; // incremento en valores cuando se pulsa el boton FAST
int goto_speed = 1; // velocidad para los GoTos (stack/slow/fast)
int drv_model = 3; // modelo de driver (para calcular/ajustar los micropasos si es necesario)
int drv_disable = 2; // deshabilitar driver 1=nunca, 2=en movimientos, pero no en el apilado, 3=a cada movimiento
                     // ajusta las otras variables *disable_on_stacking* y *disable_easydriver*
int delay_microseconds = 5; // microsegundos entre HIGH/LOW para el avance del motor

// main button toggle (Start/stop)
volatile int main_button_state = HIGH;		// current state of the output pin
volatile int main_button_reading;			// current reading from the input pin
volatile int main_button_previous = LOW;	// previous reading from the input pin
volatile long main_button_time = 0;			// last time the output pin was toggled
volatile long main_button_debounce = 400;	// debounce time, increase if the output flickers

// rotary encoder momentary button toggle
volatile int rotary_button_state = HIGH;	// current state of the output pin
volatile int rbreading;							// current reading from the input pin
volatile int rbprevious = HIGH;				// previous reading from the input pin
volatile long rbtime = 0;						// last time the output pin was toggled
volatile long fast_time = 0;            // last time the output pin was toggled
volatile long rbdebounce = 400;				// debounce time, increase if the output flickers

// posiciones inicio/fin/actual
int start_position=0;
bool start_position_set=false;
int end_position=0;
bool end_position_set=false;
int current_position=0;

// posiciones de limite inicio (home) y final (end of path)
int home_position = 0;
int end_of_path_position = 0;

// opciones adicionales
int driver_microsteps = 1; // micropasos del driver: 1,2,4,8,16,32,64,128,256...
int motor_steps = 200; // pasos del motor 1.8=200, 0.9=400
int shaft_revolution = 1250; // en micrones, lo que recorre el eje en una vuelta completa
int manual_min_steps =1; // pasos a avanzar de golpe en el control manual del motor
int goto_start=0; // si estan en 1, indican que hay que desplazarse a la posicion inicio/fin
int goto_end=0;
int set_slice_depth_option=0; // para abrir directamente la opcion de pasos tras marcar el fin de recorrido y que calcule
bool forward_stack = true; // segun la posicion inicial y final, sentido de desplazamiento
int read_manual_buttons = 1; // leer botones o joystick para el control manual (1=botones, 0=joystick)
int manual_latency = 25; // para no estar leyendo cada vez, asumimos que al menos estará pulsado esos milisegundos
int forward_control_value = HIGH;
int backward_control_value = HIGH;
volatile long mbf_prevMillis = 0;
volatile long mbb_prevMillis = 0;

const int MCP_PINS = 16; // numero de pins del MPC
volatile long mcp_prevMillis [MCP_PINS] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int mcp_pin_value [MCP_PINS] = {HIGH,HIGH,HIGH,HIGH,HIGH,HIGH,HIGH,HIGH,HIGH,HIGH,HIGH,HIGH,HIGH,HIGH,HIGH,HIGH};
volatile long mcp_latency = 100; // para no estar leyendo cada vez, asumimos que al menos estará pulsado esos milisegundos

const int MENU_COUNT = 35;
enum menu_options { START, END, SLICE_DEPTH, SHOTS, RETURN, EXPOSURE, EXPO_PAUSE, MOTOR_PAUSE, SPEED, MANUAL_SPEED, MANUAL_HI_SPEED, MANUAL_LATENCY, GOTO_SPEED, MIRROR, BRACKET, UNIT, MOTOR_STEPS, DRIVER_MODEL, DRIVER_MICROSTEPS, DRIVER_DISABLE, SHAFT_REVOLUTION, MANUAL_STEP_BUNCH, BACKSLASH_SLICE, FAST_INCREMENT, RAMP_START, READ_BUTTONS, CONFIG_SAVE, CONFIG_LOAD, DEFAULTS, CAMERA_SHOT, SHOW_STEPS, GO_FWD, GO_BCK, GOTO_START, GOTO_END, END_OF_MENU };
bool menuFast_options [MENU_COUNT] { 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1  };

bool stepper_disabled = false;

enum joystick_axis { LEFT, RIGHT, UP, DOWN };

volatile long loop_timestamp = 0; // tiempo en micros de paso anterior por el bucle principal, para aplicarlo al cálculo de las RPMs
volatile long loop_timeslice = 0; // diferencia de tiempo (en micros) respecto al paso anterior por el bucle 
volatile long step_timeslice = 0; // tiempo de pausa del motor, para descontarlo en el paso del bucle

// ---- rampa,velocidades en RPM
int ramp_start = 0; // si fast_manual_speed, etc [pausa en micros] es *menor*, se aplica rampa de aceleracion - basado en: delay in microseconds between motor steps 
float ramp_value = 0;
float rpm_stack_speed = 0;
float rpm_fast_speed = 0;
float rpm_manual_speed = 0;
float rpm_ramp_start = 0;
float ramp_increment = 0.05;

// --- prototipos necesarios para que compile (habría que pasarlos a un .H...)
void stepper_direction(bool do_backslash_adjust);
void goto_position(int pos, bool compensate_backlash);



void setup() 
{
#ifdef ESP8266
// ------------------deshabilitamos la wifi----------------------------------
WiFi.forceSleepBegin();                  // turn off ESP8266 RF
delayMillis(1);                                // give RF section time to shutdown
// --------------------------------------------------------------------------
#endif

  //EEPROM.begin(512);
  SPIFFS.begin();

  Serial.begin(115200);	// may interfere with LCD pins connected to 0 and 1 (RX-TX)
  Serial.println("");
  Serial.println("Initializing...");
  Serial.println("");

  // test if alternate I2C address for LCD1602
  Wire.begin();
  Wire.beginTransmission(I2C_ADDR_ALT);       // check for alternate address here
  if (Wire.endTransmission() == 0) 
  {  // alt LCD found
     lcd = LiquidCrystal_I2C(I2C_ADDR_ALT,En_pin,Rw_pin,Rs_pin,D4_pin,D5_pin,D6_pin,D7_pin); // reinitialise lcd with new address
  }
  // ----

  mcp.begin();      // use default address 0
  lcd.begin(16, 2);
  //lcd.begin(20, 4);
  lcd.setBacklightPin(BACKLIGHT_PIN,POSITIVE);
  lcd.setBacklight(LED_ON);
  lcd.clear();
  lcd.home();
  lcd.backlight();  //Backlight ON if under program control
  
	attachInterrupt(digitalPinToInterrupt(main_button), button_main_change, CHANGE);			// main button on interrupt 0
	attachInterrupt(digitalPinToInterrupt(encoder_button), button_rotary_change, CHANGE);	// encoder button on interrupt 1

	pinMode(main_button, INPUT);
	pinMode(encoder_button, INPUT);
	mcp.pinMode(set_direction, OUTPUT);   
	//mcp.pinMode(do_step, OUTPUT);
  pinMode(do_step, OUTPUT);
//---- OJO
	mcp.pinMode(focus, OUTPUT);
	mcp.pinMode(shutter, OUTPUT);
//---- OJO
// no son necesarios (se lee por otro lado)
	//pinMode(ENC_A, INPUT);
	//pinMode(ENC_B, INPUT);
  
	mcp.pinMode(forward_control, INPUT);
  mcp.pullUp(forward_control, HIGH);  // turn on a 100K pullup internally
  mcp.pinMode(backward_control, INPUT); 
  mcp.pullUp(backward_control, HIGH);  // turn on a 100K pullup internally
	mcp.pinMode(enable, OUTPUT);
	mcp.pinMode(limit_switches, INPUT);
  mcp.pullUp(limit_switches, HIGH);  // turn on a 100K pullup internally
	
  mcp.pinMode(fast_manual, INPUT);  // D1
  mcp.pullUp(fast_manual, HIGH);  // turn on a 100K pullup internally

	mcp.pinMode(stacking, OUTPUT);	// 
  mcp.pinMode(photo_led, OUTPUT); //
  mcp.pinMode(fast_led, OUTPUT); // 

  mcp.pinMode(photo_button, INPUT);  //
  mcp.pullUp(photo_button, HIGH);  // turn on a 100K pullup internally

	digitalWrite(main_button, HIGH);
	digitalWrite(encoder_button, HIGH);
	mcp.digitalWrite(set_direction, HIGH);
	//mcp.digitalWrite(do_step, LOW);
 digitalWrite(do_step, LOW);
//---- PENDENTS 
	mcp.digitalWrite(focus, LOW);
	mcp.digitalWrite(shutter, LOW);
	//digitalWrite(ENC_A, HIGH);
	//digitalWrite(ENC_B, HIGH);
//---- PENDENTS
	mcp.digitalWrite(enable, LOW);
	mcp.digitalWrite(limit_switches, HIGH);
  mcp.digitalWrite(forward_control, HIGH);
	mcp.digitalWrite(backward_control, HIGH);
	//mcp.digitalWrite(fast_manual, HIGH);
//---- LEDS
	mcp.digitalWrite(stacking, LOW);
  mcp.digitalWrite(photo_led, LOW);
  mcp.digitalWrite(fast_led, LOW);
// -------

// ---- joystick
//  pinMode(xAnalogPin,INPUT);
// ----

	lcd.setCursor(0, 0);
	lcd.print("ByE StackDuino");
	delayMillis(1000);
	lcd.clear();
	loadDefaults();
	//readEEPROM();
  readConfigFile();

  stepper_direction(false);

  Serial.println("Initialized!");
  Serial.println("");
}

/* RETURN CURRENT STATE OF MAIN PUSH BUTTON (START/STOP) ---------------------------------------------- */
ICACHE_RAM_ATTR void button_main_change() {
	main_button_reading = digitalRead(main_button);
	if (main_button_reading == LOW && main_button_previous == HIGH && millis() - main_button_time > main_button_debounce) {
		if (main_button_state == HIGH)
			main_button_state = LOW;
		else
			main_button_state = HIGH;
		main_button_time = millis();    
	}
	main_button_previous = main_button_reading;
}

/* RETURN CURRENT STATE OF ROTARY ENCODER'S PUSH BUTTON ----------------------------------------------- */
ICACHE_RAM_ATTR void button_rotary_change() {
	rbreading = digitalRead(encoder_button);
	if (rbreading == LOW && rbprevious == HIGH && millis() - rbtime > rbdebounce) {
		if (rotary_button_state == HIGH)
			rotary_button_state = LOW;
		else
			rotary_button_state = HIGH;
		rbtime = millis();    
	}
	rbprevious = rbreading;
  update_screen = true;
}



/*////////////////////////////////////////////////////////////////////////////////////////////////////////
//  THE MAIN ARDUINO LOOP!                                                                              //
////////////////////////////////////////////////////////////////////////////////////////////////////////*/

void loop() {
//Serial.println("entering loop");
  microsteps = slice_depth; // hemos pasado a trabajar directamente con pasos
  if (mcp_pin_read(limit_switches) == LOW) {
    limit_abort();
	}
	if(main_button_state == LOW) 
	{
Serial.println("main_button_state LOW - STACK sequence");
     stack_sequence();
     return; // es un continue para el main loop
	}
 
  // proceso normal - botones, movimiento, etc.
	menu_functions();
  check_gotos();

  // boton FAST
  if(mcp_pin_read(fast_manual) == LOW)
  {
    loop_timestamp=0;
    fast_toggle();
    //delayMillis(rbdebounce/2+1);
  }
  do_yield();

}
