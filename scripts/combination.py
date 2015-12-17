blsize = ['8', '16', '32', '64']
msize = ['512', '256','128', '64']
replacement = 'l'
cache_type = '1'
benchmarks = {
	'anagram' : ['cc1.ss', '1stmt.i', 'OUT']
}

entry = ['8']
path = '/home/ubuntu/'

idir = 'harsh'
# -cache:dl1 dl1:4:64:1:l -cache:il1 il1:4:64:1:l
# ./sim-cache /home/ubuntu/harsh/little_endian_binaries/Little/anagram.ss </home/ubuntu/harsh/benchmarks/anagram.in>OUT

base_command = './sim-cache'


ss_path = '/little_endian_binaries/Little/'
in_path = '/benchmarks/'

data = {}
for k,v in benchmarks.items():
	commands = []
	for m in range(len(msize)):

		il1 = '-cache:il1 il1:' + msize[m] + ':' + blsize[m] + ':' + cache_type + ':' + replacement
		dl1 = '-cache:dl1 dl1:' + msize[m] + ':' + blsize[m] + ':' + cache_type + ':' + replacement

		dl2 = '-cache:dl2 dl2:2048:64:4:l'

		command = base_command + ' ' + dl1 + ' ' + il1 + ' ' +  dl2 + ' ' + path + idir + ss_path + v[0] + ' <' +  path + idir + in_path + v[1] + '>' + v[2]




		commands.append(command)


		for en in entry:
			dmcache = '-cache:dmcache dmcache:' + en +  ':' + replacement
			imcache = '-cache:imcache imcache:' + en +  ':' + replacement
			dvictim = '-cache:dvictim dvictim:' + en +  ':' + replacement
			ivictim = '-cache:ivictim ivictim:' + en +  ':' + replacement
			
			command = base_command + ' ' + dl1 + ' ' + il1 + ' ' +  dl2 + ' ' + dmcache + ' ' + imcache + ' ' + path + idir + ss_path + v[0] + ' <' +  path + idir + in_path + v[1] + '>' + v[2]

			commands.append(command)

			command = base_command + ' ' + dl1 + ' ' + il1 + ' ' +  dl2 + ' ' + dvictim + ' ' + ivictim + ' ' + path + idir + ss_path + v[0] + ' <' +  path + idir + in_path + v[1] + '>' + v[2]
			commands.append(command)

	# commands.append({'msize:' + })
	# data[k] = commands
	# print command
print data
import json
f = open('cc1_c.txt', 'w')
# json = json.dumps(data, indent=4)
# f.write(json)
for x in commands:
	f.write(x)
	f.write('\n\n\n')
	# f.write('\n\n')
f.close()
