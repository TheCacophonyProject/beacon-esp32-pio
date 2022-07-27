# beacon-esp32-pio

## Speaker Setup
- SD card setup
    - Format SD card to be FAT32. Deleting all files.
    - Insert the SD card into the speaker and restart the speaker.
    - After a few seconds power off the speaker and remove the SD card.
    - The SD card should now have a file structure like the folowing.
    ```
    SD CARD
    │
    ├── 📂audio
    │   ├── 📂monday
    │   ├── 📂tuesday
    │   ├── 📂wednesday
    │   ├── 📂thursday
    │   ├── 📂friday
    │   ├── 📂saturday
    │   └── 📂sunday
    ├── 📂logs
    │   ├── 📃audio.log
    │   ├── 📃audio.log
    └── 📃cameraDeviceID.txt
    ```
- If you want only one device to trigger the speaker enter in the devices id (just the number) to `cameraDeviceID.txt`.
- Audio files setup
    - Convert the audio files folowing these instructions https://learn.adafruit.com/adafruit-wave-shield-audio-shield-for-arduino/check-your-files
    - For each night of the week put the audio files that you want to play in that folder.
    - Each time the speaker is triggered it will play one sound, if there is a sound for that night.
    - The order of the audio files played is determined by the names of the files, so if order is important then prefixing the files with "01-", "02-" is recommended.
    - Below is an example where:
        - On monday night each time the speaker is triggered it will play `rat-sound.wav`.
        - On Tuesday night the first time the speaker is triggered it will play `01-possum.wav`, then next time `02-possum.wav`, then back to `01-possum.wav` and so on.
        - All other nights nothing will be played.
    ```
    SD CARD
    │
    ├── 📂audio
    │   ├── 📂monday
    │   │   └── 🎵rat-sound.wav
    │   ├── 📂tuesday
    │   │   ├── 🎵01-possum.wav
    │   │   └── 🎵02-possum.wav
    │   ├── 📂wednesday
    │   ├── 📂thursday
    │   ├── 📂friday
    │   ├── 📂saturday
    │   └── 📂sunday
    ```
## Testing speaker
- To test the speaker you can manually trigger a bluetooth beacon from your phone using the Sidekick App.
- Setting up Beacon on Sidekick:
    - Instal Sidekick from the Play Store.
    - Go to the settings page.
    - Click on the version number, about 10 times, until the developers page opens.
    - Click on "Open BLE" (Bluetooth Low Energy).
    - From here you can send a classification beacon that will trigger the speaker.
    - The speaker will only trigger on some classificatoins depending on how it was configered.
    - Setup the values for the beacon test.
    - Device ID:  Should match the device ID that was set in the config file. If no device ID was set then any number here will do.
    - Classificatoin: One of the animals that the speaker was set to trigger on in the config file.
    - Confidence: Equal or above the trigger confidence set in the speakers config file.
    - Duration: How long the beacon will run for. The default 10 seconds should be enough to trigger the speaker.
- Setup speaker for testing
    - To conserve energy the Speaker will only listen for beacons during the night. This is set at starting 1 hour before sunset and ending 1 hour after sunrise.
    - If you want to test the speaker outside of these hours you can turn off then on again the speaker, or press the restart button. After this you sould here the buzzer in the speaker and it will then trigger on beacons for the next 20 seconds. After the 20 seconds if no beacons triggers the speaker it will go back to sleep.
    - Make sure that you have audio files in the correct folders to play when you are testing it.
    - Note that the folders are for the nights of the week, not days. So if you are testing on Thursday morning (before 12:00) the audio that will be trigger will be for Wednesday night, as it is closer to Wednesday night than Thursday night.