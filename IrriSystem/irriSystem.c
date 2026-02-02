//Aisosa Okunbor and Caleb Smith Team 12
//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL Evaluation Board
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

// Hardware configuration:
// Red LED:
//   PF1 drives an NPN transistor that powers the red LED
// Green LED:
//   PF3 drives an NPN transistor that powers the green LED
// UART Interface:
//   U0TX (PA1) and U0RX (PA0) are connected to the 2nd controller
//   The USB on the 2nd controller enumerates to an ICDI interface and a virtual COM port
//   Configured to 115,200 baud, 8N1

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "timer.h"
#include "uart0.h"
#include "irriSystem.h"
#include "utility.h"

//Variables are static so that other files dont mess with them 
static char* token;
static char* zone;
static uint8_t zoneClocks[ZONE_COUNT];
static uint8_t zonePause[ZONE_COUNT];
static bool zoneToggle[ZONE_COUNT];
static uint8_t sleep = 0;
static uint8_t unit = 0; // 0 for seconds, 1 for minutes, 2 for hours
static bool deepSleep = false;
static bool hazard = false;

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------


void irrigationTick()
{
    int i = 0;
    for (i = 0; i < ZONE_COUNT; i++)
    {
        if (zonePause[i] > 0)
        {
            zonePause[i]--;
        }
        if (zoneClocks[i] > 0)
        {
            zoneClocks[i]--;
        }
        if (sleep > 0)
        {
            sleep--;
        }

        // Drive Zone Pin
        if (!hazard && zoneToggle[i])
        {
            if (i == 0) ZONE_ONE = 1;
            if (i == 1) ZONE_TWO = 1;
            if (i == 2) ZONE_THREE = 1;
        }
        else if(zoneClocks[i] == 0 || zonePause[i] > 0 || sleep > 0 || hazard || deepSleep)
        {
            if (i == 0) ZONE_ONE = 0;
            if (i == 1) ZONE_TWO = 0;
            if (i == 2) ZONE_THREE = 0;
        }
        else if (zoneClocks[i] > 0)
        {
            if (i == 0) ZONE_ONE = 1;
            if (i == 1) ZONE_TWO = 1;
            if (i == 2) ZONE_THREE = 1;
        }
    }
}

// uint8_t asciiToUint8(const char str[])
// {
//     uint8_t data;
//     if (str[0] == '0' && tolower(str[1]) == 'x')
//         sscanf(str, "%hhx", &data);
//     else
//         sscanf(str, "%hhu", &data);
//     return data;
// }

void processCommand(char *topic, char *data)
{
    token = strtok(data, " ");
    uint8_t i = 0;
    uint8_t duration = 0;

    // Give commands
    if (strcmp(topic, "irrihelp") == 0 || strcmp(topic, "help") == 0 || strcmp(topic, "irricommands") == 0 || strcmp(topic, "irricmds") == 0 || strcmp(topic, "irricmd") == 0 || strcmp(topic, "commands") == 0 || strcmp(topic, "cmds") == 0 || strcmp(topic, "cmd") == 0)
    {
        putsUart0("Available Commands:\n");
        putsUart0("  irriadd <zones> <t>       - Add <t> time units to the specified zone(s)\n");
        putsUart0("  irrihazard <bool>         - Set, clear, or toggle hazard mode (disables all output)\n");
        putsUart0("  irrihelp                  - Show this command list\n");
        putsUart0("  irripause <zones> <t>     - Pause the specified zone(s) for <t> time units\n");
        putsUart0("  irrireset                 - Reset all zones, timers, sleep, and hazard state\n");
        putsUart0("  irrisleep <t>|deep        - Sleep for <t> time units or until manual wake\n");
        putsUart0("  irristart <zones> <t>     - Start the specified zone(s) for <t> time units\n");
        putsUart0("  irristatus                - Display current irrigation system status and state\n");
        putsUart0("  irristop <zones>          - Immediately stop specified zone(s)\n");
        putsUart0("  irritoggle <zones> <bool> - Enable, disable, or toggle manual override per zone\n");
        putsUart0("  irriunit <unit>           - Set time unit: seconds, minutes, or hours\n");
        putsUart0("  irriwake                  - Exit sleep or deep sleep mode\n");
        putsUart0("\nNote:\n");
        putsUart0("  <bool>:   true | on | false | off\n");
        putsUart0("  <t>:      duration in current time unit (e.g., 5 if unit is seconds)\n");
        putsUart0("  <unit>:   seconds | minutes | hours\n");
        putsUart0("  <zones>:  1, 2, 3, comma-separated list (e.g., 1,3), or 'all'\n");
    }

    // Add time to zone
    else if (strcmp(topic, "irriadd") == 0)
    {
        if (strcmp(token, "all") == 0)
        {
            token = strtok(NULL, " ");
            if (token == NULL)
            {
                putsUart0("Missing duration\n> ");
                return;
            }
            duration = asciiToUint8(token);
            for (i = 0; i < ZONE_COUNT; i++)
            {
                zoneClocks[i] += duration;
            }
        }
        else
        {
            char* zoneString = token;
            char* sDuration = strtok(NULL, " ");
            if (sDuration == NULL)
            {
                putsUart0("Missing duration\n> ");
                return;
            }
            duration = asciiToUint8(sDuration);
            zone = strtok(zoneString, ",");
            while(zone)
            {
                if (strcmp(zone, "1") == 0)
                {
                    zoneClocks[0] += duration;
                }
                else if (strcmp(zone, "2") == 0)
                {
                    zoneClocks[1] += duration;
                }
                else if (strcmp(zone, "3") == 0)
                {
                 zoneClocks[2] += duration;
                }
                else
                {
                    putsUart0("Invalid Zone: ");
                    putsUart0(zone);
                    putcUart0('\n');
                }
                zone = strtok(NULL, ",");
            }
        }
    }

    // Pause zone
    else if (strcmp(topic, "irripause") == 0)
    {
        if (strchr(token, '_') != NULL)
        {
            // Format is: 3-bit mask + underscore + duration (e.g., 101_60)
            char *zoneMask = strtok(token, "_");
            char *sDuration = strtok(NULL, "_");

            if (zoneMask == NULL || sDuration == NULL)
            {
                putsUart0("Invalid format. Expected e.g., 101_60\n> ");
                return;
            }

            duration = asciiToUint8(sDuration);

            if (strlen(zoneMask) != 3)
            {
                putsUart0("Invalid zone mask length. Use 3 digits (e.g., 110)\n> ");
                return;
            }

            for (i = 0; i < ZONE_COUNT; i++)
            {
                if (zoneMask[i] == '1')
                {
                    zonePause[i] = duration;
                }
            }
        }
        else if (strcmp(token, "all") == 0)
        {
            token = strtok(NULL, " ");
            if (token == NULL)
            {
                putsUart0("Missing duration\n> ");
                return;
            }
            duration = asciiToUint8(token);
            for (i = 0; i < ZONE_COUNT; i++)
            {
                zonePause[i] = duration;
            }
        }
        else
        {
            char* zoneString = token;
            char* sDuration = strtok(NULL, " ");
            if (sDuration == NULL)
            {
                putsUart0("Missing duration\n> ");
                return;
            }
            duration = asciiToUint8(sDuration);
            zone = strtok(zoneString, ",");
            while(zone)
            {
                if (strcmp(zone, "1") == 0)
                {
                    zonePause[0] = duration;
                }
                else if (strcmp(zone, "2") == 0)
                {
                    zonePause[1] = duration;
                }
                else if (strcmp(zone, "3") == 0)
                {
                    zonePause[2] = duration;
                }
                else
                {
                    putsUart0("Invalid Zone: ");
                    putsUart0(zone);
                    putcUart0('\n');
                }
                zone = strtok(NULL, ",");
            }
        }
    }

    // Reset to default
    else if (strcmp(topic, "irrireset") == 0)
    {
        unit = 0;
        stopTimer(irrigationTick);
        startPeriodicTimer(irrigationTick, UNIT_SECONDS);
        deepSleep = false;
        hazard = false;
        for (i = 0; i < ZONE_COUNT; i++)
        {
            zoneClocks[i] = 0;
            zonePause[i] = 0;
            zoneToggle[i] = false;
        }
    }

    // Output status
    else if (strcmp(topic, "irristatus") == 0)
    {
        char buff[100];
        if (hazard)
        {
            putsUart0("Hazards Detected, Irrigation paused\n");
        }
        else
        {
            putsUart0("No Hazards\n");
        }

        putsUart0("System Units: ");
        switch(unit)
        {
            case 0: putsUart0("Seconds\n"); break;
            case 1: putsUart0("Minutes\n"); break;
            case 2: putsUart0("Hours\n"); break;
            default: putsUart0("what did you do???\n"); break;
        }

        if (deepSleep)
        {
            putsUart0("System Sleep Mode: Deep Sleep (manual wake required)\n");
        }
        else if (sleep > 0)
        {
            snprintf(buff, 100, "System Sleep Mode: sleeping for %d %s(s)\n", sleep,
                     unit == 0 ? "second" : unit == 1 ? "minute" : "hour");
            putsUart0(buff);
        }
        else
        {
            putsUart0("System Sleep Mode: Awake\n");
        }

        for (i = 0; i < ZONE_COUNT; i++)
        {
            snprintf(buff, 100, "Zone %-10d\tStatus: %-10s\tSpray: %-10d\tPause: %-10d\tToggle: %-10s\n",
                     i + 1,
                     hazard ? "Hazard" : zoneToggle[i] ? "Toggled" : deepSleep ? "Sleeping Deeply" : (sleep > 0) ? "Sleeping" : zonePause[i] != 0 ? "Paused" : zoneClocks[i] == 0 ? "Off" : "On",
                     zoneToggle[i] ? -1 : zoneClocks[i],
                     deepSleep ? -1 : sleep > 0 ? sleep : zonePause[i],
                     zoneToggle[i] ? "Enabled" : "Disabled");
            putsUart0(buff);
        }
    }

    // Set Internal Hazard Flag
    else if (strcmp(topic, "irrihazard") == 0)
    {
        token = strtok(data, " ");
        if(strcmp(token, "true") == 0 || strcmp(token, "on") == 0 || strcmp(token, "1") == 0)
        {
            hazard = true;
        }
        else if(strcmp(token, "false") == 0 || strcmp(token, "off") == 0 || strcmp(token, "0") == 0)
        {
            hazard = false;
        }
        else if(token == NULL)
        {
            hazard = !(hazard);
        }
        else
        {
            putsUart0("Invalid Command");
        }
    }

    // Start zone, set clocks to duration
    else if (strcmp(topic, "irristart") == 0)
    {
        if (strchr(token, '_') != NULL)
        {
            // Format is: 3-bit mask + underscore + duration (e.g., 010_120)
            char *zoneMask = strtok(token, "_");
            char *sDuration = strtok(NULL, "_");

            if (zoneMask == NULL || sDuration == NULL)
            {
                putsUart0("Invalid format. Expected e.g., 010_120\n> ");
                return;
            }

            duration = asciiToUint8(sDuration);

            // Mask format: zoneMask[0] = Z1, [1] = Z2, [2] = Z3
            if (strlen(zoneMask) != 3)
            {
                putsUart0("Invalid zone mask length. Use 3 digits (e.g., 110)\n> ");
                return;
            }

            for (i = 0; i < ZONE_COUNT; i++)
            {
                if (zoneMask[i] == '1')
                {
                    zoneClocks[i] = duration;
                }
            }
        }
        else if (strcmp(token, "all") == 0)
        {
            token = strtok(NULL, " ");
            if (token == NULL)
            {
                putsUart0("Missing duration\n> ");
                return;
            }
            duration = asciiToUint8(token);
            for (i = 0; i < ZONE_COUNT; i++)
            {
                zoneClocks[i] = duration;
            }
        }
        else
        {
            char* zoneString = token;
            if (zoneString == NULL)
            {
                putsUart0("Invalid Zones\n> ");
                return;
            }
            char* sDuration = strtok(NULL, " ");
            if (sDuration == NULL)
            {
                putsUart0("Missing duration\n> ");
                return;
            }
            duration = asciiToUint8(sDuration);
            zone = strtok(zoneString, ",_");
            while(zone)
            {
                if (strcmp(zone, "1") == 0)
                {
                    zoneClocks[0] = duration;
                }
                else if (strcmp(zone, "2") == 0)
                {
                    zoneClocks[1] = duration;
                }
                else if (strcmp(zone, "3") == 0)
                {
                    zoneClocks[2] = duration;
                }
                else
                {
                    putsUart0("Invalid Zone: ");
                    putsUart0(zone);
                    putcUart0('\n');
                }
                zone = strtok(NULL, ",");
            }
        }
    }

    // Stop zone, reset clocks to 0
    else if (strcmp(topic, "") == 0)
    {
        if (strlen(token) == 3 && (token[0] == '0' || token[0] == '1'))  // bitmask check
            {
                // Format: 3-digit binary like 101
                for (i = 0; i < ZONE_COUNT; i++)
                {
                    if (token[i] == '1')
                    {
                        zoneClocks[i] = 0;
                    }
                }
            }
        else
        {
        zone = strtok(token, ",");
        while(zone)
        {
            if (strcmp(zone, "1") == 0)
            {
                zoneClocks[0] = 0;
            }
            else if (strcmp(zone, "2") == 0)
            {
                zoneClocks[1] = 0;
            }
            else if (strcmp(zone, "3") == 0)
            {
                zoneClocks[2] = 0;
            }
            else if (strcmp(zone, "all") == 0)
            {
                for (i = 0; i < ZONE_COUNT; i++)
                {
                    zoneClocks[i] = 0;
                }
            }
            else
            {
                putsUart0("Invalid Zone: ");
                putsUart0(zone);
                putcUart0('\n');
            }
            zone = strtok(NULL, ",");
        }
        }
    }

    // Stop zone, reset clocks to 0
    else if (strcmp(topic, "irristop") == 0)
    {
        if (strlen(token) == ZONE_COUNT && (token[0] == '0' || token[0] == '1'))  // bitmask check
            {
                // Format: 3-digit binary like 101
                for (i = 0; i < ZONE_COUNT; i++)
                {
                    if (token[i] == '1')
                    {
                        zoneClocks[i] = 0;
                    }
                }
            }
        else
        {
            zone = strtok(token, ",");
            while(zone)
            {
                if (strcmp(zone, "1") == 0)
                {
                    zoneClocks[0] = 0;
                }
                else if (strcmp(zone, "2") == 0)
                {
                    zoneClocks[1] = 0;
                }
                else if (strcmp(zone, "3") == 0)
                {
                    zoneClocks[2] = 0;
                }
                else if (strcmp(zone, "all") == 0)
                {
                    for (i = 0; i < ZONE_COUNT; i++)
                    {
                        zoneClocks[i] = 0;
                    }
                }
                else
                {
                    putsUart0("Invalid Zone: ");
                    putsUart0(zone);
                    putcUart0('\n');
                }
                zone = strtok(NULL, ",");
            }
        }
    }

    // Start sleeping
    else if (strcmp(topic, "irrisleep") == 0)
    {
        if (strcmp(data, "deep") == 0)
        {
            deepSleep = true;
        }
        else
        {
            sleep = asciiToUint8(data);
        }
    }

    // Toggle Zone
    else if (strcmp(topic, "irritoggle") == 0)
    {
        bool globalToggle = zoneToggle[0];
        if (strcmp(token, "all") == 0)
        {
            token = strtok(NULL, " ");
            if (token == NULL)
            {
                for (i = 0; i < ZONE_COUNT; i++)
                {
                    zoneToggle[i] = !globalToggle;
                }
            }
            else if(strcmp(token, "true") == 0 || strcmp(token, "on") == 0)
            {
                for (i = 0; i < ZONE_COUNT; i++)
                {
                    zoneToggle[i] = true;
                }
            }
            else if (strcmp(token, "false") == 0 || strcmp(token, "off") == 0)
            {
                for (i = 0; i < ZONE_COUNT; i++)
                {
                    zoneToggle[i] = false;
                }
            }
            else
            {
                putsUart0("Invalid Toggle: ");
                putsUart0(token);
                putsUart0(". Use true/false\n");
            }
        }
        else
        {
            char* zoneString = token;
            char* boolean = strtok(NULL, " ");
            zone = strtok(zoneString, ",");
            while(zone)
            {
                uint8_t index = asciiToUint8(zone);
                if (0 < index && index <= ZONE_COUNT)
                {
                    if (boolean == NULL)
                    {
                        zoneToggle[index - 1] = !globalToggle;
                    }
                    else if (strcmp(boolean, "true") == 0 || strcmp(boolean, "on") == 0)
                    {
                        zoneToggle[index - 1] = true;
                    }
                    else if (strcmp(boolean, "false") == 0 || strcmp(boolean, "off") == 0)
                    {
                        zoneToggle[index - 1] = false;
                    }
                    else
                    {
                        putsUart0("Invalid Toggle: ");
                        putsUart0(boolean);
                        putsUart0(". Use true/false\n");
                    }
                }
                else
                {
                    putsUart0("Invalid Zone: ");
                    putsUart0(zone);
                    putcUart0('\n');
                }
                zone = strtok(NULL, ",");
            }
        }
    }

    // Change Irrigation Unit
    else if (strcmp(topic, "irriunit") == 0)
    {
        if (strcmp(token, "second") == 0 || strcmp(token, "seconds") == 0)
        {
            unit = 0;
            stopTimer(irrigationTick);
            startPeriodicTimer(irrigationTick, UNIT_SECONDS);
        }
        else if (strcmp(token, "minute") == 0 || strcmp(token, "minutes") == 0)
        {
            unit = 1;
            stopTimer(irrigationTick);
            startPeriodicTimer(irrigationTick, UNIT_MINUTES);
        }
        else if (strcmp(token, "hour") == 0 || strcmp(token, "hours") == 0)
        {
            unit = 2;
            stopTimer(irrigationTick);
            startPeriodicTimer(irrigationTick, UNIT_HOURS);
        }
        else
        {
            putsUart0("Invalid Unit: ");
            putsUart0(token);
            putcUart0('\n');
        }
    }

    // Start sleeping
    else if (strcmp(topic, "irriwake") == 0)
    {
        sleep = 0;
        deepSleep = false;
    }

    else
    {
        putsUart0("Invalid Command\n");
    }
    putsUart0("> ");
}

void Timer4A_Handler(void)
{
    tickIsr();
}
