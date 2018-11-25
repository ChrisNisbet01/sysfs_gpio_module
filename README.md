Provides a UBUS interface to 'sysfs' GPIO.

Configure the application using a json file. This allows the user to specify which
GPIO are inputs and which are outputs. It also allows the user to configure how 
many of each are available using the UBUS interface.

See gpio_config.json for an example.

UBUS calls

The obtain the type and number of the GPIO types supported by the module:
```
ubus call sysfs.gpio counts
```

Typical response:
```
{
	"counts": [
		{
			"io type": "binary-input",
			"count": 8
		},
		{
			"io type": "binary-output",
			"count": 8
		}
	]
}
```
This indicates that the appication is configured with 8 binary-inputs and
8 binary outputs.

Read a single GPIO:
This is a call to read binary input 0
```
ubus call sysfs.gpio get "{\"gpios\":[{\"io type\":\"binary-input\", \"instance\":0}]}"
```
Typical response:
```
{
	"results": [
		{
			"value": true,
			"io type": "binary-input",
			"instance": 0,
			"result": true
		}
	]
}
```

The "result" field indicates if the IO was able to be read.
The "value" field indiates the state of the input (true for ON, false for OFF).
The "io type" and "instance" fields are taken from the request and indicate which
binary-input was read.

Read multiple GPIO:
This is a call to read binary inputs 0 and 1
```
ubus call sysfs.gpio get "{\"gpios\":[{\"io type\":\"binary-input\", \"instance\":0},{\"io type\":\"binary-input\", \"instance\":1}]}"
```
Typical response:
```
{
	"results": [
		{
			"value": true,
			"io type": "binary-input",
			"instance": 0,
			"result": true
		},
		{
			"value": false,
			"io type": "binary-input",
			"instance": 0,
			"result": true
		}
	]
}
```
In this case binary-input 0 is ON and binary-input 0 is OFF. Both inputs were
read successfully.

To set outputs:
```
call sysfs.gpio set "{\"gpios\":[{\"io type\":\"binary-output\", \"instance\":0, \"value\":true},{\"io type\":\"binary-output\", \"instance\":1,\"value\":false}]}"
```

Typical response:
```
{
	"results": [
		{
			"io type": "binary-output",
			"instance": 0,
			"result": true
		},
		{
			"io type": "binary-output",
			"instance": 1,
			"result": false
		}
	]
}
```

In this case, binary-output 0 was written successfully but binary-output 1 was not.

Note that there is an older interface that only allows a single input/output to
be read/written with each ubus call. This interface described here should cut down 
on traffic where the caller wishes to update multiple IO with a single call.
At this point there is no intention to support reading and writing IO with a single
message. Once all GPIO modules have been updated to use the 'multiple' read/write 
API the old API will be removed.

