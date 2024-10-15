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
static int pwmActual = 0;         // Valor de PWM actual.

// Variables de Termistor
static const int tBeta = 4021; // Valor B del termistor. Intermedio entre NTC3950 100K y EPKOS 4092 100K.
static const int tR0 = 100000; // Valor de resistencia a 25 grados Celsius.
static const int tT0 = 298.15; // Temperatura de referencia en Kelvin.
static const int tR1 = 10000;  // Resistencia en serie (10K ohms). De referencia para el divisor de voltaje.
static const int kelvin = 273.15; // Valor de Kelvin para convertir a Celsius.
static int lecturaTermistor = 0; // Lectura del termistor.

// Estructura para almacenar pares de temperatura y porcentaje de PWM
struct TempPWM {
    int temperatura;
    int porcentajePWM;
};

// Tabla del usuario
static TempPWM tableUserTempSpeed[] = {
    {40, 25},
    {60, 50},
    {70, 70},
    {80, 100}};

// Tabla de conversion de temperatura a PWM
static TempPWM tableTempSpeed[] = {
    {0, 0},
    {40, pwmMin},
    {60, 50},
    {70, 70},
    {80, pwmMax}};

// Número de elementos en la matriz
static int cantElementosArray = sizeof(tableTempSpeed) / sizeof(tableTempSpeed[0]);

// Declaraciones de funciones
int temperaturaTermistor();
void setVelocidadPWM(int velocidad);
int calcularPWM(int temperatura);
void actualizarTabla();

// Inicializa el sistema
void setup() {
    Serial.println("---------------------------------");
    Serial.println("Iniciando sistema...");
    Serial.begin(BAUD_RATE); // Inicia la comunicación serial.
    Serial.println("Configurando pines...");
    pinMode(pinPWM, OUTPUT);	  // Setea el pin 9 como salida.
    analogWriteFreq(pwmFreq);	  // Setea la frecuencia de PWM.
    Serial.println("Configurando tabla de conversion...");
    actualizarTabla(); // Actualiza la tabla de conversion.
    Serial.println("Sistema iniciado correctamente.");
    Serial.println("---------------------------------");
}

// Loop principal
void loop() {
  // Lectura de temperatura
    setVelocidadPWM(calcularPWM(temperaturaTermistor()));// Setea el valor de PWM.
  delay(1000);
}

// Actualiza la tabla de conversion de temperatura a PWM
void actualizarTabla() {
    Serial.println("Actualizando tabla de conversion...");
    // Limpiar la tabla de conversion de temperatura a PWM
    memset(tableTempSpeed, 0, sizeof(tableTempSpeed));

    // Colocar el primer valor 0,0
    tableTempSpeed[0].temperatura = 0;
    tableTempSpeed[0].porcentajePWM = 0;

    int cantElementosUserArray = sizeof(tableUserTempSpeed) / sizeof(tableUserTempSpeed[0]);
    int tamanoTableTempSpeed = sizeof(tableTempSpeed) / sizeof(tableTempSpeed[0]);

    // Copiar los valores del usuario a la tabla de conversion
    for (int i = 0; i < cantElementosUserArray; i++) {
        tableTempSpeed[i + 1].temperatura = tableUserTempSpeed[i].temperatura;
        tableTempSpeed[i + 1].porcentajePWM = tableUserTempSpeed[i].porcentajePWM;
    }

    // Valida valores ascendentes y entre pwmMin y pwmMax
    for (int i = 1; i < cantElementosUserArray; i++) {
        if (tableTempSpeed[i].porcentajePWM < pwmMin) {
            tableTempSpeed[i].porcentajePWM = pwmMin;
        } else if (tableTempSpeed[i].porcentajePWM > pwmMax) {
            tableTempSpeed[i].porcentajePWM = pwmMax;
        }
        if (tableTempSpeed[i].temperatura > tableTempSpeed[i + 1].temperatura) { // Si la temperatura es mayor a la siguiente, se reemplaza el valor por un promedio entre el siguiente y el anterior.
            tableTempSpeed[i].temperatura = (tableTempSpeed[i + 1].temperatura + tableTempSpeed[i - 1].temperatura) / 2;
        }
    }

    // Valida que el ultimo valor sea igual a pwmMax
    if (cantElementosUserArray < tamanoTableTempSpeed - 1) {
        if (tableTempSpeed[cantElementosUserArray].porcentajePWM != pwmMax) {
            tableTempSpeed[cantElementosUserArray + 1].temperatura = tableTempSpeed[cantElementosUserArray].temperatura + 1;
            tableTempSpeed[cantElementosUserArray + 1].porcentajePWM = pwmMax;
        }
    }

    // Actualiza tamaño de la tabla
    cantElementosArray = tamanoTableTempSpeed;

    // Imprime la tabla de conversion
    Serial.println("Tabla de conversion actualizada:");
    for (int i = 0; i < cantElementosArray; i++) {
        Serial.print("Temperatura: ");
        Serial.print(tableTempSpeed[i].temperatura);
        Serial.print("°C, PWM: ");
        Serial.print(tableTempSpeed[i].porcentajePWM);
        Serial.println("%");
    }
}

int temperaturaTermistor() {
    lecturaTermistor = analogRead(pinTermistor); // lecturaTermistor del termistor.
    if (lecturaTermistor < 0 || lecturaTermistor > 1023) { // Si la lecturaTermistor esta fuera de rango, devuelve 0.
        Serial.println("Error: lecturaTermistor del termistor fuera de rango");
        return tableTempSpeed[cantElementosArray - 1].temperatura; // Devuelve la temperatura maxima.
    }
    int temperatura = (1.0 / (1.0 / tT0 + log((tR1 * (1023.0 / lecturaTermistor - 1.0)) / tR0) / tBeta)) - kelvin ; // Temperatura en Kelvin.
    Serial.print("Temperatura: ");
    Serial.print(temperatura);
    Serial.println("°C");
	temperatura = temperatura < tableTempSpeed[0].temperatura ? tableTempSpeed[0].temperatura : temperatura;
temperatura = temperatura > tableTempSpeed[cantElementosArray - 1].temperatura ? tableTempSpeed[cantElementosArray - 1].temperatura : temperatura;
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
    if (temperatura >= tableTempSpeed[cantElementosArray - 1].temperatura) {
        Serial.println("Temperatura mayor a la maxima. Encendiendo motor al maximo.");
        return pwmMax; // Enciende el motor al maximo si la temperatura es mayor a la maxima.
    }
  for (int i = 0; i < cantElementosArray - 1; i++) {
    if (temperatura >= tableTempSpeed[i].temperatura && temperatura <= tableTempSpeed[i + 1].temperatura) {
      int porcentajePWM = tableTempSpeed[i].porcentajePWM +
                          (tableTempSpeed[i + 1].porcentajePWM - tableTempSpeed[i].porcentajePWM) *
                              (temperatura - tableTempSpeed[i].temperatura) /
                              (tableTempSpeed[i + 1].temperatura - tableTempSpeed[i].temperatura);
      Serial.print("Porcentaje de PWM: ");
      Serial.print(porcentajePWM);
      Serial.println("%");
      return porcentajePWM >= pwmMin ? porcentajePWM : pwmOff;
    }
  }
  Serial.println("Error: No se encontro el valor de PWM para la temperatura.");
    return pwmMax; // Valor por defecto en caso de error
}