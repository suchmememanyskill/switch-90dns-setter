// Include the most common headers from the C standard library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include the main libnx system header, for Switch development
#include <switch.h>

const char *regions[] = {
    "Japan",
    "America",
    "Europe",
    "Australia/New Zealand",
    "Hong Kong/Taiwan/Korea",
    "China",
};

void rebootSystem(){
    spsmInitialize();
    spsmShutdown(true);
    spsmExit();
    printf("Something went wrong while rebooting!");
}

int main(int argc, char* argv[])
{
    consoleInit(NULL);

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);

    PadState pad;
    padInitializeDefault(&pad);

    bool disableX = false;
    Result res = 0;
    SetRegion region;
    u32 europeDns = 0xdb8daca3; // 163.172.141.219
    u32 americaDns = 0x4d79f6cf; // 207.246.121.77

    u32 primaryDns = 0;
    u32 secondaryDns = 0;

    printf("90dns Setter tool\n\n");

    res = setInitialize();
    if (res){
        printf("Failed to initialise set! Err %x\n", res);
        disableX = true;
    }
    else {
        res = setsysInitialize();
        if (res){
            printf("Failed to initialise setsys! Err %x\n", res);
            disableX = true;
        }
        else {
            res = setGetRegionCode(&region);
            if (res){
                printf("Failed to get region! Err %x\n", res);
                disableX = true;
            }
            else {
                if (region <= SetRegion_CHN){
                    printf("Detected %s as region\n", regions[region]);
                    if (region == SetRegion_USA){
                        printf("American dns will be used as primary\n");
                        primaryDns = americaDns;
                        secondaryDns = europeDns;
                    }
                    else {
                        printf("Europe dns will be used as primary\n");
                        primaryDns = europeDns;
                        secondaryDns = americaDns;
                    }
                }
                else {
                    printf("Unknown region? Europe dns will be used as primary\n");
                    primaryDns = europeDns;
                    secondaryDns = americaDns;
                }
                
            }
        }
    }

    printf("\nPress + To exit\nPress Y to reboot the system\n");
    if (!disableX)
        printf("Press X to apply 90dns to all wifi networks\n");

    while (appletMainLoop())
    {
        // Scan the gamepad. This should be done once for each frame
        padUpdate(&pad);

        // padGetButtonsDown returns the set of buttons that have been
        // newly pressed in this frame compared to the previous one
        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_Plus)
            break; // break in order to return to hbmenu

        if (kDown & HidNpadButton_Y){
            rebootSystem();
        }

        if (kDown & HidNpadButton_X && !disableX){
            consoleInit(NULL);

            SetSysNetworkSettings* wifiSettings = malloc(sizeof(SetSysNetworkSettings) * 0x200);

            if (wifiSettings != NULL){
                s32 entryCount = 0;
                res = setsysGetNetworkSettings(&entryCount, wifiSettings, 0x200);
                if (res){
                    printf("Getting wifi networks failed! Err %x\n", res);
                }
                else {
                    printf("Wifi networks found: %d\n", entryCount);
                    for (int i = 0; i < entryCount; i++){
                        wifiSettings[i].primary_dns = primaryDns;
                        wifiSettings[i].secondary_dns = secondaryDns;
                        wifiSettings[i].auto_settings &= ~SetSysAutoSettings_AutoDns;
                    }

                    if (entryCount){
                        res = setsysSetNetworkSettings(wifiSettings, entryCount);
                        if (res){
                            printf("Setting wifi networks failed! Err %x\n", res);
                        }
                        else {
                            printf("Done!\nTo apply the changes, reboot the system\n");
                        }
                    }
                }
            }
            else {
                printf("Memory allocation failed!\n");
            }

            disableX = true;
            printf("\nPress + To exit\nPress Y to reboot the system\n");
            free(wifiSettings);
        }

        // Update the console, sending a new frame to the display
        consoleUpdate(NULL);
    }

    // Deinitialize and clean up resources used by the console (important!)
    consoleExit(NULL);
    setExit();
    setsysExit();
    return 0;
}
