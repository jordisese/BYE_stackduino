/* LOAD THE DEFAULT VALUES FOR MENU SELECTABLE VARIABLES-------------------------------- */
void loadDefaults() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Loading defaults");
  bracket = 1;              // number of images to bracket per focus slice
  mirror_lockup = 1;          // set to 2 to enable mirror lockup
  return_to_start = 2;          // whether camera/ subject is returned to starting position at end of stack - set to 2 for yes
  exp_time = 200;           // delay needed for camera to process focus and shutter signals (milliseconds)
  exp_post = 2000;            // default time to wait for camera to take picture (milliseconds)
  slice_depth = 20;           // default number of microns stepper motor should make between pictures
  slices = 5;               // default total number of pictures to take
  stepper_speed = 1500;       // STACK - delay in microseconds between motor steps, governing motor speed in stack motions
  stepper_speed_manual = 1500;    // NORMAL - delay in microseconds between motor steps, governing motor speed in manual motions and returning
  fast_manual_speed = 500;      // FAST - delay in microseconds between motor steps, governing motor speed in fast manual motions
  unit_of_measure = 1;          // microns
  unit_of_measure_multiplier = 1; // microns
  motor_post_delay = 500;       // optionally set a delay to allow vibrations from stage movement to settle (milliseconds)
  motor_steps=200;
  driver_microsteps=1;
  shaft_revolution=1250;
  manual_min_steps=1; // pasos a dar de una vez en cada avance manual (si va de uno en uno a veces pierde pasos)
  delayMillis(500);
  lcd.setCursor(0, 1);
  lcd.print("Done");
  delayMillis(500);
  encoder_pos = 0;        //set menu option display to first
  fast_increment = 10;
  read_manual_buttons = 1; // leer botones para desplazamiento manual
  goto_speed = 1;
  drv_model = 3;
  drv_disable = 2;
  sync_driver_disable();
  manual_latency = 25;
  backslash_adjust = 0;
  ramp_start = 500;
  update_screen = true;   //allow the first menu item to be printed to the screen

  rpm_stack_speed=10;
  rpm_manual_speed=10;
  rpm_fast_speed=20;
  rpm_ramp_start=10;
 
  recalculate_speed();
  
Serial.println("Defaults loaded");

}

void writeConfigFile()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Saving ...");
  String configstr;
  File f = SPIFFS.open("/bye.config", "w");
  configstr =         String(slice_depth);
  configstr += "\n" + String(slices);
  configstr += "\n" + String(return_to_start);
  configstr += "\n" + String(exp_time);
  configstr += "\n" + String(exp_post);
  configstr += "\n" + String(motor_post_delay);
  //configstr += "\n" + String(stepper_speed);
  //configstr += "\n" + String(stepper_speed_manual);
  //configstr += "\n" + String(fast_manual_speed);
  configstr += "\n" + String((rpm_stack_speed*10));
  configstr += "\n" + String((rpm_manual_speed*10));
  configstr += "\n" + String((rpm_fast_speed*10));
  configstr += "\n" + String(mirror_lockup);
  configstr += "\n" + String(bracket);
  configstr += "\n" + String(unit_of_measure);
  configstr += "\n" + String(motor_steps);
  configstr += "\n" + String(driver_microsteps);
  configstr += "\n" + String(shaft_revolution);
  configstr += "\n" + String(manual_min_steps);
  configstr += "\n" + String(backslash_slice);
  configstr += "\n" + String(fast_increment);
  configstr += "\n" + String(read_manual_buttons);
  configstr += "\n" + String(goto_speed);
  configstr += "\n" + String(drv_model);
  configstr += "\n" + String(manual_latency);
  configstr += "\n" + String(drv_disable);
  configstr += "\n" + String((rpm_ramp_start*10));
  f.println(configstr);
  f.close();
  lcd.setCursor(0, 1);
  lcd.print("Done");
  delayMillis(500);
  encoder_pos = 1;        //set menu option display to first
  update_screen = true;   //allow the first menu item to be printed to the screen
}

void readConfigFile()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Loading Config");
  String s;
  File f = SPIFFS.open("/bye.config", "r");
  if (!f)
  {
Serial.println("Error: config file not found");
    return;
  }
  
  s=f.readStringUntil('\n');
  slice_depth=s.toInt();
  s=f.readStringUntil('\n');
  slices=s.toInt();
  s=f.readStringUntil('\n');
  return_to_start=s.toInt();
  s=f.readStringUntil('\n');
  exp_time=s.toInt();
  s=f.readStringUntil('\n');
  exp_post=s.toInt();
  s=f.readStringUntil('\n');
  motor_post_delay=s.toInt();
  s=f.readStringUntil('\n');
  //stepper_speed=s.toInt();
  rpm_stack_speed=(float)s.toInt() / 10.0;
  s=f.readStringUntil('\n');
  //stepper_speed_manual=s.toInt();
  rpm_manual_speed=(float)s.toInt() / 10.0;
  s=f.readStringUntil('\n');
  //fast_manual_speed=s.toInt();
  rpm_fast_speed=(float)s.toInt() / 10.0;
  s=f.readStringUntil('\n');
  mirror_lockup=s.toInt();
  s=f.readStringUntil('\n');
  bracket=s.toInt();
  s=f.readStringUntil('\n');
  unit_of_measure=s.toInt();
  s=f.readStringUntil('\n');
  motor_steps=s.toInt();
  s=f.readStringUntil('\n');
  driver_microsteps=s.toInt();
  s=f.readStringUntil('\n');
  shaft_revolution=s.toInt();
  s=f.readStringUntil('\n');
  manual_min_steps=s.toInt();
  s=f.readStringUntil('\n');
  backslash_slice=s.toInt();
  s=f.readStringUntil('\n');
  fast_increment=s.toInt();
  s=f.readStringUntil('\n');
  read_manual_buttons=s.toInt();
  s=f.readStringUntil('\n');
  goto_speed=s.toInt();
  s=f.readStringUntil('\n');
  drv_model=s.toInt();
  s=f.readStringUntil('\n');
  manual_latency=s.toInt();
  s=f.readStringUntil('\n');
  drv_disable=s.toInt();
  s=f.readStringUntil('\n');
  rpm_ramp_start=(float)s.toInt() / 10.0;

  f.close();

  recalculate_speed();
  sync_driver_disable();

  delayMillis(500);
  lcd.setCursor(0, 1);
  lcd.print("Done");
  delayMillis(500);
  encoder_pos = 0;        // set menu option display to first
  update_screen = true;   // allow the first menu item to be printed to the screen
}
