#if defined (OLED_MODE)

#define I2C_ADDRESS 0x3C

// Define proper RST_PIN if required.
#define RST_PIN -1

//------------------------------------------------------------------------------
void oledSetup() {
  Wire.begin();
#if RST_PIN >= 0
  oled.begin(&Adafruit128x64, I2C_ADDRESS, RST_PIN);
#else // RST_PIN >= 0
  oled.begin(&Adafruit128x64, I2C_ADDRESS);
#endif // RST_PIN >= 0
    oled.setFont(X11fixed7x14B);
    oled.println();
    oled.clear();
    oled.setRow(0);
    oled.print("MTC:");
    oled.println(0,DEC);
    oled.print("LTC:");
    oled.println(0,DEC);
    oled.print("t-dif:");
    oled.println(0,DEC);
    oled.print("f-dif:");
    oled.println(0,DEC);
    writeLTCOut = false;
  return;
}

void oledLTC(uint32_t mtc, uint32_t ltc,uint32_t timeDif,uint32_t frameDif)
{
    oled.setRow(0);
    oled.print("MTC:");
    oled.println(mtc,DEC);
    oled.print("LTC:");
    oled.println(ltc,DEC);
    oled.print("t-dif:");
    oled.println(timeDif,DEC);
    oled.print("f-dif:");
    oled.println(frameDif,DEC);
    writeLTCOut = false;
    return;
  }

#endif
