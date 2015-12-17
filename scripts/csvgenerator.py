import csv

f = open('final_result.json','r')


icsv = open('instruction.csv','wb')
dcsv = open('data.csv','wb')
import json
benchmark = ['anagram', 'go', 'compress95', 'cc1', 'perl']
data = json.loads(f.read())
# dl1.miss_rate
# il1.miss_rate
# command
for x in data:
	command = x['command']
	dl1 = 'dl1:'
	# il1 = 'il1:'
	# print command
	command = command[command.find(dl1)+len(dl1):]
	msize = command[:command.find(':')]

	
	command = command[command.find(':') + 1 :]
	bsize = command[ :command.find(':') ]

	command = command[:command.find('.ss')]
	bench = command[ command.rfind('/') +1: ]
	print bench
	if int(bsize) >= 128:
		continue
	# print bsize
	# print msize
	# print command

	try:
		# pass
		dans = [bench, msize, bsize, x['dl1.miss_rate']]
		ians = [bench, msize, bsize, x['il1.miss_rate']]
		dr = csv.writer(dcsv, quoting=csv.QUOTE_ALL)
		ir = csv.writer(icsv, quoting=csv.QUOTE_ALL)
		dr.writerow(dans)
		ir.writerow(ians)
		# print x['dl1.miss_rate']
		# print x['il1.miss_rate']
	except:
		pass

