arduino-sensor
==============

Arduino code that allows remote GPIO configuration over nrf24l01 wireless modules.
The setup uses two nrf24l01 modules-

* One on the raspberry pi
* One on the arduino which is running this code.

To setup the nrf24l01 module, use instructions at http://conoroneill.net/arduino-and-raspberry-pi-communicating-over-2-4ghz-with-cheap-nrf24l01-modules/

For sending commands from Pi to Arduino wirelessly, modify and use one of the examples in https://github.com/stanleyseow/RF24/tree/master/librf24-rpi/librf24/examples

Command Syntax
==============

Initial configuration
-------------

Once the arduino is up and running this code, it starts waiting for configuration.
Send it the following command to configure-

    list_of_input_pins;list_of_output_pins

For example-

    2,3;4,5

means that make pins 2 and 3 as input and make 4 and 5 output.

Once configured, this code sends out the values on the input pins over the nrf24l01 channel.

Setting output pins
-------------

Once configured, you can send it a command in this syntax to set values on output pins-

    2,1:3,0

This will make pin 2 HIGH and pin 3 LOW.

Note that,

* The pins must be one of the pins that have been in the list of output pins while configuring
* If you want to use Analog pins, make sure to use 14 for A0, 15 for A1 and so on.

Thanks To
==============

* RF24 library from https://github.com/stanleyseow/RF24
* Awesome setup instructions from http://conoroneill.net/arduino-and-raspberry-pi-communicating-over-2-4ghz-with-cheap-nrf24l01-modules/#comment-5997

