# REAL TIME EMBEDDED SYSTEMS FINAL PROJECT - Spring 2023

Members:<br><br>
    Andres Bravo - afb389<br>
    Andre Duong - ld2784<br>
    Anshika Gupta - ag8800<br>
    Aris Panousopoulos - ap6487

# Description
    This project is a security system that uses a gyroscope to record the angular acceleration on the device. This
    information is used to create a lock type system in which the user can create a key by recording it and lock
    the device. The device will the only unlock if the correct password is entered and matches the key. The device
    can also allow the key to be changed under the correct conditions.

# Usage
    When the device is first turned on, the user will be prompted to enter a key. The device needs a key to be entered
    before it can be locked.
    Afterwards the device is locked.

    To unlock device:
        -user must short press the button and move the device correctly matching the key while device is locked.
    To lock device:
        -user must short press the button while device is unlocked.
    To change key:
        -user must long press (3s by default) the button while device is unlocked. NOTE: user can not
        change the key if the device is locked.
