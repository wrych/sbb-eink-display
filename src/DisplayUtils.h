#ifndef DISPLAY_UTILS_H
#define DISPLAY_UTILS_H

#include <Arduino.h>

// --- HELPER: Fix Umlauts ---
inline String utf8ToAscii(String input)
{
    String output = "";
    for (int i = 0; i < input.length(); i++)
    {
        char c = input[i];
        if (c == 0xC3)
        {
            i++;
            switch (input[i])
            {
            case 0xA4:
                output += "ae";
                break;
            case 0xB6:
                output += "oe";
                break;
            case 0xBC:
                output += "ue";
                break;
            case 0x84:
                output += "Ae";
                break;
            case 0x96:
                output += "Oe";
                break;
            case 0xDC:
                output += "Ue";
                break;
            default:
                output += "?";
                break;
            }
        }
        else
        {
            output += c;
        }
    }
    return output;
}

#endif
