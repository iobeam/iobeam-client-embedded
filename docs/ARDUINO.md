# Using iobeam on Arduino #

**[iobeam](http://iobeam.com)** is a data platform for
connected devices. 

These instructions are about connecting to the iobeam Cloud on the
Arduino platform. For more information on the iobeam Cloud, please 
read our  [full API documentation](http://docs.iobeam.com).

*Please note that we are currently invite-only. You will need an invite 
to generate a valid token and use our APIs. 
(Sign up [here](http://iobeam.com) for an invite.)*


## Before you start ##

Before you can start sending data to the iobeam Cloud, you'll need a 
`project_id` and  `project_token` (with write-access enabled) for a 
valid **iobeam** account. You can get these easily with our
[command-line interface tool](https://github.com/iobeam/iobeam).

You'll also need the [Arduino IDE](http://www.arduino.cc/en/Main/Software).

## Installation ##

This library has been designed to work with the Arduino IDE and has
been tested on version 1.6.3.

First, you'll want to clone thie repo into the directory where you
keep your Arduino libraries. This is called `libraries` and, e.g.,
exists at `~\Documents\Arduino` on Windows.

	git clone https://github.com/iobeam/iobeam-client-embedded.git Iobeam

This tutorial assumes you call the directory `Iobeam`. After this is
done, restart the Arduino IDE to make it available in your list of
libraries.

To use the libary in your sketch, you'll need to include it:
	
	#include <Iobeam.h>

## Sample Sketch ##

In this repo we have included an example of a simple sketch using the
libary located at `examples/BasicIobeam.ino`. This sketch simply sends
a data point (the analog reading of pin 0) every 15s. The rest of the
documentation will explore basic iobeam concepts by using this as an
example.

## Getting Started ##

This section deals with actually writing a sketch using the iobeam
libary, including configuring the client for your project, initializing
the client and registering the device, and sending data.

### Project configuration ###

As mentioned above, you'll need a `project_id` and `project_token` to
send data to iobeam. We will assume you have signed up and gotten those
via the CLI. Now, to set up your sketch, near the top you'll want 
something similar to what we have in our `BasicIobeam` sketch:

	#include <EEPROM.h>
	#include <Ethernet.h>
	#include <Iobeam.h>

	// [ethernet setup code here]

	// Necessary for iobeam. The project token should be PROGMEM to save RAM.
	#define PROJECT_ID [your project id]
	PROGMEM const char token[] = "your project token";

	// Initialize the Ethernet and iobeam libraries.
	EthernetClient client;
	Iobeam iobeam(client);

You'll see we define a constant PROJECT_ID that you'll replace with your
`project_id` (which should be an integer), and a `char[]` called `token`
for our project token. We use the macro `PROGMEM` to store this is
program memory space rather than RAM as these can be somewhat long.
(*Note: Tokens expire after 30 days and need to manually reloaded currently. We are working on automatic refresh.*)

Afterwards we create an `EthernetClient` and use it to create our
`Iobeam` object called `iobeam`. The `Iobeam` class can take any object
that is an Arduino `Client`, so you can do something similar if you're
using WiFi instead.

### Initializing and registration ###

You'll want to finish initializing the `Iobeam` object in `setup()`. In
our `BasicIobeam` sketch, it looks like this:

	iobeam.init(PROJECT_ID, token, 0);
	iobeam.registerDevice(0);
	iobeam.startTimeKeeping();

You can see we call `init()` with our PROJECT_ID and token. The third
argument, `0`, is the EEPROM memory address where our library should
look for its device ID. We use EEPROM to persist this device ID
across reloads so the device doesn't keep registering new IDs. 

If no device ID is found at the address, it will not be set and
you'll need to register one (the next line).

Similarly, `registerDevice()` takes a memory address of where to write
the device ID it gets from iobeam. The return value of
`registerDevice()` will tell you have many bytes of EEPROM were
written, so if you need EEPROM you know where you can write. This
function returns immediately if the device ID is already in EEPROM.

Finally, `startTimeKeeping()` has the iobeam client approximate itself
to global time by contacting the iobeam cloud for the current time. 
This is only necessary if you are interested in your timestamps being
expressed relative to global time. This method is a rough approximation
so if you need something more precise, you will have to manage and 
provide timestamps with your data yourself.

Now we're ready to start sending data.

### Sending data points ###

Sending data to the iobeam cloud is very simple:

	int temp = analogRead(0);
	boolean success = iobeam.send("analog", temp);

First we get a value we want to send and store it in `temp`. Then, to
send that value, we simply call `send()` and supply on a series name
for this data type (for example, if you were measuring temperature you
might choose the name "temperature") and the data value, either as an
integer or a real number. `send()` returns a boolean of whether the
value was successfully sent to iobeam or not.

If you are providing your own timestamps, you will first need to create
a `Iobeam::Timeval` struct with your timestamp. `Timeval` has two 
fields, both 32-bit usigned ints, called `sec` and `msec` which
correspond to the number of seconds and milliseconds respectively for
this timestamp. Your `send()` call would then look like this:

	int temp = analogRead(0);
	Iobeam::Timeval tv;
	tv.sec = ...;
	tv.msec = ...;
	boolean success = iobeam.send("analog", tv, temp);

These instructions should be enough to get you started in using
iobeam on Arduino!

### Full Example ###

Here's the full source code for our example:

	#include <EEPROM.h>
	#include <Ethernet.h>
	#include <Iobeam.h>

	// [ethernet setup code here]

	// Necessary for iobeam. The project token should be PROGMEM to save RAM.
	#define PROJECT_ID [your project id]
	PROGMEM const char token[] = "your project token";

	// Initialize the Ethernet and iobeam libraries.
	EthernetClient client;
	Iobeam iobeam(client);

	void setup() {
 		// [ethernet setup]
 
 		// iobeam initialization
 		iobeam.init(PROJECT_ID, token, 0);
 		iobeam.registerDevice(0);
		iobeam.startTimeKeeping();
	}

	void loop() {
		int temp = analogRead(0);
  		boolean success = iobeam.send("analog", temp);
		delay(15000);
	}
