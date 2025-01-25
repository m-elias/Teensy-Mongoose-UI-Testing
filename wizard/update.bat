@ECHO OFF
:: This batch file copies/updates the Wizard generated Mongoose library files for VScode/PlatformIO

ECHO Copying/updating new Wizard generated Mongoose files
:: mongoose.c has mg_log_prefix change to print decimal millis()
REM copy mongoose.* ..\Teensy_UI
:: config.h has manually added static IP config
REM copy mongoose_config.h ..\Teensy_UI
copy mongoose_fs.c ..\Teensy_UI
copy mongoose_impl.c ..\Teensy_UI

:: Don't overwrite these without comparing and merging them apporpriately
REM copy mongoose_glue.* ..\Teensy_UI
