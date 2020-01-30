# MOOS-uReflector
MOOS endpoint for data reflector service

# Requirements & Dependencies
* libcurl
* rapidjson (https://github.com/Tencent/rapidjson/)
* cppcodec (https://github.com/tplgy/cppcodec)

# Theory of Operation
The reflector has a set of channels, each of which can be read or written to via REST calls. Each channel maps to a single REDIS key, and may in principle contain any data that can be stored in a REDIS byte string, but typical payload is a JSON string. Each write to a channel overwrites all previous data. Therefore, the typical mode of operation is that each channel is a one-way one-to-one or one-to-many communication on each channel. Therefore, a bidrectional communication, such as between ship and shore, takes two channels -- a ship to shore channel and a shore to ship channel.

Each subscribed message is packaged into a corresponding JSON type. DOUBLEs are encoded as JSON numbers; STRINGs are encoded as JSON strings, and BINARYs are base-64 encoded as JSON strings. Encoded binary strings are prefaced with \*\*DEADBEEF\*\*  

# Configuration

## HOST
This is the internet address of the reflector service.

## TRANSMIT_ID
This is the ID used to transmit data to the reflector.

## RECEIVE_ID
This is the ID used to fetch data from the reflector.

## KEY_FILE
This is the path to the file with the keys for the reflector.

## VARIABLE
The name of a variable to subscribe to locally and pack into the JSON string sent to the reflector.

# Subscribed variables
The reflector subscribes to all the variables named by VARIABLE fields in the configuration file.

# Published Variables

## INCOMING_VARIABLES
The reflector publishes the value of any numeric or string field in the incoming JSON as an outgoing variable under the name of the key.

## REFLECTOR_CONFIG_WARNING
This variable is published if there is a problem in the configuration.

## REFLECTOR_UNHANDLED_CONFIG_WARNING
This variable is published if there is an unhandled key in the configuration.

## REFLECTOR_RUN_WARNING
This variable is published if there is unhandled mail or some other run-time error.
