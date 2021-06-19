void fast_toggle()
{
//  if(fast_time > 0 && millis() - fast_time < mcp_latency*2)
//    return;

  if(fast_on)
  {
    fast_on=false;
    mcp.digitalWrite(fast_led, LOW);
    Serial.println("FAST OFF");
    if(do_overclock)
      system_update_cpu_freq(80); // por si acaso
  }
  else
  {
    fast_on=true;
    mcp.digitalWrite(fast_led, HIGH);
    Serial.println("FAST ON");
  }
  delayMillis(mcp_latency);
  do_yield();
  fast_time = millis();
}

/* CHANGE THE ACTIVE MENU VARIABLE'S VALUE USING THE ENCODER ------------------------------------------ */

void encoder_update(int &variable, int lower, int upper, int multiplier = 1, bool mult=false){ /* CHANGE THE ACTIVE MENU VARIABLE'S VALUE USING THE ENCODER */
  
  variable = constrain(variable, lower, upper); //keep variable value within specified range  
  
  int8_t encoder_data = encoder_read(); //counts encoder pulses and registers only every nth pulse to adjust feedback sensitivity
  
  if(encoder_data){
    if(encoder_data == 1){
      if(mult)
        variable = variable * multiplier;
      else
        variable = variable + multiplier;
      update_screen = true; //as a variable has just been changed by the encoder, flag the screen as updatable
      encoder_counter = 0;
    }

    if(encoder_data == -1){
      if(mult)
        variable = variable / multiplier;
      else
        variable = variable - multiplier;
      update_screen = true; //as a variable has just been changed by the encoder, flag the screen as updatable
      encoder_counter = 0;
    }
  }

}

bool joystick_X(joystick_axis axis)
{
  int value = analogRead(xAnalogPin);
//Serial.print("joyStick X:");
//Serial.println(String(value));
  if(axis == LEFT && value < 800)
    return LOW;
  if(axis == RIGHT && value > 850)
    return LOW;

  return HIGH;
}

bool get_forward_control()
{
    long mb_currentMillis = millis();
    if(manual_latency < 1)
      manual_latency = 25;
    if(mb_currentMillis - mbf_prevMillis > manual_latency)
    {
      mbf_prevMillis = mb_currentMillis;
      if(read_manual_buttons)
        forward_control_value = mcp.digitalRead(forward_control);
      else
        forward_control_value = joystick_X(LEFT);
    }
    return forward_control_value;
}

bool get_backward_control()
{
    long mb_currentMillis = millis();
    if(manual_latency < 1)
      manual_latency = 25;
    if(mb_currentMillis - mbb_prevMillis > manual_latency)
    {
      mbb_prevMillis = mb_currentMillis;
      if(read_manual_buttons)
        backward_control_value = mcp.digitalRead(backward_control);
      else
        backward_control_value = joystick_X(RIGHT);
    }
    return backward_control_value;
}

// lee el encoder y retorna si la posición actual es mayor/menor/igual (1/-1/0) a la anterior
int8_t encoder_read(){

  int8_t ret = 0;
  encoder.tick();

  int newPos = encoder.getPosition();
  if (encoderPos != newPos) {
//    Serial.println(newPos);
    if(newPos > encoderPos)
      ret = 1;
    else
      ret = -1;
    encoderPos = newPos;
  } // if
  return ret;
}
