import subprocess
def runProcess(exe):    
	p = subprocess.Popen(exe, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
	while(True):
		retcode = p.poll() #returns None while subprocess is running
		line = p.stdout.readline()
		yield line
		if(retcode is not None):
			break

def anvi(lines, command, benchmark):
	data = {}
	data['command'] = command
	data['benchmark'] = benchmark
	for line in lines[1:]:
	# print line
		indexleft = line.find(' ')
		name = line[:indexleft]
		# print name

		indexright = line.find('#') - 1

		valstring = line[indexleft:indexright]

		valindex = valstring.rfind(' ')
		# print valstring
		# print valindex, indexright
		value = valstring[valindex+1:indexright]
		data[name] = value

		# print name + ' : ' + value
	return data


f = 'sim: ** simulation statistics **'

import json
datafile = open('commands.json', 'r')
commands = json.loads(datafile.read())

print commands
fl = open('result.txt', 'w')
fj = open('finalresult.json', 'w')

final_data = []
for k,v in commands.items():
	
	for command in v:
		result = ''
		result = command + '\n'
		for line in runProcess(command.split()):
			result = result + line + '\n'
		
		index = result.find(f)
		result = result[index:]
		result = result.split('\n')
	
		l = []
		for x in result:
			if x == '':
				continue
			fl.write(x)
			l.append(x)
			fl.write('\n')

		dic_data = anvi( l, command, k )
	fl.write('\n\n')
	final_data.append(dic_data)

json = json.dumps(final_data, indent=4)
fj.write(json)
fj.close()
fl.close()

