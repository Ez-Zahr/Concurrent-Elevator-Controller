#!/usr/bin/python

import os
import time

matcher_capacity = '%MAX_CAPACITY%'
matcher_elevators = '%NO_ELEVATORS%'
matcher_floors = '%FLOORS%'
matcher_passengers = '%PASSENGERS%'
matcher_no_trips = '%TRIPS_PER_PASSENGER%'

capacity = [1]
elevator = [1, 4]
floors = [5, 28]
passengers = [1, 2, 50]
no_trips = [1]
crt_test = 1

for c in capacity:
	for e in elevator:
		for f in floors:
			for p in passengers:
				for n in no_trips:
					with open('generator.txt', 'r') as file:
						contents = file.read()


					print '\n\n\n\n'
					contents = contents.replace(matcher_capacity, str(c))
					contents = contents.replace(matcher_elevators, str(e))
					contents = contents.replace(matcher_floors, str(f))
					contents = contents.replace(matcher_passengers, str(p))
					contents = contents.replace(matcher_no_trips, str(n))

					with open('hw6.h', 'w') as file:
						file.write(contents)

					print 'Running test', c
					print 'Number Elevators:', e
					print 'Number Floors:', f
					print 'Number Passengers:', p
					print 'Number Trips:', n
					time.sleep(10)

					c += 1
					os.system('make')
					os.system('./hw6')





