 int nLocations = ...;
 int nCities = ...;
 int nTypes = ...;
		   
 range L = 1..nLocations;
 range C = 1..nCities;
 range T = 1..nTypes;

 int posCities[C][1..2] = ...;
 int posLocations[L][1..2] = ...;
 int p[C] = ...;

 float d_city[T] = ...;
 int cap [T] = ...;
 int cost [T] = ...;

 float d_center = ...;
			  

// Store and compute all distances between locations and cities
float distance[L][C];
// Store 1 if compatible locations, 0 otherwise
float compatible_locations[L][L];

execute {
  for(var l in L){
    for(var c in C){
      var dx = posCities[c][1] - posLocations[l][1];
      var dy = posCities[c][2] - posLocations[l][2];
      distance[l][c] = Math.sqrt(dx*dx+dy*dy);
    }
    
    for(var l2 in L){
      var dx = posLocations[l2][1] - posLocations[l][1];
      var dy = posLocations[l2][2] - posLocations[l][2];
      compatible_locations[l][l2] = (Math.sqrt(dx*dx+dy*dy) >= d_center) ? 1.0 : 0.0;
    }
  }
  for(var l in L) compatible_locations[l][l] = 1.0;
}


dvar boolean locationCenterType[L][T]; 		// location l has center of type t
dvar boolean locationPrimaryCenter[L][C];	// location l is a primary center of city c
dvar boolean locationSecondayCenter[L][C];	// location l is a secondary center of city c

// minimize the cost of all the centers
// hint: sum(t in T) locationCenterType[l][t] <= 1
minimize sum(l in L) sum(t in T) locationCenterType[l][t] * cost[t]; 

subject to {

  forall(l in L) {
    max_one_center_type:
     sum(t in T) locationCenterType[l][t] <= 1;
  	
  	// Implication for each pair of locations, if a location A has a center then location B has no center of has a compatible center 
  	// not location used or no second location used or compatible combination
  	forall(l2 in L)
  	min_dist_between_centers:
  		(1-sum(t in T) locationCenterType[l][t]) + (1-sum(t in T) locationCenterType[l2][t]) + sum(t in T) locationCenterType[l2][t] * compatible_locations[l][l2] >= 1;
  		
  	// If center in location, its type cap greater equal than its associated cities populations by following formula
  	max_population:
  	  sum(t in T) locationCenterType[l][t] * cap[t] >= sum(c in C) (locationPrimaryCenter[l][c] * p[c] + 0.1 * locationSecondayCenter[l][c] * p[c]);
  }     
     
  forall(c in C){
    // Each city has one primary center
    one_primary_center_per_city:
     sum(l in L) locationPrimaryCenter[l][c] == 1;

     // Each city has one secondary center
    one_secondary_center_per_city:
     sum(l in L) locationSecondayCenter[l][c] == 1;
     
     // Foreach pair of location and city...
     forall(l in L) {
       // Each location is either primary or secondary or neither
      exclusive_center:
     	locationSecondayCenter[l][c] + locationPrimaryCenter[l][c] <= 1;
      
      // If location has primary center then a center of some type exists there
      if_primary_center_location:
        (1-locationPrimaryCenter[l][c])	+ (sum(t in T) locationCenterType[l][t]) >= 1;
      // If location has secondary center then a center of some type exists there
      if_secondary_center_location:
        (1-locationSecondayCenter[l][c]) + (sum(t in T) locationCenterType[l][t]) >= 1;
      // If location is asociated to a city, its type distance must be less than actual distance
      primary_center_max_dist:
        locationPrimaryCenter[l][c] * distance[l][c] <= sum(t in T) locationCenterType[l][t] * d_city[t];
      secondary_center_max_dist:
        locationSecondayCenter[l][c] * distance[l][c] <= 3 * sum(t in T) locationCenterType[l][t] * d_city[t];
    }     	

   }
 }   
   