#!/user/bin/python

from thethings import TheThingsAPI
import time

tt = TheThingsAPI("tEh7ucriAeGEDj-mC9p4u6LrmkmCqHZ52DJGLmxhYcs")

while(1):
	# Read example
	response = tt.read("temp")
	print 'value: ' + response[0]['value'] + ' date: ' + response[0]['datetime']
	time.sleep(3)

# Write example
#tt.addVar('YourVariable_1', 'YourValue_1')
#tt.addVar('YourVariable_2', 'YourValue_2')
#tt.write();