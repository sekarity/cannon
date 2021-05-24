Hi all, thanks for checking out the CANnon peripheral clock gating attack. You are one step closer to learning about a new software-only attack that can mimic a physical-access attack against a modern in-vehicle network.

Please use this code at your own risk. If used without following the processes described in the paper (i.e. please don't send diagnostic messages), there is potential to cause permanent damage. Please contact me if you require any help, especially if using a personal vehicle.

This repository is the **first** version of the techniques described in the CANnon paper at IEEE Security & Privacy '21. These files are demonstrations of CANnon's flexibility when it comes to both controlling the clock of a compromised ECU and targeting the messages of a victim ECU.

There are currently _two_ main demonstrations:
- an example of crafting variable length error frames, and
- an example of targeting a victim ECU until it enters another error state.
    
The code for these modules are written for the Arduino Due.

The wiring diagrams will be made available but the parts list is below:
- Arduino Due microcontroller
- TI VP232 CAN transceiver (any transceiver should work)
- OBD-II breakout

If you need assistance in building a demonstration or learning more about the attack, please reach out to me.

If you have any suggestions or requests, please email me (see paper) and I'm happy to work with you!

Also, please check out how this project (and others) plays a significant role in my PhD thesis: https://youtu.be/XisC_Rk0uXg
