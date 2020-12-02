import numpy as np

nLocations=100
nCities=170
nTypes=10

d_center=1.1

maxX=100
maxY=100

p=np.random.randint(0,10,nCities)
cap=np.random.randint(5,20,nTypes)
cost=cap//2 + np.random.randint(0,2)
d_city=np.random.randint(2,10,nTypes)

realDis=np.argmax(d_city)

notValid=True
while notValid:
    p=np.random.randint(0,10,nCities)
    cap=np.random.randint(5,20,nTypes)
    cost=cap//2 + np.random.randint(0,2)
    maxCap=np.amax(cap)
    totCapacity=0
    for val in p:
        totCapacity+=val*1.3
    if maxCap*nLocations>totCapacity:
        notValid=False
        
indMax = np.argmax(cap)
locCap=[cap[indMax]]*nLocations

startX=np.random.randint(0,maxX)
startY=np.random.randint(0,maxY)

posLocations=[[startX,startY]]
posCities=[]

for location in range(1,nLocations):
    print(location)
    validLoc=False
    attempts=0
    while (not validLoc):
        attempts+=1
        posX=np.random.randint(0,maxX)
        posY=np.random.randint(0,maxY)
        validPos=True
        for pastLocations in posLocations:
            aux1=pastLocations[0]-posX
            aux2=pastLocations[1]-posY
            if np.linalg.norm([aux1,aux2])<=d_center:
                validPos=False
        validLoc=validPos
        if attempts>1000:
            maxX+=10
            maxY+=10
            attempts=0
    posLocations.append([posX,posY])
    
for location in range(0,nCities):
    print(location)
    validLoc=False
    attempts=0
    while (not validLoc):
        posX=np.random.randint(0,maxX)
        posY=np.random.randint(0,maxY)
        validPrimary=False
        validSecondary=False
        for idx,pastLocations in enumerate(posLocations):
            attempts+=1
            aux1=pastLocations[0]-posX
            aux2=pastLocations[1]-posY
            dist=np.linalg.norm([aux1,aux2])
            if dist<d_city[realDis] and validPrimary==False:
                if locCap[idx]>=p[location]:
                    locCap[idx]-=p[location]
                    validPrimary=True
                elif attempts>1000:
                    locCap[idx]+=p[location]
                    cap[indMax]+=p[location]
                    cost[indMax]+=p[location]
                    for iter in range(0,nLocations):
                        locCap[iter]+=p[location]
            elif dist<d_city[realDis]*3 and validSecondary==False:
                if locCap[idx]>=p[location]*0.3:
                    locCap[idx]-=p[location]
                    validSecondary=True
                elif attempts>1000:
                    locCap[idx]+=int(p[location]*0.3+1)
                    cap[indMax]+=int(p[location]*0.3+1)
                    cost[indMax]+=int(p[location]*0.3+1)
                    for iter in range(0,nLocations):
                        locCap[iter]+=int(p[location]*0.3+1)
            if validPrimary and validSecondary:
                validLoc=True
                break
    posCities.append([posX,posY])
             

with open('output.txt', 'w') as f:
    print("nLocations = "+str(nLocations)+";", file=f)
    print("nCities = "+str(nCities)+";", file=f)
    print("nTypes = "+str(nTypes)+";", file=f)
    stringP="["
    for val in p:
        stringP+=" "+str(val)
    stringP+=" ]"
    print("p = "+stringP+";", file=f)
    stringC="["
    for val in posCities:
        stringC+=" ["+str(val[0])+" "+str(val[1])+"]"
    stringC+=" ]"
    print("posCities = "+stringC+";", file=f)
    stringL="["
    for val in posLocations:
        stringL+=" ["+str(val[0])+" "+str(val[1])+"]"
    stringL+=" ]"
    print("posLocations = "+stringL+";", file=f)
    stringCap="d_city = ["
    for val in d_city:
        stringCap+=" "+str(val)
    stringCap+=" ]"
    print(stringCap, file=f)
    stringCap="["
    for val in cap:
        stringCap+=" "+str(val)
    stringCap+=" ]"
    print("cap = "+stringCap+";", file=f)
    stringCost="["
    for val in cost:
        stringCost+=" "+str(val)
    stringCost+=" ]"
    print("cost = "+stringCost+";", file=f)
    print("d_center = "+str(d_center)+";", file=f)