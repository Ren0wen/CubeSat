hi guys so to use the gh repo, there are 4 main functions:

    -   add
    -   push
    -   commit
    -   pull

A git repository is just a shared folder that lives on all of our computers.

When you clone the repository onto your computer, you now have access to the repository. you only need to do this once.
 
When you change, add, or remove a file from the repository, you'll have "uncommitted" changes. When you're ready to save the changes, you must add your changes, THEN commit your changes, THEN push your changes. Your changes will now be located on the repository.

Now, others can "pull" your changes. 

------------------------------------------------------------------

if you're curious on how it works, the program works as follows:

we have 3 distinct modules that we are fetching data from. the PCF (clock), MPL (barometer, thermometer), and LSM (gyroscope and accelerometer). to communicate with each, we use a data bus composed of an SDA and an SCL line that operate as a data and clock line (I2C), respectively. data is passed around in the form of voltage swings that represent digital 0s and 1s. all information passed through the data line is synchronized with the clock line.

take a look at the datasheets, make note of the I2C sections that describe the unique codes of the addresses within the component as well as how the information is encoded for us to read from.

when we are ready to fetch data from one of our components, we signal the bus with the component's unique address (every component has its own unique byte address), then the address of the unique register that lives inside of that component for which we want to fetch information from. the component then sends us the information we requested, and our program stores this information in a variable of our choosing. this communication happens really fast, and thanks to arduinos Wire library, a quick function call can do basically all of the precise timing for us.

this is one of the common ways for electronic components to talk to one-another, and it can get pretty complicated really fast, so don't fret if it takes you a very long time to understand the concept. 

------------------------------------------------------------------

datasheets:

PCF8523: https://www.nxp.com/docs/en/data-sheet/PCF8523.pdf
MPL115A2: https://www.nxp.com/docs/en/data-sheet/MPL115A2.pdf
LSM6DS3: https://content.arduino.cc/assets/st_imu_lsm6ds3_datasheet.pdf

