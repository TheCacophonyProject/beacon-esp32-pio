import datetime
f = open("src/utcTime.h", "w")
f.write("#define COMPILE_UTC_TIME " + datetime.datetime.utcnow().strftime("%s"))
f.close()
