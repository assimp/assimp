import sys

Entity_token = "ENTITY"
token = []
file = open(sys.argv[1])
output = open("step_entitylist.txt", "a")
lines = file.readlines()
for line in lines:
    pos = line.find(Entity_token)
    if pos != -1:
        token = line.split(" ")
        if len(token) > 1:
            name = token[1]
            print( "Writing entity " + name)
            output.write(name)
output.close()
file.close()


    
