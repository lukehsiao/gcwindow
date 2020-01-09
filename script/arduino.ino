void setup()
{
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);
}

void loop()
{
    static uint32_t ts1 = 0;
    static uint32_t ts2 = 0;
    static int state = 0;
    String cmd = "";

    switch (state) {
        case 0:
            digitalWrite(LED_BUILTIN, HIGH);

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
            digitalWrite(LED_BUILTIN, LOW);

            if (micros() - ts1 > 5000000) {
                ts2 = micros();
                Serial.println(ts2 - ts1);
                state = 0;
            }

            break;
    }
}
