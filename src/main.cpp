#include <Arduino.h>
#include <Arduino.h>

//Recordar que se pueden apoyar de este código pero deben
//mejorarlo porque tiene sus detalles en el Serial Monitor
//debido a que hay que seleccionar en elgunas partes con 2 dígitos
//de manera rápida, por otro lado, al seleccionar la velocidad
//este no guarda el seleccionado y muestra otra velocidad, 
//por lo que hay que corregirlo.


// Ejemplo simple de clase Motor para ESP32 usando PWM (ledc)
// Permite seleccionar motor 1 o 2, dirección (IZQUIERDA/DERECHA) y velocidad (0-255)

// Ajusta estos pines según tu hardware
const int M1_PWM_PIN = 18; // PWM pin for motor 1
const int M1_DIR_PIN = 19; // Direction pin for motor 1
const int M2_PWM_PIN = 21; // PWM pin for motor 2
const int M2_DIR_PIN = 22; // Direction pin for motor 2

// ledc channels and properties (ESP32)
const int PWM_FREQ = 20000; // 20 kHz
const int PWM_RESOLUTION = 8; // 8-bit resolution (0-255)

// Definir las tres velocidades fijas
const uint8_t VEL_BAJA = 85;   // 33% de 255
const uint8_t VEL_MEDIA = 170; // 66% de 255
const uint8_t VEL_ALTA = 255;  // 100% de 255

class Motor {
    public:
        Motor(int pwmPin, int dirPin, int channel): pwmPin(pwmPin), dirPin(dirPin), channel(channel) {}

        void begin() {
            pinMode(dirPin, OUTPUT);
            // configure ledc channel
            ledcSetup(channel, PWM_FREQ, PWM_RESOLUTION);
            ledcAttachPin(pwmPin, channel);
            stop();
        }

        // direction: true = RIGHT (CW), false = LEFT (CCW)
        void setDirection(bool direction) {
            digitalWrite(dirPin, direction ? HIGH : LOW);
            currentDirection = direction;
        }

        // Mapea cualquier valor de entrada a tres velocidades fijas
        void setSpeed(uint8_t inputSpeed) {
            uint8_t mappedSpeed;
            if (inputSpeed == 0) {
                mappedSpeed = 0;
            } else if (inputSpeed < 85) {
                mappedSpeed = VEL_BAJA;
            } else if (inputSpeed < 170) {
                mappedSpeed = VEL_MEDIA;
            } else {
                mappedSpeed = VEL_ALTA;
            }
            currentSpeed = mappedSpeed;
            ledcWrite(channel, mappedSpeed);
        }

        void stop() {
            setSpeed(0);
        }

        bool getDirection() const { return currentDirection; }
        uint8_t getSpeed() const { return currentSpeed; }
        
        // Obtener el nombre de la velocidad actual
        const char* getSpeedName() const {
            if (currentSpeed == 0) return "DETENIDO";
            if (currentSpeed == VEL_BAJA) return "BAJA";
            if (currentSpeed == VEL_MEDIA) return "MEDIA";
            if (currentSpeed == VEL_ALTA) return "ALTA";
            return "DESCONOCIDA";
        }

    private:
        int pwmPin;
        int dirPin;
        int channel;
        bool currentDirection = false;
        uint8_t currentSpeed = 0;
};

// Crear dos motores (canales 0 y 1)
Motor motor1(M1_PWM_PIN, M1_DIR_PIN, 0);
Motor motor2(M2_PWM_PIN, M2_DIR_PIN, 1);

// Helper para leer entrada serial completa
String readSerialLine() {
    String s;
    unsigned long startTime = millis();
    while ((millis() - startTime) < 100) { // timeout de 100ms
        if (Serial.available()) {
            char c = Serial.read();
            if (c == '\n' || c == '\r') {
                if (s.length() > 0) return s;
            } else {
                s += c;
                startTime = millis(); // reset timeout al recibir caracteres
            }
        }
        yield(); // permite que el ESP32 maneje otras tareas
    }
    return s; // retorna lo que tengamos después del timeout
}

void printMenu() {
    Serial.println(F("--------------------------------"));
    Serial.println(F("   MENU DE CONTROL DE MOTOR    "));
    Serial.println(F("--------------------------------"));
    Serial.println(F("1) Seleccionar MOTOR 1"));
    Serial.println(F("2) Seleccionar MOTOR 2"));
    Serial.println(F("d) Direccion  -> di=izq, dr=der"));
    Serial.println(F("v) Velocidad:"));
    Serial.println(F("   v1 = BAJA  (33%)"));
    Serial.println(F("   v2 = MEDIA (66%)"));
    Serial.println(F("   v3 = ALTA  (100%)"));
    Serial.println(F("s) STOP/Detener motor"));
    Serial.println(F("q) Query/Ver estado"));
    Serial.println(F("m) Mostrar este menu"));
    Serial.println(F("--------------------------------"));
    Serial.print(F("Comando > "));
}

Motor* selectedMotor = &motor1;

void setup() {
    Serial.begin(115200);
    // Esperar hasta que el Serial esté disponible
    for(int i = 0; i < 10; i++) {
        Serial.print("\n"); // Limpiar buffer
        delay(100);
    }
    
    Serial.println(F("\n\n========================="));
    Serial.println(F("Control de Motores v1.0"));
    Serial.println(F("=========================\n"));
    
    // Inicializar motores con feedback
    Serial.println(F(">> Configurando motores..."));
    motor1.begin();
    Serial.println(F("   Motor 1: OK"));
    motor2.begin();
    Serial.println(F("   Motor 2: OK\n"));
    
    selectedMotor = &motor1;
    
    // Forzar múltiples intentos de mostrar el menú
    for(int i = 0; i < 3; i++) {
        Serial.println(F("\n>> MENU PRINCIPAL <<"));
        printMenu();
        Serial.print(F("\n> "));
        delay(100);
    }
}

void loop() {
    static unsigned long lastCheck = 0;
    static unsigned long lastMenuShow = 0;
    static bool firstLoop = true;
    unsigned long now = millis();
    
    // En el primer loop, asegurarse de que el menú se muestra
    if (firstLoop) {
        Serial.println(F("\n\n>> MENU PRINCIPAL <<"));
        printMenu();
        firstLoop = false;
    }
    
    // Cada 5 segundos sin actividad, mostrar un recordatorio
    if (now - lastMenuShow >= 5000) {
        lastMenuShow = now;
        Serial.println(F("\nPresiona 'm' para ver el menu, 'q' para ver estado"));
        Serial.print(F("\n> "));
    }
    
    // Cada segundo mostrar que estamos vivos
    if (now - lastCheck >= 1000) {
        lastCheck = now;
        Serial.print(F(".")); // indicador de vida
    }
    
    if (Serial.available()) {
        lastMenuShow = now; // resetear el timer del recordatorio
        String line = readSerialLine();
        line.trim();
        if (line.length() == 0) return;
        char cmd = line.charAt(0);
        switch (cmd) {
            case '1':
                selectedMotor = &motor1;
                Serial.println(F("Motor 1 seleccionado."));
                break;
            case '2':
                selectedMotor = &motor2;
                Serial.println(F("Motor 2 seleccionado."));
                break;
            case 'd':
            case 'D':
                if (line.length() >= 2) {
                    char sub = line.charAt(1);
                    if (sub == 'i' || sub == 'I') {
                        selectedMotor->setDirection(false);
                        Serial.println(F("Direccion: IZQUIERDA"));
                    } else if (sub == 'r' || sub == 'R') {
                        selectedMotor->setDirection(true);
                        Serial.println(F("Direccion: DERECHA"));
                    } else {
                        Serial.println(F("Uso: d i  (izquierda) o d r  (derecha)"));
                    }
                } else {
                    Serial.println(F("Uso: d i  (izquierda) o d r  (derecha)"));
                }
                break;
            case 'v':
            case 'V':
                if (line.length() >= 2) {
                    String val = line.substring(1);
                    val.trim();
                    char nivel = val.charAt(0);
                    uint8_t speed;
                    switch(nivel) {
                        case '1':
                            speed = VEL_BAJA;
                            Serial.println(F("Velocidad: BAJA (33%)"));
                            break;
                        case '2':
                            speed = VEL_MEDIA;
                            Serial.println(F("Velocidad: MEDIA (66%)"));
                            break;
                        case '3':
                            speed = VEL_ALTA;
                            Serial.println(F("Velocidad: ALTA (100%)"));
                            break;
                        default:
                            Serial.println(F("Velocidad no válida. Use:"));
                            Serial.println(F("v1 = BAJA"));
                            Serial.println(F("v2 = MEDIA"));
                            Serial.println(F("v3 = ALTA"));
                            return;
                    }
                    selectedMotor->setSpeed(speed);
                } else {
                    Serial.println(F("Uso: v1=baja, v2=media, v3=alta"));
                }
                break;
            case 's':
            case 'S':
                selectedMotor->stop();
                Serial.println(F("Motor detenido."));
                break;
            case 'q':
            case 'Q':
                Serial.println(F("Estado actual:"));
                if (selectedMotor == &motor1) Serial.println(F("Seleccionado: Motor 1"));
                else Serial.println(F("Seleccionado: Motor 2"));
                Serial.print(F("Direccion: "));
                Serial.println(selectedMotor->getDirection() ? "DERECHA" : "IZQUIERDA");
                Serial.print(F("Velocidad: "));
                Serial.print(selectedMotor->getSpeedName());
                Serial.print(F(" ("));
                Serial.print(selectedMotor->getSpeed());
                Serial.println(F(")"));
                break;
            case 'm':
            case 'M':
                printMenu();
                break;
            default:
                Serial.println(F("Opcion no valida. Presiona 'm' para mostrar el menu."));
                break;
        }
        Serial.print(F("\n> "));
    }
    delay(10);
}