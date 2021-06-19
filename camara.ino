/*////////////////////////////////////////////////////////////////////////////////////////////////////////
//  STACK SEQUENCE FUNCTIONS                                                                                    //
////////////////////////////////////////////////////////////////////////////////////////////////////////*/
void stack_sequence()
{
        loop_timestamp=0;
        was_fast_on = fast_on;
        if(fast_on)
          fast_toggle();
        lcd.clear();
        lcd.print("Preparing seq.");
        delayMillis(500);
    if(start_position_set)
    {
Serial.println("START POS is: "+String(start_position));
Serial.println("END POS is: "+String(end_position));
Serial.println("GoTo START");
      lcd.clear();
      lcd.print("moving to START pos");
      goto_position(start_position, true); // ir a START compensando backslash si hace falta
Serial.println("current POS: "+String(current_position));
      if(forward != forward_stack)
      {
Serial.println("Reversing direction");
        stepper_direction(false);
      }
    }
    else
    {
      goto_position(current_position,true);
    }
    stepper_disable();
    
    /* THE FOCUS STACK */
    // loop the following actions for number of times dictated by var slices:
    if(!disable_on_stacking)
      stepper_enable();
    for (int i = 0; i < slices; i++){
      // stop motor and reverse if limit switch hit:
      if (mcp.digitalRead(limit_switches) == LOW) {
        stepper_retreat();
        break;
      }    
      if (slice_counter == 0) {
        lcd.clear();
        lcd.print("Start sequence");
        stacking_now=true;
        delayMillis(800);
        mcp.digitalWrite(stacking, HIGH);
        delayMillis(mcp_latency);
Serial.println("stacking:HIGH!");
      }
      // count of pictures taken so far:
      slice_counter++;
      // send signal to camera to take picture(s):
      camera_process_images();
      // if the Start/Stop stack button has been pressed, stop the stack even if not complete,
      // even if pressed during the previous camera_process_images function:
      if (main_button_state == HIGH) {
        lcd.clear();
        lcd.print("Sequence stopped");
        lcd.setCursor(0, 1);
        lcd.print("by user");
        stacking_now=false;
        mcp.digitalWrite(stacking, LOW);
Serial.println("stacking:LOW!");
        delayMillis(2000);
        break;
      }
      // stop the sequence just after the last camera shot:
      if (slices == i + 1) {
        break;
      }
      // prepare to move the carriage to the next step:
      lcd.clear();
      lcd.print("Moving ");
      lcd.print (slice_depth);
      //lcd_unit_of_measure();
      lcd.print(" steps");
      lcd.setCursor(0, 1);
      lcd.print("[");
      lcd.print (slice_counter + 1);
      lcd.print (" of ");
      lcd.print (slices);
      lcd.print("]");
      delayMillis(100);
      if(disable_on_stacking)
        stepper_enable();
      // set the stepper direction for forward travel: --- lo quitamos si hay START/END
      if(!start_position_set)
      {
        //mcp.digitalWrite(set_direction, HIGH);
        if(!forward)
          stepper_direction(false);
      }
      delayMillis(100);
      // travel the step distance based on your microstepping and gear ratio setup:
      //  original -->    for (int n = 0; n < slice_depth * gear * unit_of_measure_multiplier; n++) {
      for (int n = 0; n < microsteps; n++) {
        stepper_step();
        if(mcp.digitalRead(limit_switches) == LOW) {
          break;
        }
      }
      if(disable_on_stacking)
        stepper_disable();
      if (main_button_state == LOW) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print ((motor_post_delay / 1000.0), 2);
        lcd.print("s motor delay");
        lcd.setCursor(0, 1);
        lcd.print ("[");
        lcd.print (slice_counter + 1);
        lcd.print (" of ");
        lcd.print (slices);
        lcd.print ("]");
        delayMillis(motor_post_delay);
      }
Serial.println("["+String(slice_counter+1)+"]current POS: "+String(current_position));
    }

    if (main_button_state == LOW) {
        lcd.clear(); 
        lcd.setCursor(0, 0);
        stacking_now=false;
        mcp.digitalWrite(stacking, LOW);
Serial.println("stacking:LOW!");
        lcd.print("Stack finished");
        lcd.setCursor(0, 1);
        lcd.print(slice_counter);
        lcd.print(" slices done");
        delayMillis(2000);
    }
    if(!disable_on_stacking)
      stepper_disable();
Serial.println("end of stack - current POS: "+String(current_position));
Serial.println(String(slice_depth * (slice_counter -1))+" steps done");
    lcd.clear(); 
    /* IF ENABLED, AT END OF STACK RETURN TO START POSITION */
    if (return_to_start == 2) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Back to START..."); 
Serial.println("Back to start...");
      if(start_position_set)
        goto_position(start_position,true);
      else
        goto_position(current_position-(slice_depth * (slice_counter -1)),true);
      lcd.clear();
    }
Serial.println("current POS: "+String(current_position));
    /* RESET VARIABLES TO DEFAULTS BEFORE RETURNING TO MENU AND MANUAL CONTROL */  
    encoder_pos = 0;        // set menu option display to first
    update_screen = true;   // allow the first menu item to be printed to the screen
    slice_counter = 0;      // reset pictures counter
    main_button_state = HIGH; // return to menu options
      if(was_fast_on)
      {
        delayMillis(rbdebounce);
        fast_toggle();
      }
}


/*////////////////////////////////////////////////////////////////////////////////////////////////////////
//  CAMERA FUNCTIONS                                                                                    //
////////////////////////////////////////////////////////////////////////////////////////////////////////*/

/* SEND SIGNAL TO CAMERA TO TAKE PICTURE WITH DELAYS TO ALLOW SETTLING -------------------------------- */
void camera_process_images() {
  for (int i = 1; i <= bracket; i++) {
    lcd.clear();
    // if the Start/Stop stack button has been pressed, stop the stack even if not complete:
    if (main_button_state == HIGH) {
      break;
    }
    // if bracketing is enabled, print the current position in the bracket:
    if(bracket > 1) {
      lcd.print("Bracketed image:");
      lcd.setCursor(0, 1);
      lcd.print(i);
      lcd.print(" of ");
      lcd.print(bracket);
      delayMillis(bracket_delay);
      lcd.clear();
    }
    // send a shutter signal:
    camera_shutter_signal();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print ((exp_post / 1000.0), 2);
    lcd.print("s delay ");
    if(stacking_now)
    {
      lcd.setCursor(0, 1);
      lcd.print ("[");
      lcd.print (slice_counter);
      lcd.print (" of ");
      lcd.print (slices);
      lcd.print ("]");
    }
    // pause to allow for camera to take picture and to allow flashes to recharge before next shot:
    delayMillis(exp_post);
    lcd.clear();
  }
}

void testShot() {
  mcp.digitalWrite(photo_led, HIGH);
  lcd.clear();
  lcd.print("Photo!");
  // trigger camera autofocus - camera may not take picture in some modes if this is not triggered first:
  mcp.digitalWrite(focus, HIGH);
Serial.println("focus:HIGH!");
  // trigger camera shutter:
  mcp.digitalWrite(shutter, HIGH);
Serial.println("shutter:HIGH!");
  // delay needed for camera to process above signals:
  delayMillis(exp_time); 
  // switch off camera trigger signal:
  mcp.digitalWrite(shutter, LOW);
Serial.println("shutter:LOW!");
  // switch off camera focus signal:
  mcp.digitalWrite(focus, LOW);
Serial.println("focus:LOW!");
  update_screen = true;
  delayMillis(exp_post);
  lcd.clear();
  mcp.digitalWrite(photo_led, LOW);
  delayMillis(mcp_latency);
}

/* SEND SHUTTER SIGNAL --------------------------------------------------------------------------------- */ 
void camera_shutter_signal() {
  mcp.digitalWrite(photo_led, HIGH);
  delayMillis(mcp_latency);
  for (int i = 0; i < mirror_lockup; i++) {
    if(mirror_lockup == 2 && i == 0) {
      lcd.clear();
      lcd.print("Mirror up");
    }
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Taking photo...");
    if(stacking_now)
    {
      lcd.setCursor(0, 1);
      lcd.print ("[");
      lcd.print (slice_counter);
      lcd.print (" of ");
      lcd.print (slices);
      lcd.print ("]");
    }
    // trigger camera autofocus - camera may not take picture in some modes if this is not triggered first:
    mcp.digitalWrite(focus, HIGH);
    delayMillis(mcp_latency);
Serial.println("focus:HIGH!");
    // trigger camera shutter:
    mcp.digitalWrite(shutter, HIGH);
    delayMillis(mcp_latency);
Serial.println("shutter:HIGH!");
    // delay needed for camera to process above signals:
    delayMillis(exp_time); 
    // switch off camera trigger signal:
    mcp.digitalWrite(shutter, LOW);
    delayMillis(mcp_latency);
Serial.println("shutter:LOW!");
    // switch off camera focus signal:
    mcp.digitalWrite(focus, LOW);
    delayMillis(mcp_latency);
Serial.println("focus:LOW!");
    if(mirror_lockup == 2 && i == 0) {
      // delay between mirror up and shutter:
      delayMillis(mirror_delay);
      lcd.clear();
    }
  }
  mcp.digitalWrite(photo_led, LOW);
  delayMillis(mcp_latency);
}

void photo_control()
{
  //if(mcp.digitalRead(photo_button) == LOW)
  if(mcp_pin_read(photo_button) == LOW)
  {
    loop_timestamp=0;
    if(fast_on)
      fast_toggle();
    testShot();
  }
}
