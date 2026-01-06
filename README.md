ELECROW Display Setup Tutorial
===================================
This is a guide and tutorial for setting up an ELECROW ESP32 7.0 Inch HMI Display (DIS08070H) [[specs]](https://media-cdn.elecrow.com/wiki/esp32-display-702727-intelligent-touch-screen-wi-fi26ble-800480-hmi-display.html). All files referenced throughout this tutorial are available in this same repository and are free to use. There are 3 folders in this repository to have in mind:  
- **Tutorial Project Assets:** Files used for creating the UI presented in this tutorial, including the entire platformIO project and UI assets, use these if you desire to follow along or wish to have a reference vantage point.  
  
- **Ignore Images:** Images used to illustrate the tutorial better, __ignore__.
    
- **Common Assets:** Files provided for general use in various projects, I recommend using these as they will hopefully help you tidy up your future Elecrow Display projects shenanigans. (Jump to section iewjfiewfn for more details about this files.)  
  
-----------------------------------
**Tutorial Overview**
-----------------------------------
In this tutorial, we will be setting up an User Interface in the ELECROW screen, as well creating a simulation of telemetry transmission for a driver of Shell ECO Marathon Battery-Electric Vehicles. (by communicating it with an ESP32 Dev Module). It is important to establish that this UI is **NOT ideal** to be used as an actual UI, since it introduces way too much visual clutter for the driver to manage. The tutorial's high usage of features is mostly for show and to highlight the performance, capabilities and configurations of the screen; **not for actual implementation inside of the competition.**

![Final_UI](tutorial-image-assets/final_ui.png)

The ELECROW ESP32 7.0 Inch HMI Display, as the name suggests, uses an ESP32 microcontroller for handling its entire programming; meaning it can comfortably use ESPIDF or Arduino's framework through either ArduinoIDE or PlatformIO. For this tutorial, we'll be using the following softwares:
- **SquareLine Studio** (v1.4.0): It is important that you do not install nor update to newer versions. [(Guide)](https://www.elecrow.com/wiki/Get_Started_with_SquareLine_Studio.html)
- **Visual Studio Code** with PlatformIO extension installed [(Guide)](https://programarfacil.com/blog/arduino-blog/platformio/)  

This example is inspired by [Elecrow's official display example tutorial](https://www.elecrow.com/wiki/CrowPanel_ESP32_7.0-inch_with_PlatformIO.html), but it expands on it by incrementing the complexity and number of both UI elements and communication protocols. This tutorial will use an extra ESP32 DEV Module used for simulation of what would otherwise be the Telemetry WiFi Communication Board (as well as some extra simple components for testing)

![Components](tutorial-page-assets/component_list.png)

In this example tutorial, the objective is to leave a clear explanation on the implementation methods of the following elements for your future display projects:
- UI design
- Updating text labels
- Managing Arcs and Sliders
- Masked slider indicators
- Non-blocking display timers
- Plotting in Graphs
- Display button interaction
- UART Communication
- ~~CAN Communication Pending~~

In order to cover this, the tutorial is divided in the following sections:  

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
  
The display has the following pinout configuration. It is important to note that the BAT input cannot be directly to a 5v source as one might think based on the proximity to the USB C port; nor it should be a 3.3v entry. The recommended power source when not connected to the USB C is a **3.7v Li-Po / Li-ion battery**; that has a full-charge voltage of 4.2v
  
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
- Simultaneous graph of both current and voltage consumption during the last 2 minutes.
- Current lap timer and full attempt timer 
- Lap counter and interactive button for resetting it and the timer.
- Message box for displaying custom sent messages.
- Display power indicator.

And the next step will be to do a rough sketch of the UI layout in any graphic software, considering the resolution of the display (800px x 480px). This doesn't have to be pretty, it just has to present the order that the elements are going to be displayed in. 

![Initial Rough Layout](ignore-images/rough_layout.png)

Once you have all the elements layed out and considerations in mind; you can begin designing your aesthetically pleasing user interface. Some recomendations I can give:
- As mentioned, look for inspirations of other UI designs, but do not steal! Look for mobile apps, webpages or other designs, you'll find mostly modern-esque interfaces, but for a reason, since they provide a clean and easy look for your UI.
- If you're like me and struggle with colors, I recommend pages like [Coolors](https://coolors.co/) or [Color Hunt](https://colorhunt.co/) for finding proper color palettes.
- Though the use of a professional software like Ilustrator or Figma would be prefered, it's understood that they are a bit of a hassle to setup and learn. Canva is an all time classic alternative and with your student account you have access to all of the premium features.
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

8. Set up color and font type of labels by opening their "Style Settings" window and clicking the **Text Color** and **Text Font** checkboxes and setting them accordingly.

![Squareline10](ignore-images/squareline10.png)

9. Create new labels faster by duplicating existing labels to mantain properties, or add new ones.

![Squareline11](ignore-images/squareline11.png)

Quick look at the labels all in place and with style properties configured.
![Squareline12](ignore-images/squareline12.png)

10. Rename labels to an actual significant name; this is the way they'll be called in the code later on.

![Squareline13](ignore-images/squareline13.png)
![Squareline14](ignore-images/squareline14.png)

11.

-----------------------------------
**4. Setting Up Files**
-----------------------------------
This is the text that describes part 4.  
  
-----------------------------------
**5. PlatformIO**
-----------------------------------
This is the text that describes part 5.  
  