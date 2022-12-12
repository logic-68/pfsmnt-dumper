# pfsmnt-dumper
dump app0 / patch0 if exist OR FULL DUMP

Dumper raw data pfsmnt/title_id-app0 and pfsmnt/title_id-patch0
# version
Bêta 1.0.0

# update 1.0.1 

- Full PFSMNT backup is now possible. Uncomment the FULL_DUMP line of the conf.ini file generated by the application on your USB.
- Starting the application for the first time will build a conf.ini file in the root of the usb. Edit your system language for dump    ignoring other language.
You will have 20 seconds to remove the USB and the application will wait for its return to continue.(default fr)
 {"us", "se", "ru", "pt", "pl", "no", "nl", "la", "jp", "it", "fr", "fi", "es", "dk", "de", "br", "ar" }
- If the application is already present on your USB it will be erased. (notification during the process, wait)
- In the event of a write error, the application will attempt to write it again
- Ff the file really could not be written correctly the application will write the error in the file log_error.txt on usb. You can send me this report for improvement of the application.
- Calculates the number of files well written for comparison with the source and will display Success or Failed
- Transfer speed indicator(may still need to be improved)

# update 1.0.2
- Added backup of app0-nest folder (only)

by default app0 / patch0 if exist
OR Full dump of pfsmnt
OR app0-nest only

- Notifications now show size in GB to transfer
- New make file conf.ini

# update 1.0.3
- Pure code
- Correction of an oversight (during the tests)


- You can follow the process with netcat ( nc64.exe -l -p 5655 -v )
- Good dump thank you for your feedback (once again I unfortunately do not have an accessible game to perform all the tests myself)




# Credits
- [PS5Dev](https://github.com/PS5Dev) 
- [OpenOrbis](https://github.com/OpenOrbis)
- [@notzecoxao](https://twitter.com/notzecoxao)

# required
HDD SATA or USB DISK Exfat (Prefer rear ports for high speed)
SSD speed ~ 100Mb/s
Mecanic speed ~ 32Mb/s

Ubuntu 20.04
Clang-10
# debug mode
use netcat or other (for netcat and wX64: nc64.exe -l -p 5655 -v )


