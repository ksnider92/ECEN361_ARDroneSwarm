def write_file(command):
    fileName = open("toC.txt", "a+")
    fileName.write(command)
    fileName.close()

def main():
    "List of function you can perform"
    print "Choose your command:"
    print "0 - Emergency"
    print "1 - Hover"
    print "2 - Take Off"
    print "3 - Land"
    print "4 - Forward"
    print "5 - Backward"
    print "6 - Left"
    print "7 - Right"
    print "8 - Calibrate sensors"
    print "a - Reset"
    print "9 - Quit"
    result = ""
    while result != "9":
        result = raw_input(">")
	write_file(result)

if __name__ == "__main__":
    main()

