Homegear
========

Homegear is a free and open source program to interface your home automation devices or services with your home automation software or scripts. Currently supported are the following device families/systems:

* KNX
* EnOcean
* Beckhoff BK90x0
* HomeMatic BidCoS
* HomeMatic Wired
* Insteon
* Philips Hue
* Sonos
* MAX!
* Intertechno
* Kodi
* IP cams
* more to come...
* New devices can be created with ease using XML and PHP.

Homegear features the following interfaces to control your devices - all with SSL support:

* XML-RPC
* Binary-RPC
* JSON-RPC
* MQTT
* WebSockets with PHP session authentication
* HTTP (GET and POST) 

If needed new interfaces can easily be added to the source code. Homegear also features:

* A built-in rich featured web server with PHP 7 and IP cam proxy support. Together with WebSockets and the script engine you can easily create web pages to bidirectionally interact with all devices known to Homegear.
* A built-in script engine using PHP 7:

	* All devices and device functions are directly accessible.
	* All PHP modules can be used:

		* Thread support using the PHP module "pthreads".
		* Low level peripheral support:
		
			* Directly access serial devices, I²C devices and GPIOs.
			* You immediately get notified on new data and GPIO state changes. No polling is necessary.
			* Using threads you can implement bidirectional and event driven communication. 
	* A base library to easily implement your own device families.
	* XML device description files with PHP script support to easily implement individual devices.
	* Support for customized licensing modules supporting module online activation, license verification and script encryption.

Homegear is written to make it as easy as possible to integrate new devices making them accessible through all the above interfaces.

For more information, please visit https://homegear.eu, https://doc.homegear.eu and https://ref.homegear.eu. Please post your questions in our forum on https://forum.homegear.eu.
