// valor seguro para una pausa mínima en microsegundos
const int pulseWidthMicros = 20;  // microseconds

void do_yield()
{
#ifdef ESP8266
  yield();
#else
  delayMicroseconds(pulseWidthMicros);
#endif
}

void delayMillis(unsigned long period)
{
  unsigned long time_now = millis();
  while(millis() < time_now + period){
    do_yield();
  }
}

void delayMicros(unsigned long period)
{
  unsigned long time_now = micros();
  while(micros() < time_now + period){
    do_yield();
  }
}

bool mcp_pin_read(int pin)
{
    long currentMillis = millis();
    if(currentMillis - mcp_prevMillis[pin] > mcp_latency)
    {
      mcp_prevMillis[pin] = currentMillis;
      mcp_pin_value[pin] = mcp.digitalRead(pin);
    }
    return mcp_pin_value[pin];
}

// teoría: microsegundos de pausa a partir de revs/min
// stepper_delay = 60L * 1000L * 1000L / steps_per_rev / rpm_speed;

float rpm_to_delay(float rpm_speed)
{
  float steps_revolution=(float)driver_microsteps*(float)motor_steps;
  return 60.00 * 1000.00 * 1000.00 / steps_revolution / rpm_speed;
}

float delay_to_rpm(float driver_delay)
{
  float steps_revolution=(float)driver_microsteps*(float)motor_steps;
  return 60.00 * 1000.00 * 1000.00 / steps_revolution / driver_delay;
}
