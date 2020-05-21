/*
  Cabina de Spray v10 para Arduino UNO y Nano

  Este sketch se utiliza para actuar una o dos electroválvulas en un sistema de sanitización.
  El sistema cuanta con 3 entradas y 4 salidas.

  Entrada 1: PIR. Debe actuar sobre la salida si y solo si hay agua suficiente en el depósito.
  Entrada 2: Control de nivel de agua.
  Entrada 3: Botón de prueba. Actua sobre la salida haya o no agua en el depósito.
  Salida 1 y 2: Relé. Permite el encendido o apagado de la electroválvula.
  Salida 3: LED Rojo. Parpadea durante la inicialización del PIR o cuando no hay agua.
  Salida 4: LED Verde. Parpadea durante la inicialización del PIR o cuando se activa el relé.

  The circuit:
  list the components attached to each input
  Botón (HW) con debounce (SW) en D2. HIGH si no presioné, sino LOW.
  Nivel de agua (HW) sin debounce (SW) en D3. HIGH si hay agua, sino LOW.
  PIR en D4. HIGH si detecta movimiento, sino LOW.
  list the components attached to each output
  Relé #0 en D5. Normal cerrado (enciendo bomba) en LOW.
  Relé #1 en D6 (opcional según modelo).  Normal cerrado (enciendo bomba) en LOW.
  LED verde en D9.
  LED rojo en D10.

  Creado:  1 de mayo del 2020
  Autor: Norberto Padín
  Modificado: 5 de mayo del 2020 por Norberto Padín
  Modificado: 15 de mayo del 2020 por Norberto Padín

*/

//=======
// Include Libraries

//=======
// Pin Definitions
const byte BUTTON_PIN_SIG = 2;
const byte FLOATSWITCH_PIN_SIG = 3;
const byte PIR_PIN_SIG = 4;
const byte RELAYMODULE2CH_PIN_IN1 = 5;
const byte RELAYMODULE2CH_PIN_IN2 = 6;
const byte LED_VERDE = 9;
const byte LED_ROJO = 10;

//=======
// Global variables and defines
const unsigned long ledInterval = 500;           // interval at which to blink (milliseconds)
const unsigned long pumpInterval = 6000;           // interval at which to turn on the PUMP (milliseconds)
const unsigned long debounceDelay = 100;    // the debounce time; increase if the output flickers
const int calibrationTime = 5;          // calibration time 30seg

//=======
// Define an array for the 2ch relay module pins
int RelayModule2chPins[] = { RELAYMODULE2CH_PIN_IN1, RELAYMODULE2CH_PIN_IN2 };

//=======
//------- VARIABLES (will change)
unsigned long currentMillis;       // stores the value of millis() in each iteration of loop()
unsigned long lastDebounceTime;    // the last time the output pin was toggled
unsigned long previousLedMillis;   // will store last time LED was updated
unsigned long previousPumpMillis;  // will store last time PUMP was updated

bool ledState = LOW;               // ledState used to set the LED
bool pumpState = HIGH;             // pumpState used to set the PUMP
bool pirState = LOW;               // we start, assuming no motion detected
bool motion = LOW;
bool reading = HIGH;
bool buttonState = HIGH;           // the current reading from the input pin
bool lastButtonState = HIGH;       // the previous reading from the input pin
bool button = false;
bool water = false;
bool movement = false;
bool enabled = false;
bool readyToPump = true;

//========
// Inicialización
void setup() {

  // set the Led pins as output:
  pinMode(LED_ROJO, OUTPUT);
  pinMode(LED_VERDE, OUTPUT);

  // set the pushbutton pin as input with an external pullup resistor to ensure it defaults to HIGH
  pinMode(BUTTON_PIN_SIG, INPUT);

  // initialize the water level sensor pin as an input with an external pullup resistor to ensure it defaults to HIGH
  pinMode(FLOATSWITCH_PIN_SIG, INPUT);

  // initialize the movement sensor pin as an input:
  pinMode(PIR_PIN_SIG, INPUT);

  // initialize the relays pins as an output and turn them off:
  pinMode(RELAYMODULE2CH_PIN_IN1, OUTPUT);
  pinMode(RELAYMODULE2CH_PIN_IN2, OUTPUT);
  digitalWrite(RelayModule2chPins[0], HIGH);
  digitalWrite(RelayModule2chPins[1], HIGH);

  // Turn the LEDs off
  digitalWrite(LED_ROJO, LOW);
  digitalWrite(LED_VERDE, LOW);

  //  Calibrate PIR sensor
  for (int i = 0; i < calibrationTime; i++) {
    digitalWrite(LED_ROJO, HIGH);
    delay(500);
    digitalWrite(LED_ROJO, LOW);
    digitalWrite(LED_VERDE, HIGH);
    delay(500);
    digitalWrite(LED_VERDE, LOW);
    digitalWrite(LED_ROJO, HIGH);
  }

  //  Cargar sistema
  pumpState = LOW;
  digitalWrite(LED_VERDE, !pumpState);
  digitalWrite(RelayModule2chPins[0], pumpState);   // Prendo la bomba activando el relay
  digitalWrite(RelayModule2chPins[1], pumpState);   // Prendo la electroválvula activando el relay
  delay(6000);
  pumpState = HIGH;
  enabled = false;
  readyToPump = false;
  digitalWrite(LED_VERDE, !pumpState);
  digitalWrite(RelayModule2chPins[0], pumpState);   // Apago la bomba activando el relay
  digitalWrite(RelayModule2chPins[1], pumpState);   // Apago la electroválvula activando el relay

}

// Global functions declaration

//=======
// Función de detección de nivel de agua, retorna FALSE si falta agua, retorna TRUE si esta OK
bool Water_Sensor() {
  if (digitalRead(FLOATSWITCH_PIN_SIG) == 0) {    //LOW-LEVEL
    return false;
  } else {   //HIGH-LEVEL
    return true;
  }
}

//=======
// Función de detección de movimiento
bool Movement_Sensor() {
  motion = digitalRead(PIR_PIN_SIG);
  if (motion == HIGH) {
    if (pirState == LOW) {
      pirState = HIGH;
      return true;
    }
  } else if (pirState == HIGH) {
    pirState = LOW;
    return false;
  }
  else {
    return false;
  }
  return false;
}

//=======
// Función de encendido de bomba
void Prender_Bomba() {
  pumpState = digitalRead(RelayModule2chPins[0]);  // Leo el estado de la bomba, encendida LOW, apagada HIGH
  if (readyToPump == true && pumpState == HIGH) {  // Si (presionaron botón || (se detectó movimiento && hay agua)) && bomba apagada
    previousPumpMillis = currentMillis;              // Capto el inicio del bombe
    enabled = true;                                  // Bomba habilitada
  }
  if (enabled == true) {                           // software timer is active ?
    if (currentMillis - previousPumpMillis >= pumpInterval) {  // Acumulo tiempo de actividad de la bomba y comparo contra el seteo
      pumpState = HIGH;                              // Inicializo variable para apagar relé y LED verde
      enabled = false;                               // Desactivo el software timer
      readyToPump = true;                            // Habilito lecturas de los sensores
      digitalWrite(LED_VERDE, !pumpState);           // Apago LED verde
      digitalWrite(RelayModule2chPins[0], pumpState); // Apago la bomba activando el relay
      digitalWrite(RelayModule2chPins[1], pumpState); // Apago la electroválvula activando el relay
    } else {                                          // Si no transcurrió el timer de actividad, prender todo
      pumpState = LOW;                                // Inicializo variable para encender relé y LED verde
      enabled = true;                                 // Activo el software timer
      readyToPump = false;                            // Dejo de aceptar lecturas de los sensores
      digitalWrite(LED_VERDE, !pumpState);
      digitalWrite(RelayModule2chPins[0], pumpState); // Prendo la bomba activando el relay
      digitalWrite(RelayModule2chPins[1], pumpState); // Prendo la electroválvula activando el relay
    }
  }
}
//=======
// Función de encendido de LED rojo, fijo o titilante
void red_Led(bool nivel) {
  if (nivel) {
    ledState = HIGH;
    digitalWrite(LED_ROJO, ledState);
  } else {
    if (currentMillis - previousLedMillis >= ledInterval) {
      previousLedMillis = currentMillis;  // save the last time you blinked the LED
      if (ledState == LOW) {              // if the LED is off turn it on and vice-versa:
        ledState = HIGH;
      } else {
        ledState = LOW;
      }
      digitalWrite(LED_ROJO, ledState);   // set the LED with the ledState of the variable:
    }
  }
}
//=======
// Función de lectura del botón con debouncing
bool readButton() {
  reading = digitalRead(BUTTON_PIN_SIG);     /* read the state of the switch into a local variable:
                                      check to see if you just pressed the button
                                      (i.e. the input went from HIGH to LOW), and you've waited long enough
                                      since the last press to ignore any noise:
*/
  if (reading != lastButtonState) {          // If the switch changed, due to noise or pressing:
    lastDebounceTime = currentMillis;        // reset the debouncing timer
  }
  if (currentMillis - lastDebounceTime >= debounceDelay) {
    /* whatever the reading is at, it's been there for longer than
                                                          the debounce delay, so take it as the actual current state:
    */
    if (reading != buttonState) {            // if the button state has changed:
      buttonState = reading;
      if (buttonState == LOW) {              // return true if the new button state is LOW
        lastButtonState = reading;
        return true;
      }
    }
  }
  lastButtonState = reading;
  return false;
}
//=======
// Main programm
void loop() {
  currentMillis = millis();
  button = readButton();
  water = Water_Sensor();
  movement = Movement_Sensor();
  if (button || (water && movement)) {
    readyToPump = true;
  } else {
    readyToPump = false;
  }
  Prender_Bomba();
  red_Led(water);
}
