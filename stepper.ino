void limit_abort()
{
    if(fast_on)
      fast_toggle();
    stepper_disable();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Limit switch ON!");
    lcd.setCursor(0, 1); 
    // provide manual motor control to move away from limit switch and resolve error:
    stepper_control();
    update_screen = true;
}

// comprobacion de GOTOs
void check_gotos()
{
    if(goto_end==1)
  {
    loop_timestamp=0;
    was_fast_on = fast_on;
    if(fast_on)
      fast_toggle();
    if(end_position_set)
    {
      if(current_position==end_position)
      {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Already there!");
        delayMillis(1000);
        update_screen = true;  
      }
      else
      {
Serial.println("GoTo END POS: "+String(end_position));
        goto_position(end_position,false);
      }
    }
    rotary_button_state = HIGH;
    goto_end=0;
Serial.println("current POS: "+String(current_position));
      if(was_fast_on)
      {
        delayMillis(rbdebounce);
        fast_toggle();
      }
  }
  if(goto_start==1)
  {
    loop_timestamp=0;
    was_fast_on = fast_on;
    if(fast_on)
      fast_toggle();
    if(start_position_set)
    {
Serial.println("GoTo START POS: "+String(start_position));
      goto_position(start_position,true);
    }
    rotary_button_state = HIGH;
    goto_start=0;
Serial.println("current POS: "+String(current_position));
      if(was_fast_on)
      {
        delayMillis(rbdebounce);
        fast_toggle();
      }
  }

}



/* REVERSE THE CURRENT STEPPER DIRECTION -------------------------------------------------------------- */
void stepper_direction(bool do_backslash_adjust=false) {
  if(forward) {
    mcp.digitalWrite(set_direction, LOW);
    forward = false;
  } else {
    mcp.digitalWrite(set_direction, HIGH);
    forward = true;
  }
  if(do_backslash_adjust && backslash_adjust > 0)
  {
//Serial.println("adjusting current position by "+String(backslash_adjust)+" steps");
    if(forward)
      current_position-=backslash_adjust;
    else
      current_position+=backslash_adjust;
  }
  delayMillis(mcp_latency/2);
  do_yield();
//Serial.println("forward ["+String(forward)+"]");
//Serial.println("forward_stack ["+String(forward_stack)+"]");
}

void set_stepper_direction(bool pin_state, bool do_backslash_adjust=false)
{
  delayMillis(mcp_latency); // si no se hace, se pueden perder pasos
  // sin cambio de sentido, se aplica para confirmar
  if(forward && pin_state==HIGH)
  {
    if(force_dir_write)
    {
      mcp.digitalWrite(set_direction, HIGH);
      delayMillis(mcp_latency/2);
      do_yield();
    }
    return;
  }
  if(!forward && pin_state==LOW)
  {
    if(force_dir_write)
    {
      mcp.digitalWrite(set_direction, LOW);
      delayMillis(mcp_latency/2);
      do_yield();
    }
    return;
  }
  // cambio de sentido
  return stepper_direction(do_backslash_adjust);
}

// sincroniza las variables de enable/disable driver *disable_on_stacking* y *disable_easydriver*
void sync_driver_disable()
{
   switch (drv_disable)
   { 
     case 1:
            disable_easydriver = false;
            disable_on_stacking = false;
            break;
     case 2:
            disable_easydriver = true;
            disable_on_stacking = false;
            break;
     case 3: 
            disable_easydriver = true;
            disable_on_stacking = true;
            break;
     default:
            drv_disable = 2;
            disable_easydriver = true;
            disable_on_stacking = false;
   }
//Serial.println("disable_easyDrv["+String(disable_easydriver)+"]");
//Serial.println("disable_on_stacking["+String(disable_on_stacking)+"]");
}

void stepper_step_manual(boolean add_ramp=false) {
//Serial.println("step_timeslice["+String(step_timeslice)+"]");
//Serial.println("loop_timeslice["+String(loop_timeslice)+"]");
  //---------------
  if(step_timeslice==0)
  {
    loop_timeslice=0;
  }
  else
  {
    if(loop_timeslice > step_timeslice)
      loop_timeslice-=step_timeslice;
  }
  //---------------
  float stepper_delay_manual=(fast_on ? (float)fast_manual_speed-(float)loop_timeslice:(float)stepper_speed_manual-(float)loop_timeslice);
//Serial.println("stepper_delay_manual["+String(stepper_delay_manual)+"]");
  if(add_ramp)
  {
    stepper_delay_manual+=ramp_value;
//Serial.println("--->ramp_value["+String(ramp_value)+"]");
//Serial.println("--->stepper_delay_manual["+String(stepper_delay_manual)+"]");
  }
  step_timeslice=stepper_delay_manual;
  return stepper_step_sp(stepper_delay_manual);

}

void goto_position(int pos, bool compensate_backlash=false) // este último parámetro es para la START pos...
{
  loop_timestamp = loop_timeslice = step_timeslice =0;
  bool do_compensation=false;
  int distance=pos-current_position;
  delayMillis(rbdebounce/2+1);
Serial.println("GoTo target position ["+String(pos)+"]");
Serial.println("current position ["+String(current_position)+"]");
Serial.println("distance ["+String(distance)+"]");
  if(compensate_backlash)
  {
    if(forward_stack)
      distance=(pos-backslash_slice)-current_position;
    else
      distance=(pos+backslash_slice)-current_position;
//Serial.println("adding "+String(backslash_slice)+" steps for backslash compensation");
    do_compensation=true;
  }

  if(distance==0)
    return;
  if(fast_on) // desactivamos FAST por si acaso
  {
    delayMillis(rbdebounce/2+1);
    fast_toggle();
  }
  stepper_enable();
  // set the stepper direction
  if(distance > 0)
  {
    set_stepper_direction(HIGH);
  }
  else
  {
    set_stepper_direction(LOW);
  }
  delayMillis(100);
  distance=abs(distance);
  boolean did_fast=false;
  boolean limited=false;
  if(goto_speed == 3) // activamos FAST solamente si es el caso
  {
    delayMillis(rbdebounce/2+1);
    fast_toggle();
  }
  float ramp_count=0;
  for (int n = 0; n < distance; n++) 
  {
    ramp_update(ramp_count);
    if(goto_speed > 1)
              stepper_step_manual(true);
    else
              stepper_step();
    do_yield();
    ramp_count+=ramp_increment;
    if (mcp_pin_read(limit_switches) == LOW) {
        limited=true;
        stepper_retreat();
        break;
    }
  }
  stepper_disable();

  if(do_compensation && !limited) // si hemos hecho una compensación, volvemos a llamarnos de forma recursiva para ir a la posición correcta
    goto_position(pos,false);

  if(fast_on) // desactivamos FAST por si acaso
  {
    delayMillis(rbdebounce/2+1);
    fast_toggle();
  }
}

void ramp_update(float count)
{
//Serial.println("ramp_count["+String(count)+"]");
  float sp=(fast_on ? (float)fast_manual_speed:(float)stepper_speed_manual);
//Serial.println("sp["+String(sp)+"]");
//Serial.println("ramp_start["+String(ramp_start)+"]");
  if(ramp_start==0 || ramp_start < sp) // disabled?
  {
    ramp_value=0;
  }
  else   // la rampa se debería ir decrementando, ya que se suma a la velocidad y cuando llega al final ha de ser cero.
  {
    ramp_value=(float)ramp_start-sp-count;
    if(ramp_value < 0)
      ramp_value=0;
  }
//Serial.println("ramp_value["+String(ramp_value)+"]");
}

void stepper_control() {
  float ramp_count=0;
  bool stepper_enabled=false;
  bool overclocked=false;
  bool forwarded=false;
  long micros_now=0;
  if(do_overclock)
    system_update_cpu_freq(80); // por si acaso

  ramp_count=0; // reset ramp counter
  loop_timestamp=loop_timeslice=0;
  while (get_forward_control() == LOW) {
    micros_now = micros();
    if(loop_timestamp > 0 && loop_timestamp < micros_now)
    {
      loop_timeslice = micros_now - loop_timestamp;
    }
    loop_timestamp = micros_now;
    ramp_update(ramp_count);
//Serial.println("forward_control:LOW");
    if(!stepper_enabled)
    {
      stepper_enable();
      stepper_enabled=true;
    }
    if(!forwarded || force_dir_write)
    {
      set_stepper_direction(HIGH, true);
      forwarded = true;
      do_yield();
    }
    if(!overclocked && fast_on)
    {
      if(do_overclock)
        system_update_cpu_freq(160);
      overclocked=true;
    }
    if(overclocked && !fast_on)
    {
      if(do_overclock)
        system_update_cpu_freq(80);
      overclocked=false;
    }
    for (int i = 0; i< manual_min_steps; i++) {
      // move forward:
      stepper_step_manual(true);
      // stop motor and reverse if limit switch hit:
      if (mcp_pin_read(limit_switches) == LOW) {
        stepper_retreat();
        break;
      }
      // boton FAST dentro del bucle
      if (mcp_pin_read(fast_manual) == LOW)
        fast_toggle();
    }
    ramp_count+=ramp_increment;
  }
  forwarded=false;
  ramp_count=0; // reset ramp counter
  loop_timestamp=loop_timeslice=0;
  while (get_backward_control() == LOW) {
//Serial.println("backward_control:LOW");
    micros_now = micros();
    if(loop_timestamp > 0 && loop_timestamp < micros_now)
    {
      loop_timeslice = micros_now - loop_timestamp;
    }
    loop_timestamp = micros_now;
    ramp_update(ramp_count);
    if(!stepper_enabled)
    {
      stepper_enable();
      stepper_enabled=true;
    }
    if(!forwarded || force_dir_write)
    {
      set_stepper_direction(LOW, true);
      forwarded = true;
      do_yield();
    }
    if(!overclocked && fast_on)
    {
      if(do_overclock)
        system_update_cpu_freq(160);
      overclocked=true;
    }
    if(overclocked && !fast_on)
    {
      if(do_overclock)
        system_update_cpu_freq(80);
      overclocked=false;
    } 
    for (int i = 0; i< manual_min_steps; i++) {
      // move backward:
      stepper_step_manual(true);
      // stop motor and reverse if limit switch hit:
      if (mcp_pin_read(limit_switches) == LOW) {
        stepper_retreat();
        break;
      }
      //fast
      if (mcp_pin_read(fast_manual) == LOW)
        fast_toggle(); 
    }
    ramp_count+=ramp_increment;
  }
  if(overclocked)
  {
    if(do_overclock)
      system_update_cpu_freq(80);
  }
  stepper_disable();
}



/* DISABLE THE EASYDRIVER WHEN NOT IN USE IF OPTION SET ----------------------------------------------- */
void stepper_disable() {
  step_timeslice=0;
  if(stepper_disabled)
    return;
//Serial.println("disable_easyDrv["+String(disable_easydriver)+"]");
  if(disable_easydriver == true) {
    mcp.digitalWrite(enable, HIGH);
  } else {
    return;
    //mcp.digitalWrite(enable, LOW); // aqui no deberia llegar
//Serial.println("stepper ??????");    
  }
  stepper_disabled=true;
Serial.println("stepper DISABLE");
  delayMillis(mcp_latency);
}

/* ENABLE THE EASYDRIVER ------------------------------------------------------------------------------ */
void stepper_enable() {
  step_timeslice=0;
  if(!stepper_disabled)
    return;
  mcp.digitalWrite(enable, LOW);
  stepper_disabled=false;
Serial.println("stepper ENABLE");
  delayMillis(mcp_latency);
}


void pasito() {
      digitalWrite(do_step,LOW); 
      //do_yield();
      digitalWrite(do_step,HIGH);
}

/* SEND A STEP SIGNAL TO EASYDRIVER TO TURN MOTOR IN STACKING MODE ------------------------------------- */
void stepper_step_sp(float stepper_delay)
{
//Serial.println("stepper_delay["+String(stepper_delay)+"]");
  pasito();
  // delay time between steps, too fast and motor stalls:
  delayMicros(stepper_delay); 
  if(forward)
      current_position++;
  else
      current_position--;
  //do_yield();
}


void stepper_step() {
  return stepper_step_sp(stepper_speed);
}


/* PULL CARRIAGE BACK FROM TRIPPED LIMIT SWITCH ------------------------------------------------------- */
void stepper_retreat() {
  if(fast_on)
    fast_toggle();
  stepper_enable();
  // switch the motor direction:
  stepper_direction();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("End of travel!");
  lcd.setCursor(0, 1);
  mcp.digitalWrite(stacking, LOW);
  stacking_now=false;
  delayMillis(mcp_latency);
Serial.println("stacking:LOW!");
  lcd.print("Reversing ...");
  // iterate do_step signal for as long as eitherlimit switch remains pressed:
  while (mcp.digitalRead(limit_switches) == LOW) {  
    stepper_step_manual(false);
    if(forward)
      current_position+manual_min_steps;
    else
      current_position-manual_min_steps;
    //delayMillis(mcp_latency);
  }
  // reset motor back to original direction once limit switch is no longer pressed:
  stepper_direction();
  lcd.clear();
  stepper_disable();
  encoder_pos = 1;        //set menu option display to first
  update_screen = true;   //allow the first menu item to be printed to the screen
  slice_counter = 0;      //reset pic counter
  main_button_state = HIGH; //return to menu options
}
