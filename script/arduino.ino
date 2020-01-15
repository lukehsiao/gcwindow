int sensorPin = A0;
int ledPin = 10;

void setup()
{
    Serial.begin(115200);
    pinMode(ledPin, OUTPUT);
    pinMode(sensorPin, INPUT);
}

void loop()
{
    static uint32_t ts1 = 0;
    static uint32_t ts2 = 0;
    static int state = 0;
    static int sensorValue = 0;
    String cmd = "";

    switch (state) {
        case 0:
            digitalWrite(ledPin, HIGH);

            // poll new command
            if (Serial.available() > 0) {
                cmd = Serial.readString();
                cmd.trim();

                if (cmd == "g") {
                    ts1 = micros();

                    // Move to next state
                    state = 1;
                }
            }
            break;

        case 1:
            digitalWrite(ledPin, LOW);
            sensorValue = analogRead(sensorPin);

            // Rising edge trigger condition
            if (sensorValue > 512) {
                // Log time difference and send back to host PC
                ts2 = micros();
                Serial.println(ts2 - ts1);
                state = 0;
                sensorValue = 0;
            }

            break;
    }
}
