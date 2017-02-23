# Aquarium-Controller
Arduino Outlet Controller with Temperature and Time Events

This controller is designed with the saltwater aquarist in mind, although controlling outlets has much broader applications. I guess I should also point out that this is my first Arduino project, so there is probably lots of room for improvement in efficiency, features and flexibility - I welcome your feedback!

Initial development was done on an Arduino Uno R3 board with supporting modules and sensors as noted in the sketch. The sketch is pretty straight forward, and I've added comments to describe what's going on and how you can customize this project to suit your needs.

The initial version of this project has four relays, each controlling an outlet:
Relay1 - turns ON when I want the protein skimmer and refugium to turn OFF (during peak viewing hours)
Relay2 - turns ON when a set temperature (81 degrees F) is detected, and OFF when the temp goes below 80 degrees
Relay3 - 50% dutycycle ON/OFF, set at 30 minutes
Relay4 - Currently unused
