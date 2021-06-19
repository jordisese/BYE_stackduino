
// a float to tune the distance travelled based on your setup, see note below:
// The 'gear' value depends on stepper's number of steps per full rotation, the distance traveled by your carriage by one stepper's
// shaft rotation (in um) and the EasyDriver's microstepping value.
// In my case (200 steps per rev stepper, M8x1.25 lead screw and 8 microstepping ), gear is (200/1250)*8 = 1.28
void recalculate_gear()
{
  gear=((float)motor_steps/(float)shaft_revolution)*(float)driver_microsteps;
//Serial.println("gear ["+String(gear,4)+"]");
}

void recalculate_speed()
{
  stepper_speed=(int)rpm_to_delay(rpm_stack_speed);
  stepper_speed_manual=(int)rpm_to_delay(rpm_manual_speed);
  fast_manual_speed=(int)rpm_to_delay(rpm_fast_speed);
  ramp_start=(int)rpm_to_delay(rpm_ramp_start);
//Serial.println("stepper_speed ["+String(stepper_speed)+"]");
//Serial.println("stepper_speed_manual ["+String(stepper_speed_manual)+"]");
//Serial.println("fast_manual_speed ["+String(fast_manual_speed)+"]");
//Serial.println("ramp_start ["+String(ramp_start)+"]");
}
void get_speed()
{
  rpm_stack_speed=delay_to_rpm(stepper_speed);
  rpm_manual_speed=delay_to_rpm(stepper_speed_manual);
  rpm_fast_speed=delay_to_rpm(fast_manual_speed);
  rpm_ramp_start=delay_to_rpm(ramp_start);
}

// recalculo de num. expos y distancia entre expos en pasos al cambiar los valores de una u otra variable cuando hay inicio/fin
void recalculate_expos()
{
//Serial.println("---recalculate_expos");
  if(start_position_set && end_position_set)
  {
//Serial.println("start position ["+String(start_position)+"]");
//Serial.println("end position ["+String(end_position)+"]");
    int distance=end_position-start_position;
//Serial.println("distance ["+String(distance)+"]");   
    if(distance > 0)
      forward_stack=true;
    else
      forward_stack=false;
    distance=abs(distance);
    if(distance != 0)
    {
      slices=distance/slice_depth;
      if(slices*slice_depth < distance) // si queda alguna colgada por decimales, también la añadimos
      {
        slices++;
      }
      slices++; // siempre añadimos la expo inicial de la posición de inicio;
    }
//Serial.println("forward ["+String(forward)+"]");
//Serial.println("forward_stack ["+String(forward_stack)+"]");
  }
}
void recalculate_slice()
{
//Serial.println("---recalculate_slice");
  if(start_position_set && end_position_set)
  {
    int distance=end_position-start_position;
    if(distance > 0)
      forward_stack=true;
    else
      forward_stack=false;
    if(distance != 0)
    {
      // ------ calcular la distancia en cada slice y convertirla a slice_depth aplicando el gear
      slice_depth=abs(distance/slices);
    }
  }
}

void lcd_unit_of_measure() {        
  if (unit_of_measure==1) {
    lcd.print(" um");
  }
  if (unit_of_measure==2) {
    lcd.print(" mm");
  }
  if (unit_of_measure==3) {
    lcd.print(" cm");
  }
}

void menu_functions() {
			if (rotary_button_state == HIGH) {
				// -------------- SAVE SETTINGS OR LOAD DEFAULTS 
				// after confirming the menu command to save current values:
				if (save_settings==1) {
					save_settings = 2;
					//saveSettings();
          writeConfigFile();
          encoder_pos = 0;
				}
				// after confirming the menu command to load defaults values:
				if (load_defaults==1) {
					load_defaults = 2;
					loadDefaults();
          encoder_pos = 0;
				}
				// after confirming the menu command to load last saved values:
				if (load_lastSaved==1) {
					load_lastSaved = 2;
					//readEEPROM();
          readConfigFile();
          encoder_pos = 0;
				}
				// after confirming the menu command to shoot a test shot:
				if (test_shot==1) {
					test_shot = 2;
					testShot();
				}
			}

			/* MENUS AND MANUAL FWD/BWD/PHOTO CONTROL */
			// enable manual motor control to position stage before stack:
			stepper_control();
      photo_control();
			// use encoder to scroll through menu of settings:
			if (rotary_button_state == HIGH) {
        loop_timestamp=0;
        last_encoder_pos=encoder_pos;
        encoder_update(encoder_pos, 0, END_OF_MENU);
        if(fast_on && last_encoder_pos != encoder_pos)
        {
          int enc_inc=(last_encoder_pos < encoder_pos ? 1:-1);
          while(!menuFast_options[encoder_pos] && encoder_pos > 0 && encoder_pos < END_OF_MENU) // si está en FAST, restringimos las opciones          
            encoder_pos+=enc_inc;
        }
        if (encoder_pos >= END_OF_MENU) {
          encoder_pos = 0;
        }
        if (encoder_pos < 0) {
          // reset it to 1 again to create a looping navigation:
          encoder_pos = END_OF_MENU-1;
        } 
			} 
			// the menu options:
//Serial.println("encoder_pos["+String(encoder_pos)+"]");
			switch (encoder_pos) {

//-------------------------------------------------------------------------------
/*
                // para seleccionar posición:
                // mover con las flechas y confirmar con el botón (select/rotary - que sean un duplicado)
                // --- mostrar un contador de pasos o algo así... sobretodo si se ha calibrado con /5/
      
        case 1: // set start position

                // si la establece, pasa automáticamente a la siguiente
                
        case 2: // set end position

// si las dos posiciones están establecidas, pasar automáticamente a preguntar 'distancia' entre fotos para que se haga el cálculo

        case 3: // goto start position - para comprobar
        case 4: // goto end position - para comprobar

        case 5: // test limit switches / set home & end of path - proceso para que se calibre solo (o manualmente)
                // y marque posiciones a cero (home) y máxima (end of path)
                // (para después mostrar posicion en los movimientos)
                
                // ---> otra opción: marcar posición cero en la que se da de inicio y aceptar posiciones negativas?
                //      (o simplemente, número de pasos entre posiciones y dirección de la inicial a la final)

// ---------- CONFIG. BASICA
        case 6: // set driver microsteps 2/4/8/16/32/64/128
        case 7: // set motor steps per revolution - 0.9=400, 1.8=200...
        case 8: // distance per shaft rotation - distancia recorrida por el eje en una rotación completa - en um

         // --- OJO: recalcular 'GEAR' con cada cambio en 6/7/8

        // --------------
        // añadir reset para que te lo pida todo????? 
        // ---> si todo está a cero, debería pedir las cosas básicas (steps/shaft/micro)
        //      y hacer una calibración de posiciones antes de permitir nada más
        // --------------
*/       
//-------------------------------------------------------------------------------

        case START:
                if(rotary_button_state == LOW)
                {
                  start_position=current_position;
                  encoder_pos++; // pasa a la siguiente opción
                  update_screen = true;
                  recalculate_expos();
                }
                if (update_screen) { // the screen hardware is slow so only print to it when something changes
                  lcd.clear();
                  lcd.setCursor(0, 0);
                  lcd.print("Set START pos.");
                  if(start_position_set && end_position_set)
                    lcd.print(" *");
                  lcd.setCursor(0, 1);
                  if(rotary_button_state == LOW)
                  {
                      lcd.print("SET:");
                      lcd.print(start_position);
                      delayMillis(1500);
                      rotary_button_state = HIGH;
                      start_position_set = true;
                  }
                  else
                  {
                      lcd.print("Move motor & set");
                      update_screen = false;
                  }
                }
                break;
        case END:
                if(rotary_button_state == LOW)
                {
                  end_position=current_position;
                  encoder_pos++; // pasa a la siguiente opción
                  update_screen = true;
                  recalculate_expos();
                  set_slice_depth_option=true;
                }
                if (update_screen) { // the screen hardware is slow so only print to it when something changes
                  lcd.clear();
                  lcd.setCursor(0, 0);
                  lcd.print("Set END pos.");
                  if(start_position_set && end_position_set)
                    lcd.print("   *");
                  lcd.setCursor(0, 1);
                  if(rotary_button_state == LOW)
                  {
                      lcd.print("SET:");
                      lcd.print(end_position);
                      delayMillis(1500);
                      rotary_button_state = HIGH;
                      end_position_set = true;
                  }
                  else
                  {
                      lcd.print("Move motor & set");
                      update_screen = false;
                  }
                }
                break;

				case SLICE_DEPTH: // this menu screen changes the distance to move each stacking step
					// if rotary encoder button is toggled within a menu, enabled editing of menu variable with encoder:
          if (set_slice_depth_option==1)
          {
            recalculate_expos();
            set_slice_depth_option=0;
            rotary_button_state = LOW;
          }
					if (rotary_button_state == LOW) {
						//if (mcp.digitalRead(fast_manual) == HIGH) {
  					if(!fast_on) {
  							encoder_update(slice_depth, 1, 10000, 1);
							if (slice_depth > 10000) {slice_depth = 1;}
						} else {
							encoder_update(slice_depth, fast_increment, 10000, fast_increment);
							if (slice_depth > 10000) {slice_depth = fast_increment;}
						}
  					if (slice_depth < 1) {slice_depth = 10000;}
            //recalculate_expos();
					}
          
					if (update_screen) { // the screen hardware is slow so only print to it when something changes
            recalculate_expos();
						lcd.clear();
						lcd.setCursor(0, 0);
						lcd.print("Step distance");
						lcd.setCursor(0, 1);
            if(rotary_button_state == LOW)
              lcd.print(">");
            else
              lcd.print(" ");
						lcd.print(slice_depth);
            lcd.print(" (");
            if(!fast_on) {
              int m=slice_depth/(gear * unit_of_measure_multiplier);
              lcd.print(m);
						  lcd_unit_of_measure();
            }
            else {
              lcd.print(slices);
              lcd.print(" shots");
            }
            lcd.print(")");
						update_screen = false;
					}      
					break;

				case SHOTS: // this menu screen changes the number of pictures to take in the stack
					if (rotary_button_state == LOW) {
						encoder_update(slices, 1, 5000, 1);
						if (slices < 1) {slices = 5000;}
						if (slices > 5000) {slices = 1;}
            recalculate_slice();
					}
					if (update_screen) {
						lcd.clear();
						lcd.setCursor(0, 0);
						lcd.print("Number of shots");
            if(start_position_set && end_position_set)
              lcd.print("*");
						lcd.setCursor(0, 1);
            if(rotary_button_state == LOW)
              lcd.print(">");
            else
              lcd.print(" ");
						lcd.print(slices);
						update_screen = false;
					} 
					break;

				case RETURN: // toggles whether camera/subject is returned the starting position at the end of the stack
					if (rotary_button_state == LOW) {
						encoder_update(return_to_start, 1, 2, 1);
						if (return_to_start == 3) {return_to_start = 1;}
						if (return_to_start == 0) {return_to_start = 2;}
					}
					if (update_screen) {
						lcd.clear();
						lcd.setCursor(0, 0);
						lcd.print("Return to start");
						lcd.setCursor(0, 1);
            if(rotary_button_state == LOW)
              lcd.print(">");
            else
              lcd.print(" ");
						if(return_to_start == 2){
							lcd.print ("Enabled");
						} else {
							lcd.print ("Disabled");
						}
						update_screen = false;
					}
					break; 

        case EXPOSURE: // delay needed for camera to process focus and shutter signals (milliseconds)
          if (rotary_button_state == LOW) {
            encoder_update(exp_time, 25, 50000, 25);
            if (exp_time > 50000){exp_time = 25;}
            if (exp_time < 25){exp_time = 50000;}
          }
          if (update_screen == true){
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Exposure time");
            lcd.setCursor(0, 1);
            if(rotary_button_state == LOW)
              lcd.print(">");
            else
              lcd.print(" ");
            lcd.print (exp_time);
            lcd.print (" ms");
            update_screen = false;
          }      
          break;

        case EXPO_PAUSE: // this menu screen changes the number of seconds to wait for the camera to take a picture before moving again
              // you may want longer if using flashes for adequate recharge time or shorter with continuous lighting
              // to reduce overall time taken to complete the stack
          if (rotary_button_state == LOW) {
            encoder_update(exp_post, 25, 100000, 25);
            if (exp_post < 25) {exp_post = 100000;}
            if (exp_post > 100000) {exp_post = 25;}
          }
          if (update_screen) {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Exp. post delay");
            lcd.setCursor(0, 1);
            if(rotary_button_state == LOW)
              lcd.print(">");
            else
              lcd.print(" ");
            lcd.print ((exp_post / 1000.0), 2); // divide millis by 1000 to display in seconds and 2 decimals
            lcd.print(" seconds");  
            update_screen = false;
          }
          break;

        case MOTOR_PAUSE: // set a delay to allow vibrations from stage movement to settle (milliseconds)
          if (rotary_button_state == LOW) {
            encoder_update(motor_post_delay, 25, 200000, 25);
            if (motor_post_delay > 200000){motor_post_delay = 25;}
            if (motor_post_delay < 25){motor_post_delay = 200000;}
          }
          if (update_screen == true){
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Motor post delay");
            lcd.setCursor(0, 1);
            if(rotary_button_state == LOW)
              lcd.print(">");
            else
              lcd.print(" ");
            lcd.print (motor_post_delay);
            lcd.print (" ms");
            update_screen = false;
          }      
          break;

				case SPEED:	// Adjusts the stepper motor speed (delay in microseconds) between stacking steps,
							// setting this too low (ie a faster motor speed) may cause the motor to begin stalling or failing to move at all
					if (rotary_button_state == LOW) {
						/*encoder_update(stepper_speed, 25, 200000, 25);
						if (stepper_speed > 200000) {stepper_speed = 25;}
						if (stepper_speed < 25) {stepper_speed = 200000;}*/
            int irpm=rpm_stack_speed*10;
            encoder_update(irpm, 1, 5000, (fast_on ? 25:1));
            if (irpm > 5000){irpm = 1;}
            if (irpm < 1){irpm = 5000;}
           rpm_stack_speed=(float)irpm/10.0;
           recalculate_speed();    
					}
					if (update_screen == true) {
						lcd.clear();
						lcd.setCursor(0, 0);
						lcd.print("Stacking speed");
						lcd.setCursor(0, 1);
            if(rotary_button_state == LOW)
              lcd.print(">");
            else
              lcd.print(" ");
						lcd.print (String(rpm_stack_speed, 2));
						lcd.print (" rpm");
						update_screen = false;
					}
					break; 

				case MANUAL_SPEED:	// Adjusts the stepper motor speed (delay in microseconds) in manual mode,
							// slow manual control
					if (rotary_button_state == LOW) {
            int irpm=rpm_manual_speed*10;
            encoder_update(irpm, 1, 5000, (fast_on ? 25:1));
            if (irpm > 5000){irpm = 1;}
            if (irpm < 1){irpm = 5000;}
           rpm_manual_speed=(float)irpm/10;
           recalculate_speed();
					}
					if (update_screen) {
						lcd.clear();
						lcd.setCursor(0, 0);
						lcd.print("Step speed *slow");
						lcd.setCursor(0, 1);
            if(rotary_button_state == LOW)
              lcd.print(">");
            else
              lcd.print(" ");
						lcd.print (String(rpm_manual_speed, 2));
						lcd.print (" rpm");
						update_screen = false;
					}     
					break;

				case MANUAL_HI_SPEED:	// Adjusts the stepper motor speed (delay in microseconds) in fast manual mode,
							// fast manual control
					if (rotary_button_state == LOW) {
            int irpm=rpm_fast_speed*10;
						encoder_update(irpm, 1, 5000, (fast_on ? 25:1));
						if (irpm > 5000){irpm = 1;}
						if (irpm < 1){irpm = 5000;}
           rpm_fast_speed=(float)irpm/10.0;
           recalculate_speed(); 
					}
					if (update_screen) {
						lcd.clear();
						lcd.setCursor(0, 0);
						lcd.print("Step speed *fast");
						lcd.setCursor(0, 1);
            if(rotary_button_state == LOW)
              lcd.print(">");
            else
              lcd.print(" ");
						lcd.print (String(rpm_fast_speed, 2));
						lcd.print (" rpm");
						update_screen = false;
					}   
					break;
          
        case MANUAL_LATENCY: // duracion de la pulsación de los botones alante/atras (para evitar leerlos demasiado seguido)
          if (rotary_button_state == LOW) {
            encoder_update(manual_latency, 1, 1000, 1);
            if (manual_latency > 1000){manual_latency = 1;}
            if (manual_latency < 1){manual_latency = 1000;}
          }
          if (update_screen == true){
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Button latency");
            lcd.setCursor(0, 1);
            if(rotary_button_state == LOW)
              lcd.print(">");
            else
              lcd.print(" ");
            lcd.print (manual_latency);
            lcd.print (" us");
            update_screen = false;
          }  
          break;

        case GOTO_SPEED:  // set the speed for goto/back home/end: stack/manual/fast
          if (rotary_button_state == LOW) {
            encoder_update(goto_speed, 1, 3, 1);
            if (goto_speed > 3) {goto_speed = 1;}
            if (goto_speed < 1) {goto_speed = 3;}
          }
          if (update_screen == true){
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("GoTo speed");
            lcd.setCursor(0, 1);
            if(rotary_button_state == LOW)
              lcd.print(">");
            else
              lcd.print(" ");
            switch (goto_speed){ 
              case 1:
                lcd.print ("Stacking");
                break;
              case 2:
                lcd.print ("Manual (slow)");
                break;
              case 3: 
                lcd.print ("Manual (fast)");
                break;
            }
            update_screen = false;
          }
          break; 

				case MIRROR: // this menu screen toggles mirror lockup mode - if enabled two delay-seperated shutter signals are sent per image
					if (rotary_button_state == LOW) {
						encoder_update(mirror_lockup, 1, 2, 1);
						if (mirror_lockup == 3){mirror_lockup = 1;}
						if (mirror_lockup == 0){mirror_lockup = 2;}
					}
					if (update_screen == true) {
						lcd.clear();
						lcd.setCursor(0, 0);
						lcd.print("Mirror lockup");
						lcd.setCursor(0, 1);
            if(rotary_button_state == LOW)
              lcd.print(">");
            else
              lcd.print(" ");
						if(mirror_lockup == 1) {
							lcd.print ("Disabled");  
						} else {
							lcd.print ("Enabled");  
						}          
						update_screen = false;
					}      
					break;

				case BRACKET: // this menu screen changes the number of images to take per focus slice (ie supports bracketing)
					if (rotary_button_state == LOW) {
						encoder_update(bracket, 1, 10, 1);
						if (bracket > 10){bracket = 1;}
						if (bracket < 1){bracket = 10;}
					}
					if (update_screen == true){
						lcd.clear();
						lcd.setCursor(0, 0);
						lcd.print("Bracketing");
						lcd.setCursor(0, 1);
            if(rotary_button_state == LOW)
              lcd.print(">");
            else
              lcd.print(" ");
						lcd.print(bracket);
						update_screen = false;
					}      
					break;

        case UNIT:  // set the unit of measure to use for stacking steps: Microns, Millimimeters or Centimeteres
          if (rotary_button_state == LOW) {
            encoder_update(unit_of_measure, 1, 3, 1);
            if (unit_of_measure > 3) {unit_of_measure = 1;}
            if (unit_of_measure < 1) {unit_of_measure = 3;}
          }
          if (update_screen == true){
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Unit of measure");
            lcd.setCursor(0, 1);
            if(rotary_button_state == LOW)
              lcd.print(">");
            else
              lcd.print(" ");
            switch (unit_of_measure){ 
              case 1:
                lcd.print ("Microns (um)");
                unit_of_measure_multiplier = 1;
                break;
              case 2:
                lcd.print ("Millimeters (mm)");
                unit_of_measure_multiplier = 1000;
                break;
              case 3: 
                lcd.print ("Centimeters (cm)");
                unit_of_measure_multiplier = 10000;
                break;
            }
            update_screen = false;
          }
          break; 

//----------------


        case DRIVER_MODEL:  //
          if (rotary_button_state == LOW) {
            encoder_update(drv_model, 1, 3, 1);
            if (drv_model > 3) {drv_model = 1;}
            if (drv_model < 1) {drv_model = 3;}
          }
          if (update_screen == true){
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Driver model");
            lcd.setCursor(0, 1);
            if(rotary_button_state == LOW)
              lcd.print(">");
            else
              lcd.print(" ");
            switch (drv_model){ 
              case 1:
                lcd.print ("DRV8825");
                break;
              case 2:
                lcd.print ("A4988");
                break;
              case 3: 
                lcd.print ("LV8729");
                break;
            }
            update_screen = false;
          }
          break; 

        case DRIVER_DISABLE:  //
          if (rotary_button_state == LOW) {
            int dis=drv_disable;
            encoder_update(drv_disable, 1, 3, 1);
            if (drv_disable > 3) {drv_disable = 1;}
            if (drv_disable < 1) {drv_disable = 3;}
            if(dis!=drv_disable)
              sync_driver_disable();
          }
          if (update_screen == true){
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Driver disable");
            lcd.setCursor(0, 1);
            if(rotary_button_state == LOW)
              lcd.print(">");
            else
              lcd.print(" ");
            switch (drv_disable){ 
              case 1:
                lcd.print ("Never!");
                break;
              case 2:
                lcd.print ("Only manual m.");
                break;
              case 3: 
                lcd.print ("When possible");
                break;
            }
            update_screen = false;
          }
          break; 

        case MOTOR_STEPS:
          if (rotary_button_state == LOW) {
              //if (mcp.digitalRead(fast_manual) == HIGH) {
              if(!fast_on) {
                encoder_update(motor_steps, 64, 100000, 1);
              } else {
                encoder_update(motor_steps, 64, 100000, fast_increment);
              }
              if (motor_steps > 100000){motor_steps = 64;}
              if (motor_steps < 64){motor_steps = 100000;}
              recalculate_gear();
          }
          if (update_screen == true){
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Motor steps/rev.");
            lcd.setCursor(0, 1);
            if(rotary_button_state == LOW)
              lcd.print(">");
            else
              lcd.print(" ");
            lcd.print(motor_steps);
            update_screen = false;
          }   
          break;

        case DRIVER_MICROSTEPS:
          if (rotary_button_state == LOW) {
            encoder_update(driver_microsteps, 1, 256, 2, true);
            if (driver_microsteps > 512){driver_microsteps = 1;}
            if (driver_microsteps < 1){driver_microsteps = 512;}
            recalculate_gear();
          }
          if (update_screen == true){
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Driver uSteps");
            lcd.setCursor(0, 1);
            if(rotary_button_state == LOW)
              lcd.print(">");
            else
              lcd.print(" ");
            lcd.print(driver_microsteps);
            update_screen = false;
          }   
          break;

        case SHAFT_REVOLUTION:
          if (rotary_button_state == LOW) {
            //if (mcp.digitalRead(fast_manual) == HIGH) {
            if(!fast_on) {
              encoder_update(shaft_revolution, 250, 10000, 1);
            } else {
               encoder_update(shaft_revolution, 250, 10000, fast_increment);
            }
            if (shaft_revolution > 100000){shaft_revolution = 50;}
            if (shaft_revolution < 50){shaft_revolution = 100000;}             
            recalculate_gear();
          }
          if (update_screen == true){
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Shaft revolution");
            lcd.setCursor(0, 1);
            if(rotary_button_state == LOW)
              lcd.print(">");
            else
              lcd.print(" ");
            lcd.print(shaft_revolution);
            lcd.print (" um");
            update_screen = false;
          }   
          break;

        case MANUAL_STEP_BUNCH:
          if (rotary_button_state == LOW) {
            encoder_update(manual_min_steps, 1, 512, 2, true);
            if (manual_min_steps > 512){manual_min_steps = 1;}
            if (manual_min_steps == 0){manual_min_steps = 512;}
          }
          if (update_screen == true){
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Min. bunch of steps");
            lcd.setCursor(0, 1);
            if(rotary_button_state == LOW)
              lcd.print(">");
            else
              lcd.print(" ");
            lcd.print(manual_min_steps);
            update_screen = false;
          }   
          break;


        case BACKSLASH_SLICE:
          if (rotary_button_state == LOW) {
              if(!fast_on) {
                encoder_update(backslash_slice, 10, 10000, 1);
              } else {
                encoder_update(backslash_slice, 10, 10000, fast_increment);
              }
            if (backslash_slice > 10000){backslash_slice = 10;}
            if (backslash_slice < 10){backslash_slice = 10000;}
          }
          if (update_screen == true){
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Backslash steps");
            lcd.setCursor(0, 1);
            if(rotary_button_state == LOW)
              lcd.print(">");
            else
              lcd.print(" ");
            lcd.print(backslash_slice);
            update_screen = false;
          }   
          break;
/*
        case BACKSLASH_ADJUST:
          if (rotary_button_state == LOW) {
              if(!fast_on) {
                encoder_update(backslash_adjust, 0, 1000, 1);
              } else {
                encoder_update(backslash_adjust, 0, 1000, fast_increment);
              }
            if (backslash_adjust > 1000){backslash_adjust = 0;}
            if (backslash_adjust < 0){backslash_adjust = 1000;}
          }
          if (update_screen == true){
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Backslash adjust");
            lcd.setCursor(0, 1);
            if(rotary_button_state == LOW)
              lcd.print(">");
            else
              lcd.print(" ");
            lcd.print(backslash_adjust);
            update_screen = false;
          }   
          break;
*/
        case FAST_INCREMENT:
          if (rotary_button_state == LOW) {
            encoder_update(fast_increment, 10, 100, 1);
            if (fast_increment > 100){fast_increment = 10;}
            if (fast_increment < 10){fast_increment = 100;}
          }
          if (update_screen == true){
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("FAST increment");
            lcd.setCursor(0, 1);
            if(rotary_button_state == LOW)
              lcd.print(">");
            else
              lcd.print(" ");
            lcd.print(fast_increment);
            update_screen = false;
          }   
          break;

//----------------

				case CONFIG_SAVE: // select and press Confirm to save current settings in EEPROM
					if (rotary_button_state == LOW) {
						encoder_update(save_settings, 1, 2, 1);
						if (save_settings == 3) {save_settings = 1;}
						if (save_settings == 0) {save_settings = 2;}
					}
					if (update_screen) {
						lcd.clear();
						lcd.setCursor(0, 0);
						lcd.print("Save settings");
						lcd.setCursor(0, 1);
            if(rotary_button_state == LOW)
              lcd.print(">");
            else
              lcd.print(" ");
						if(save_settings==1){
							lcd.print ("confirm!");
						} else {
							lcd.print ("no");
						}
						update_screen = false;
					}
					break; 

				case DEFAULTS: // select and press Confirm to load defaults settings
					if (rotary_button_state == LOW) {
						encoder_update(load_defaults, 1, 2, 1);
						if (load_defaults == 3) {load_defaults = 1;}
						if (load_defaults == 0) {load_defaults = 2;}
					}
					if (update_screen) {
						lcd.clear();
						lcd.setCursor(0, 0);
						lcd.print("Load defaults");
						lcd.setCursor(0, 1);
            if(rotary_button_state == LOW)
              lcd.print(">");
            else
              lcd.print(" ");
						if(load_defaults==1){
							lcd.print ("confirm!");
						} else {
							lcd.print ("no");
						}
						update_screen = false;
					}
					break; 

				case CONFIG_LOAD: // select and press Confirm to load last saved settings from EEPROM
					if (rotary_button_state == LOW) {
						encoder_update(load_lastSaved, 1, 2, 1);
						if (load_lastSaved == 3) {load_lastSaved = 1;}
						if (load_lastSaved == 0) {load_lastSaved = 2;}
					}
					if (update_screen) {
						lcd.clear();
						lcd.setCursor(0, 0);
						lcd.print("Load last saved");
						lcd.setCursor(0, 1);
            if(rotary_button_state == LOW)
              lcd.print(">");
            else
              lcd.print(" ");
						if(load_lastSaved==1){
							lcd.print ("confirm!");
						} else {
							lcd.print ("no");
						}
						update_screen = false;
					}
					break; 

				case CAMERA_SHOT: // select and press Confirm to shoot a test shot
					if (rotary_button_state == LOW) {
						encoder_update(test_shot, 1, 2, 1);
						if (test_shot == 3) {test_shot = 1;}
						if (test_shot == 0) {test_shot = 2;}
					}
					if (update_screen) {
						lcd.clear();
						lcd.setCursor(0, 0);
						lcd.print("Camera test shot");
						lcd.setCursor(0, 1);
            if(rotary_button_state == LOW)
              lcd.print(">");
            else
              lcd.print(" ");
						if(test_shot==1){
							lcd.print ("confirm!");
						} else {
							lcd.print ("no");
						}
						update_screen = false;
					}
					break; 

				case SHOW_STEPS: // 
					if (update_screen) {
						lcd.clear();
						lcd.setCursor(0, 0);
						lcd.print("uSteps & gear");
						lcd.setCursor(0, 1);
            lcd.print(" ");
						lcd.print (microsteps);
            lcd.print(" ");
            lcd.print (gear);
						update_screen = false;
					}
					break; 

        case READ_BUTTONS:
          if (rotary_button_state == LOW) {
            encoder_update(read_manual_buttons, 0, 1, 1);
            if (read_manual_buttons > 1) {read_manual_buttons = 0;}
            if (read_manual_buttons < 0) {read_manual_buttons = 1;}
          }
#ifdef disableJoystick
          read_manual_buttons=1;
#endif
          if (update_screen == true){
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Manual source");
            lcd.setCursor(0, 1);
            if(rotary_button_state == LOW)
              lcd.print(">");
            else
              lcd.print(" ");
            switch (read_manual_buttons){ 
              case 1:
                lcd.print ("Buttons");
                read_manual_buttons = 1;
                break;
              case 0:
                lcd.print ("JoyStick");
                read_manual_buttons = 0;
                break;
            }
            update_screen = false;
          }
          break;
          
        case GO_FWD:
            loop_timestamp=0;
            if (rotary_button_state == LOW) {
              goto_position(current_position+slice_depth,false);
              delayMillis(rbdebounce/2);
              rotary_button_state = HIGH;
            }
            if (update_screen) {
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("GoFWD ");
              lcd.print(String(slice_depth));
              lcd.print(" steps");
              update_screen = false;
            }  
          break;

        case GO_BCK:
            loop_timestamp=0;
            if (rotary_button_state == LOW) {
              goto_position(current_position-slice_depth,false);
              delayMillis(rbdebounce/2);
              rotary_button_state = HIGH;
            }
            if (update_screen) {
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("GoBCK ");
              lcd.print(String(slice_depth));
              lcd.print(" steps");
              update_screen = false;
            }          
          break;

        case GOTO_START:
            loop_timestamp=0;
            if (rotary_button_state == LOW) {
              encoder_update(goto_start, 1, 2, 1);
              if (goto_start == 3) {goto_start = 1;}
              if (goto_start == 0) {goto_start = 2;}
            }
            if (update_screen) {
              lcd.clear();
              lcd.setCursor(0, 0);
              if(start_position_set)
                lcd.print("GoTo START pos.");
              else
                lcd.print("[GoTo START pos]");
              update_screen = false;
            }
          break; 

        case GOTO_END:
            loop_timestamp=0;
            if (rotary_button_state == LOW) {
              encoder_update(goto_end, 1, 2, 1);
              if (goto_end == 3) {goto_end = 1;}
              if (goto_end == 0) {goto_end = 2;}
            }
            if (update_screen) {
              lcd.clear();
              lcd.setCursor(0, 0);
              if(end_position_set)
                lcd.print("GoTo END pos.");
              else
                lcd.print("[GoTo END pos.]");
              update_screen = false;
            }
          break; 

        case RAMP_START:  // 
          if (rotary_button_state == LOW) {
            int irpm=rpm_ramp_start*10;
            encoder_update(irpm, 1, 5000, (fast_on ? 25:1));
            if (irpm > 5000){irpm = 1;}
            if (irpm < 1){irpm = 5000;}
           rpm_ramp_start=(float)irpm/10.0;
           recalculate_speed();  
          }
          if (update_screen) {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Ramp start");
            lcd.setCursor(0, 1);
            if(rotary_button_state == LOW)
              lcd.print(">");
            else
              lcd.print(" ");
            lcd.print (String(rpm_ramp_start,2));
            lcd.print (" rpm");
            update_screen = false;
          }
          break;

			} // end of switch statement
      
}
