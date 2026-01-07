ELECROW Display Setup Tutorial
===================================
This is a guide and tutorial for setting up an ELECROW ESP32 7.0 Inch HMI Display (DIS08070H) [[specs]](https://media-cdn.elecrow.com/wiki/esp32-display-702727-intelligent-touch-screen-wi-fi26ble-800480-hmi-display.html). All files referenced throughout this tutorial are available in this same repository and are free to use. There are 5 folders in this repository to consider:  
- **Tutorial Project Assets:** Files used for creating the UI presented in this tutorial, including the entire platformIO project and UI assets, use these if you desire to follow along this tutorial or wish to have a reference vantage point.  
  
- **Ignore Images:** Images used to illustrate this readme, __ignore__.
    
- **Common Helper Assets:** Files provided for general use in various projects, I recommend using these as they will hopefully help you tidy up your future Elecrow Display projects shenanigans. (Jump to section 4 for more details about this files.)  
  
- **ui_assets_tutorial:** PlatformIO Display Project that handles UART reception and graphic handling.

- **esp32:** PlatformIO ESP32 Dev Module Project that generates random telemetry and transmits it to the Display via UART.


-----------------------------------
**Tutorial Overview**
-----------------------------------
In this tutorial, we will be setting up an User Interface in the ELECROW screen, by creating a simulation of telemetry transmission for a driver screen of Shell ECO Marathon Battery-Electric Vehicles. (by communicating it with an ESP32 Dev Module). It is important to establish that this UI is **NOT ideal** to be used as an actual UI, since it introduces way too much visual clutter for the driver to manage. The tutorial's high usage of features is mostly for show and to highlight the performance, capabilities and configurations of the screen; **not for actual implementation inside of the competition.**

![Final_UI](ignore-images/squarelinefinal.png)

The ELECROW ESP32 7.0 Inch HMI Display, as the name suggests, uses an ESP32 microcontroller for handling its entire programming; meaning it can use ESPIDF or Arduino's framework through either ArduinoIDE or PlatformIO. For this tutorial, we'll be using the following softwares:
- **SquareLine Studio** (v1.4.0): It is important that you do not install nor update to newer versions. [(Guide)](https://www.elecrow.com/wiki/Get_Started_with_SquareLine_Studio.html)
- **Visual Studio Code** with PlatformIO extension installed [(Guide)](https://programarfacil.com/blog/arduino-blog/platformio/)  

This example is inspired by [Elecrow's official display example tutorial](https://www.elecrow.com/wiki/CrowPanel_ESP32_7.0-inch_with_PlatformIO.html), but it expands on it by incrementing the complexity and number of both UI elements and communication protocols. This tutorial will use an extra ESP32 DEV Module used for simulation of what would otherwise be the Telemetry Board (as well as some extra simple components for testing simulation)

![Components](tutorial-page-assets/component_list.png)

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

In order to cover this, the tutorial is divided in the following sections. **Consider this the readme index; click into any step to go straight to it.**

1. Important Display Considerations
2. Invisioning your UI 
3. SquareLine Studio  
4. PlatformIO Project setup
5. Library Management  
5. Display Code Structure Basis  
6. Test Circuit  
7. ESP32 Telemetry Simulation Code
8. Display UART and responsive Code
9. ~~CAN Implementation~~  
10. Common Asseets
  

-----------------------------------
**1. Important Display Considerations**
-----------------------------------
  
The display has the following pinout configuration. It is important to note that the BAT input cannot be fed by a 5v source, as one might assume based on the proximity to the USB C port; nor should it be a 3.3v entry. The recommended power source when not connected to the USB C is a **3.7v Li-Po / Li-ion battery**; that has a full-charge voltage of 4.2v
  
![Display pinout](ignore-images/pinout.png)

Although the display exposes several GPIOs, their practical usability is limited due to internal PCB routing and the ESP32-S3’s peripheral architecture.
- GPIO38 is the only truly free general-purpose pin that can be used as a digital input or output. However, unlike classic ESP32 variants where ADC1 channels are mapped to GPIOs 32–39, the ESP32-S3 maps ADC channels to GPIOs 1–10. As a result, **GPIO38 cannot be used as an analog input**, despite being within the “ADC range” of other ESP32 cores.
- The ESP32-S3 provides three UART controllers (UART0–UART2), and the pins labeled as UART0 on the silkscreen (GPIO43 and GPIO44) can be reassigned to any UART controller (or used as GPIO) thanks to the ESP32 pin matrix. However, UART0 is electrically tied to the USB-C interface used for flashing and serial monitoring. Because of this, any external device actively driving GPIO43/44 can interfere with code uploading or serial communication, even if those pins are configured as Serial1 or Serial2. This means **external UART communication must be disconnected or powered down while flashing or using the USB serial monitor.**
- **GPIO19 (SDA) and GPIO20 (SCL) must be treated as I²C-only pins.** These pins are hard-wired on the PCB to the capacitive touch controller (GT911), including external pull-up resistors to 3.3 V. Even if touch functionality is disabled in software, the touch controller remains electrically connected, adding pull-ups, capacitance, and potential bus contention.

With this in mind, we can begin the tutorial.

-----------------------------------
**2. Invisioning and designing UI**
-----------------------------------
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
**3. SquareLine Studio Project**
-----------------------------------
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

12. Place the arc object as desired and set its size by adjusting equally its width and height. Also set the range of values of the data it will be displaying.

![Squareline16](ignore-images/squareline16.png)

13. Set the knob (the little ball) as invisible for aesthetic purposes. Scroll down in the Inspector tab of the arc to **STYLE (KNOB)** and set its blend opacity to 0.

![Squareline17](ignore-images/squareline17.png)

14. Configure more aesthetic changes of the arc color and width by configuring the Arc settings of both **STYLE (MAIN)** and **STYLE (INDICATOR)**

![Squareline18](ignore-images/squareline18.png)

15. Add a button component and place it where desired.

![Squareline19](ignore-images/squareline19.png)

16. Scale its size, preserve pixel size on width and height.

![Squareline20](ignore-images/squareline20.png)

17. Scroll down in the Inspector tab to add the corresponding button image as the style's background image.

![Squareline21](ignore-images/squareline21.png)

18. Configure some more aesthetic changes to the button's shadow and background image to match the UI's colors.

![Squareline22](ignore-images/squareline22.png)

![Squareline23](ignore-images/squareline23.png)

19. Scroll down in the button's inspector tab and click on add event.

![Squareline31](ignore-images/squareline31.png)

20. Configure the event with any action and any name; and click on add; this is currently irrelevant as it will be changed later in code.

![Squareline32](ignore-images/squareline32.png)

![Squareline33](ignore-images/squareline33.png)

21. At the top of the inspector tab, open the component section and create a component for the button object  (Free SquareLine licencse only allows one component) _**(OPTIONAL)**_

![Squareline24](ignore-images/squareline24.png)

22. Add an image component, and add the battery empty asset; configure its settings. (Note that this image component needs scaling since the exported image was 100x100 pixels and we need it to be smaller to fit the UI, default scaling is 256 for images, set it to 200.)

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

27. Open the _**X Axis**_, _**Primary Y Axis**_ and _**Secondary Y Axis**_ windows and configure the following data for each. These correspond for the layout of the axis labels.

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
**4. Common files and General PlatformIO setup**
-----------------------------------
There are a couple of steps that need to be done before you can actually get to properly programming the display with C++. In this section, I will cover the overall steps that need to be done for any project and UI designed for this display. So basically, you need to do this section for every UI project done.

On section 5, there will be the steps for the tutorial-project-specific configurations, especially regarding charts and button actions.

1. Make sure you have installed the PlatformIO extension into Visual Studio Code. [(Guide)](https://programarfacil.com/blog/arduino-blog/platformio/)  

2. Add the board file found in the common-helper-assets folder (esp32-s3-devkitc-1-myboard.json) into the following directory:

![CommonSetup1](ignore-images/commonsetup_1.png)

3. Go into VSCode and open the PlatformIO home page.

![CommonSetup2](ignore-images/common2.png)

4. Create a new project, select the custom board created (Espressif ESP32-S3-DevKitC-1-N8 -ELECROW) and the Arduino Framework. Choose a custom project location if you wish.

![CommonSetup3](ignore-images/common3.png)

5. Go into your platformo.ini file located inside the project folder. Currently it should look something like this:

![CommonSetup4](ignore-images/common4.png)

6. Add the following lines of code to the file:
```cpp
platform_packages = framework-arduinoespressif32 @ https://github.com/espressif/arduino-esp32#2.0.3
build_flags = 
	-D LV_LVGL_H_INCLUDE_SIMPLE
	-I./include
board_build.partitions = huge_app.csv
```
The platform.ini file should currently look like this:

![CommonSetup5](ignore-images/common5.png)

7. Open once more the PlatformIO main page, but this time click on Libraries.

![CommonSetup6](ignore-images/common6.png)

8. Look for the **lvgl** by LVGL library.

![CommonSetup7](ignore-images/common7.png)

9. Select the corresponding version and add it to the project.

![CommonSetup8](ignore-images/common8.png)
![CommonSetup9](ignore-images/common9.png)

This should've been automatically addded to the platform.ini file:
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

11. Once added the platform.io file should look like this:

![Libraries8](ignore-images/libraries8.png)

12. Go into the following directoy and open it in the file explorer.

![Libraries9](ignore-images/libraries9.png)

13. Copy the "demos" and "examples" folders to "src" folder

![Libraries10](ignore-images/libraries10.png)

14. Go into the following directoy of your project in your file explorer and add the **lv_conf.h** file. (Located in the **common-helper-assets** folder of this repository)

![Libraries11](ignore-images/libraries11.png)

15. Locate once more the folder where you exported all of the SquareLine Studio files. Identify which ones are header (.h) and which one are source files (.c).

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
**5. Tutorial Specific Setup and Code**
-----------------------------------

In this part of the tutorial, we will be setting up files, libraries and code that are specific to this telemetry simulation project. To begin, lets establish the functionalities of the display with the secondary ESP32 used for telemetry. 

The telemetry-ESP32 will generate a bunch of random data for the display to show; as well as simulate the laps done by counting presses from a button; and velocity by an ADC reading of a potentiometer. Sending them in specific batches of bytes via UART according to the desired visual update speed. Also, the screen will detect when the reset button is pressed, so that the displayed timers and lap counter go back to 0. Here's a simple diagram of how the data will be transmitted:

![UART Diagram](ignore-images/uart_diagram1.png)

Most, if not all of the entire logic, variables and code for component behaviour is located at 3 files:  
- **ui.h**
- **ui.c**
- **ui_Screen1.c**  

Considering this, we need to set a variable that will store the reset button status when pressed. And we also need to set certain variables as global; this will allow us to configure the behaviour of the chart in the main.cpp file.

1. Go into the **ui.h** file and add the following declarations in the corresponding part:
```cpp
extern lv_chart_series_t * ui_Chart1_series_1;
extern lv_chart_series_t * ui_Chart1_series_2;
extern lv_coord_t ui_Chart1_series_1_array[60];
extern lv_coord_t ui_Chart1_series_2_array[60];

extern uint8_t button_aux;
```
  
![ui.h replace](ignore-images/part5-1.png)
  
2. Now, go into the **ui.c** and look for the function that defines the buttons behaviour.

![ui.c button function](ignore-images/part5-2.png)

3. Comment out the current action and swap it for the variable behaviour accordingly.

![ui.c replace](ignore-images/part5-3.png)

With the _button_aux_ variable now set up, it is easy to control any logic for the button action inside the main code, however, it is important to remember that the variable needs to be set low (or 0) after the desired function has been executed, so that it can be ran once more.

4. For finishing the set up for the dynamic chart behaviour, go into the **ui_Screen1.c** file. And first, add this lines of code at the start, after the **#include "ui.h"** line:
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

    lv_obj_add_event_cb(ui_Button1, ui_event_Button1_Button1, LV_EVENT_ALL, NULL);

```
Comparison before and after of **ui_Chart1 = lv_chart_create(Screen1)** code with this modification:
- Before: 
![Chart Create Before](ignore-images/chartbefore.png)

- After:
![Chart Create After](ignore-images/chartafter.png)

This concludes the setup for the tutorial's project. With this done, any element, component or widget (labels, arcs, charts, buttons and images) from the UI can be configured from the main code or from other source files.

-----------------------------------
**6. Display Code**
-----------------------------------
ESP32 Setup, Code and Circuit

-----------------------------------
**7. Telemetry simulation code and circuit**
-----------------------------------