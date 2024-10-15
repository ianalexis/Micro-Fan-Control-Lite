#include <Arduino.h>

// Pines de conexion
static const unsigned int pinPWM = PIN_PWM;         // Pin de control de velocidad del motor.
static const unsigned int pinTacometro = PIN_TACOMETRO; // Pin de lectura del tacometro.
static const unsigned int pinTermistor = PIN_TERMISTOR; // Pin de lectura del termistor.

// Variables de control de velocidad
static const int pwmMax = 100;     // Valor maximo de PWM
static const int pwmBits = PWMBITS; // Resolucion de PWM en bits.
static const int pwmOff = 0;     // Valor de PWM para apagar el motor.
static const int pwmFreq = 6250;    // Frecuencia de PWM en Hz.
static const int pwmMin = 25;        // Valor minimo de PWM.
int pwmActual = 0;         // Valor de PWM actual.

// Variables de Termistor
const int tBeta = 4021; // Valor B del termistor. Intermedio entre NTC3950 100K y EPKOS 4092 100K.
const int tR0 = 100000; // Valor de resistencia a 25 grados Celsius.
const int tT0 = 298.15; // Temperatura de referencia en Kelvin.
const int tR1 = 10000;  // Resistencia en serie (10K ohms). De referencia para el divisor de voltaje.
const int kelvin = 273.15; // Valor de Kelvin para convertir a Celsius.
int lecturaTermistor = 0; // Lectura del termistor.

// Estructura para almacenar pares de temperatura y porcentaje de PWM
struct TempPWM {
    int temperatura;
    int porcentajePWM;
};

// Tabla de conversion de temperatura a PWM
const TempPWM tempPWMArray[] = {
    {10, 1},
    {20, 25},
    {30, 30},
    {35, 99},
    {40, 100}}; // Tabla de prueba TODO: Borrar

// Declaraciones de funciones
int temperaturaTermistor();
void setVelocidadPWM(int velocidad);
int calcularPWM(int temperatura);

// Número de elementos en la matriz
const int cantElementosArray = sizeof(tempPWMArray) / sizeof(tempPWMArray[0]);

// Declaraciones de funciones
int temperaturaTermistor();
void setVelocidadPWM(int velocidad);
int calcularPWM(int temperatura);

// Inicializa el sistema
void setup() {
    Serial.println("---------------------------------");
    Serial.println("Iniciando sistema...");
    Serial.begin(BAUD_RATE); // Inicia la comunicación serial.
    Serial.println("Configurando pines...");
    pinMode(pinPWM, OUTPUT);	  // Setea el pin 9 como salida.
    analogWriteFreq(pwmFreq);	  // Setea la frecuencia de PWM.
    Serial.println("Sistema iniciado correctamente.");
    Serial.println("---------------------------------");
}

// Loop principal
void loop() {
  // Lectura de temperatura
    setVelocidadPWM(calcularPWM(temperaturaTermistor()));// Setea el valor de PWM.
  delay(1000);
}

int temperaturaTermistor() {
    lecturaTermistor = analogRead(pinTermistor); // lecturaTermistor del termistor.
    if (lecturaTermistor < 0 || lecturaTermistor > 1023) { // Si la lecturaTermistor esta fuera de rango, devuelve 0.
        Serial.println("Error: lecturaTermistor del termistor fuera de rango");
        return tempPWMArray[cantElementosArray - 1].temperatura; // Devuelve la temperatura maxima.
    }
    int temperatura = (1.0 / (1.0 / tT0 + log((tR1 * (1023.0 / lecturaTermistor - 1.0)) / tR0) / tBeta)) - kelvin ; // Temperatura en Kelvin.
    Serial.print("Temperatura: ");
    Serial.print(temperatura);
    Serial.println("°C");
	temperatura = temperatura < tempPWMArray[0].temperatura ? tempPWMArray[0].temperatura : temperatura;
temperatura = temperatura > tempPWMArray[cantElementosArray - 1].temperatura ? tempPWMArray[cantElementosArray - 1].temperatura : temperatura;
  return temperatura;
}

// Set de velocidad en % a PWM en bits
void setVelocidadPWM(int velocidad) {
    if (velocidad != pwmActual) {
        Serial.print("Velocidad a: ");
        Serial.print(velocidad);
        Serial.println("%");
        pwmActual = velocidad >= pwmMin ? velocidad : pwmOff;
        analogWrite(pinPWM, map(velocidad, 0, 100, 0, pwmBits));
    }
}

// Calcula el PWM a partir de la temperatura
int calcularPWM(int temperatura) { // Devuelve el valor de PWM basado en la temperatura.
    if (temperatura >= tempPWMArray[cantElementosArray - 1].temperatura) {
        Serial.println("Temperatura mayor a la maxima. Encendiendo motor al maximo.");
        return pwmMax; // Enciende el motor al maximo si la temperatura es mayor a la maxima.
    }
  for (int i = 0; i < cantElementosArray - 1; i++) {
    if (temperatura >= tempPWMArray[i].temperatura && temperatura <= tempPWMArray[i + 1].temperatura) {
      int porcentajePWM = tempPWMArray[i].porcentajePWM +
                          (tempPWMArray[i + 1].porcentajePWM - tempPWMArray[i].porcentajePWM) *
                              (temperatura - tempPWMArray[i].temperatura) /
                              (tempPWMArray[i + 1].temperatura - tempPWMArray[i].temperatura);
      Serial.print("Porcentaje de PWM: ");
      Serial.print(porcentajePWM);
      Serial.println("%");
      return porcentajePWM >= pwmMin ? porcentajePWM : pwmOff;
    }
  }
  Serial.println("Error: No se encontro el valor de PWM para la temperatura.");
    return pwmMax; // Valor por defecto en caso de error
}