
token = []
file = open(sys.argv[1])
output = open("step_entitylist.txt", "a")
lines = file.readlines()
for line in lines:
    pos = line.find("ENTITY")
    if pos != -1:
        
    
