# AIR QUALITY MONITOR

Air Quality Monitors/displays are set up at several public spots in Chandigarh with
the main aim to make people aware of the present air quality. But the data 
displayed is around 1-3 days old. Therefore, in order to spread the right 
information at the right time, we devised an IOT based solution.

We have categorized our solution into two categories:-

1. Making a Low cost STATIC hardware device (without GPS) which calculates the
 Live AQI data and sends it to a cloud, where it gets stored. This data can be 
 accessed by anyone. We can setup such static hardware devices at all points of 
 evaluation and the live data can be collected on a cloud and projected using a 
 website or mobile application to the users.
2. Making a MOBILE device similar to the first model but with additional GPS for 
location tracking. This device can be attached to a mobile vehicle (car, bus etc.) 
which traverses through the points of evaluation regularly and stores the data on 
the cloud.

Using AI and Deep Learning , we can transform this data into an application which
 can predict which areas will have high pollution during certain period of a day. 
 We can combine this prediction with Google maps to prompt the user to take an 
 alternative route of traversal.

# TECHNOLOGY STACK:-

1. esp8266 module - for posting the data to cloud
2. MQ series sensors - for gas concentration calculation(e.g.- MQ-135, MQ-7, MQ2, 
   MQ-131 etc.)
3. GPS module - for tracking the location of device
4. Particulate matter sensors - like PPD42NS(dust sensor) for finding the 
   concentration of particulate matter in air.
5. Grove - Multichannel Gas Sensor
6. Nova PM Sensor SDS011 High Precision Laser PM2.5
7. Arduino UNO for calculating the data.
8. Deep Learning for predicting the air quality. 



![alt text](https://raw.githubusercontent.com/pallav1299/Air_Quality_monitor/master/Block_Diagram.png)

For project details go to this link:
http://dic.puchd.ac.in/?p=1938
