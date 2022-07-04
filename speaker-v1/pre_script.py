from datetime import datetime,timezone
f = open("src/utcTime.h", "w")
f.write("#define COMPILE_UTC_TIME " + str(int(datetime.now(timezone.utc).timestamp())))

# Set different time for testing purposes.
year = 2022
month = 9
day = 25
hour = 0
minute = 2
second = 1
#f.write("#define COMPILE_UTC_TIME " + str(int(datetime(year, month, day, hour, minute, second).timestamp())))
f.close()
