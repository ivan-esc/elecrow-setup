ELECROW Display Setup Tutorial
===================================
This is a guide and tutorial for setting up an ELECROW ESP32 7.0 Inch HMI Display (DIS08070H) [[specs]](https://media-cdn.elecrow.com/wiki/esp32-display-702727-intelligent-touch-screen-wi-fi26ble-800480-hmi-display.html). All files referenced throughout this tutorial are available in this same repository and are free to use. There are 5 folders in this repository to consider:  
- **Tutorial Project Assets:** Files used for creating the UI presented in this tutorial, including the entire platformIO project and UI assets, use these if you desire to follow along this tutorial or wish to have a reference vantage point.  
  
- **Ignore Images:** Images used to illustrate this readme, __ignore__.
    
- **Common Helper Assets:** Files provided for general use in various projects, I recommend using these as they will hopefully help you tidy up your future Elecrow Display projects shenanigans. (Jump to section 4 for more details about this files.)  
  
- **ui_assets_tutorial:** PlatformIO Display Project that updates UI widgets based on recieved UART data.

- **esp32-telemetry-sim:** PlatformIO ESP32 Dev Module Project that generates random telemetry and transmits it to the Display via UART.


-----------------------------------
**Tutorial Overview**
-----------------------------------
In this tutorial, we will be setting up an User Interface in the ELECROW screen, by creating a simulation of telemetry transmission for a driver screen of Shell ECO Marathon Battery-Electric Vehicles. (by communicating it with an ESP32 Dev Module). It is important to establish that this UI is **NOT ideal** to be used as an actual UI, since it introduces way too much visual clutter for the driver to manage. The tutorial's high usage of features is mostly for show and to highlight the performance, capabilities and configurations of the screen; **not for actual implementation inside of the competition.**

![Video_UI](ignore-images/elecrow_video.gif)

![Final_UI](ignore-images/squarelinefinal.png)

The ELECROW ESP32 7.0 Inch HMI Display, as the name suggests, uses an ESP32 microcontroller for handling its entire programming; meaning it can use ESPIDF or Arduino's framework through either ArduinoIDE or PlatformIO. For this tutorial, we'll be using the following softwares:
- **SquareLine Studio** (v1.4.0): It is important that you do not install nor update to newer versions. [(Guide)](https://www.elecrow.com/wiki/Get_Started_with_SquareLine_Studio.html)
- **Visual Studio Code** with PlatformIO extension installed [(Guide)](https://programarfacil.com/blog/arduino-blog/platformio/)  

This example is inspired by [Elecrow's official display example tutorial](https://www.elecrow.com/wiki/CrowPanel_ESP32_7.0-inch_with_PlatformIO.html), but it expands on it by incrementing the use of UI elements. 

**IMPORTANT!!**: The [official LVGL documentation](https://docs.lvgl.io/8.0/index.html) is probably the most useful website to consult when working on the functions that control the entire behavior of the widgets and elements used.

This tutorial will use an extra ESP32 DEV Module used for simulation of what would otherwise be the Telemetry Board (as well as some extra simple components for testing simulation): 

- Common Potenciometer
- Regular Push Button
- Several wires
- 4 pin JST connector cables. 

In this example tutorial, the objective is to leave a clear explanation on the implementation methods of the following elements for future display projects:
- UI design
- Updating text labels
- Managing Arcs and Sliders
- Masked slider indicators
- Non-blocking ESP32 timers
- Plotting in Charts
- Display button interactions
- UART Communication
- ~~CAN Communication Pending~~

In order to cover this, the tutorial is divided in the following sections. **Consider this a readme index; click into any step to go straight to it.**

1. [Important Display Considerations](#1.-Important-Display-Considerations)
2. [Invisioning and designing UI](#2.-Invisioning-and-designing-UI)
3. [SquareLine Studio Project](#3.-SquareLine-Studio-Project)
4. [Common files and General PlatformIO setup](#4.-Common-files-and-General-PlatformIO-setup)
5. [Tutorial Specific Setup and Code](#5.-Tutorial-Specific-Setup-and-Code)
6. [Display Code](#6.-Display-Code)
7. [Telemetry simulation code and circuit](7.-Telemetry-simulation-code-and-circuit)
8. [Helper Init code explanation](#8.-Helper-Init-code-explanation)
9. CAN Implementation 

-----------------------------------
## 1. Important Display Considerations
  
The display has the following pinout configuration. It is important to note that the BAT input cannot be fed by a 5v source, as one might assume based on the proximity to the USB C port; nor should it be a 3.3v entry. The recommended power source when not connected to the USB C is a **3.7v Li-Po / Li-ion battery**; that has a full-charge voltage of 4.2v
  
![Display pinout](ignore-images/pinout.png)

Although the display exposes several GPIOs, their practical usability is limited due to internal PCB routing and the ESP32-S3’s peripheral architecture.
- GPIO38 is the only truly free general-purpose pin that can be used as a digital input or output. However, unlike classic ESP32 variants where ADC1 channels are mapped to GPIOs 32–39, the ESP32-S3 maps ADC channels to GPIOs 1–10. As a result, **GPIO38 cannot be used as an analog input**, despite being within the “ADC range” of other ESP32 cores.
- The ESP32-S3 provides three UART controllers (UART0–UART2), and the pins labeled as UART0 on the silkscreen (GPIO43 and GPIO44) can be reassigned to any UART controller (or used as GPIO) thanks to the ESP32 pin matrix. However, UART0 is electrically tied to the USB-C interface used for flashing and serial monitoring. Because of this, any external device actively driving GPIO43/44 can interfere with code uploading or serial communication, even if those pins are configured as Serial1 or Serial2. This means **external UART communication must be disconnected or powered down while flashing or using the USB serial monitor.**
- **GPIO19 (SDA) and GPIO20 (SCL) must be treated as I²C-only pins.** These pins are hard-wired on the PCB to the capacitive touch controller (GT911), including external pull-up resistors to 3.3 V. Even if touch functionality is disabled in software, the touch controller remains electrically connected, adding pull-ups, capacitance, and potential bus contention.

With this in mind, we can begin the tutorial.

-----------------------------------
## 2. Invisioning and designing UI

The first towards designing any UI is establishing what is the interface actually going to show. It is important to have in mind how the information or interactive elements are going to be presented to the user. As I like to say, **intuitiveness is the most important part of any project** you intend on having some sort of user interaction where the user isnt oneself; if you have to rigurosly explain how to operate the UI, then it's not a good UI.
Not overly bombarding the user with tens of options, buttons, labels and numbers is a good idea; and looking for inspiration on other UIs (mobile apps, webpages, car screens) is always a favorable starting point.

Once again, the final product of this tutorial isnt reliable, ideal or friendly to an external user; but it is good enough to display the idea of design choices and the creative process behind them.

As this is a telemetry simulation; first we need to establish what information and interactive/visual elements will be presented; for the tutorial, I set for the following:
- Speedometer with velocity and RPMs indicator.
- Consumption in kWh.
- Efficiency in km/kWh.
- Simultaneous chart of both current and voltage consumption during the last 2 minutes.
- Current lap timer and full attempt timer 
- Lap counter and interactive button for resetting it and the timer.
- Message box for displaying custom sent messages.
- Display power indicator.

And the next step will be to do a rough sketch of the UI layout in any graphic software, considering the resolution of the display (800px x 480px). This doesn't have to be pretty, it just has to present the order that the elements are going to be displayed in. 

![Initial Rough Layout](ignore-images/rough_layout.png)

Once you have all the elements layed out and considerations in mind; you can begin designing your aesthetically pleasing user interface. Some recomendations I can give:
- As mentioned, look for inspirations of other UI designs, but do not steal! Look for mobile apps, webpages or other designs, you'll find mostly modern-esque interfaces, but for a reason, since they provide a clean and easy look for your UI.
- If you're like me and struggle with colors, I recommend pages like [Coolors](https://coolors.co/) or [Color Hunt](https://colorhunt.co/) for finding proper color palettes.
- Though the use of a professional software like Ilustrator or Figma would be prefered, it's understood that they are a bit of a hassle to setup and learn. Canva is an all time classic alternative and with your student account you have access to all of the premium features (look for Canva for Education and sign in as educator).
- For commercial purposes use royalty free icons, elements and fonts (find some at [Flaticon](https://www.flaticon.com/) and fonts in DaFont or FontSquirrel) but generally, you'll probably be fine using some of Canva's elements anyway.
- For the actual design, consider proper formatting of the labels, texts and numbers used. This is an special remark for labels of numbers that contain decimal spaces; **remember to consider how many to concatenate to, if any.**

Once you are done with your first designed prototype, you should be left with something like this (remember the screen's resolution of **800px x 480px** when making this design):
![First prototype](ignore-images/prototype1.png)

**Next step** is to be aware of what elements are "dynamic" and which ones are "static". By dynamic elements I mean everything that is going to either be an interactive element (like a button or a selection tab), label or graphic element that will be constantly changing based on recieved data; static elements are the ones that will not be affected nor modified at all. Here are the dynamic elements of my interface, each one highlighted in a red square; as you can see, there's 12 of them:

![Dynamic Assets](ignore-images/dynamic_assets.png)

And now, export a copy of the designed UI without these "dynamic" elements. You can find this file as **"bg-ui-tutorial"** in the **"tutorial-project-assest"** folder.

![Static UI](tutorial-project-assets/bg-ui-tutorial.png)

While you could technically design the entire UI directly in SquareLine Studio; making it this way not only makes it easier for you to have a clear idea of what every element is doing; but also makes the SquareLine project easier to manage and helps the rendering done by the display to be more efficient (as every "static" element is rendered as one single image, instead of rendering individually every square and text label.)

We have some custom graphic elements we have to consider (fonts as well since SquareLine only has the Monsterrat font loaded as default):

![Custom Assets](ignore-images/customassets.png)

These shall be exported as .png and preferably in the actual pixel size they will be implemented as in the UI; though you can reescale them in SquareLine Studio later.

For fonts, look for the specific .ttf file of the styled font; considering if the one you used is semibold, regular, italic, bold, etc. (entire font folder "packs" can't be uploaded) [(Download here Open Sans ExtraBold and SemiBold used for this tutorial)](https://www.1001fonts.com/open-sans-font.html)

With your files ready you can proceed with the SquareLine project.

-----------------------------------
## 3. SquareLine Studio Project

1. Open up the SquareLine Stuidio software (v1.4.0) [(Installation and basic Guide)](https://www.elecrow.com/wiki/Get_Started_with_SquareLine_Studio.html). And create a new **Arduino with TFT_eSPI** project.

![Squareline1](ignore-images/squareline1.png)

2. Configure the proper resolution and properties as follows, and create the project.

![Squareline2](ignore-images/squareline2.png)

3. Add the corresponding assets (UI static image, button image and battery indicator images) (these are the png files located in the **tutorial-project-assets** folder)

![Squareline3](ignore-images/squareline3.png)
![Squareline4](ignore-images/squareline4.png)

4. Set the static UI as the background image of the Screen1.

![Squareline5](ignore-images/squareline5.png)

5. Add a label component and drag it across to your desired position.

![Squareline6](ignore-images/squareline6.png)

6. Configure label properties such as dimensions and positions with precise coordinates and set the default text.

![Squareline7](ignore-images/squareline7.png)

7. To add custom fonts, go into the font tab and select the custom font file previously downloaded. [(Download here Open Sans ExtraBold and SemiBold used for this tutorial)](https://www.1001fonts.com/open-sans-font.html). Name it as desired, set the sizing and hit create. Be aware that this process has to be done for every size requiered.

![Squareline8](ignore-images/squareline8.png)
![Squareline9](ignore-images/squareline9.png)

8. Set up color and font type of labels by opening their "Style Settings" window and clicking the _Text Color_ and _Text Font_ checkboxes and setting them accordingly.

![Squareline10](ignore-images/squareline10.png)

9. Create new labels faster by duplicating existing labels to mantain properties, or add fresh new ones.

![Squareline11](ignore-images/squareline11.png)

Quick look at the labels all in place and with style properties configured.
![Squareline12](ignore-images/squareline12.png)

10. Rename labels to an actual significant name; this is the way they'll be called in the code later on.

![Squareline13](ignore-images/squareline13.png)
![Squareline14](ignore-images/squareline14.png)

11. Create the arc object that will work as the speedometer.

![Squareline15](ignore-images/squareline15.png)

12. Place the arc object as desired and set its size by adjusting equally its width and height. Also set the range of values of the data it will be displaying,, for this project, velocity values range from 0 to 99.9.

![Squareline16](ignore-images/squareline16.png)

13. Set the knob (the little ball) as invisible for aesthetic purposes. Scroll down in the Inspector tab of the arc to **STYLE (KNOB)** and set its blend opacity to 0.

![Squareline17](ignore-images/squareline17.png)

14. Configure more aesthetic changes of the arc color and width by configuring the Arc settings of both **STYLE (MAIN)** and **STYLE (INDICATOR)**

![Squareline18](ignore-images/squareline18.png)

15. Add a button component and place it where desired.

![Squareline19](ignore-images/squareline19.png)

16. Scale its size, maintain equal pixel size on width and height.

![Squareline20](ignore-images/squareline20.png)

17. Scroll down in the Inspector tab to add the corresponding button image as the style's background image.

![Squareline21](ignore-images/squareline21.png)

18. Configure some more aesthetic changes to the button's shadow and background image to match the UI's colors.

![Squareline22](ignore-images/squareline22.png)

![Squareline23](ignore-images/squareline23.png)

19. Scroll down in the button's inspector tab and click on add event.

![Squareline31](ignore-images/squareline31.png)

20. Configure the event with any action and any name; and click on add; the selected action is currently irrelevant as it will be changed later in code, but it is important that the event is created properly.

![Squareline32](ignore-images/squareline32.png)

![Squareline33](ignore-images/squareline33.png)

21. At the top of the inspector tab, open the component section and create a component for the button object  (Free SquareLine licencse only allows one component) _**(OPTIONAL)**_

![Squareline24](ignore-images/squareline24.png)

22. Add an image widget, and add the battery empty asset; configure its settings. (Note that this image widget needs scaling since the exported image was 100x100 pixels and we need it to be smaller to fit the UI, default scaling is 256 for images, set it to 200 for this.)

![Squareline25](ignore-images/squareline25.png)

23. Do the exact same for the other battery image asset; consider that they need to be in the exact same x-y coordinate.

![Squareline26](ignore-images/squareline26.png)

24. Scroll down in the Widgets window to add the chart visualiser widget. Adjust it freely with the cursor to the desired size and position.

![Squareline27](ignore-images/squareline27.png)

25. In the inspector window of the chart widget; make sure to check/uncheck any of the following boxes:

![Squareline28](ignore-images/squareline28.png)

26. Since the data for this chart simulation will be added every 2 seconds and show the last 2 minutes of data; the number of points will be set to 60. Add a new series since the chart will display two types of data; and click on add new series (this way _Data 2_ will be added)

![Squareline29](ignore-images/squareline29.png)

Configure custom colors for both axis; and remember which one is which. In this case, it's yellow (primary) for current and green for voltage (secondary).  
A legend would've been ideal but honestly I forgot for this tutorial.  
The series Data textbox can be left empty, these are just placeholder values that will be modified in execution.

27. Open the _**X Axis**_, _**Primary Y Axis**_ and _**Secondary Y Axis**_ windows and configure the following data for each. These configure the layout of the axis labels and spacing lines.

![Squareline30](ignore-images/squareline30.png)

28. That concludes the configuration of the widgets, but before the file is exported. Go into **File >> Project Settings**

![SquarelineFinal](ignore-images/squarelinefinal.png)

![Squareline34](ignore-images/squareline34.png)

29. Set the Project Export Root and UI files Export Path as the same folder; ideally an empty one.

![Squareline35](ignore-images/squareline35.png)

30. Scroll down and add _lvgl.h_ as the LVGL Include Path; and check Flat Export. Properly close this window by clicking on Apply Changes.

![Squareline37](ignore-images/squareline37.png)

31. Finally, at the top, click **Export >> Export UI Files**

![Squareline36](ignore-images/squareline36.png)

This should export all the needed source and header files for your project into the folder you selected.

![SquarelineExport](ignore-images/squareline_export.png)

For this tutorial, you can find the exact export used in (tutorial-project-assets/SquareLine-Export).

-----------------------------------
## 4. Common files and General PlatformIO setup

There are a couple of steps that need to be done before you can actually get to properly programming the display with C++. In this section, I will cover the overall steps that need to be done for any project and UI designed for this display. This configuration setup (from step 4 onwards) needs to be repeated for every project done for the display.

On section 5, there will be the steps for the tutorial-project-specific configurations, especially regarding charts and button actions.

1. Make sure you have installed the PlatformIO extension into Visual Studio Code. [(Guide)](https://programarfacil.com/blog/arduino-blog/platformio/)  

2. Add the board file found in the **common-helper-assets** folder (esp32-s3-devkitc-1-myboard.json) into the following directory:

![CommonSetup1](ignore-images/commonsetup_1.png)

3. Go into VSCode and open the PlatformIO home page.

![CommonSetup2](ignore-images/common2.png)

4. Create a new project, select the custom board created (Espressif ESP32-S3-DevKitC-1-N8 -ELECROW) and the Arduino Framework. Choose a custom project location if you wish.

![CommonSetup3](ignore-images/common3.png)

5. Go into your platformio.ini file located inside the project folder. Currently it should look something like this:

![CommonSetup4](ignore-images/common4.png)

6. Add the following lines of code to the file:
```cpp
platform_packages = framework-arduinoespressif32 @ https://github.com/espressif/arduino-esp32#2.0.3
build_flags = 
	-D LV_LVGL_H_INCLUDE_SIMPLE
	-I./include
board_build.partitions = huge_app.csv
```
The platformio.ini file should currently look like this:

![CommonSetup5](ignore-images/common5.png)

7. Open once more the PlatformIO main page, but this time click on Libraries.

![CommonSetup6](ignore-images/common6.png)

8. Look for the **lvgl** by LVGL library.

![CommonSetup7](ignore-images/common7.png)

9. Select the corresponding version and add it to the project.

![CommonSetup8](ignore-images/common8.png)
![CommonSetup9](ignore-images/common9.png)

This should've been automatically addded to the platformio.ini file:
![Libraries1](ignore-images/libraries1.png)

10. Add the rest of the requiered libraries:
    - **lvgl** by LVGL
    - **Adafruit GFX Library** by Adafruit
    - **GFX library for Arduino** by Moon On Our Nation
    - **LovyanGFX** by lovyan03
    - **TAMC_GT911** by TAMC
    - **Adafruit BusIO** by Adafruit
    - **PCA9557-arduino** by Sonthaya Nongnuch
   
![Libraries2](ignore-images/libraries2.png)
![Libraries3](ignore-images/libraries3.png)
![Libraries4](ignore-images/libraries4.png)
![Libraries5](ignore-images/libraries5.png)
![Libraries6](ignore-images/libraries6.png)
![Libraries7](ignore-images/libraries7.png)

11. Once added the platformio.ini file should look like this:

![Libraries8](ignore-images/libraries8.png)

12. Go into the following directoy and open it in the file explorer.

![Libraries9](ignore-images/libraries9.png)

13. Copy the "demos" and "examples" folders to "src" folder

![Libraries10](ignore-images/libraries10.png)

14. Go into the following directoy of your project in your file explorer and add the **lv_conf.h** file. (Located in the **common-helper-assets** folder of this repository)

![Libraries11](ignore-images/libraries11.png)

15. Locate the folder where you exported all of the SquareLine Studio files. Identify which ones are header files (.h) and which one are source files (.c).

![Common11](ignore-images/common11.png)

16. Add all source (.c) files into your projects **src** folder. The folder also has the **main.cpp** file.

![Common12](ignore-images/common12.png)

17. Add the header (.h) files into the projects **include** folder. The folder also has a readme file created by default by PlatformIO.

![Common13](ignore-images/common13.png)

18. Add the **lv_conf.h** file in the **include** folder as well. (Located in the **common-helper-assets** folder of this repository)

![Common14](ignore-images/common14.png)

19. Continuing on the **include** folder, add the following files (Located in the **common-helper-assets** folder of this repository):
    - **touch.h** -> Used for initializing the capacitive touch controller (GT911 for this specific display)
    - **init_helper_ELYOS.h** -> File made to declare variables in order to make the main.cpp code easier to handle. (Complement of **init_helper_ELYOS.c**)

![Common15](ignore-images/common15.png)

20. Go into the **src** folder of your project; and add the **init_helper_ELYOS.cpp** file. As mentioned, this file was made with the purpose of helping to tidy up the main code file.

![Common16](ignore-images/common16.png)

21. Open the **main.cpp** file of your project and paste the following code (this is also the **main_bare.cpp** found in the **common-helper-assets** folder):

```cpp
#include <Arduino.h>
#include "init_helper_ELYOS.h"
#include "ui.h"
#include <stdlib.h>

void setup()
{
    Serial.begin(115200);

    elyos_display_init();
    elyos_lvgl_init();
    elyos_touch_init();
    elyos_backlight_init(200); 

    ui_init();
}

void loop()
{
    lv_timer_handler();
    delay(5);
}
```
>Note: Use "elyos_backlight_init(uint8_t brightness)"; to adjust the screen brightness, allowing values from 0 to 255.

With this configuration, the PlatformIO project can be built and flashed directly to the display. The screen will render the exported SquareLine UI exactly as designed. However, at this stage, dynamic elements (such as labels or gauges) will remain static, and interactive components (buttons, sliders, arcs, etc.) will visually respond to touch but will not execute any application logic.

-----------------------------------
## 5. Tutorial Specific Setup and Code


In this part of the tutorial, we will be setting up files, libraries and code that are specific to this telemetry simulation project. To begin, lets establish the functionalities of the display with the secondary ESP32 used for telemetry. 

The telemetry-ESP32 will generate a bunch of random data for the display to show; as well as simulate the laps done by counting presses from a button; and velocity by an ADC reading of a potentiometer. Sending them in specific batches of bytes via UART according to the desired visual update speed. Also, the screen will detect when the reset button is pressed, so that the displayed timers and lap counter go back to 0. Here's a simple diagram of how the data will be transmitted:

![UART Diagram](ignore-images/uart_diagram1.png)

The files that will be modified for this stage for component behavior are the following 3:
- **ui.h**
- **ui.c**
- **ui_Screen1.c**  

Considering this, we need to set a variable that will be triggered by the button event code. And we also need to set certain variables as global; this will allow us to configure the behavior of the chart in the main.cpp file.

1. Go into the **ui.h** file and add the following declarations in the corresponding part:
```cpp
extern lv_chart_series_t * ui_Chart1_series_1;
extern lv_chart_series_t * ui_Chart1_series_2;
extern lv_coord_t ui_Chart1_series_1_array[60];
extern lv_coord_t ui_Chart1_series_2_array[60];

extern uint8_t button_aux;
```
  
![ui.h replace](ignore-images/part5-1.png)
  
2. Now, go into the **ui.c** and look for the function that defines the buttons behavior.

![ui.c button function](ignore-images/part5-2.png)

3. Comment out the current action and swap it for the button event variable accordingly.

![ui.c replace](ignore-images/part5-3.png)

With the _button_aux_ variable now set up, it is easy to control any logic for the button event inside the main code, however, it is important to remember that the variable needs to be set low (or 0) after the desired function has been executed, so that it can be ran more than once.

4. For finishing the set up for the dynamic chart behavior, go into the **ui_Screen1.c** file. And first, add this lines of code at the start, after the **#include "ui.h"** line:
```cpp
lv_chart_series_t * ui_Chart1_series_1 = NULL;
lv_chart_series_t * ui_Chart1_series_2 = NULL;

lv_coord_t ui_Chart1_series_1_array[60] = {0};
lv_coord_t ui_Chart1_series_2_array[60] = {0};
```
![chart modify 2](ignore-images/part5-4.png)

5. Next, scroll down in the **ui_Screen1_screen_init()** function and find the code that creates the chart object (**ui_Chart1 = lv_chart_create(ui_Screen1);** or similar). And replace only from the series definition and below (under **lv_chart_set_axis_tick(...)**), with the provided code. This is done because we already declared the needed variables with the code above and in the header file; this way we avoid multiple declaration errors and re-definition clashes.
```cpp
    ui_Chart1_series_1 = lv_chart_add_series(ui_Chart1, lv_color_hex(0xFFBA00),
                                                                 LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_ext_y_array(ui_Chart1, ui_Chart1_series_1, ui_Chart1_series_1_array);
    ui_Chart1_series_2 = lv_chart_add_series(ui_Chart1, lv_color_hex(0x40CC25),
                                                                 LV_CHART_AXIS_SECONDARY_Y);
    lv_chart_set_ext_y_array(ui_Chart1, ui_Chart1_series_2, ui_Chart1_series_2_array);
```
Comparison before and after of **ui_Chart1 = lv_chart_create(Screen1)** code with this modification:
- Before: 

![Chart Create Before](ignore-images/chartbefore.png)

- After:

![Chart Create After](ignore-images/chartafter.png)

This concludes the setup for the tutorial's project. With this done, any element, component or widget (labels, arcs, charts, buttons and images) from the UI can be configured from the main code or from other source files.

-----------------------------------
## 6. Display Code

The code explained here can be found on the directory: **ui_assets_tutorial/src/main.cpp**

First up, there's the fixed data structure _DisplayData_ that mirrors what the telemetry ESP32 will be sending. Each C variable was chosen to match physical quantities, in the case of integers (which by default use 4 bytes of memory) they were shortened accordingly to match the data. As a side note, remember floats occupy 4 bytes of memory by default too.

```cpp
typedef struct {
    float battery_voltage;  // 22.0 to 26.0 Volts
    float current_amps;     // 7.0 to 10.0 Amps
    uint8_t laps;           // Lap counter
    float consumption;      //120.0 to 150.0 kWh
    float efficiency;       //160.0 to 200.0 km/kWh
    uint16_t rpms;          //Allows for 0 to 999 with no issue
    float velocity;         //0 to 99.9
    uint8_t tx_message;     //Type of message recieved (0-2)
} DisplayData;
DisplayData received_data = {0}; //Define it empty for now
```
Communicating devices via UART comes with its set of advantages and disadvantages. In the plus side, for one, it is simple, direct and allows for full-duplex commnication (send and receive simultaneously). However, as the name indicates, it is an asynchronous method of communication, lacking a shared clock line between devices, only relying on baud rate for speed data; this makes it prone to easily lose bytes over noisy environments which lead to the main concern of this communication method which is desyncing. You cannot ensure that devices know when does the "full messsage" you are sending starts, since UART only sends batches or packets of bytes. Meaning you need to create a specific message structure to overcome these issues. This code uses the following:
```
[ 0xAA ] [ CMD ] [ LEN ] [ PAYLOAD... ]
  SYNC    1b      1b        LEN bytes
```

Where CMD tells the code what type of data and LEN tells it how long is the byte packet it is currently receiving. This being done because it is generally preferred to not send the entire data structure at once. There are 5 commands:
```
0x01 CMD_FAST      : fastest (every ~10ms)  payload: rpms (2 bytes MSB-first), velocity (4-byte float)  => LEN=6
0x02 CMD_AWARENESS : medium (100ms)         payload: laps (1 byte), consumption (float), efficiency (float) => LEN=9
0x03 CMD_GRAPH     : slow (1s)              payload: battery_voltage (float), current_amps (float) => LEN=8
0x04 CMD_HEARTBEAT : heartbeat (500ms)      payload: tx_message (1 byte) => LEN=1
0x05 CMD_MESSAGE   : custom message         payload: N bytes text (N<=MAX_CUSTOM_MSG_LEN)
```
Using an enumeration data type, the UART byte processing makes the logic avoid possible misinterpretation or message corruption.
```
WAIT_SYNC     // waiting for 0xAA
WAIT_CMD      // next byte is command id
WAIT_LEN      // next byte is length; prepare buffer
WAIT_PAYLOAD  // read LEN bytes into buffer, then decode
```
There are 2 helper functions in the code that allow processing bytes and converting them into the proper variable format:
- **bytesToU16(...)** -> Integers using more than a byte of memory have what is called an MSB (most significant byte) and LSB (least significant byte); converting the MSB into a sort of "higher order" magnitude value. This function converts two 8 bit integers into the 16 bit one, expecting the MSB to go first.
```
received bytes: [0x01, 0x2C] -> bytesToU16 -> 0x012C = 300 RPM
```
- **bytesToFloat(...)** -> The 4 bytes that make up a float variables are interpreted based on the IEEE-754 regulation, which is natively built in. This function takes the 4 bytes recieved, and stores them into the memory location of the desired float variable; automatically converting them to float when interpreted.
```
float 10.0 -> 0x41200000 (hex)
bytes sent (from LSB to MSB): [0x00, 0x00, 0x20, 0x41]
receiver memcpy into union.u8 yields v.f == 10.0
```

If you've worked with Arduino projects before you are probably familiar with the _delay(ms)_ function, which "halts" the code for a moment to allow something to happen. There is a big downside to this function; which is that it is a blocking function, meaning it basically sends the entire CPU to sleep, stopping any task being executed and wasting processing resources. 

To avoid this, the code has a series of _if_ conditionals built with the milis() function, which is an unsigned long (32bit) integer that exists by default on Arduinos and ESP32 (with Arduino framework) and returns the amount of miliseconds the program has been running for. 
```
uint32_t now = millis();
if (now - last_100ms_update >= 100) { last_100ms_update = now; update_100ms(); }
```
With this, the code can execute several tasks in set time margin independently without halting any process. The tasks done are the following:
- **5ms update:** Handles the LVGL screen refreshes, input device reading, and animation updates. 
- **10ms update:** Manages arc value, certain logic regarding lap counts and the following labels: _rpms, velocity, full timer, lap timer, lap counter_.
- **100ms update:** Manages arc color, the image custom gauge clipping, plus _efficiency_ and _consumption_ labels.
- **2s update:** Handles the chart updates.

The milis() function wraps after 49.7 days but the unsigned propery and the substraction inside the _if_ condition makes it so it logically doesn't brake even in that extreme scenario.

_**Button Event Variable**_  
The code that uses the event variable that was set up earlier is very simple; and it is located in the UART handling function, since we want the button press to sent a coded byte back to the telemetry ESP32 to reset the count of the lap counter. As mentioned previously, it is important to reset the event variable flag back to 0 or false, so that it can execute more than once when pressed again.

```cpp
        if(button_aux){ //Trigger when button pressed TX
            DisplaySerial.write(SYNC_BYTE);
            button_aux = 0;
        }
```

_**Label Updates**_

The way labels are updated uses safe string formatting and LVGL calls. Meaning that numeric values have to be converted to short strings and then sent to the corresponding LVGL labels. For example, here's the way the rpms and velocity labels are handled inside of the 10ms update function:
```cpp
    // ---------- RPM ----------
    static uint16_t last_rpm = 0;
    if (received_data.rpms != last_rpm) {
        last_rpm = received_data.rpms;
        char buf[8];
        snprintf(buf, sizeof(buf), "%3u rpm", last_rpm);
        lv_label_set_text(ui_rpmLabel, buf);
    }

    // ---------- Velocity (1 decimal) ----------
    static float last_velocity = -1.0f;
    if (received_data.velocity != last_velocity) {
        last_velocity = received_data.velocity;
        char buf[8];
        snprintf(buf, sizeof(buf), "%.1f", last_velocity);
        lv_label_set_text(ui_velocityLabel, buf);
    }
```
Where the **!=** conditional is to avoid overprocessing when data remains the same. 
The char variable protects the label from overflowing and decimal and integer count is formatted and concatenated in the snprintf(...) function.  

You can directly set a predetermined text directly in the lv_label_set_text(...) by just adding it in quotation marks; like these lines, where the timer label is reset when pressing the button and being acknowledged by the telemetry ESP32:
```cpp
// Reset button pressed
else if(received_data.laps == 0){
    lv_label_set_text(ui_fullattemptLabel, "00:00.00");
    lv_label_set_text(ui_laptimeLabel, "00:00.00");
    attempt_start_ms = 0;
    lap_start_ms = 0;
}
```
Or also modify text directly with a char array like so:
```cpp
lv_label_set_text(ui_messageLabel, custom_msg_buf);
```
>Where **custom_msg_buf** is declared as **static char custom_msg_buf[33]**

_**Arc Configuration**_  
Both arc and slider widgets are technically touch-interactive elements; however, they can be easily set to a specific value with a 16bit integer value, with the following functions:
```cpp
lv_arc_set_value(lv_obj_t *obj, int16_t value)
lv_slider_set_value(lv_obj_t *obj, int16_t value)
```
For the code implementation, the arc value is updated every 10ms; however, abrupt changes in values show "choppiness" in how the arc meter changes when rendered by the display. To minimize this effect, a Low-pass smoothing digital filter was added. It is important to convert the variable into the 16bit integer so that the graphic library can handle the value.
```cpp
    // ---------- Arc (velocity 0–100 → arc 0–99) ----------
    uint16_t arc_val = (uint16_t)(received_data.velocity);
    if (arc_val > 99) arc_val = 99;

    //Low pass for smoother animation
    static float arc_display = 0.0f;   // persistent displayed value
    arc_display += (arc_val - arc_display) * 0.18f;
    lv_arc_set_value(ui_Arc1, (uint16_t)arc_display);
```
In this filter, the **0.18** value corresponds to the alpha constant that determines how "responsive" the system is and how "effective" it is at smoothing out the signal. A higer value (~0.30) shows a smoother behavior but a higher delay in response, while a lower value (~0.05) behaves with less delay but a lot noisier and "choppier".

_**Color Management**_  
You can also easily personalize the color of labels and several other elements (like arc segments) mid-code by simply using the desired color with the following functions:
```cpp
//Labels
void lv_obj_set_style_text_color(_lv_obj_t *obj, lv_color_t value, lv_style_selector_t selector)

//Arc Elements
void lv_obj_set_style_arc_color(_lv_obj_t *obj, lv_color_t value, lv_style_selector_t selector)
```
Colors can be set using RGB or HSV values, as well as a set palette of predetermined colors in the **value** parameter (like **lv_palette_main(LV_PALETTE_RED)** for red). You can also mix colors using the following function (where mix is a value from 0 to 255 that determines the ratio of the first color):
```cpp
extern "C" static inline lv_color_t lv_color_mix(lv_color_t c1, lv_color_t c2, uint8_t mix)
```

For more information regarding the color macros and formatting, checkout the [official LVGL color summary](https://docs.lvgl.io/8.0/overview/color.html).

In the code, the message label color is set when receiving a message. And the arc indicator element color is set every 100ms based on a proportion that works as follow:
- 0 - 50: Blue
- 50 - 90: Transitions from Blue to Purple
- 90 - 100: Transitions from Purple to Red

```cpp
//Set message label color using HEX Color
lv_obj_set_style_text_color(ui_messageLabel, lv_color_hex(0xD4D3D6), LV_PART_MAIN | LV_STATE_DEFAULT);

//Set arc indicator element color using LVGL palette colors and the mix function
    float v = received_data.velocity;
    lv_color_t c;
    if (v <= 50.0f) {
        c = lv_palette_main(LV_PALETTE_BLUE);
    } else if (v <= 90.0f) {
        uint8_t t = (uint8_t)((v - 50.0f) * 255.0f / 40.0f);
        c = lv_color_mix(lv_palette_main(LV_PALETTE_PURPLE),
                         lv_palette_main(LV_PALETTE_BLUE), t);
    } else {
        uint8_t t = (uint8_t)((v - 90.0f) * 255.0f / 10.0f);
        c = lv_color_mix(lv_palette_main(LV_PALETTE_RED),
                         lv_palette_main(LV_PALETTE_PURPLE), t);
    }
    lv_obj_set_style_arc_color(ui_Arc1, c, LV_PART_INDICATOR);
```

_**Chart Behaviour**_  
Modifying chart plotting is quite simple, all the data plotted is contained inside of the array:
```cpp
ui_yourChartName_series_1_array[NUMBER_OF_POINTS]
```
And to refresh the chart once new data has been added, just add the following function after the corresponding logic:
```cpp
// Declaration
void lv_chart_refresh(lv_obj_t *obj) 
//Implementation
lv_chart_refresh(ui_yourChartName); 
```

Chart values are defined by regular integers (32 bit), meaning the values have to be converted from any other integer size and no floats are allowed. In the case of this project, considering the desired series values are floats that range from 22.0 to 26.0 and from 7.0 to 10.0; the values are scaled by 10 in order to keep the decimal resolution visually. Also, the code uses the built in C function **memmove(...)** in order to displace the current values one to the left, when new data is added. Since the plot contains 60 points, the most recent one is added on index 59, since 0 is counted as the first index.

```cpp
//memmove(...) delcaration
void *memmove(void *, const void *, size_t)

//Chart logic implementation
    //Convertir a enteros *10, mantiene resolucion de 1 decimal
    int32_t amps_val = (int32_t)(received_data.current_amps * 10.0f + 0.5f);
    int32_t volt_val = (int32_t)(received_data.battery_voltage * 10.0f + 0.5f);

    // Mover previas
    memmove(&ui_Chart1_series_1_array[0],
            &ui_Chart1_series_1_array[1],
            (60 - 1) * sizeof(lv_coord_t));
    memmove(&ui_Chart1_series_2_array[0],
            &ui_Chart1_series_2_array[1],
            (60 - 1) * sizeof(lv_coord_t));
    // Mas reciente a la derecha
    ui_Chart1_series_1_array[60 - 1] = amps_val;
    ui_Chart1_series_2_array[60 - 1] = volt_val;

    lv_chart_refresh(ui_Chart1);
```

_**Custom Gauges by Image Clipping**_  

The idea of the battery indicator was to create but instead custom gauges built from images (providing alternatives from arcs, and sliders). These gauges work by clipping or masking an image based on a numeric value, giving the illusion of a dynamic bar, fill level, or progress indicator. 

In concept, the battery meter "fills up" as the read value is set higher. And in practice, there are two images, an static one and the filler second image, which gets clipped vertically from bottom to top. For simplicity purposes, the variable attached to this custom gauge corresponds to the velocity variable that sets the speedometer arc value.

LVGL allows modifying an object’s size and position at runtime using functions such as:
```cpp
void lv_obj_set_height(lv_obj_t *obj, lv_coord_t h);
void lv_obj_set_y(lv_obj_t *obj, lv_coord_t y);
```
In the implementation, the "filler" image is used as a parent object to create a secondary object used as the image being clipped; copying its properties but aligning it at the bottom; since we want the custom gauge to be filled from top to bottom. In the code, this creation is ran in the main code once. (Done this way to avoid tinkering more in the generated SquareLine C files)

```cpp
    static lv_obj_t *bat_clip = NULL;
    static lv_coord_t full_h;
    static lv_coord_t base_y;

    /* ---------- One-time setup ---------- */
    if (!bat_clip) {
        bat_clip = lv_obj_create(lv_obj_get_parent(ui_batFull));
        lv_obj_remove_style_all(bat_clip);
        lv_obj_set_pos(bat_clip,lv_obj_get_x(ui_batFull),lv_obj_get_y(ui_batFull));
        lv_obj_set_size(bat_clip, lv_obj_get_width(ui_batFull),lv_obj_get_height(ui_batFull));

        lv_obj_set_style_clip_corner(bat_clip, true, 0);

        /* Move image inside clip */
        lv_obj_set_parent(ui_batFull, bat_clip);
        lv_obj_clear_flag(ui_batFull, LV_OBJ_FLAG_FLOATING);
        lv_obj_align(ui_batFull, LV_ALIGN_BOTTOM_MID, 0, 0);

        full_h = lv_obj_get_height(ui_batFull);
        base_y = lv_obj_get_y(bat_clip);
    }
```
Then, since the data that controls this gauge ranges from 0 to 99.9; the scaling of the pixel height of the object is done proportionally and converted into an integer (**lv_coord_t** defines 16bit signed integers) and the resulting clipped image is placed on the pixel _y_ coordinate relative to its current height.
```cpp
    /* ---------- Battery fill logic ---------- */
    float v = received_data.velocity;
    if (v < 0)   v = 0;
    if (v > 100) v = 100;

    lv_coord_t vis_h = (lv_coord_t)(full_h * (v / 100.0f));
    if (vis_h < 1) vis_h = 1;

    lv_obj_set_height(bat_clip, vis_h);
    lv_obj_set_y(bat_clip, 339 + base_y + (full_h - vis_h)); 
```

_**Custom Message Handling**_

The message handling uses the same principle as the bytes to float helper function. In this case, an array with char variable type is created. And when the bytes are received via UART, we use the **memcpy(...)** function to write the bytes inside the char array (through copying the block of memory); and by default, they are interpreted as char when called later on. In the UART logic, a flag is set to true that indicates that a new message has arrived and that the label has to be set accordingly. There are also 2 extra states of messages aside from the custom one; one that sets by default or when idle, and one that sets when the reset button press is acknowledged by the 

_UART Message Handler_
```cpp
case CMD_MESSAGE:
            if (len > 0 && len <= MAX_CUSTOM_MSG_LEN) {
                memcpy(custom_msg_buf, buf, len);
                custom_msg_buf[len] = '\0';   // null terminate
                message_dirty = true;
            }
            break;
```

_Label Update_
```cpp
void update_message_label(void)
{
    if (!message_dirty && received_data.tx_message == last_message_type)
        return;

    last_message_type = received_data.tx_message;
    message_dirty = false;

    switch (received_data.tx_message) {

        case IDLE_MSG:
            lv_label_set_text(ui_messageLabel, "Waiting for message...");
            lv_obj_set_style_text_color(ui_messageLabel, lv_color_hex(0x7D7E82), LV_PART_MAIN | LV_STATE_DEFAULT);
            break;

        case RESET_MSG:
            lv_label_set_text(ui_messageLabel, "Full attempt timer reset.");
            lv_obj_set_style_text_color(ui_messageLabel, lv_color_hex(0x7D7E82), LV_PART_MAIN | LV_STATE_DEFAULT);
            break; 

        case CUSTOM_MSG:
            lv_label_set_text(ui_messageLabel, custom_msg_buf);
            lv_obj_set_style_text_color(ui_messageLabel, lv_color_hex(0xD4D3D6), LV_PART_MAIN | LV_STATE_DEFAULT);
            break;

        default:
            break;
    }
}
```

-----------------------------------
## 7. Telemetry simulation code and circuit
The code for this circuit can be found on the directory: **esp32-telemetry-sim/src/main.cpp**

If you wish to follow along this tutorial; create a separate PlatformIO project from the Display one, and add the following library:
![Telemetry1](ignore-images/telemetry1.png)
It helps with debouncing issues with regular push buttons and makes the code much simpler.

As the entire UART message encoding and data structure transmission has already been explained in the previous section; I will make just a brief list of what goes on in the code:
- **Map GPIO18 and GPIO19 for Serial1 communication via UART**
- **ADC2 Reading in GPIO4** for simulating the velocity and rpms. (0-4095 resolution)
- **EZButton in GPIO23** for lap counting simulation 
- Accurately converts linear velocity into rpms by assuming a wheel radius of 50cm.
- **Reads terminal serial monitor for custom messages**, allowing a maximum of 32 characters for custom messages and using the following syntaxis:
    - Typing "idle" resets the message to the default "Waiting for message..."
    - For sending a custom message type "msg Your Message" to display "Your Message" into the UI.
- **Random data generated** for battery, current, consumption and efficiency.
- **Helper functions** for converting floats and multiple-byte integers into bytes.
- The UART message transmission at a baud rate of 115200 is layed out like this:
    - **Every 10ms (CMD 0x01):** RPM and velocity data.
    - **Every 100ms (CMD 0x02):** Consumption, efficiency and laps data.
    - **Every 1s (CMD 0x03):** Battery and current data for charts.
    - **Every 200ms (CMD 0x04):** Message sent awareness check 
    - **When available (CMD 0x05):** Send the custom message characters

-----------------------------------
## 8. Helper Init code explanation
With the project now done, it is important to know what is actually going on in the initialization helper file provided in this repository.
_**Library usage:**__

- LVGL: UI library
- PCA9557.h: This library is designed for the PCA9557 I2C LED driver, which allows you to control the brightness of multiple components via I2C communication
- SPI.h: Screen driven library
- Adafruit_GFX.h: Adafruit_GFX library is a graphics library provided by Adafruit for drawing graphics on various display devices
- LovyanGFX.hpp: LovyanGFX library is a library for drawing graphics on embedded systems, providing rich graphics functions and APIs
- lgfx/v1/platforms/esp32s3/Panel_RGB.hpp: lgfx/v1/platforms/esp32s3/Bus_RGB.hpp:  
    - These two libraries are part of the LovyanGFX library, specifically for driving RGB color panels on ESP32-S3 microcontrollers. Panel_RGB.hpp provides functions for communicating with RGB panels, while Bus_RGB.hpp provides functions for communicating with RGB buses, which are usually used to control devices such as LCD screens.

The helper source file begins with the LGFX class declaration, which is a subclass of lgfx::LGFX_Device. The main function is to configure and initialize an RGB color panel, and then connect it to the RGB bus on the ESP32-S3 microcontroller to display graphics on the panel.

Screen refresh function, this code is for the refresh function driven by the display in the LittlevGL (LVGL) graphics library. LVGL is an open-source graphics library for embedded systems, used to create graphical user interfaces (GUIs). This function my_disp_flush is a user-defined callback function used to refresh the content of the graphical interface onto the display device.
```cpp
/* ---------- LVGL Flush Callback ---------- */
static void disp_flush(lv_disp_drv_t *disp,
                            const lv_area_t *area,
                            lv_color_t *color_p)
{
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;

    lcd.pushImageDMA(area->x1, area->y1, w, h,
                     (lgfx::rgb565_t *)&color_p->full);
    lcd.waitDMA();              // SCREEN TEARING AVOID ?
    lv_disp_flush_ready(disp);
}
```

A simple touch input device reading function, which is used to convert touch events into the input device data format required by the LVGL library.
```cpp
/* ---------- Touch Callback (external driver) ---------- */
static void elyos_touch_read(lv_indev_drv_t *indev,
                             lv_indev_data_t *data)
{
    if (touch_has_signal() && touch_touched())
    {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = touch_last_x;
        data->point.y = touch_last_y;
    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
    }
}
```

CAN implementation pending dddsdfsdf