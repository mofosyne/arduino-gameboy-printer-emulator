# arduino-gameboy-printer-emulator
Code to emulate a gameboy printer via the gameboy link cable


# Gameboy Printer Timing

```
                       1.153ms
        <--------------------------------------->
         0   1   2   3   4   5   6   7             0   1   2   3   4   5   6   7
     __   _   _   _   _   _   _   _   ___________   _   _   _   _   _   _   _   _
CLK:   |_| |_| |_| |_| |_| |_| |_| |_|           |_| |_| |_| |_| |_| |_| |_| |_| 
DAT: ___XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX____________XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX_
       <-->                           <---------->
       127.63 us                         229.26 us

```


* Clock Frequency: 8kHz (127.63 us)
* Transmission Speed: 867 baud (1.153ms per 8bit symbol)
* Between Symbol Period: 229.26 us


