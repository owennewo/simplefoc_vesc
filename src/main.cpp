#include <Arduino.h>
#include <board_vesc_4_12.h>
#include <SimpleFOC.h>

#define MAX_TARGET 50

HallSensor sensor = HallSensor(HALL_1, HALL_2, HALL_3, 15);
HardwareSerial Serial6(USART6_RX, USART6_TX);
BLDCMotor motor(15);
BLDCDriver6PWM driver(H1, L1, H2, L2, H3, L3, EN_GATE);

void doA() { sensor.handleA(); }
void doB() { sensor.handleB(); }
void doC() { sensor.handleC(); }

volatile long servo_duty_period_micros = 0; // typically 800us to 2200us

void onServo()
{
  int state = digitalRead(SERVO);
  static long start_micros = 0;
  if (state == LOW)
  {
    servo_duty_period_micros = micros() - start_micros;
  }
  else
  {
    start_micros = micros();
  }
}

void setup()
{

  Serial6.begin(115200);
  delay(1000);
  Serial6.println("Setup");

  sensor.enableInterrupts(doA, doB, doC);
  sensor.init();

  motor.linkSensor(&sensor);

  attachInterrupt(SERVO, onServo, CHANGE);

  driver.voltage_power_supply = 32;
  driver.init();

  motor.useMonitoring(Serial6);
  motor.linkDriver(&driver);

  motor.voltage_sensor_align = 1;
  motor.voltage_limit = 16;  // [V]
  motor.velocity_limit = 20; // [rad/s]

  motor.PID_velocity.P = 0.2;
  motor.PID_velocity.I = 1;
  motor.PID_velocity.output_ramp = 200;
  motor.P_angle.P = 2;

  motor.controller = ControlType::velocity; // ControlType::angle;

  motor.init();
  motor.initFOC();
}

void loop()
{

  float target = map(servo_duty_period_micros, 950, 2350, -MAX_TARGET * 100, MAX_TARGET * 100) / 100.0; // division by 100 to get some float/decimal point precision

  motor.loopFOC();
  motor.move(target);

}
