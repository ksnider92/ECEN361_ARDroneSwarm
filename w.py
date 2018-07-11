if __name__ == "__main__":
    while(True):

        result = raw_input(">")
        fileName = open("toC.txt","a+")
        fileName.write(result)
        fileName.close()

