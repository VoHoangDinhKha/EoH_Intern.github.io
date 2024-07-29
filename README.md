# Mini weather station combined with decorative clock
## Description
Using the SPI protocol of the ESP32 to communicate with the ILI9225 TFT display with a resolution of 176x220. The screen is set up with two main interfaces: Interface 1 displays the clock time, lunar calendar, moon phase, and temperature and humidity data from the DHT22; Interface 2 shows weather information in the area using the OpenWeather API. Switching between the two interfaces is controlled by a button. When the button is pressed, a specific section of the screen will change to display weather information. Additionally, we can view these results on the Web and Mobile using the E-RA Platform.
## Project functions
### ***Real-Time Clock***
Real-time data is stored in the DS1307 module when there is a Wi-Fi connection. The system will compare preset conditions to update accurate time data from the NTP server. When the network connection is lost, the time will still run accurately based on the data stored from the previous update.  

![demo1](https://github.com/user-attachments/assets/251e206d-7713-4cf9-8c87-69cb065625a9)

### ***Display temperature and humidity sensor data***
Temperature and humidity information around the clock will be collected from the DHT22 sensor and displayed on the screen. The collected data will be updated to the E-Ra platform.

![demo2](https://github.com/user-attachments/assets/be470469-87d7-4804-b6b4-dcb561c8e416)

### ***Update the lunar calendar and moon phase in real-time***
Through calculation algorithms, the system can accurately compute the lunar calendar and moon phases using the input data from the Gregorian calendar.

![demo3](https://github.com/user-attachments/assets/2797e1ab-e094-46d1-b7b8-002ba06be1c2)

### ***Update weather data based on the selected area***
Weather parameters are updated from the OpenWeather API with user-selected coordinates. Based on the API data, display icons and numerical data on the LCD screen for easy viewing.

![demo4](https://github.com/user-attachments/assets/8dbc9462-9156-4a32-b91e-c03786418ba3)

The weather icons are accurately updated based on the parameters collected from the API. When there is a change in the weather, the icons will also be updated accordingly. Afterwards, all data is updated to the E-Ra platform.

![demo5](https://github.com/user-attachments/assets/62be16b5-c3f2-41fa-ab62-f576873e97b6)

### ***Additional features***
Alarm: In addition to the main functions, the system also features an alarm function using a buzzer according to the time set by the user.
RTOS: Using FreeRTOS to handle tasks for LED lights at different frequencies and to process tasks for uploading data to the E-Ra platform.

## Demo
__[Link Video](https://www.youtube.com/watch?v=-zrkdetbYBs)__
